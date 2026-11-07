#include "features/config.h"
#include "sdkconfig.h"
#if XTOUCH_FEATURE_PIXEL_SCREEN
#include "connection.h"
#include "activity.h"
#include "link_protocol.h"
#include "picture_key.h"
#include "platform/idf/ble_runtime.h"
#include "platform/idf/runtime.h"
#include "platform/idf/heap_buffer.h"
#include <host/ble_hs.h>
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>
#include <os/os_mbuf.h>
#include <freertos/queue.h>
#include <atomic>
#include <cstring>
#include <ctime>
#if !CONFIG_BT_NIMBLE_ROLE_CENTRAL || !CONFIG_BT_NIMBLE_GATT_CLIENT
#error "Pixel screen requires NimBLE central/GATT client: build with boards/build.py --pixel-screen"
#endif
namespace pixel_screen {
std::atomic<uint32_t> clockGeneration{0};
void connectionTimeChanged(){++clockGeneration;}

namespace {
const ble_uuid16_t serviceUuid = BLE_UUID16_INIT(0x00fa), writeUuid = BLE_UUID16_INIT(0xfa02),
                   notifyUuid = BLE_UUID16_INIT(0xfa03), cccdUuid = BLE_UUID16_INIT(0x2902);
portMUX_TYPE guard = portMUX_INITIALIZER_UNLOCKED;
ConnectionConfig requested;
Snapshot desired;
uint32_t revision = 0, seenAt = 0;
ble_addr_t seen = {};
bool addressSeen = false;
struct FoundDevice {
    char mac[18] = {}, name[32] = {};
    int rssi = -127;
    uint32_t seen = 0;
};
FoundDevice found[12];
// Advertising names can be in scan responses. Collect only during an explicit scan.
void collect(const ble_gap_event *e) {
    if (!xtouch_ble::scanRemainingMs())
        return;
    char name[32] = {};
    bool candidate = false;
    const uint8_t uuid[16] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0, 0, 0x80,
                              0,    0x10, 0,    0,    0xfa, 0, 0, 0};
    const uint8_t *data = e->disc.data;
    size_t size = e->disc.length_data;
    for (size_t pos = 0; pos < size;) {
        size_t n = data[pos++];
        if (!n)
            break;
        if (n > size - pos)
            return;
        uint8_t type = data[pos];
        const uint8_t *value = data + pos + 1;
        size_t length = n - 1;
        if (type == 8 || type == 9) {
            size_t count = length < sizeof(name) - 1 ? length : sizeof(name) - 1;
            for (size_t i = 0; i < count; ++i)
                name[i] = value[i] >= 32 && value[i] < 127 ? char(value[i]) : '?';
            name[count] = 0;
        }
        if (type == 2 || type == 3 || type == 0x16) {
            size_t limit = type == 0x16 ? (length >= 2 ? 2 : 0) : length;
            for (size_t i = 0; i + 1 < limit; i += 2)
                candidate |= value[i] == 0xfa && value[i + 1] == 0;
        }
        if (type == 6 || type == 7)
            for (size_t i = 0; i + 16 <= length; i += 16)
                candidate |= !memcmp(value + i, uuid, 16);
        pos += n;
    }
    candidate |= !strncmp(name, "IDM-", 4);
    char mac[18];
    const auto &a = e->disc.addr.val;
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", a[5], a[4], a[3], a[2], a[1], a[0]);
    portENTER_CRITICAL(&guard);
    int slot = -1, oldest = 0;
    for (int i = 0; i < 12; ++i) {
        if (!strcmp(found[i].mac, mac))
            slot = i;
        if (found[i].seen < found[oldest].seen)
            oldest = i;
    }
    if (slot >= 0 || candidate) {
        if (slot < 0) {
            slot = oldest;
            found[slot] = FoundDevice{};
        }
        auto &item = found[slot];
        memcpy(item.mac, mac, sizeof(mac));
        if (name[0])
            memcpy(item.name, name, sizeof(name));
        item.rssi = e->disc.rssi;
        item.seen = millis();
    }
    portEXIT_CRITICAL(&guard);
}
struct Status {
    const char *phase = "disabled";
    const char *failure = "";
    bool connected = false, authenticated = false, controlsApplied = false;
    bool nativeClockActive = false;
    unsigned clockSyncs = 0;
    unsigned brightness = 0;
    bool screenOn = true, rotate180 = false;
    uint32_t attempts = 0, uploads = 0, bytes = 0;
    uint32_t lastUploadBytes = 0, lastUploadMs = 0;
    uint32_t resets = 0, passwordRestores = 0, lastResetMs = 0, clears = 0;
    bool passwordProtected = false;
    unsigned mtu = 23, stack = 0;
} state;
std::atomic<uint32_t> epoch{0};
std::atomic<bool> overflow{false};
TaskHandle_t workerTask = nullptr;
enum class Kind : uint8_t {
    Connect,
    Disconnect,
    Mtu,
    Service,
    ServicesDone,
    Characteristic,
    CharacteristicsDone,
    Descriptor,
    DescriptorsDone,
    Write,
    Notify
};
struct Event {
    Kind kind;
    uint32_t epoch;
    int error = 0;
    uint16_t handle = 0, other = 0, properties = 0;
    uint8_t length = 0, data[32] = {};
};
QueueHandle_t events = nullptr;
void phase(const char *value, const char *failure = "") {
    portENTER_CRITICAL(&guard);
    state.phase = value;
    state.failure = failure;
    portEXIT_CRITICAL(&guard);
}
void clear(void *p, size_t n) {
    volatile uint8_t *v = static_cast<volatile uint8_t *>(p);
    while (n--)
        *v++ = 0;
}
ConnectionConfig config(uint32_t *version = nullptr) {
    portENTER_CRITICAL(&guard);
    auto copy = requested;
    if (version)
        *version = revision;
    portEXIT_CRITICAL(&guard);
    return copy;
}
void send(Event event) {
    if (!events || xQueueSend(events, &event, 0) != pdTRUE)
        overflow = true;
}
uint32_t token(void *arg) {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arg));
}
int gap(ble_gap_event *e, void *arg) {
    Event v{};
    v.epoch = token(arg);
    if (e->type == BLE_GAP_EVENT_CONNECT) {
        v.kind = Kind::Connect;
        v.error = e->connect.status;
        v.handle = e->connect.conn_handle;
        send(v);
    } else if (e->type == BLE_GAP_EVENT_DISCONNECT) {
        v.kind = Kind::Disconnect;
        v.error = e->disconnect.reason;
        send(v);
    } else if (e->type == BLE_GAP_EVENT_NOTIFY_RX) {
        v.kind = Kind::Notify;
        v.handle = e->notify_rx.attr_handle;
        unsigned n = OS_MBUF_PKTLEN(e->notify_rx.om);
        v.length = n > sizeof(v.data) ? sizeof(v.data) : n;
        if (n <= sizeof(v.data) && !os_mbuf_copydata(e->notify_rx.om, 0, n, v.data))
            send(v);
    }
    return 0;
}
int observe(ble_gap_event *e, void *) {
    if (e->type != BLE_GAP_EVENT_DISC)
        return 0;
    collect(e);
    portENTER_CRITICAL(&guard);
    if (requested.enabled && requested.mac[0]) {
        char mac[18];
        auto &a = e->disc.addr.val;
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", a[5], a[4], a[3], a[2], a[1],
                 a[0]);
        if (!strcmp(mac, requested.mac)) {
            seen = e->disc.addr;
            seenAt = millis();
            addressSeen = true;
        }
    }
    portEXIT_CRITICAL(&guard);
    return 0;
}
int mtuCallback(uint16_t, const ble_gatt_error *error, uint16_t mtu, void *arg) {
    Event e{};
    e.kind = Kind::Mtu;
    e.epoch = token(arg);
    e.error = error->status;
    e.handle = mtu;
    send(e);
    return 0;
}
int serviceCallback(uint16_t, const ble_gatt_error *error, const ble_gatt_svc *svc, void *arg) {
    Event e{};
    e.epoch = token(arg);
    e.error = error->status;
    if (!error->status && svc) {
        e.kind = Kind::Service;
        e.handle = svc->start_handle;
        e.other = svc->end_handle;
    } else
        e.kind = Kind::ServicesDone;
    send(e);
    return 0;
}
int characteristicCallback(uint16_t, const ble_gatt_error *error, const ble_gatt_chr *chr,
                           void *arg) {
    Event e{};
    e.epoch = token(arg);
    e.error = error->status;
    if (!error->status && chr) {
        e.kind = Kind::Characteristic;
        e.handle = chr->val_handle;
        e.other = chr->def_handle;
        e.properties = chr->properties;
        if (!ble_uuid_cmp(&chr->uuid.u, &writeUuid.u))
            e.data[0] = 2;
        else if (!ble_uuid_cmp(&chr->uuid.u, &notifyUuid.u))
            e.data[0] = 3;
    } else
        e.kind = Kind::CharacteristicsDone;
    send(e);
    return 0;
}
int descriptorCallback(uint16_t, const ble_gatt_error *error, uint16_t chr, const ble_gatt_dsc *dsc,
                       void *arg) {
    Event e{};
    e.epoch = token(arg);
    e.error = error->status;
    if (!error->status && dsc) {
        e.kind = Kind::Descriptor;
        e.handle = dsc->handle;
        e.other = chr;
        e.data[0] = !ble_uuid_cmp(&dsc->uuid.u, &cccdUuid.u);
    } else
        e.kind = Kind::DescriptorsDone;
    send(e);
    return 0;
}
int written(uint16_t, const ble_gatt_error *error, ble_gatt_attr *, void *arg) {
    Event e{};
    e.kind = Kind::Write;
    e.epoch = token(arg);
    e.error = error->status;
    send(e);
    return 0;
}
struct Session {
    uint32_t generation = 0, version = 0;
    uint16_t conn = BLE_HS_CONN_HANDLE_NONE, writeHandle = 0, notifyHandle = 0;
    unsigned mtu = 23;
    bool live = false, radioLease = false, restoringPassword = false;
    Event pending[8];
    unsigned count = 0;
    const char *failure = "";
    void *arg() {
        return reinterpret_cast<void *>(static_cast<uintptr_t>(generation));
    }
    bool current() {
        // Finish restoring the verified password before honouring a concurrent rebind.
        if (restoringPassword && live && xtouch_ble::ready() && !overflow) return true;
        uint32_t v;
        auto c = config(&v);
        bool ok = v == version && c.enabled && xtouch_ble::ready() && !overflow;
        clear(c.password, sizeof(c.password));
        if (!ok)
            failure = overflow ? "event_overflow" : "cancelled";
        return ok;
    }
    bool next(Event &event, uint32_t deadline) {
        while (current() && int32_t(deadline - millis()) > 0) {
            if (xQueueReceive(events, &event, pdMS_TO_TICKS(50)) != pdTRUE)
                continue;
            if (event.epoch != generation)
                continue;
            if (event.kind == Kind::Disconnect) {
                live = false;
                failure = "disconnected";
                return false;
            }
            return true;
        }
        if (!failure[0])
            failure = "timeout";
        return false;
    }
    bool wait(Kind kind, Event &out, uint32_t timeout = 5000) {
        uint32_t deadline = millis() + timeout;
        Event e{};
        while (next(e, deadline)) {
            if (e.kind == Kind::Notify) {
                if (e.handle == notifyHandle) {
                    if (count >= 8) {
                        failure = "event_overflow";
                        return false;
                    }
                    pending[count++] = e;
                }
                continue;
            }
            if (e.kind == kind) {
                out = e;
                return true;
            }
        }
        return false;
    }
    void flush() {
        count = 0;
        Event e{};
        while (xQueueReceive(events, &e, 0) == pdTRUE) {
            if (e.epoch == generation && e.kind == Kind::Disconnect) {
                live = false;
                failure = "disconnected";
            }
        }
    }
    bool write(const uint8_t *data, size_t size, uint16_t handle = 0, uint32_t deadline = 0) {
        if (!current() || !live)
            return false;
        size_t limit = link::writeSize(mtu);
        if (!handle)
            handle = writeHandle;
        for (size_t offset = 0; offset < size;) {
            if (deadline && int32_t(deadline - millis()) <= 0) {
                failure = "write_timeout";
                return false;
            }
            size_t n = size - offset < limit ? size - offset : limit;
            int rc = ble_gattc_write_flat(conn, handle, data + offset, n, written, arg());
            if (rc) {
                failure = "write_failed";
                return false;
            }
            Event e{};
            if (deadline && int32_t(deadline - millis()) <= 0) {
                    failure = "write_timeout";
                    return false;
                }
                uint32_t budget = deadline ? uint32_t(deadline - millis()) : 5000;
            if (budget > 5000)
                budget = 5000;
            if (!wait(Kind::Write, e, budget) || e.error) {
                if (!failure[0])
                    failure = "write_failed";
                return false;
            }
            offset += n;
        }
        return true;
    }
    bool notification(Event &out, uint32_t deadline) {
        if (count) {
            out = pending[0];
            for (unsigned i = 1; i < count; ++i)
                pending[i - 1] = pending[i];
            --count;
            return true;
        }
        Event e{};
        while (next(e, deadline)) {
            if (e.kind == Kind::Notify && e.handle == notifyHandle) {
                out = e;
                return true;
            }
        }
        return false;
    }
    link::Reply reply(uint8_t cmd, uint8_t sub, uint32_t deadline, bool upload = false) {
        Event e{};
        while (notification(e, deadline)) {
            auto result = upload ? link::uploadReply(e.data, e.length)
                                 : link::commandReply(e.data, e.length, cmd, sub);
            if (result != link::Reply::Ignore)
                return result;
        }
        return link::Reply::Ignore;
    }
    bool command(const uint8_t *data, size_t size) {
        flush();
        if (!live || !write(data, size))
            return false;
        auto result = reply(data[2], data[3], millis() + 1000);
        if (result != link::Reply::Accepted) {
            failure = result == link::Reply::Rejected ? "command_rejected" : "command_timeout";
            return false;
        }
        return true;
    }
    bool connect(const ble_addr_t &address) {
        if (!xtouch_ble::clientActive(true)) {
            failure = "radio_unavailable";
            return false;
        }
        radioLease = true;
        generation = ++epoch;
        overflow = false;
        flush();
        xtouch_ble::pauseScan(true);
        phase("connecting");
        portENTER_CRITICAL(&guard);
        ++state.attempts;
        portEXIT_CRITICAL(&guard);
        if (ble_gap_connect(xtouch_ble::addressType(), &address, 10000, nullptr, gap, arg())) {
            failure = "connect_failed";
            return false;
        }
        Event e{};
        if (!wait(Kind::Connect, e, 12000) || e.error) {
            if (!failure[0])
                failure = "connect_failed";
            return false;
        }
        conn = e.handle;
        live = true;
        xtouch_ble::pauseScan(false);
        portENTER_CRITICAL(&guard);
        state.connected = true;
        portEXIT_CRITICAL(&guard);
        phase("discovering");
        int rc = ble_gattc_exchange_mtu(conn, mtuCallback, arg());
        if (!rc) {
            if (!wait(Kind::Mtu, e))
                return false;
            if (!e.error)
                mtu = e.handle;
        } else if (rc != BLE_HS_EALREADY) {
            failure = "mtu_failed";
            return false;
        }
        if (mtu < 23)
            mtu = 23;
        portENTER_CRITICAL(&guard);
        state.mtu = mtu;
        portEXIT_CRITICAL(&guard);
        if (ble_gattc_disc_svc_by_uuid(conn, &serviceUuid.u, serviceCallback, arg())) {
            failure = "service_discovery_failed";
            return false;
        }
        uint16_t start = 0, end = 0;
        uint32_t deadline = millis() + 5000;
        while (next(e, deadline)) {
            if (e.kind == Kind::Service) {
                start = e.handle;
                end = e.other;
            } else if (e.kind == Kind::ServicesDone) {
                if (e.error != BLE_HS_EDONE || !start) {
                    failure = "service_missing";
                    return false;
                }
                break;
            }
        }
        if (!start || failure[0])
            return false;
        if (ble_gattc_disc_all_chrs(conn, start, end, characteristicCallback, arg())) {
            failure = "characteristic_discovery_failed";
            return false;
        }
        uint16_t notifyEnd = end;
        bool writable = false, notifiable = false;
        deadline = millis() + 5000;
        while (next(e, deadline)) {
            if (e.kind == Kind::Characteristic) {
                if (notifyHandle && e.other > notifyHandle && e.other - 1 < notifyEnd)
                    notifyEnd = e.other - 1;
                if (e.data[0] == 2) {
                    writeHandle = e.handle;
                    writable = e.properties & BLE_GATT_CHR_PROP_WRITE;
                } else if (e.data[0] == 3) {
                    notifyHandle = e.handle;
                    notifiable = e.properties & BLE_GATT_CHR_PROP_NOTIFY;
                }
            } else if (e.kind == Kind::CharacteristicsDone) {
                if (e.error != BLE_HS_EDONE) {
                    failure = "characteristic_discovery_failed";
                    return false;
                }
                break;
            }
        }
        if (failure[0] || !writeHandle || !notifyHandle || !writable || !notifiable) {
            failure = "characteristic_missing";
            return false;
        }
        if (ble_gattc_disc_all_dscs(conn, notifyHandle, notifyEnd, descriptorCallback, arg())) {
            failure = "subscription_failed";
            return false;
        }
        uint16_t cccd = 0;
        deadline = millis() + 5000;
        while (next(e, deadline)) {
            if (e.kind == Kind::Descriptor && e.other == notifyHandle && e.data[0])
                cccd = e.handle;
            else if (e.kind == Kind::DescriptorsDone) {
                if (e.error != BLE_HS_EDONE) {
                    failure = "subscription_failed";
                    return false;
                }
                break;
            }
        }
        if (!cccd || failure[0]) {
            failure = "subscription_failed";
            return false;
        }
        phase("subscribing");
        const uint8_t subscribe[] = {1, 0};
        return write(subscribe, sizeof(subscribe), cccd);
    }
    size_t timePacket(uint8_t *out) {
        time_t now = time(nullptr);
        tm local{};
        localtime_r(&now, &local);
        if (local.tm_year < 120) return 0;
        const uint8_t packet[] = {11, 0, 1, 128,
            uint8_t((local.tm_year + 1900) % 100), uint8_t(local.tm_mon + 1),
            uint8_t(local.tm_mday), uint8_t(local.tm_wday ? local.tm_wday : 7),
            uint8_t(local.tm_hour), uint8_t(local.tm_min), uint8_t(local.tm_sec)};
        memcpy(out, packet, sizeof(packet));
        return sizeof(packet);
    }
    bool readInfo(link::DeviceInfo &info) {
        uint8_t sync[11];
        const size_t timeSize = timePacket(sync);
        const uint8_t query[] = {4, 0, 1, 128};
        // The official app synchronises first. Some panels ignore the short query.
        for (unsigned attempt = 0; attempt < (timeSize ? 2u : 1u); ++attempt) {
            flush();
            if (!write(timeSize && !attempt ? sync : query,
                       timeSize && !attempt ? timeSize : sizeof(query))) return false;
            Event e{};
            const uint32_t deadline = millis() + 3000;
            while (notification(e, deadline))
                if (link::deviceInfo(e.data, e.length, info)) return true;
            if (!live || !current()) return false;
            failure = "";
        }
        failure = "device_info_timeout";
        return false;
    }
    bool verifySavedPassword(const ConnectionConfig &c) {
        if (c.passwordUnavailable) { failure = "password_unavailable"; return false; }
        if (!link::passwordValid(c.password)) { failure = "password_required"; return false; }
        phase("authenticating");
        uint8_t verify[7];
        link::verifyPassword(c.password, verify);
        flush();
        const bool sent = write(verify, sizeof(verify));
        clear(verify, sizeof(verify));
        if (!sent) return false;
        const auto result = reply(5, 2, millis() + 3000);
        if (result == link::Reply::Accepted) return true;
        failure = result == link::Reply::Rejected ? "password_rejected" : "authentication_unknown";
        return false;
    }
    bool resetPreservingPassword(const ConnectionConfig &c, link::DeviceInfo &info) {
        if (!current()) return false;
        const bool protectedBefore = info.passwordProtected;
        const uint32_t started = millis();
        phase("resetting");
        uint8_t reset[4];
        link::resetDevice(reset);
        restoringPassword = protectedBefore;
        const bool resetOkay = command(reset, sizeof(reset));
        bool passwordOkay = !protectedBefore;
        if (protectedBefore && live) {
            uint8_t restore[8];
            const size_t size = link::restorePassword(c.password, restore);
            for (unsigned attempt = 0; size && attempt < 2; ++attempt) {
                if (command(restore, size)) { passwordOkay = true; break; }
                if (!live) break;
                failure = "";
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            clear(restore, sizeof(restore));
        }
        restoringPassword = false;
        if (!passwordOkay) { failure = "password_restore_failed"; return false; }
        if (!resetOkay) { failure = "reset_failed"; return false; }
        if (!readInfo(info)) { failure = "reset_failed"; return false; }
        if (info.screenType != 3 || info.passwordProtected != protectedBefore) {
            failure = protectedBefore ? "password_restore_failed" : "reset_failed";
            return false;
        }
        if (protectedBefore && !verifySavedPassword(c)) {
            failure = "password_restore_failed";
            return false;
        }
        portENTER_CRITICAL(&guard);
        ++state.resets;
        state.passwordRestores += protectedBefore;
        state.lastResetMs = millis() - started;
        portEXIT_CRITICAL(&guard);
        return true;
    }
    bool clearStoredMaterials(const ConnectionConfig &c, link::DeviceInfo &info) {
        if (!current()) return false;
        const bool protectedBefore = info.passwordProtected;
        phase("clearing");
        uint8_t packet[17];
        link::clearMaterials(packet);
        if (!command(packet, sizeof(packet)) || !readInfo(info) ||
            info.screenType != 3 || info.passwordProtected != protectedBefore ||
            (protectedBefore && !verifySavedPassword(c))) {
            failure = "clear_failed";
            return false;
        }
        portENTER_CRITICAL(&guard);
        ++state.clears;
        portEXIT_CRITICAL(&guard);
        return true;
    }
    bool authenticate(const ConnectionConfig &c) {
        phase("identifying");
        link::DeviceInfo info{};
        if (!readInfo(info)) return false;
        if (info.screenType != 3) { failure = "unsupported_screen"; return false; }
        if (info.passwordProtected && !verifySavedPassword(c)) return false;
        if (c.resetOnConnect && !resetPreservingPassword(c, info)) return false;
        if (c.clearOnConnect && !clearStoredMaterials(c, info)) return false;
        if (!applyControls(c, true)) return false;
        portENTER_CRITICAL(&guard);
        state.authenticated = true;
        state.passwordProtected = info.passwordProtected;
        portEXIT_CRITICAL(&guard);
        return true;
    }
    bool showClock() {
        uint8_t sync[11], mode[8];
        const size_t timeSize = timePacket(sync);
        if (!timeSize) return false;
        link::clockMode(mode);
        if (!write(sync, timeSize) || !command(mode, sizeof(mode))) return false;
        portENTER_CRITICAL(&guard);
        state.nativeClockActive = true;
        ++state.clockSyncs;
        portEXIT_CRITICAL(&guard);
        return true;
    }
    bool applyControls(const ConnectionConfig &c, bool force = false) {
        Status applied;
        portENTER_CRITICAL(&guard);
        applied = state;
        portEXIT_CRITICAL(&guard);
        const uint8_t light[] = {5, 0, 4, 128, uint8_t(c.brightness)};
        const uint8_t rotation[] = {5, 0, 6, 128, uint8_t(c.rotate180)};
        const uint8_t power[] = {5, 0, 7, 1, uint8_t(c.screenOn)};
        if ((force || applied.brightness != c.brightness) && !command(light, sizeof(light)))
            return false;
        if ((force || applied.rotate180 != c.rotate180) && !command(rotation, sizeof(rotation)))
            return false;
        if ((force || applied.screenOn != c.screenOn) && !command(power, sizeof(power)))
            return false;
        portENTER_CRITICAL(&guard);
        state.brightness = c.brightness;
        state.rotate180 = c.rotate180;
        state.screenOn = c.screenOn;
        state.controlsApplied = true;
        portEXIT_CRITICAL(&guard);
        return true;
    }
    void close() {
        if (!radioLease) {
            clear(pending, sizeof(pending));
            return;
        }
        if (conn != BLE_HS_CONN_HANDLE_NONE) {
            ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
            conn = BLE_HS_CONN_HANDLE_NONE;
        } else if (ble_gap_conn_active())
            ble_gap_conn_cancel();
        live = false;
        xtouch_ble::pauseScan(false);
        portENTER_CRITICAL(&guard);
        state.connected = false;
        state.nativeClockActive = false;
        state.controlsApplied = false;
        state.authenticated = false;
        portEXIT_CRITICAL(&guard);
        xtouch_ble::clientActive(false);
        radioLease = false;
        clear(pending, sizeof(pending));
    }
};
struct Upload {
    Session &session;
    bool done = false;
    size_t total = 0, consumed = 0;
};
bool block(const uint8_t *data, size_t size, void *context) {
    auto &u = *static_cast<Upload *>(context);
    if (u.done)
        return true;
    auto &s = u.session;
    s.flush();
    const size_t fragments = (size + link::writeSize(s.mtu) - 1) / link::writeSize(s.mtu);
    uint32_t deadline = millis() + 5000 + uint32_t(fragments * 60);
    if (!s.live || !s.write(data, size, 0, deadline))
        return false;
    auto result = s.reply(1, 0, deadline, true);
    u.consumed += size - 16;
    // The panel may already hold this CRC and finish before all chunks are sent.
    if (result == link::Reply::Done) {
        u.done = true;
        return true;
    }
    if (result == link::Reply::Next && u.consumed < u.total)
        return true;
    s.failure = result == link::Reply::NoSpace    ? "no_space"
                : result == link::Reply::Rejected ? "upload_rejected"
                : result == link::Reply::Next     ? "upload_incomplete"
                                                  : "upload_timeout";
    return false;
}
void worker(void *) {
    unsigned failures = 0;
    uint32_t blockedRevision = UINT32_MAX;
    for (;;) {
        uint32_t version;
        auto c = config(&version);
        if (!c.enabled || !c.mac[0]) {
            phase("disabled");
            clear(c.password, sizeof(c.password));
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }
        if (version == blockedRevision) {
            clear(c.password, sizeof(c.password));
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }
        if (!xtouch_ble::ready()) {
            phase("waiting_radio");
            clear(c.password, sizeof(c.password));
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }
        ble_addr_t address{};
        portENTER_CRITICAL(&guard);
        bool found = addressSeen && uint32_t(millis() - seenAt) < 15000;
        if (found)
            address = seen;
        portEXIT_CRITICAL(&guard);
        if (!found) {
            phase("searching");
            clear(c.password, sizeof(c.password));
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }
        Session s;
        s.version = version;
        bool ok = s.connect(address) && s.authenticate(c);
        clear(c.password, sizeof(c.password));
        if (ok) {
            NativeHeapBuffer retainedMemory;
            PictureKeys previousKeys{};
            uint32_t previousCrc = 0, lastAt = 0;
            bool sent = false, nativeClock = false, forcePicture = false;
            uint32_t appliedClock = clockGeneration.load();
            uint32_t appliedSource = 0;
            phase("ready");
            while (s.live && s.current()) {
                Event e{};
                while (xQueueReceive(events, &e, 0) == pdTRUE) {
                    if (e.epoch == s.generation && e.kind == Kind::Disconnect) {
                        s.live = false;
                        s.failure = "disconnected";
                    }
                }
                if (!s.live)
                    break;
                if(appliedClock!=clockGeneration.load()){nativeClock=false;appliedClock=clockGeneration.load();}
                auto latest = config();
                clear(latest.password, sizeof(latest.password));
                if (!s.applyControls(latest))
                    break;
                c.interval = latest.interval;
                c.brightness = latest.brightness;
                c.screenOn = latest.screenOn;
                c.rotate180 = latest.rotate180;
                Snapshot value;
                portENTER_CRITICAL(&guard);
                value = desired;
                portEXIT_CRITICAL(&guard);
                unsigned margin = uxTaskGetStackHighWaterMark(nullptr);
                portENTER_CRITICAL(&guard);
                state.stack = margin;
                portEXIT_CRITICAL(&guard);
                uint32_t now = millis();
                if (!value.sourceReady) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                    continue;
                }
                bool changedSource = appliedSource != value.sourceToken;
                if (changedSource) {
                    appliedSource = value.sourceToken;
                    forcePicture = true;
                }
                auto plan = displayPlan(value, latest.idleDisplay, value.stateAgeMs);
                if (c.screenOn && plan.nativeClock) {
                    if (!nativeClock) {
                        if (!s.showClock()) {
                            if (!s.failure[0])
                                s.failure = "clock_time_unavailable";
                            break;
                        }
                        nativeClock = true;
                        failures = 0;
                        phase("ready");
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                    continue;
                }
                value = plan.picture;
                if (nativeClock && c.screenOn) {
                    nativeClock = false;
                    forcePicture = true;
                }
                const PictureKeys keys = pictureKeys(value);
                if (c.screenOn &&
                    link::uploadReady(sent, forcePicture || keys.full != previousKeys.full,
                                      forcePicture || keys.scene != previousKeys.scene,
                                      c.interval, now, lastAt,
                                      keys.progress == previousKeys.progress)) {
                    constexpr size_t transferScratch = 4112;
                    // PSRAM boards retain animation quality; internal-only MCUs use a
                    // bounded transient workspace, released between uploads.
                    NativeHeapBuffer transientMemory;
                    auto &memory = true ? retainedMemory : transientMemory;
                    size_t gifCapacity = 60000;
                    constexpr size_t stillCapacity = 2400;
                    if (!memory.ensure(gifCapacity + pixels + transferScratch))
                        gifCapacity = stillCapacity;
                    if (!memory.ensure(gifCapacity + pixels + transferScratch)) {
                        phase("memory_wait", "no_memory");
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        continue;
                    }
                    unsigned frames = animationFrames(value);
                    if (gifCapacity == stillCapacity) frames = 1;
                    size_t size = gif(value, frames, memory.data() + gifCapacity,
                                      memory.data(), gifCapacity,
                                      memory.data() + gifCapacity + pixels, transferScratch);
                    if (!size && frames > 1)
                        size = gif(value, 1, memory.data() + gifCapacity, memory.data(), gifCapacity,
                                   memory.data() + gifCapacity + pixels, transferScratch);
                    if (!size) {
                        s.failure = "render_failed";
                        break;
                    }
                    uint32_t crc = crc32(memory.data(), size);
                    if (forcePicture || !sent || crc != previousCrc) {
                        phase("uploading");
                        const uint32_t started = millis();
                        Upload transfer{s, false, size, 0};
                        const bool complete = upload(Format::Gif, memory.data(), size,
                                    memory.data() + gifCapacity + pixels, transferScratch, block, &transfer) &&
                                    transfer.done;
                        portENTER_CRITICAL(&guard);
                        state.lastUploadBytes = size;
                        state.lastUploadMs = millis() - started;
                        portEXIT_CRITICAL(&guard);
                        if (!complete) break;
                        portENTER_CRITICAL(&guard);
                        ++state.uploads;
                        state.nativeClockActive = false;
                        state.bytes += transfer.consumed;
                        portEXIT_CRITICAL(&guard);
                        previousCrc = crc;
                        failures = 0;
                    }
                    forcePicture = false;
                    previousKeys = keys;
                    lastAt = millis();
                    sent = true;
                    phase("ready");
                }
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }
        // A rejected GIF must not be hammered into a full or unhealthy panel.
        // Keep the display useful without resetting or deleting the user's assets.
        if (s.live && (!strcmp(s.failure, "upload_rejected") || !strcmp(s.failure, "no_space"))) {
            const char *failure = s.failure;
            s.showClock();
            s.failure = failure;
        }
        s.close();
        const char *reason = s.failure[0] ? s.failure : "disconnected";
        bool blocked =
            !strcmp(reason, "password_required") || !strcmp(reason, "password_rejected") ||
            !strcmp(reason, "password_unavailable") || !strcmp(reason, "authentication_unknown") ||
            !strcmp(reason, "unsupported_screen") || !strcmp(reason, "upload_rejected") ||
            !strcmp(reason, "no_space") || !strcmp(reason, "reset_failed") ||
            !strcmp(reason, "password_restore_failed") || !strcmp(reason, "clear_failed");
        if (blocked) {
            blockedRevision = version;
            phase(reason, reason);
        } else {
            phase("retry_wait", reason);
            uint32_t until = millis() + link::retryDelay(failures++);
            while (int32_t(until - millis()) > 0) {
                uint32_t v;
                auto updated = config(&v);
                clear(updated.password, sizeof(updated.password));
                if (v != version)
                    break;
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}
} // namespace
void connectionConfigure(const ConnectionConfig &c) {
    portENTER_CRITICAL(&guard);
    bool identityChanged = requested.enabled != c.enabled || strcmp(requested.mac, c.mac) ||
                           strcmp(requested.password, c.password) ||
                           requested.passwordUnavailable != c.passwordUnavailable ||
                           requested.resetOnConnect != c.resetOnConnect ||
                           requested.clearOnConnect != c.clearOnConnect;
    clear(requested.password, sizeof(requested.password));
    requested = c;
    if (identityChanged) {
        ++revision;
        addressSeen = false;
    }
    portEXIT_CRITICAL(&guard);
    xtouch_ble::demand(1, c.enabled && c.mac[0] && !c.passwordUnavailable);
}
void connectionSnapshot(const Snapshot &s) {
    portENTER_CRITICAL(&guard);
    desired = s;
    portEXIT_CRITICAL(&guard);
}
void connectionBegin() {
    if (workerTask)
        return;
    events = xQueueCreate(32, sizeof(Event));
    if (!events) {
        phase("error", "no_memory");
        return;
    }
    xtouch_ble::observer(1, observe);
    if (xTaskCreate(worker, "pixel-link", true ? 8192 : 6144, nullptr, 1, &workerTask) != pdPASS) {
        vQueueDelete(events);
        events = nullptr;
        phase("error", "no_memory");
    }
}
bool connectionScanStart() {
    if (!xtouch_ble::requestActiveScan())
        return false;
    portENTER_CRITICAL(&guard);
    for (auto &item : found)
        item = FoundDevice{};
    portEXIT_CRITICAL(&guard);
    return true;
}
void connectionScanStatus(JsonObject out) {
    FoundDevice copy[12];
    portENTER_CRITICAL(&guard);
    memcpy(copy, found, sizeof(copy));
    portEXIT_CRITICAL(&guard);
    out["radioReady"] = xtouch_ble::ready();
    out["remainingMs"] = xtouch_ble::scanRemainingMs();
    out["active"] = xtouch_ble::scanRemainingMs() != 0;
    JsonArray devices = out.createNestedArray("devices");
    for (unsigned pass = 0; pass < 12; ++pass) {
        int strongest = -1;
        for (unsigned i = 0; i < 12; ++i)
            if (copy[i].mac[0] && uint32_t(millis() - copy[i].seen) < 30000 &&
                (strongest < 0 || copy[i].rssi > copy[strongest].rssi))
                strongest = int(i);
        if (strongest < 0)
            break;
        auto &item = copy[strongest];
        JsonObject device = devices.createNestedObject();
        device["mac"] = item.mac;
        device["name"] = item.name;
        device["rssi"] = item.rssi;
        device["ageMs"] = uint32_t(millis() - item.seen);
        item.mac[0] = 0;
    }
}
void connectionStatus(JsonObject out) {
    portENTER_CRITICAL(&guard);
    Status copy = state;
    const unsigned interval = requested.interval;
    const bool resetOnConnect = requested.resetOnConnect;
    const bool clearOnConnect = requested.clearOnConnect;
    portEXIT_CRITICAL(&guard);
    out["renderTransport"] = "gif";
    out["lastUploadBytes"] = copy.lastUploadBytes;
    out["lastUploadMs"] = copy.lastUploadMs;
    out["transportReady"] = workerTask != nullptr;
    out["connectionState"] = copy.phase;
    out["connectionError"] = copy.failure;
    out["connected"] = copy.connected;
    out["authenticated"] = copy.authenticated;
    out["resetOnConnect"] = resetOnConnect;
    out["clearOnConnect"] = clearOnConnect;
    out["clears"] = copy.clears;
    out["resets"] = copy.resets;
    out["passwordRestores"] = copy.passwordRestores;
    out["lastResetMs"] = copy.lastResetMs;
    if (copy.authenticated) out["devicePasswordProtected"] = copy.passwordProtected;
    else out["devicePasswordProtected"] = nullptr;
    out["connectionAttempts"] = copy.attempts;
    out["uploads"] = copy.uploads;
    out["uploadedBytes"] = copy.bytes;
    out["mtu"] = copy.mtu;
    if (copy.stack)
        out["minimumStackBytes"] = copy.stack;
    else
        out["minimumStackBytes"] = nullptr;
    out["controlsApplied"] = copy.controlsApplied;
    if (copy.controlsApplied) {
        out["appliedBrightness"] = copy.brightness;
        out["appliedScreenOn"] = copy.screenOn;
        out["appliedRotate180"] = copy.rotate180;
    }
    out["gifCooldownMs"] = link::uploadGapMs(interval, false);
    out["detailCooldownMs"] = link::uploadGapMs(interval, false, true);
    out["stateCooldownMs"] = link::uploadGapMs(interval, true);
    out["clockSyncs"] = copy.clockSyncs;
    out["nativeClockActive"] = copy.nativeClockActive && copy.screenOn;
}
} // namespace pixel_screen
#endif
