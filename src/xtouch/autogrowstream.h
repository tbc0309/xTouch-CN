#pragma once
#include <Arduino.h>
#include <Stream.h>

// Bounded MQTT frame storage. Small blocks avoid requiring a contiguous 60KB
// allocation beside TLS and LVGL. Rewind supports two filtered JSON passes.
class XtouchAutoGrowBufferStream : public Stream {
    static constexpr unsigned BlockSize = 1024, BlockCount = 60, MaxBytes = 61439;
    uint8_t *blocks[BlockCount] = {};
    uint32_t length = 0, cursor = 0;
    bool broken = false;
    uint8_t at(uint32_t n) const { return blocks[n / BlockSize][n % BlockSize]; }

  public:
    XtouchAutoGrowBufferStream() { setTimeout(0); }
    ~XtouchAutoGrowBufferStream();
    XtouchAutoGrowBufferStream(const XtouchAutoGrowBufferStream &) = delete;
    XtouchAutoGrowBufferStream &operator=(const XtouchAutoGrowBufferStream &) = delete;
    uint32_t current_length() const { return length; }
    bool failed() const { return broken; }
    void rewind() { cursor = 0; }
    bool includes(const char *target) const;
    size_t write(uint8_t byte) override;
    int read() override { return cursor < length ? at(cursor++) : -1; }
    int peek() override { return cursor < length ? at(cursor) : -1; }
    int available() override { return length - cursor; }
    void flush() override;
    using Print::write;
};
