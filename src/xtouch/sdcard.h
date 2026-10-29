#ifndef _XLCD_SDCARD
#define _XLCD_SDCARD
#include "xtouch/card_fs.h"
#include "firmware_target.h"
#include "storage.h"
#include "power.h"
#include "ui/settings_gesture.h"
#include "board.h"

static bool xtouch_sdcard_firmware_waiting = false;

static bool xtouch_sdcard_probe()
{
    return true;
}

bool xtouch_sdcard_setup()
{
    if (!xtouch_sdcard_probe()) return false;
    bool available = board_card_begin() && SD.cardType() != CARD_NONE;
    if (!available) SD.end();
    xtouch_storage_set_sd(available);
    if (available) {
        if (!SD.exists("/xtouch")) SD.mkdir("/xtouch");
        ConsoleInfo.printf("[SD] mounted %lluMB\n", SD.cardSize() / (1024 * 1024));
    }
    return available;
}

void xtouch_sdcard_poll(bool runtime = true)
{
    static uint32_t next = 0;
    if ((int32_t)(millis() - next) < 0) return;
    // No empty-slot retries in boot Wi-Fi wait or network standby.
    if ((!runtime && !xtouch_storage_sd_available()) || (runtime && xtouch_power_status().screenSleeping)) return;
    // Do not perform card insertion probes immediately after user interaction.
    if (runtime && (lv_disp_get_inactive_time(NULL) < 3000 || ui_scroll_busy(3000))) return;
    next = millis() + (xtouch_storage_sd_available()?10000u:60000u);
    if (xtouch_storage_sd_available()) {
        uint8_t sector[512];
        if (!SD.readRAW(sector, 0)) {
            xtouch_storage_set_sd(false); SD.end();
            ConsoleInfo.println("[SD] removed; internal configuration remains active");
            return;
        }
    } else if (!xtouch_sdcard_setup()) return;
    xtouch_storage_sync(runtime);
    if (runtime && SD.exists("/firmware.bin")) {
        File firmware = SD.open("/firmware.bin", FILE_READ);
        uint8_t imageHeader[36]={0};size_t headerSize=firmware?firmware.read(imageHeader,sizeof(imageHeader)):0;
        xtouch_sdcard_firmware_waiting = firmware && firmware.size() <= XTOUCH_FIRMWARE_MAX_BYTES && xtouch_firmware_header_valid(imageHeader,headerSize);
        firmware.close();
    }
}
#endif
