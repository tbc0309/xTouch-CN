#include "i2c.h"
namespace {
struct Lock {
    SemaphoreHandle_t mutex;
    bool held;
    explicit Lock(SemaphoreHandle_t value)
        : mutex(value), held(value && xSemaphoreTake(value, portMAX_DELAY) == pdTRUE) {}
    ~Lock() {
        if (held)
            xSemaphoreGive(mutex);
    }
};
} // namespace
NativeI2cBus xtouch_i2c, xtouch_i2c_aux;
NativeI2cBus::~NativeI2cBus() {
    if (end() == ESP_OK && mutex)
        vSemaphoreDelete(mutex);
}
esp_err_t NativeI2cBus::device(uint8_t address, i2c_master_dev_handle_t &handle) {
    Device *empty = nullptr;
    for (auto &entry : devices) {
        if (entry.handle && entry.address == address) {
            handle = entry.handle;
            return ESP_OK;
        }
        if (!entry.handle && !empty)
            empty = &entry;
    }
    if (!empty)
        return ESP_ERR_NO_MEM;
    i2c_device_config_t config{};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = address;
    config.scl_speed_hz = frequency;
    auto result = i2c_master_bus_add_device(bus, &config, &handle);
    if (result == ESP_OK) {
        empty->address = address;
        empty->handle = handle;
    }
    return result;
}
bool NativeI2cBus::begin(int dataPin, int clockPin, uint32_t hz) {
    if (!mutex)
        mutex = xSemaphoreCreateMutex();
    Lock lock(mutex);
    if (!lock.held)
        return false;
    if (bus)
        return sda == dataPin && scl == clockPin && frequency == hz;
    i2c_master_bus_config_t config{};
    config.i2c_port = -1;
    config.sda_io_num = gpio_num_t(dataPin);
    config.scl_io_num = gpio_num_t(clockPin);
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = true;
    if (i2c_new_master_bus(&config, &bus) != ESP_OK)
        return false;
    sda = dataPin;
    scl = clockPin;
    frequency = hz;
    return true;
}
esp_err_t NativeI2cBus::end() {
    if (!mutex)
        return ESP_OK;
    Lock lock(mutex);
    if (!lock.held)
        return ESP_ERR_TIMEOUT;
    if (!bus)
        return ESP_OK;
    for (auto &entry : devices)
        if (entry.handle) {
            auto result = i2c_master_bus_rm_device(entry.handle);
            if (result != ESP_OK)
                return result;
            entry.address = 0;
            entry.handle = nullptr;
        }
    auto result = i2c_del_master_bus(bus);
    if (result == ESP_OK) {
        bus = nullptr;
        sda = scl = -1;
        frequency = 0;
    }
    return result;
}
esp_err_t NativeI2cBus::probe(uint8_t address, unsigned timeoutMs) {
    if (address > 0x7f)
        return ESP_ERR_INVALID_ARG;
    Lock lock(mutex);
    if (!lock.held || !bus)
        return ESP_ERR_INVALID_STATE;
    return i2c_master_probe(bus, address, timeoutMs);
}
esp_err_t NativeI2cBus::transfer(uint8_t address, const uint8_t *write, size_t writeBytes,
                                 uint8_t *read, size_t readBytes, unsigned timeoutMs) {
    if (address > 0x7f || (writeBytes && !write) || (readBytes && !read) ||
        (!writeBytes && !readBytes))
        return ESP_ERR_INVALID_ARG;
    Lock lock(mutex);
    if (!lock.held || !bus)
        return ESP_ERR_INVALID_STATE;
    i2c_master_dev_handle_t handle = nullptr;
    auto result = device(address, handle);
    if (result != ESP_OK)
        return result;
    if (!readBytes)
        return i2c_master_transmit(handle, write, writeBytes, timeoutMs);
    if (!writeBytes)
        return i2c_master_receive(handle, read, readBytes, timeoutMs);
    return i2c_master_transmit_receive(handle, write, writeBytes, read, readBytes, timeoutMs);
}
