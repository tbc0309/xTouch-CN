#pragma once
#include <cstddef>

namespace XTouchPort {
struct LcdEvModulePins {
    int sda, scl, data6, data7;
};
constexpr LcdEvModulePins lcdEvModulePins(size_t psramBytes) {
    if (psramBytes == 0)
        return {-1, -1, -1, -1};
    return psramBytes > 8U * 1024U * 1024U ? LcdEvModulePins{47, 48, 8, 18}
                                           : LcdEvModulePins{8, 18, 47, 48};
}
} // namespace XTouchPort
