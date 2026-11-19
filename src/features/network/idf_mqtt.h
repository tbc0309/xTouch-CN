#pragma once
#include <mqtt_client.h>
#include "mqtt_buffer_policy.h"
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <atomic>
#include <functional>
#include <cstring>
#include <string>
#include <new>

// IDF owns transport, TLS and keepalive. Only loop() invokes application code:
// MQTT's worker must never touch LVGL or the active printer's parser state.
class XTouchIdfMqtt {
  public:
    struct Stats {
        unsigned dataEvents, queuedFrames, deliveredFrames, droppedFrames, rejectedFrames,
            allocationFailures, published;
    };

  private:
    std::atomic<unsigned> dataEvents{0}, queuedFrames{0}, deliveredFrames{0}, droppedFrames{0},
        rejectedFrames{0}, allocationFailures{0}, published{0};
    struct Frame {
        char topic[160]{};
        unsigned length = 0, used = 0;
        uint8_t *body = nullptr;
        ~Frame() {
            heap_caps_free(body);
        }
    };
    static constexpr unsigned MaxFrame = 61439;
    esp_mqtt_client_handle_t client = nullptr;
    QueueHandle_t frames = nullptr;
    Frame *incoming = nullptr;
    std::string host;
    const char *ca = nullptr;
    unsigned port = 8883, keepalive = 10;
    unsigned outputBytes = 0;
    std::atomic<bool> online{false}, finished{false};
    std::atomic<int> result{-1}, subscription{-1};
    std::function<void(char *, uint8_t *, unsigned)> callback;
    static void event(void *context, esp_event_base_t, int32_t id, void *data) {
        auto *self = static_cast<XTouchIdfMqtt *>(context);
        auto *e = static_cast<esp_mqtt_event_handle_t>(data);
        if (id == MQTT_EVENT_CONNECTED) {
            self->result = 0;
            self->online = true;
            self->finished = true;
        } else if (id == MQTT_EVENT_DISCONNECTED) {
            self->online = false;
            self->finished = true;
            delete self->incoming;
            self->incoming = nullptr;
        } else if (id == MQTT_EVENT_ERROR) {
            if (e->error_handle &&
                e->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
                self->result = int(e->error_handle->connect_return_code);
            self->finished = true;
        } else if (id == MQTT_EVENT_SUBSCRIBED)
            self->subscription = e->msg_id;
        else if (id == MQTT_EVENT_DATA)
            self->receive(e);
    }
    void receive(esp_mqtt_event_handle_t e) {
        ++dataEvents;
        if (e->current_data_offset == 0) {
            delete incoming;
            incoming = nullptr;
            if (e->total_data_len < 0 || unsigned(e->total_data_len) > MaxFrame ||
                e->topic_len <= 0 || unsigned(e->topic_len) >= sizeof(Frame::topic)) {
                ++rejectedFrames;
                return;
            }
            incoming = new (std::nothrow) Frame;
            if (!incoming) {
                ++allocationFailures;
                return;
            }
            incoming->length = unsigned(e->total_data_len);
            memcpy(incoming->topic, e->topic, e->topic_len);
            incoming->body = static_cast<uint8_t *>(
                heap_caps_malloc(incoming->length + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
            if (!incoming->body)
                incoming->body =
                    static_cast<uint8_t *>(heap_caps_malloc(incoming->length + 1, MALLOC_CAP_8BIT));
            if (!incoming->body) {
                ++allocationFailures;
                delete incoming;
                incoming = nullptr;
                return;
            }
        }
        if (!incoming)
            return;
        if (e->total_data_len != int(incoming->length) || e->data_len < 0 ||
            e->current_data_offset < 0 || unsigned(e->current_data_offset) != incoming->used ||
            unsigned(e->data_len) > incoming->length - incoming->used) {
            ++rejectedFrames;
            delete incoming;
            incoming = nullptr;
            return;
        }
        memcpy(incoming->body + incoming->used, e->data, e->data_len);
        incoming->used += unsigned(e->data_len);
        if (incoming->used == incoming->length) {
            incoming->body[incoming->length] = 0;
            if (xQueueSend(frames, &incoming, 0) != pdTRUE) {
                ++droppedFrames;
                delete incoming;
            } else
                ++queuedFrames;
            incoming = nullptr;
        }
    }
    bool await(const std::function<bool()> &ready, unsigned milliseconds) {
        const TickType_t start = xTaskGetTickCount(), budget = pdMS_TO_TICKS(milliseconds);
        while (!ready() && xTaskGetTickCount() - start < budget)
            vTaskDelay(pdMS_TO_TICKS(5));
        return ready();
    }

  public:
    Stats stats() const {
        return {dataEvents.load(),    queuedFrames.load(),   deliveredFrames.load(),
                droppedFrames.load(), rejectedFrames.load(), allocationFailures.load(),
                published.load()};
    }
    XTouchIdfMqtt() = default;
    ~XTouchIdfMqtt() {
        disconnect();
        if (frames)
            vQueueDelete(frames);
    }
    XTouchIdfMqtt(const XTouchIdfMqtt &) = delete;
    void setServer(const char *name, unsigned number) {
        host = name ? name : "";
        port = number;
    }
    void setCACert(const char *pem) {
        ca = pem;
    }
    void setKeepAlive(unsigned seconds) {
        keepalive = seconds;
    }
    template <class Callback> void setCallback(Callback cb) {
        callback = cb;
    }
    bool connected() const {
        return online.load();
    }
    unsigned outBufferSize() const {
        return outputBytes;
    }
    int state() const {
        return result.load();
    }
    bool connect(const char *id, const char *user, const char *password) {
        disconnect();
        result = -1;
        finished = false;
        subscription = -1;
        dataEvents = 0;
        queuedFrames = 0;
        deliveredFrames = 0;
        droppedFrames = 0;
        rejectedFrames = 0;
        allocationFailures = 0;
        published = 0;
        if (!frames)
            frames = xQueueCreate(2, sizeof(Frame *));
        if (!frames || host.empty())
            return false;
        const size_t outbound = XTouchMqttBuffers::output(
            id ? strlen(id) : 0, user ? strlen(user) : 0, password ? strlen(password) : 0);
        if (!outbound)
            return false;
        esp_mqtt_client_config_t config{};
        config.broker.address.hostname = host.c_str();
        config.broker.address.port = port;
        config.broker.address.transport = MQTT_TRANSPORT_OVER_SSL;
        config.broker.verification.certificate = ca;
        config.broker.verification.skip_cert_common_name_check = !ca;
        config.credentials.client_id = id;
        config.credentials.username = user;
        config.credentials.authentication.password = password;
        config.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
        config.session.keepalive = keepalive;
        config.network.timeout_ms = 2000;
        config.network.disable_auto_reconnect = true;
        config.buffer.size = 2048;
        config.buffer.out_size = outbound;
        config.task.stack_size = 6144;
        config.outbox.limit = 16384;
        client = esp_mqtt_client_init(&config);
        if (!client)
            return false;
        outputBytes = outbound;
        if (esp_mqtt_client_register_event(client, esp_mqtt_event_id_t(ESP_EVENT_ANY_ID), event,
                                           this) != ESP_OK ||
            esp_mqtt_client_start(client) != ESP_OK) {
            disconnect();
            return false;
        }
        await([&] { return finished.load(); }, 6000);
        if (!connected()) {
            int error = result;
            disconnect();
            result = error;
            return false;
        }
        return true;
    }
    bool subscribe(const char *topic) {
        if (!connected())
            return false;
        subscription = -1;
        int id = esp_mqtt_client_subscribe(client, topic, 0);
        return id >= 0 && await([&] { return subscription == id || !connected(); }, 2000) &&
               connected() && subscription == id;
    }
    bool publish(const char *topic, const char *body) {
        bool ok = connected() && esp_mqtt_client_enqueue(client, topic, body, 0, 0, 0, true) >= 0;
        if (ok)
            ++published;
        return ok;
    }
    void loop() {
        Frame *frame = nullptr;
        if (frames && xQueueReceive(frames, &frame, 0) == pdTRUE) {
            if (connected() && callback) {
                ++deliveredFrames;
                callback(frame->topic, frame->body, frame->length);
            }
            delete frame;
        }
    }
    void disconnect() {
        if (client) {
            esp_mqtt_client_stop(client);
            esp_mqtt_client_destroy(client);
            client = nullptr;
        }
        outputBytes = 0;
        online = false;
        delete incoming;
        incoming = nullptr;
        Frame *frame = nullptr;
        while (frames && xQueueReceive(frames, &frame, 0) == pdTRUE)
            delete frame;
    }
};
