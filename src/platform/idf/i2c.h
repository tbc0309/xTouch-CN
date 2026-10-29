#pragma once
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <array>
#include <cstddef>
#include <cstdint>

// Initialize before launching bus users. Complete transactions are atomic;
// callers own their buffers and never retain a lock between API calls.
class NativeI2cBus {
    struct Device {
        uint8_t address = 0;
        i2c_master_dev_handle_t handle = nullptr;
    };
    i2c_master_bus_handle_t bus = nullptr;
    SemaphoreHandle_t mutex = nullptr;
    std::array<Device, 8> devices{};
    int sda = -1, scl = -1;
    uint32_t frequency = 0;
    esp_err_t device(uint8_t address, i2c_master_dev_handle_t &handle);

  public:
    NativeI2cBus() = default;
    ~NativeI2cBus();
    NativeI2cBus(const NativeI2cBus &) = delete;
    NativeI2cBus &operator=(const NativeI2cBus &) = delete;
    bool begin(int dataPin, int clockPin, uint32_t hz = 100000);
    esp_err_t end();
    esp_err_t probe(uint8_t address, unsigned timeoutMs = 20);
    esp_err_t transfer(uint8_t address, const uint8_t *write, size_t writeBytes, uint8_t *read,
                       size_t readBytes, unsigned timeoutMs = 20);
};
extern NativeI2cBus xtouch_i2c, xtouch_i2c_aux;
