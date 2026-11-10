#include "storage_profile.h"
#include "platform/idf/runtime.h"
namespace {
bool configure(sdmmc_host_t &host, sdmmc_slot_config_t &slot) {
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
    host.slot = 1;
    host.flags = SDMMC_HOST_FLAG_1BIT | SDMMC_HOST_FLAG_DEINIT_ARG;
    slot.clk = GPIO_NUM_14;
    slot.cmd = GPIO_NUM_17;
    slot.d0 = GPIO_NUM_16;
    slot.width = 1;
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
