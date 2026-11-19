#include "settings.h"
#include "head_motion.h"
#include "activity.h"
#include "display_preferences.h"
#include <atomic>
#include "connection.h"
#include "link_protocol.h"
#include "xtouch/vault.h"
#include "xtouch/storage.h"
#include "xtouch/types.h"
#include <cctype>
#include <cstdio>
#include <cstring>

namespace pixel_screen {
namespace {
bool enabled = false;
char selectedMac[18] = {};
unsigned interval = 2, brightness = 80;
bool screenOn = true, rotate180 = false;
bool resetOnConnect = false, clearOnConnect = false;
bool progressLayout = false, passwordSaved = false,
     passwordUnavailable = false;
Snapshot current;
SourceGate sourceGate;
HeadMotion headMotion;
uint32_t sourceNow = 0, stateAt = 0;
std::atomic<bool> displayReady{false}, displayOn{true};
std::atomic<int> pendingDisplay{-1};
bool encryptedPassword = true;
IdleDisplay idleDisplay = IdleDisplay::Clock;
bool macValid(const char *mac) {
    if (!mac || strlen(mac) != 17)
        return false;
    for (unsigned i = 0; i < 17; ++i)
        if (i % 3 == 2 ? mac[i] != ':' : !isxdigit(static_cast<unsigned char>(mac[i])))
            return false;
    bool zero = true, broadcast = true;
    for (unsigned i = 0; i < 17; ++i)
        if (i % 3 != 2) {
            zero = zero && mac[i] == '0';
            broadcast = broadcast && toupper(static_cast<unsigned char>(mac[i])) == 'F';
        }
    return !zero && !broadcast;
}
void apply(JsonVariantConst input) {
    enabled = input["enabled"] | false;
    interval = input["interval"] | 2;
    if (interval > 5)
        interval = 5;
    progressLayout = !strcmp(input["layout"] | "printer", "progress");
    const char *mac = input["mac"] | "";
    for (unsigned i = 0; i < sizeof(selectedMac); ++i)
        selectedMac[i] = 0;
    for (unsigned i = 0; i < 17 && mac[i]; ++i)
        selectedMac[i] = toupper(static_cast<unsigned char>(mac[i]));
    String secret;
    bool sealed = input.containsKey("passwordEnc");
    bool plain = input.containsKey("passwordPlain");
    passwordSaved = sealed || plain;
    encryptedPassword = !plain;
    if (plain)
        secret = input["passwordPlain"] | "";
    passwordUnavailable =
        passwordSaved && ((sealed && !xtouch_vault_open(input["passwordEnc"] | "",
                                                        String("pixel:") + selectedMac, secret)) ||
                          !link::passwordValid(secret.c_str()));
    ConnectionConfig config;
    config.enabled = enabled;
    config.interval = interval;
    brightness = input["brightness"] | 80u;
    screenOn = input["screenOn"] | true;
    rotate180 = input["rotate180"] | false;
    config.brightness = brightness;
    config.screenOn = screenOn;
    config.rotate180 = rotate180;
    config.resetOnConnect = resetOnConnect = input["resetOnConnect"] | false;
    config.clearOnConnect = clearOnConnect = input["clearOnConnect"] | false;
    idleDisplay = idleDisplayFromName(input["idleDisplay"] | "clock");
    config.idleDisplay = idleDisplay;
    pendingDisplay = -1;
    displayOn = screenOn;
    displayReady = enabled && selectedMac[0] && !passwordUnavailable;
    config.passwordUnavailable = passwordUnavailable;
    memcpy(config.mac, selectedMac, sizeof(selectedMac));
    if (passwordSaved && !passwordUnavailable)
        memcpy(config.password, secret.c_str(), 6);
    for (auto &c : secret)
        c = 0;
    secret.clear();
    connectionConfigure(config);
    volatile char *wipe = config.password;
    for (unsigned i = 0; i < sizeof(config.password); ++i)
        wipe[i] = 0;
}
} // namespace
bool valid(JsonVariantConst input, bool legacy) {
    if (!input.is<JsonObjectConst>())
        return false;
    if (input.size() == 0)
        return true;
    const char *mac = input["mac"] | "";
    return input["enabled"].is<bool>() && input["mac"].is<const char *>() &&
           (!mac[0] ? (!input["enabled"].as<bool>() && !input.containsKey("passwordEnc") &&
                       !input.containsKey("passwordPlain"))
                    : macValid(mac)) &&
           input["interval"].is<unsigned>() && input["interval"].as<unsigned>() >= 1 &&
           input["interval"].as<unsigned>() <= (legacy ? 60u : 5u) &&
           (!input.containsKey("passwordEnc") ||
            (input["passwordEnc"].is<const char *>() &&
             !strncmp(input["passwordEnc"] | "", "v1:", 3) &&
             strlen(input["passwordEnc"] | "") >= 40 && strlen(input["passwordEnc"] | "") < 256)) &&
           (!input.containsKey("passwordPlain") ||
            (input["passwordPlain"].is<const char *>() &&
             link::passwordValid(input["passwordPlain"] | ""))) &&
           !(input.containsKey("passwordPlain") && input.containsKey("passwordEnc")) &&
           !input.containsKey("password") && !input.containsKey("clearPassword") &&
           !input.containsKey("encrypt") &&
           (!input.containsKey("brightness") ||
            (input["brightness"].is<unsigned>() && input["brightness"].as<unsigned>() >= 5 &&
             input["brightness"].as<unsigned>() <= 100)) &&
           (!input.containsKey("screenOn") || input["screenOn"].is<bool>()) &&
           (!input.containsKey("rotate180") || input["rotate180"].is<bool>()) &&
           (!input.containsKey("resetOnConnect") || input["resetOnConnect"].is<bool>()) &&
           (!input.containsKey("clearOnConnect") || input["clearOnConnect"].is<bool>()) &&
           (!input.containsKey("idleDisplay") ||
            (input["idleDisplay"].is<const char *>() && idleDisplayValid(input["idleDisplay"]))) &&
           (!input.containsKey("layout") ||
            (input["layout"].is<const char *>() && printLayoutValid(input["layout"])));
}
void begin() {
    auto doc = xtouch_storage_read(config_path, 0);
    if (valid(doc.as<JsonVariantConst>(), true))
        apply(doc.as<JsonVariantConst>());
    connectionBegin();
}
bool store(JsonVariantConst input) {
    if (!valid(input, true))
        return false;
    if (input.size() == 0) {
        if (!xtouch_storage_delete(config_path))
            return false;
        apply(input);
        return true;
    }
    StaticJsonDocument<768> doc;
    doc["enabled"] = input["enabled"];
    char canonical[18] = {};
    const char *mac = input["mac"] | "";
    for (unsigned i = 0; i < 17 && mac[i]; ++i)
        canonical[i] = toupper(static_cast<unsigned char>(mac[i]));
    doc["mac"] = canonical;
    unsigned seconds = input["interval"] | 2;
    doc["interval"] = seconds > 5 ? 5 : seconds;
    doc["layout"] = input["layout"] | "printer";
    for (const char *key : {"brightness", "screenOn", "rotate180", "idleDisplay", "resetOnConnect", "clearOnConnect"})
        if (input.containsKey(key))
            doc[key] = input[key];
    if (input.containsKey("passwordEnc"))
        doc["passwordEnc"] = input["passwordEnc"];
    if (input.containsKey("passwordPlain"))
        doc["passwordPlain"] = input["passwordPlain"];
    if (doc.overflowed() || !xtouch_storage_write(config_path, doc))
        return false;
    apply(doc.as<JsonVariantConst>());
    return true;
}
bool save(JsonVariantConst input) {
    if (!input.is<JsonObjectConst>())
        return false;
    if (input.size() == 0)
        return store(input);
    if (input.containsKey("buzzerEnabled") || input.containsKey("volume"))
        return false;
    if (input.containsKey("passwordEnc") || input.containsKey("passwordPlain"))
        return false;
    if (input.containsKey("password") && !input["password"].is<const char *>())
        return false;
    if (input.containsKey("clearPassword") && !input["clearPassword"].is<bool>())
        return false;
    if (input.containsKey("encrypt") && !input["encrypt"].is<bool>())
        return false;
    const char *typed = input["password"] | "";
    bool clear = input["clearPassword"] | false;
    if ((typed[0] && !link::passwordValid(typed)) || (clear && typed[0]))
        return false;
    StaticJsonDocument<768> doc;
    doc.set(input);
    doc.remove("password");
    doc.remove("clearPassword");
    doc.remove("encrypt");
    if (!valid(doc.as<JsonVariantConst>()))
        return false;
    char canonical[18] = {};
    const char *mac = doc["mac"] | "";
    if (typed[0] && !mac[0])
        return false;
    for (unsigned i = 0; i < 17 && mac[i]; ++i)
        canonical[i] = toupper(static_cast<unsigned char>(mac[i]));
    doc["mac"] = canonical;
    auto previous = xtouch_storage_read(config_path, 0);
    bool same = !strcmp(previous["mac"] | "", canonical);
    bool wasPlain = same && previous["passwordPlain"].is<const char *>();
    bool protect = input.containsKey("encrypt") ? input["encrypt"].as<bool>() : !wasPlain;
    String secret = typed;
    if (!typed[0] && !clear && same) {
        if (wasPlain)
            secret = previous["passwordPlain"] | "";
        else if (previous["passwordEnc"].is<const char *>()) {
            if (protect)
                doc["passwordEnc"] = previous["passwordEnc"];
            else if (!xtouch_vault_open(previous["passwordEnc"] | "", String("pixel:") + canonical,
                                        secret))
                return false;
        }
    }
    if (!clear && !secret.empty()) {
        if (!link::passwordValid(secret.c_str()))
            return false;
        if (protect) {
            String sealed = xtouch_vault_seal(secret, String("pixel:") + canonical);
            if (sealed.empty())
                return false;
            doc["passwordEnc"] = sealed;
        } else
            doc["passwordPlain"] = secret;
    }
    for (auto &c : secret)
        c = 0;
    secret.clear();
    if (same)
        for (const char *key : {"brightness", "screenOn", "rotate180", "idleDisplay", "resetOnConnect", "clearOnConnect"})
            if (!doc.containsKey(key) && previous.containsKey(key))
                doc[key] = previous[key];
    return store(doc.as<JsonVariantConst>());
}
const char *stateName(State value) {
    static const char *names[] = {"offline",  "idle",      "preparing", "printing",   "paused",
                                  "complete", "cancelled", "error",     "environment"};
    unsigned index = static_cast<unsigned>(value);
    return names[index < 9 ? index : 0];
}
void source(unsigned slot, const char *identity, bool switching, uint32_t now) {
    uint32_t token = 2166136261u ^ slot;
    if (identity)
        for (const unsigned char *p = reinterpret_cast<const unsigned char *>(identity); *p; ++p)
            token = (token ^ *p) * 16777619u;
    sourceNow = now;
    if (current.sourceToken != token) {
        stateAt = now;
        headMotion.reset();
    }
    current.stateAgeMs = uint32_t(now - stateAt);
    current.sourceToken = token;
    current.sourceReady = sourceGate.observe(token, switching, now);
}
void update(bool online, int printState, int percent, bool fault, bool preparing, int currentLayer,
            int totalLayers, int stage, uint32_t filamentColor, uint32_t feedColor,
            bool filamentKnown, int amsMain, int currentTray, int targetTray, bool chamberHeating) {
    const auto previousActivity = activityFor(current);
    State state = State::Idle;
    if (!online)
        state = State::Offline;
    else if (fault)
        state = State::Error;
    else
        switch (printState) {
        case XTOUCH_PRINT_STATUS_RUNNING:
            state = preparing ? State::Preparing : State::Printing;
            break;
        case XTOUCH_PRINT_STATUS_PREPARE:
            state = State::Preparing;
            break;
        case XTOUCH_PRINT_STATUS_PAUSED:
            state = State::Paused;
            break;
        case XTOUCH_PRINT_STATUS_FINISHED:
            state = State::Complete;
            break;
        case XTOUCH_PRINT_STATUS_FAILED:
            state = State::Cancelled;
            break;
        default:
            break;
        }
    if (current.state != state) {
        stateAt = sourceNow;
        current.stateAgeMs = 0;
    }
    current.state = state;
    current.percent = uint8_t(percent < 0 ? 0 : percent > 100 ? 100 : percent);
    current.currentLayer = online ? currentLayer : -1;
    current.totalLayers = online ? totalLayers : 0;
    current.stage = online ? stage : -1;
    current.amsMain = online ? amsMain : -1;
    current.filamentColor = online ? filamentColor : 0;
    current.feedColor = online ? feedColor : 0;
    current.filamentKnown = online && filamentKnown;
    current.chamberHeating = online && chamberHeating;
    current.headParked = headMotion.observe(current, currentTray, targetTray);
    if (activityFor(current) != previousActivity) {
        stateAt = sourceNow;
        current.stateAgeMs = 0;
    }
}
void telemetry(const char *model, const char *clock, int leftMinutes, int nozzle, int bed,
               bool known, double temperature, double humidity) {
    for (unsigned i = 0; i < sizeof(current.model); ++i)
        current.model[i] = 0;
    if (model)
        for (unsigned i = 0; i < 6 && model[i]; ++i)
            current.model[i] = toupper(static_cast<unsigned char>(model[i]));
    if (!current.model[0])
        strcpy(current.model, "--");
    snprintf(current.clock, sizeof(current.clock), "%s",
             clock && strlen(clock) == 5 ? clock : "--:--");
    bool online = current.state != State::Offline;
    current.leftMinutes = online ? leftMinutes : -1;
    current.nozzle = online ? nozzle : -1;
    current.bed = online ? bed : -1;
    current.environmentKnown = known;
    current.temperature = temperature;
    current.humidity = humidity;
    connectionSnapshot(snapshot());
}
Snapshot snapshot() {
    auto value = current;
    value.progressLayout = progressLayout;
    return value;
}
void status(JsonObject out) {
    out["enabled"] = enabled;
    out["mac"] = selectedMac;
    out["interval"] = interval;
    out["brightness"] = brightness;
    out["screenOn"] = displayOn.load();
    out["displayControlAvailable"] = displayReady.load();
    out["encrypted"] = encryptedPassword;
    out["rotate180"] = rotate180;
    out["idleDisplay"] = idleDisplayName(idleDisplay);
    out["resetOnConnect"] = resetOnConnect;
    out["clearOnConnect"] = clearOnConnect;
    out["sourceSettling"] = !current.sourceReady;
    out["sourceSettleMs"] = 3000;
    connectionStatus(out);
    out["passwordSaved"] = passwordSaved;
    out["passwordUnavailable"] = passwordUnavailable;
    out["state"] = stateName(current.state);
    out["effectiveState"] =
        stateName(displayPlan(current, idleDisplay, current.stateAgeMs).picture.state);
    out["stateAgeMs"] = current.stateAgeMs;
    out["terminalHoldMs"] = 30000;
    out["percent"] = current.percent;
    out["stage"] = current.stage;
    out["amsMain"] = current.amsMain;
    out["headParked"] = current.headParked;
    out["chamberHeating"] = current.chamberHeating;
    out["currentLayer"] = current.currentLayer;
    out["totalLayers"] = current.totalLayers;
    out["filamentColor"] = current.filamentColor;
    out["feedColor"] = current.feedColor;
    out["filamentKnown"] = current.filamentKnown;
    out["width"] = 32;
    out["height"] = 32;
    out["layout"] = progressLayout ? "progress" : "printer";
    out["model"] = current.model;
    out["clock"] = current.clock;
    out["leftMinutes"] = current.leftMinutes;
    out["nozzle"] = current.nozzle;
    out["bed"] = current.bed;
    out["environmentAvailable"] = environmentAvailable(current);
    if (environmentAvailable(current)) {
        out["temperature"] = current.temperature;
        out["humidity"] = current.humidity;
    }
}
} // namespace pixel_screen

extern "C" bool xtouch_pixel_display_get(bool *on) {
    int pending = pixel_screen::pendingDisplay.load();
    if (on)
        *on = pending >= 0 ? pending != 0 : pixel_screen::displayOn.load();
    return pixel_screen::displayReady.load();
}
extern "C" bool xtouch_pixel_display_request(bool on) {
    if (!pixel_screen::displayReady.load())
        return false;
    pixel_screen::pendingDisplay = on ? 1 : 0;
    return true;
}
extern "C" void xtouch_pixel_display_poll(void) {
    int pending = pixel_screen::pendingDisplay.exchange(-1);
    if (pending < 0)
        return;
    auto doc = xtouch_storage_read(pixel_screen::config_path, 0);
    if (!(doc["enabled"] | false) || !pixel_screen::valid(doc.as<JsonVariantConst>(), true))
        return;
    doc["screenOn"] = pending != 0;
    if (!pixel_screen::store(doc.as<JsonVariantConst>()))
        printf("Pixel display preference save failed\n");
}
