#pragma once
#include <stddef.h>
#include <stdint.h>

namespace pixel_screen {
constexpr unsigned width = 32, height = 32, pixels = width * height;
enum class State : uint8_t {
    Offline,
    Idle,
    Preparing,
    Printing,
    Paused,
    Complete,
    Cancelled,
    Error,
    Environment
};
struct Snapshot {
    State state = State::Offline;
    uint32_t sourceToken = 0, stateAgeMs = 0;
    bool sourceReady = true;
    uint8_t percent = 0;
    bool progressLayout = false;
    int leftMinutes = -1, nozzle = -1, bed = -1;
    bool environmentKnown = false;
    double temperature = 0, humidity = 0;
    char model[7] = "--", clock[6] = "--:--";
    int currentLayer = -1, totalLayers = 0;
    int stage = -1;
    int amsMain = -1;
    uint32_t filamentColor = 0, feedColor = 0;
    bool filamentKnown = false;
    bool headParked = false;
    bool chamberHeating = false;
};
unsigned modelHeight(const Snapshot &snapshot);
// Indexed frame buffers are caller-owned; neither rendering nor framing allocates memory.
unsigned animationFrames(const Snapshot &snapshot);
void render(const Snapshot &snapshot, unsigned phase, uint8_t *frame);
size_t png(const uint8_t *frame, uint8_t *output, size_t capacity, uint32_t filamentColor = 0,
           uint32_t feedColor = 0);
size_t gif(const Snapshot &snapshot, unsigned frames, uint8_t *frame, uint8_t *output,
           size_t capacity, uint8_t *workspace = nullptr, size_t workspaceCapacity = 0);
uint32_t crc32(const uint8_t *data, size_t size);
uint32_t visibleFilamentColor(uint32_t color);
uint32_t rgb(uint8_t index, uint32_t filamentColor = 0, uint32_t feedColor = 0);
enum class Format : uint8_t { Png, Gif };
// A sink consumes an entire packet synchronously; false cancels the upload immediately.
using Sink = bool (*)(const uint8_t *, size_t, void *);
bool upload(Format format, const uint8_t *image, size_t size, uint8_t *scratch, size_t capacity,
            Sink sink, void *context);
constexpr const char *write_uuid = "0000fa02-0000-1000-8000-00805f9b34fb";
constexpr const char *notify_uuid = "0000fa03-0000-1000-8000-00805f9b34fb";
} // namespace pixel_screen
