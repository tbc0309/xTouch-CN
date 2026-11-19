#ifndef XTOUCH_NATIVE_IDF_LOG_H
#define XTOUCH_NATIVE_IDF_LOG_H
#pragma once
#include <esp_log.h>
#define log_e(...) ESP_LOGE("xtouch",__VA_ARGS__)
#define log_w(...) ESP_LOGW("xtouch",__VA_ARGS__)
#define log_i(...) ESP_LOGI("xtouch",__VA_ARGS__)
#define log_d(...) ESP_LOGD("xtouch",__VA_ARGS__)
#define log_v(...) ESP_LOGV("xtouch",__VA_ARGS__)

#endif
