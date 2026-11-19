#include "storage_profile.h"
#include <driver/spi_master.h>
const BoardStorageProfile &xtouch_board_storage_profile() {
    static const auto profile = [] {
        BoardStorageProfile p;
        p.bus = BoardStorageProfile::Bus::Spi;
        p.spiHost = SPI3_HOST;
        p.clock = 18;
        p.mosi = 23;
        p.miso = 19;
        p.cs = 5;
        p.maxFrequencyKhz = 10000;
        return p;
    }();
    return profile;
}
