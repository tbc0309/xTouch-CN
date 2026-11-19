#ifndef _XLCD_COLDBOOT
#define _XLCD_COLDBOOT

#include <esp_system.h>

#include "platform/idf/runtime.h"
#include "config.h"

void xtouch_coldboot_check()
{
    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        DynamicJsonDocument config = xtouch_load_config();
        int coldbootTimeout = config["coldboot"] | 5000;
        coldbootTimeout = constrain(coldbootTimeout, 0, 30000);
        lv_label_set_text(introScreenCaption, XTOUCH_TEXT_STARTING);
        lv_obj_set_style_text_color(introScreenCaption, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_timer_handler();
        delay(coldbootTimeout);
    }
}

#endif
