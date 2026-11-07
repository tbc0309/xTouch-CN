#include "storage_profile.h"
namespace {
bool configure(sdmmc_host_t &host, sdmmc_slot_config_t &slot) {
    host.slot = 1;
    host.flags = SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_DEINIT_ARG;
    slot.clk = GPIO_NUM_5;
    slot.cmd = GPIO_NUM_4;
    slot.d0 = GPIO_NUM_6;
    slot.d1 = GPIO_NUM_7;
    slot.d2 = GPIO_NUM_2;
    slot.d3 = GPIO_NUM_3;
    slot.width = 4;
    return true;
}
} // namespace
const BoardStorageProfile &xtouch_board_storage_profile() {
    static const auto profile = [] {
        BoardStorageProfile p;
        p.bus = BoardStorageProfile::Bus::Sdmmc;
        p.configure = configure;
        return p;
    }();
    return profile;
}
