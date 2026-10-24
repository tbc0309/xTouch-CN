#pragma once
#include <esp_cpu.h>
#include <esp_err.h>
#include <esp_private/rtc_clk.h>
#include <esp_private/startup_internal.h>
#include <soc/rtc.h>

// Include from the board's display translation unit so the linker retains
// this init descriptor along with the referenced board entry points.
// Early heap/task initialization runs at 200 MHz; the application's power
// policy subsequently enables the normal 100..400 MHz DFS range.
ESP_SYSTEM_INIT_FN(p4_startup_clock, CORE, BIT(0), 5)
{
    rtc_cpu_freq_config_t previous, startup;
    rtc_clk_cpu_freq_get_config(&previous);
    if (!rtc_clk_cpu_freq_mhz_to_config(200, &startup)) return ESP_ERR_NOT_SUPPORTED;
    rtc_clk_cpu_freq_set_config(&startup);
    esp_cpu_set_cycle_count(uint64_t(esp_cpu_get_cycle_count()) * startup.freq_mhz / previous.freq_mhz);
    return ESP_OK;
}
