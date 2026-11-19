#pragma once
#include <driver/sdmmc_host.h>
#include <soc/soc_caps.h>
#include <cstdint>

// Board wiring and power belong to the adapter; mounting and I/O lifetime
// belong to the shared native storage implementation. Missing adapters are off.
struct BoardStorageProfile {
    enum class Bus { None, Spi, Sdmmc };
    Bus bus = Bus::None;
    int spiHost = 0, clock = -1, mosi = -1, miso = -1, cs = -1;
    uint32_t maxFrequencyKhz = 20000;
#if SOC_SDMMC_HOST_SUPPORTED
    bool (*configure)(sdmmc_host_t &, sdmmc_slot_config_t &) = nullptr;
#endif
    void (*release)() = nullptr;
};
extern "C" const BoardStorageProfile &xtouch_board_storage_profile();
