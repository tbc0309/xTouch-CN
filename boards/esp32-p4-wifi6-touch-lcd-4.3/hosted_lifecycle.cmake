# Hosted 3 has an official opt-out for early initialization.
if(CONFIG_ESP_HOSTED_AUTO_CALL_INIT_BEFORE_APP_MAIN)
 message(FATAL_ERROR "This board initializes Hosted from Arduino setup; disable early auto init")
endif()

# P4 OTA does not update C6 firmware. Arduino's automatic update advisory sends
# a version RPC that factory C6 firmware may not implement; retain the explicit
# update APIs, but do not issue this unused probe during every Wi-Fi startup.
idf_component_get_property(ARDUINO_DIR espressif__arduino-esp32 COMPONENT_DIR)
idf_component_get_property(ARDUINO_LIB espressif__arduino-esp32 COMPONENT_LIB)
file(READ "${ARDUINO_DIR}/cores/esp32/esp32-hal-hosted.c" ARDUINO_HOSTED_CODE)
set(HOSTED_UPDATE_PROBE "  hosted_initialized = true;\n  hostedHasUpdate();\n  return true;")
string(FIND "${ARDUINO_HOSTED_CODE}" "${HOSTED_UPDATE_PROBE}" HOSTED_PROBE_INDEX)
if(HOSTED_PROBE_INDEX EQUAL -1)
 message(FATAL_ERROR "Arduino Hosted startup changed; review the optional update probe")
endif()
string(REPLACE "${HOSTED_UPDATE_PROBE}" "  hosted_initialized = true;\n  return true;" ARDUINO_HOSTED_CODE "${ARDUINO_HOSTED_CODE}")
set(ARDUINO_HOSTED_FIXED "${CMAKE_CURRENT_BINARY_DIR}/board_esp32_hal_hosted.c")
file(CONFIGURE OUTPUT "${ARDUINO_HOSTED_FIXED}" CONTENT "${ARDUINO_HOSTED_CODE}" @ONLY)
get_target_property(ARDUINO_HOSTED_SOURCES ${ARDUINO_LIB} SOURCES)
list(FILTER ARDUINO_HOSTED_SOURCES EXCLUDE REGEX "/esp32-hal-hosted[.]c$")
set_property(TARGET ${ARDUINO_LIB} PROPERTY SOURCES "${ARDUINO_HOSTED_SOURCES}")
target_sources(${ARDUINO_LIB} PRIVATE "${ARDUINO_HOSTED_FIXED}")
