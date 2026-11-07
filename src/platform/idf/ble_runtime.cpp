#include "ble_runtime.h"
#if XTOUCH_FEATURE_XIAOMI_BLE || XTOUCH_FEATURE_PIXEL_SCREEN
#include "runtime.h"
#include <esp_heap_caps.h>
#include <atomic>
#include <mutex>
#include <nimble/nimble_port.h>
#include <host/ble_hs.h>
#include <host/util/util.h>
#if !CONFIG_IDF_TARGET_ESP32P4
#include <esp_bt.h>
#endif
#if XTOUCH_FEATURE_BLE_PROVISIONING
#include "ble_provisioning.h"
#endif
#if CONFIG_IDF_TARGET_ESP32P4
#include "hosted_ble_binding.h"
#endif
namespace xtouch_ble {
namespace {
std::atomic<bool> started{false}, synced{false}, paused{false}, transition{false},
    hostRunning{false}, client{false};
std::atomic<bool> consumers[2] = {};
std::atomic<unsigned> starts{0}, stops{0};
std::atomic<uint32_t> transitionAt{0}, syncWaitAt{0};
uint32_t lastNeeded = 0, lifecycleRetry = 0;
std::atomic<uint32_t> scanRetry{0};
TaskHandle_t hostTask = nullptr;
std::atomic<int> lastError{0};
std::atomic<uint8_t> ownType{0};
std::atomic<bool> cleanupRequired{false};
bool hostAllocated = false;
std::atomic<bool> restartRequired{false};
#if CONFIG_IDF_TARGET_ESP32P4
bool hostedBound = false;
#endif
Observer listeners[2] = {};
std::mutex scanGuard;
std::atomic<uint32_t> activeUntil{0};
bool activeMode = false;
portMUX_TYPE guard = portMUX_INITIALIZER_UNLOCKED;
int gap(ble_gap_event *event, void *) {
    Observer copy[2];
    portENTER_CRITICAL(&guard);
    copy[0] = listeners[0];
    copy[1] = listeners[1];
    portEXIT_CRITICAL(&guard);
    for (auto callback : copy)
        if (callback)
            callback(event, nullptr);
    return 0;
}
void sync() {
    uint8_t type = 0;
    int e = ble_hs_util_ensure_addr(0);
    if (!e)
        e = ble_hs_id_infer_auto(0, &type);
    ownType = type;
    lastError = e;
    synced = e == 0;
}
void reset(int reason) {
    synced = false;
    syncWaitAt = millis();
    lastError = reason;
}
void host(void *) {
    hostRunning = true;
    nimble_port_run();
    hostRunning = false;
    // The lifecycle task deletes this suspended host before freeing NimBLE.
    vTaskSuspend(nullptr);
}
bool shutdown();
void initialize() {
    int e = 0;
#if CONFIG_IDF_TARGET_ESP32P4
    e = xtouch_hosted_ble_bind();
    hostedBound = e == 0;
#endif
    if (!e) {
        e = nimble_port_init();
        // Partial SDK initialization is not safely re-entrant; avoid an assertion loop.
        if (e)
            restartRequired = true;
    }
    lastError = e;
    if (e) {
        cleanupRequired = true;
        shutdown();
        lastError = e;
        return;
    }
    hostAllocated = true;
    cleanupRequired = false;
    scanRetry = 0;
    ble_hs_cfg.sync_cb = sync;
    ble_hs_cfg.reset_cb = reset;
    hostRunning = true;
    syncWaitAt = millis();
    if (xTaskCreatePinnedToCore(host, "nimble_host", NIMBLE_HS_STACK_SIZE, nullptr,
                                configMAX_PRIORITIES - 4, &hostTask, NIMBLE_CORE) != pdPASS) {
        hostRunning = false;
        cleanupRequired = true;
        shutdown();
        lastError = ESP_ERR_NO_MEM;
        return;
    }
    ++starts;
}
void initTask(void *) {
    initialize();
    if (hostAllocated && !cleanupRequired)
        lifecycleRetry = 0;
    transition = false;
    vTaskDelete(nullptr);
}
bool shutdown() {
    synced = false;
    {
        std::lock_guard<std::mutex> lock(scanGuard);
        paused = true;
        if (hostAllocated)
            ble_gap_disc_cancel();
    }
    int e = hostRunning ? nimble_port_stop() : 0;
    uint32_t until = millis() + 2000;
    while (!e && hostRunning && int32_t(until - millis()) > 0)
        vTaskDelay(pdMS_TO_TICKS(1));
    if (!e && hostRunning)
        e = ESP_ERR_TIMEOUT;
    if (!e && hostTask) {
        vTaskDelete(hostTask);
        hostTask = nullptr;
    }
    // Deinitialize in the SDK's order, retaining completed stages so an error
    // can be retried without freeing the same host twice.
    if (!e && hostAllocated) {
        e = esp_nimble_deinit();
        if (!e)
            hostAllocated = false;
    }
#if CONFIG_IDF_TARGET_ESP32P4
    if (!e && hostedBound) {
        xtouch_hosted_ble_unbind();
        hostedBound = false;
    }
#else
    if (!e && esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED)
        e = esp_bt_controller_disable();
    if (!e && esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
        e = esp_bt_controller_deinit();
#endif
    lastError = e;
    cleanupRequired = e != 0;
    if (!e) {
        started = false;
        ++stops;
        activeMode = false;
        paused = false;
    }
    return e == 0;
}
bool demanded() {
    bool needed = consumers[0] || consumers[1] || scanRemainingMs();
#if XTOUCH_FEATURE_BLE_PROVISIONING
    if (board_ble_busy())
        needed = false;
#endif
    return needed;
}
void stopTask(void *) {
    // A new binding may have arrived after poll queued this task.
    if (!client && (cleanupRequired || !demanded()))
        shutdown();
    if (!started)
        lifecycleRetry = 0;
    transition = false;
    vTaskDelete(nullptr);
}
} // namespace
void observer(unsigned slot, Observer callback) {
    if (slot >= 2)
        return;
    portENTER_CRITICAL(&guard);
    listeners[slot] = callback;
    portEXIT_CRITICAL(&guard);
}
void begin() {
    if (restartRequired || started || transition.exchange(true))
        return;
    transitionAt = millis();
    started = true;
    paused = false;
    if (xTaskCreate(initTask, "ble-init", 6144, nullptr, 2, nullptr) != pdPASS) {
        lastError = ESP_ERR_NO_MEM;
        started = false;
        transition = false;
    }
}
void demand(unsigned slot, bool enabled) {
    if (slot < 2)
        consumers[slot] = enabled;
}
bool clientActive(bool active) {
    std::lock_guard<std::mutex> lock(scanGuard);
    if (active && (!synced || transition || cleanupRequired || client))
        return false;
    client = active;
    return true;
}
bool initialized() {
    return started;
}
void status(JsonObject out) {
    out["initialized"] = started.load();
    out["ready"] = synced.load();
    out["transitionMs"] = transition ? uint32_t(millis() - transitionAt) : 0;
    out["idleGraceMs"] = 5000;
    out["transitioning"] = transition.load();
    out["scanning"] = scanning();
    out["demandMask"] = unsigned(consumers[0]) + 2 * unsigned(consumers[1]);
    out["scanWindowMs"] = scanRemainingMs();
    out["starts"] = starts.load();
    out["stops"] = stops.load();
    out["error"] = lastError.load();
    out["cleanupRequired"] = cleanupRequired.load();
    out["restartRequired"] = restartRequired.load();
#if CONFIG_IDF_TARGET_ESP32P4
    out["controllerState"] = started ? "host_bound" : "host_unbound";
    out["remoteControllerManaged"] = false;
    out["scope"] = "host-and-scan";
    out["controllerStateVerified"] = false;
#else
    out["controllerState"] = int(esp_bt_controller_get_status());
    out["controllerStateVerified"] = true;
#endif
}

bool ready() {
    return synced && !transition;
}
bool scanning() {
    std::lock_guard<std::mutex> lock(scanGuard);
    return synced && !transition && ble_gap_disc_active();
}
int error() {
    return lastError;
}
uint8_t addressType() {
    return ownType;
}
bool pauseScan(bool pause) {
    std::lock_guard<std::mutex> lock(scanGuard);
    paused = pause;
    if (!pause) {
        scanRetry = 0;
        return true;
    }
    if (!synced)
        return false;
    int e = ble_gap_disc_cancel();
    return e == 0 || e == BLE_HS_EALREADY;
}
uint32_t scanRemainingMs() {
    uint32_t until = activeUntil.load();
    if (!until)
        return 0;
    int32_t left = int32_t(until - millis());
    if (left > 0)
        return uint32_t(left);
    activeUntil.compare_exchange_strong(until, 0);
    return 0;
}
bool requestActiveScan() {
    std::lock_guard<std::mutex> lock(scanGuard);
    if (paused || scanRemainingMs())
        return false;
    // Main-loop poll owns mode changes; HTTP only requests a bounded window.
    uint32_t until = millis() + 10000;
    activeUntil = until ? until : 1;
    return true;
}
void poll() {
    bool needed = demanded();
    uint32_t tick = millis();
    if (needed)
        lastNeeded = tick;
    if (transition) {
        if (uint32_t(tick - transitionAt) > 10000)
            lastError = ESP_ERR_TIMEOUT;
        return;
    }
    if (!started) {
        if (!needed)
            lifecycleRetry = 0;
        if (needed && (!lifecycleRetry || int32_t(tick - lifecycleRetry) >= 0)) {
            lifecycleRetry = tick + 2000;
            begin();
        }
        return;
    }
    if (!synced && hostAllocated && !cleanupRequired && uint32_t(tick - syncWaitAt) > 10000) {
        if (!lastError)
            lastError = ESP_ERR_TIMEOUT;
        cleanupRequired = true;
    }
    bool stop = false;
    {
        // A client owns the host until its bounded connect/upload/close path exits.
        // Acquire and retirement share this guard; no new client enters after stop is queued.
        std::lock_guard<std::mutex> lock(scanGuard);
        if (!client && (cleanupRequired || (!needed && uint32_t(tick - lastNeeded) >= 5000)) &&
            (!lifecycleRetry || int32_t(tick - lifecycleRetry) >= 0)) {
            transitionAt = tick;
            transition = true;
            lifecycleRetry = tick + 3000;
            stop = true;
        }
    }
    if (stop) {
        if (xTaskCreate(stopTask, "ble-stop", 6144, nullptr, 2, nullptr) != pdPASS) {
            lastError = ESP_ERR_NO_MEM;
            transition = false;
        }
        return;
    }
    if (!needed) {
        std::lock_guard<std::mutex> lock(scanGuard);
        if (synced && ble_gap_disc_active())
            ble_gap_disc_cancel();
        return;
    }
    std::lock_guard<std::mutex> lock(scanGuard);
    if (!synced || paused)
        return;
    bool active = scanRemainingMs() != 0;
    if (ble_gap_disc_active()) {
        if (activeMode == active)
            return;
        if (ble_gap_disc_cancel())
            return;
        scanRetry = 0;
    }
    uint32_t now = millis();
    if (scanRetry && int32_t(now - scanRetry) < 0)
        return;
    ble_gap_disc_params p = {};
    p.passive = active ? 0 : 1;
    p.itvl = 160;
    p.window = 48;
    p.filter_duplicates = 0;
    int e = ble_gap_disc(ownType, BLE_HS_FOREVER, &p, gap, nullptr);
    lastError = e;
    if (!e)
        activeMode = active;
    scanRetry = now + (e ? 2000 : 200);
}
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION && CONFIG_IDF_TARGET_ESP32
bool testStop() {
    consumers[0] = false;
    consumers[1] = false;
    activeUntil = 0;
    return shutdown();
}
#endif
} // namespace xtouch_ble
#endif
