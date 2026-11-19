#include "capabilities.h"
#include "config.h"
#include "network/idf_mqtt.h"
extern XTouchIdfMqtt xtouch_pubSubClient;
#include "diagnostics/loop_timing.h"
#include "diagnostics/mqtt_timing.h"
#include "diagnostics/display_stats.h"
#include "diagnostics/stack_margin.h"
#include "network/task_budget.h"
#include "xtouch/types.h"
#include "sound/events.h"
#include "indicator/indicator.h"
#include "power/deep_sleep.h"
#include "camera/camera.h"
#include <esp_ota_ops.h>
void xtouch_feature_diagnostics(JsonDocument &d) {
    XTouchStackMargin::logPendingWarnings();
    auto ota = d.createNestedObject("ota");
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *boot = esp_ota_get_boot_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
    auto partitionInfo = [&](const char *key, const esp_partition_t *partition) {
        if (!partition) {
            ota[key] = nullptr;
            return;
        }
        auto info = ota.createNestedObject(key);
        info["label"] = partition->label;
        info["address"] = partition->address;
        info["size"] = partition->size;
    };
    partitionInfo("running", running);
    partitionInfo("boot", boot);
    partitionInfo("next", next);
    esp_ota_img_states_t otaState;
    if (running && esp_ota_get_state_partition(running, &otaState) == ESP_OK)
        ota["state"] = unsigned(otaState);
    else
        ota["state"] = nullptr;
    auto display = xtouch_board_display_stats();
    auto drawing = d.createNestedObject("displayTiming");
    drawing["available"] = display.available;
    if (display.available) {
        drawing["dma"] = display.dma;
        drawing["includesFirstTile"] = display.includesFirstTile;
        drawing["flushes"] = display.flushes;
        drawing["frames"] = display.frames;
        drawing["transactions"] = display.transactions;
        drawing["wireBytes"] = display.wireBytes;
        drawing["lastFrameUs"] = display.lastFrameUs;
        drawing["maxFrameUs"] = display.maxFrameUs;
        drawing["bufferBytes"] = display.bufferBytes;
        drawing["failedTransfers"] = display.failedTransfers;
    }
    d["runtime"] = "esp-idf";
    d["mainCore"] = xPortGetCoreID();
    auto margins = d.createNestedObject("stackMargins");
    margins["unit"] = "bytes";
    margins["scope"] = "task-lifetime minimum sampled in path";
    margins["advisoryOnly"] = true;
    margins["previewBudget"] = XTouchNetworkBudgets::PreviewWorkerStackBytes;
    margins["cameraBudget"] = XTouchNetworkBudgets::CameraWorkerStackBytes;
    unsigned measuredMask = 0, lowMarginMask = 0;
    for (unsigned i = 0; i < unsigned(XTouchStackMargin::Path::Count); ++i) {
        auto path = static_cast<XTouchStackMargin::Path>(i);
        if ((!XTOUCH_FEATURE_PRINT_PREVIEW && (path == XTouchStackMargin::Path::CloudPreview ||
                                               path == XTouchStackMargin::Path::LocalPreview)) ||
            (!XTOUCH_FEATURE_CAMERA &&
             (path == XTouchStackMargin::Path::H264 || path == XTouchStackMargin::Path::Jpeg)))
            continue;
        auto &counter = XTouchStackMargin::counter(path);
        if (counter.samples.load()) {
            margins[XTouchStackMargin::name(path)] = counter.minimum.load();
            measuredMask |= 1u << i;
        } else
            margins[XTouchStackMargin::name(path)] = nullptr;
        if (counter.warned.load())
            lowMarginMask |= 1u << i;
    }
    margins["measuredMask"] = measuredMask;
    margins["lowMarginMask"] = lowMarginMask;
    d["mainStackFree"] = uxTaskGetStackHighWaterMark(nullptr);
#if defined(XTOUCH_BOARD_P4) && INCLUDE_xTaskGetHandle
    if (auto rpc = xTaskGetHandle("eh_rpc_rx"))
        d["rpcStackFree"] = uxTaskGetStackHighWaterMark(rpc);
#endif
    auto mqtt = d.createNestedObject("mqttTiming");
    mqtt["attempts"] = xtouch_mqtt_timing.attempts;
    mqtt["transportMs"] = xtouch_mqtt_timing.transportMs;
    mqtt["loginMs"] = xtouch_mqtt_timing.loginMs;
    mqtt["subscribeMs"] = xtouch_mqtt_timing.subscribeMs;
    mqtt["totalMs"] = xtouch_mqtt_timing.totalMs;
    mqtt["cpuBeforeMHz"] = xtouch_mqtt_timing.cpuBeforeMHz;
    mqtt["cpuWorkMHz"] = xtouch_mqtt_timing.cpuWorkMHz;
    mqtt["state"] = xtouch_mqtt_timing.state;
    mqtt["transportReady"] = xtouch_mqtt_timing.transportReady;
    mqtt["connected"] = xtouch_pubSubClient.connected();
    mqtt["attemptConnected"] = xtouch_mqtt_timing.connected;
    auto report = xtouch_mqtt_report_snapshot();
    mqtt["statusReceived"] = report.received;
    mqtt["statusExpired"] = report.expired;
    if (report.received)
        mqtt["reportAgeMs"] = report.ageMs;
    else
        mqtt["reportAgeMs"] = nullptr;
    const char *phases[] = {"gui", "portal", "mqtt", "mqttConnect", "mqttReceive", "preview", "sd"};
    auto timing = d.createNestedObject("loopTiming");
    for (unsigned n = 0; n < XTOUCH_PHASE_COUNT; ++n)
        timing[phases[n]] = xtouch_loop_timing.maximum[n];
    timing["lastPhase"] = xtouch_loop_timing.lastAt ? phases[xtouch_loop_timing.lastPhase] : "none";
    timing["lastMs"] = xtouch_loop_timing.lastMs;
    timing["lastAgeMs"] = xtouch_loop_timing.lastAt ? millis() - xtouch_loop_timing.lastAt : 0;
    auto receive = xtouch_pubSubClient.stats();
    auto rx = d.createNestedObject("mqttReceive");
    rx["events"] = receive.dataEvents;
    rx["queued"] = receive.queuedFrames;
    rx["delivered"] = receive.deliveredFrames;
    rx["queueDrops"] = receive.droppedFrames;
    rx["rejected"] = receive.rejectedFrames;
    rx["allocationFailures"] = receive.allocationFailures;
    rx["published"] = receive.published;
    rx["outBufferBytes"] = xtouch_pubSubClient.outBufferSize();
    auto c = d.createNestedObject("capabilities");
    c["cloudAccountLogin"] = bool(XTOUCH_FEATURE_CLOUD_LOGIN);
    c["cloudTokenEntry"] = bool(XTOUCH_FEATURE_CLOUD_TOKEN);
    auto audio = xtouch_sound_status();
    c["audio"] = bool(XTOUCH_FEATURE_AUDIO);
    c["indicator"] = bool(XTOUCH_FEATURE_INDICATOR && XTOUCH_HAS_RGB_LED);
    auto indicator = d.createNestedObject("indicatorState");
    indicator["ready"] = xtouch_indicator_available();
    indicator["rgb"] = xtouch_indicator_color();
    c["deepSleep"] = bool(XTOUCH_FEATURE_DEEP_SLEEP);
    c["deepTouchWake"] = bool(XTOUCH_FEATURE_DEEP_SLEEP && XTOUCH_HAS_DEEP_TOUCH_WAKE);
    c["deepWakeButton"] = XTOUCH_FEATURE_DEEP_SLEEP ? XTOUCH_DEEP_WAKE_BUTTON_NAME : "";
    c["camera"] = bool(XTOUCH_FEATURE_CAMERA);
    c["printPreview"] = bool(XTOUCH_FEATURE_PRINT_PREVIEW);
    c["screenWidth"] = XTOUCH_DISPLAY_WIDTH;
    c["screenHeight"] = XTOUCH_DISPLAY_HEIGHT;
    c["iconStyle"] = XTOUCH_ICON_STYLE ? "mdi" : "original";
#if XTOUCH_FEATURE_CAMERA
    c["cameraAvailable"] = xtouch_camera_available();
#else
    c["cameraAvailable"] = false;
#endif
    auto s = d.createNestedObject("sound");
    s["volume"] = xTouchConfig.xTouchSoundVolume;
    s["playing"] = audio.playing;
    s["sleeping"] = audio.sleeping;
    s["played"] = audio.played;
    s["dropped"] = audio.dropped;
    s["error"] = audio.error;
}
