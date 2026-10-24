# Generate a bounded card-probe transport for both SDMMC boards.
# Preserve slot-scoped deinitialization so missing SD cannot tear down C6 SDIO.
idf_component_get_property(ARDUINO_DIR espressif__arduino-esp32 COMPONENT_DIR)
set(SDMMC_ORIGINAL "${ARDUINO_DIR}/libraries/SD_MMC/src/SD_MMC.cpp")
file(READ "${SDMMC_ORIGINAL}" SDMMC_CODE)
foreach(WIDTH 4 1)
 string(REPLACE "host.flags = SDMMC_HOST_FLAG_${WIDTH}BIT;" "host.flags = SDMMC_HOST_FLAG_${WIDTH}BIT | SDMMC_HOST_FLAG_DEINIT_ARG;" SDMMC_CODE "${SDMMC_CODE}")
endforeach()
# Only discovery commands get the shorter timeout; data/write/erase budgets stay intact.
string(REPLACE "sdmmc_host_t host = SDMMC_HOST_DEFAULT();" [=[sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.do_transaction = [](int slot, sdmmc_command_t *cmd) -> esp_err_t {
    if (cmd->opcode == 0 || cmd->opcode == 1 || cmd->opcode == 5 || cmd->opcode == 8 || cmd->opcode == 41 || cmd->opcode == 55) cmd->timeout_ms = 100;
    return sdmmc_host_do_transaction(slot, cmd);
  };]=] SDMMC_CODE "${SDMMC_CODE}")
set(SDMMC_FIXED "${CMAKE_CURRENT_BINARY_DIR}/board_SD_MMC.cpp")
file(CONFIGURE OUTPUT "${SDMMC_FIXED}" CONTENT "${SDMMC_CODE}" @ONLY)
get_target_property(ARDUINO_SOURCES ${ARDUINO_LIB} SOURCES)
list(FILTER ARDUINO_SOURCES EXCLUDE REGEX "/SD_MMC[.]cpp$")
set_property(TARGET ${ARDUINO_LIB} PROPERTY SOURCES "${ARDUINO_SOURCES}")
target_sources(${ARDUINO_LIB} PRIVATE "${SDMMC_FIXED}")
set_property(SOURCE "${SDMMC_FIXED}" APPEND PROPERTY INCLUDE_DIRECTORIES "${ARDUINO_DIR}/libraries/SD_MMC/src")
