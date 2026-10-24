#include "autogrowstream.h"
#include <stdlib.h>
#include <string.h>

XtouchAutoGrowBufferStream::~XtouchAutoGrowBufferStream() {
    for (auto block : blocks)
        free(block);
}
size_t XtouchAutoGrowBufferStream::write(uint8_t byte) {
    if (broken)
        return 0;
    if (length >= MaxBytes) {
        broken = true;
        return 0;
    }
    unsigned block = length / BlockSize;
    if (!blocks[block]) {
        blocks[block] = static_cast<uint8_t *>(malloc(BlockSize));
        if (!blocks[block]) {
            broken = true;
            return 0;
        }
    }
    blocks[block][length++ % BlockSize] = byte;
    return 1;
}
void XtouchAutoGrowBufferStream::flush() {
    length = cursor = 0;
    broken = false;
    // Retain only one small block for the next status/ACK.
    for (unsigned i = 1; i < BlockCount; i++) {
        free(blocks[i]);
        blocks[i] = nullptr;
    }
}
bool XtouchAutoGrowBufferStream::includes(const char *target) const {
    if (!target || !*target)
        return true;
    size_t n = strlen(target);
    if (n > length)
        return false;
    for (uint32_t i = 0; i <= length - n; i++) {
        size_t j = 0;
        while (j < n && at(i + j) == static_cast<uint8_t>(target[j]))
            j++;
        if (j == n)
            return true;
    }
    return false;
}
