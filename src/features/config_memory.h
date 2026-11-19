#pragma once
#include <stddef.h>
#include <stdint.h>

namespace XTouchConfigMemory {
constexpr size_t reserve = 8192;
inline size_t parsePool(size_t input, size_t previous) {
    if (input > SIZE_MAX / 2 || previous > SIZE_MAX - input)
        return SIZE_MAX;
    const size_t structured = input * 2;
    const size_t merged = input + previous;
    const size_t needed = structured > merged ? structured : merged;
    if (needed > SIZE_MAX - 2048)
        return SIZE_MAX;
    return needed + 2048 > 4096 ? needed + 2048 : 4096;
}
// The size estimate is an upper bound, not the parser's actual requirement.
// Use a smaller contiguous pool when fragmented RAM cannot fit that estimate;
// ArduinoJson still rejects an insufficient pool before any write is admitted.
inline size_t parseBudget(size_t requested, size_t free, size_t largest) {
    if (free <= reserve)
        return 0;
    size_t budget = free - reserve;
    if (budget > largest)
        budget = largest;
    if (budget > requested)
        budget = requested;
    return budget >= 4096 ? budget : 0;
}
inline bool available(size_t allocation, size_t free, size_t largest) {
    return allocation <= largest && allocation <= free && reserve <= free - allocation;
}
inline size_t storagePool(size_t used, size_t minimum) {
    if (used > SIZE_MAX - 1024)
        return SIZE_MAX;
    return used + 1024 > minimum ? used + 1024 : minimum;
}
} // namespace XTouchConfigMemory
