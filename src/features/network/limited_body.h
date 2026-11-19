#pragma once
#include "platform/idf/text.h"

// Keep failure latched: an HTTP event dispatcher may continue after a short write.
template <size_t Limit, class Text = String> class XTouchLimitedBody final : public Stream {
  public:
    Text body;
    bool overflow = false;
    bool failed = false;

    size_t write(uint8_t value) override {
        return write(&value, 1);
    }
    size_t write(const uint8_t *data, size_t length) override {
        if (failed)
            return 0;
        if ((!data && length) || body.length() > Limit) {
            failed = true;
            overflow = body.length() > Limit;
            return 0;
        }
        const size_t room = Limit - body.length();
        const size_t take = length < room ? length : room;
        overflow = take < length;
        const bool accepted = !take || body.concat(reinterpret_cast<const char *>(data), take);
        failed = overflow || !accepted;
        return accepted ? take : 0;
    }
    int available() override {
        return 0;
    }
    int read() override {
        return -1;
    }
    int peek() override {
        return -1;
    }
    void flush() override {}
};
