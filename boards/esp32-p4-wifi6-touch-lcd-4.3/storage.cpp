#include "storage_profile.h"
#include "platform/idf/runtime.h"
#include <sd_pwr_ctrl_by_on_chip_ldo.h>
namespace {
sd_pwr_ctrl_handle_t powerHandle;
void release() {
    if (powerHandle) {
        sd_pwr_ctrl_del_on_chip_ldo(powerHandle);
        powerHandle = nullptr;
    }
}
bool configure(sdmmc_host_t &host, sdmmc_slot_config_t &slot) {
    host.slot = 0;
    host.flags = SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_DEINIT_ARG;
    // Slot zero uses fixed IO_MUX; zero is the SDK unspecified-pin sentinel.
    slot.clk = slot.cmd = slot.d0 = slot.d1 = slot.d2 = slot.d3 = slot.d4 = slot.d5 = slot.d6 =
        slot.d7 = GPIO_NUM_0;
    slot.width = 4;
    if (!powerHandle) {
        sd_pwr_ctrl_ldo_config_t p{};
        p.ldo_chan_id = 4;
        if (sd_pwr_ctrl_new_on_chip_ldo(&p, &powerHandle) != ESP_OK)
            return false;
    }
    host.pwr_ctrl_handle = powerHandle;
    pinMode(45, OUTPUT);
    digitalWrite(45, HIGH);
    delay(10);
    digitalWrite(45, LOW);
    delay(10);
    return true;
}
} // namespace
const BoardStorageProfile &xtouch_board_storage_profile() {
    static const auto profile = [] {
        BoardStorageProfile p;
        p.bus = BoardStorageProfile::Bus::Sdmmc;
        p.configure = configure;
        p.release = release;
        return p;
    }();
    return profile;
}
