#pragma once
#include <cstddef>
#include <cstdint>
#include <esp_heap_caps.h>

enum class NativeBufferMemory { PreferPsram, PreferInternal };

// Byte workspace with bounded ownership. Prefer external RAM when available;
// boards without PSRAM retain the normal 8-bit heap fallback.
class NativeHeapBuffer {
    uint8_t *bytes = nullptr;
    size_t capacity = 0;
    NativeBufferMemory memory = NativeBufferMemory::PreferPsram;

  public:
    NativeHeapBuffer() = default;
    explicit NativeHeapBuffer(size_t size,
                              NativeBufferMemory location = NativeBufferMemory::PreferPsram)
        : memory(location) {
        ensure(size);
    }
    NativeHeapBuffer(const NativeHeapBuffer &) = delete;
    NativeHeapBuffer &operator=(const NativeHeapBuffer &) = delete;
    ~NativeHeapBuffer() {
        heap_caps_free(bytes);
    }
    bool ensure(size_t size) {
        if (size <= capacity)
            return true;
        uint8_t *next;
        if (memory == NativeBufferMemory::PreferInternal) {
            // Prefer direct flash writes; the SDK can safely copy external data
            // if internal RAM is fragmented, so retain the ordinary fallback.
            next = static_cast<uint8_t *>(
                heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
            if (!next)
                next = static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_8BIT));
        } else {
            next =
                static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
            if (!next)
                next = static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_8BIT));
        }
        if (!next)
            return false;
        heap_caps_free(bytes);
        bytes = next;
        capacity = size;
        return true;
    }
    explicit operator bool() const {
        return bytes != nullptr;
    }
    uint8_t *data() {
        return bytes;
    }
    size_t size() const {
        return capacity;
    }
};
