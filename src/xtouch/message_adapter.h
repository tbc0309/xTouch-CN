#pragma once
#include "ui/lvgl_compat.h"
// LVGL 8 invokes subscribers with (subscription, message), never message alone.
template<void (*Callback)(lv_msg_t *)>
void xtouch_message_callback(void *,lv_msg_t *message){Callback(message);}
