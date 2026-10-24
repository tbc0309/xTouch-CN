get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
option(XTOUCH_FEATURE_AUDIO "Include speaker support" ON)
option(XTOUCH_FEATURE_DEEP_SLEEP "Include optional deep sleep" ON)
option(XTOUCH_FEATURE_INDICATOR "Include controllable status LED support" ON)
option(XTOUCH_FEATURE_CAMERA "Include printer camera" ON)
option(XTOUCH_FEATURE_PRINT_PREVIEW "Include task previews" ON)
option(XTOUCH_ERROR_DETAILS "Include bilingual error descriptions and extra glyphs" OFF)
option(XTOUCH_FEATURE_BLE_PROVISIONING "Include optional BLE Wi-Fi provisioning" OFF)
option(XTOUCH_P4_C6_PREINITIALIZED_BT "Factory C6 firmware initializes the BLE controller at boot" OFF)
set(RADIO_SOURCES "")
set(RADIO_REQUIRES "")
if(IDF_TARGET STREQUAL "esp32p4")
 list(APPEND RADIO_REQUIRES espressif__esp_hosted)
endif()
if(XTOUCH_FEATURE_BLE_PROVISIONING)
 if(NOT IDF_TARGET MATCHES "^(esp32p4|esp32s3)$")
  message(FATAL_ERROR "BLE provisioning supports S3 and P4")
 endif()
 if(NOT CONFIG_BT_NIMBLE_ENABLED)
  message(FATAL_ERROR "BLE provisioning requires NimBLE enabled in sdkconfig")
 endif()
 list(APPEND RADIO_SOURCES "${ROOT}/boards/common/ble_provisioning.cpp")
endif()
file(GLOB_RECURSE UI "${ROOT}/src/ui/*.c")
if(XTOUCH_DISPLAY_WIDTH GREATER_EQUAL 480 AND XTOUCH_DISPLAY_HEIGHT GREATER_EQUAL 320)
 set(XTOUCH_UI_HIDPI_ASSETS ON)
endif()
file(GLOB_RECURSE FEATURES "${ROOT}/src/features/*.c" "${ROOT}/src/features/*.cpp")
set(AUDIO_SOURCES "")
set(AUDIO_REQUIRES "")
if(XTOUCH_FEATURE_AUDIO)
 list(APPEND AUDIO_SOURCES "${ROOT}/boards/common/audio_output.cpp" "${ROOT}/boards/${XTOUCH_BOARD}/audio.cpp")
 list(APPEND AUDIO_REQUIRES esp_driver_i2s)
 if(XTOUCH_BOARD MATCHES "[Pp]4")
  list(APPEND AUDIO_REQUIRES espressif__esp_codec_dev)
 endif()
endif()
set(POWER_SOURCES "")
set(POWER_REQUIRES "")
if(XTOUCH_FEATURE_DEEP_SLEEP AND IDF_TARGET STREQUAL "esp32p4")
 list(APPEND POWER_REQUIRES lp_touch)
endif()
if(XTOUCH_FEATURE_DEEP_SLEEP AND XTOUCH_BOARD MATCHES "[Pp]4")
 list(APPEND POWER_SOURCES "${ROOT}/boards/${XTOUCH_BOARD}/touch_wake.cpp")
endif()
file(GLOB CORE "${ROOT}/src/xtouch/*.c" "${ROOT}/src/xtouch/*.cpp")
file(GLOB_RECURSE WEB "${ROOT}/lib/XTouchWebServer/src/*.cpp")
set(DEPS "${ROOT}/boards/.deps")

if(XTOUCH_UI_HIDPI_ASSETS OR XTOUCH_BOARD MATCHES "[Pp]4" OR XTOUCH_PREVIEW_P4)
  file(GLOB LARGE_FONTS "${ROOT}/resources/ui-assets/fonts/large/*.c")
  list(FILTER UI EXCLUDE REGEX "/fonts/")
  list(FILTER UI EXCLUDE REGEX "/images/printer_models.c$")
  list(FILTER LARGE_FONTS EXCLUDE REGEX "/ui_font_(xlcd|xlcdmin|actions|xperiments).c$")
  file(GLOB SHARED_LARGE "${ROOT}/resources/ui-assets/generated/large/*.c")
  list(APPEND UI ${LARGE_FONTS} ${SHARED_LARGE})
  if(NOT XTOUCH_ERROR_DETAILS)
    list(FILTER UI EXCLUDE REGEX "/ui_font_text_14.c$")
    list(APPEND UI "${ROOT}/resources/ui-assets/fonts/large/compact/ui_font_text_14.c")
  endif()
endif()

# Extra glyphs are linked only when the optional description catalog is enabled.
if(XTOUCH_ERROR_DETAILS AND XTOUCH_BOARD MATCHES "[Ss]3")
 set(S3_ERROR_FONT "${ROOT}/resources/ui-assets/fonts/small/ui_font_error_14.c")
 list(FILTER UI EXCLUDE REGEX "/ui_font_text_14.c$")
 list(APPEND UI "${S3_ERROR_FONT}")
endif()

set(CAMERA_FONT "${ROOT}/resources/ui-assets/fonts/small/ui_font_camera_14.c")
if(XTOUCH_UI_HIDPI_ASSETS OR XTOUCH_BOARD MATCHES "[Pp]4" OR XTOUCH_PREVIEW_P4)
 set(CAMERA_FONT "")
endif()
# Arduino WiFiGeneric.h exposes network_provisioning types even with BLE off.
# Keep that SDK header dependency; the application BLE worker remains optional.
idf_component_register(SRCS ${UI} ${CORE} ${FEATURES} ${AUDIO_SOURCES} ${POWER_SOURCES} ${RADIO_SOURCES} ${WEB} "${ROOT}/src/main.cpp"
  "${ROOT}/boards/common/board.cpp" ${CAMERA_FONT}
  "${ROOT}/boards/${XTOUCH_BOARD}/display.cpp"
  "${DEPS}/PubSubClient/src/PubSubClient.cpp" "${DEPS}/JPEGDEC/src/JPEGDEC.cpp" "${DEPS}/JPEGDEC/src/jpeg.inl"
  INCLUDE_DIRS "${ROOT}/src" "${ROOT}/boards/common" "${ROOT}/resources"
  "${ROOT}/lib/XTouchWebServer/src" "${DEPS}/ArduinoJson/src" "${DEPS}/PubSubClient/src" "${DEPS}/JPEGDEC/src"
  REQUIRES ${RADIO_REQUIRES} ${POWER_REQUIRES} ${AUDIO_REQUIRES} arduino-esp32 esp_pm lvgl esp_lcd esp_driver_gpio esp_driver_ledc esp_driver_i2c esp_driver_spi esp_driver_sdmmc esp_timer esp_hw_support esp_mm esp_wifi esp_netif lwip mbedtls app_update nvs_flash spiffs espressif__network_provisioning espressif__esp-dsp espressif__esp_h264)
if(XTOUCH_FEATURE_BLE_PROVISIONING)
 target_compile_definitions(${COMPONENT_LIB} PRIVATE XTOUCH_P4_C6_PREINITIALIZED_BT=$<BOOL:${XTOUCH_P4_C6_PREINITIALIZED_BT}>)
 include("${ROOT}/boards/common/ble_transport.cmake")
endif()

target_compile_definitions(${COMPONENT_LIB} PUBLIC XTOUCH_WAVESHARE=1 LV_CONF_INCLUDE_SIMPLE=1)
include("${ROOT}/boards/${XTOUCH_BOARD}/display_config.cmake" OPTIONAL)
if(XTOUCH_BOARD MATCHES "[Pp]4")
 include("${ROOT}/boards/${XTOUCH_BOARD}/hosted_lifecycle.cmake")
 include("${ROOT}/boards/${XTOUCH_BOARD}/sdmmc_interrupt.cmake")
endif()
foreach(FEATURE AUDIO DEEP_SLEEP INDICATOR CAMERA PRINT_PREVIEW BLE_PROVISIONING)
 target_compile_definitions(${COMPONENT_LIB} PUBLIC XTOUCH_FEATURE_${FEATURE}=$<BOOL:${XTOUCH_FEATURE_${FEATURE}}>)
endforeach()

target_compile_definitions(${COMPONENT_LIB} PRIVATE XTOUCH_ERROR_DETAILS=$<BOOL:${XTOUCH_ERROR_DETAILS}>)
if(XTOUCH_FEATURE_PRINT_PREVIEW)
file(GLOB PREVIEW_PNG_C "${ROOT}/lib/PNGdec/src/*.c")
target_sources(${COMPONENT_LIB} PRIVATE "${ROOT}/lib/PNGdec/src/PNGdec.cpp" ${PREVIEW_PNG_C})
target_include_directories(${COMPONENT_LIB} PRIVATE "${ROOT}/lib/PNGdec/src")
target_compile_definitions(${COMPONENT_LIB} PRIVATE PNG_MAX_BUFFERED_PIXELS=4128)
endif()
if(XTOUCH_BOARD MATCHES "[Ss]3")
  target_compile_definitions(${COMPONENT_LIB} PUBLIC XTOUCH_BOARD_S3=1)
else()
  target_compile_definitions(${COMPONENT_LIB} PUBLIC XTOUCH_BOARD_P4=1)
endif()

idf_component_get_property(LVGL_LIB lvgl COMPONENT_LIB)
target_include_directories(${LVGL_LIB} PUBLIC "${ROOT}/resources")
target_compile_definitions(${LVGL_LIB} PUBLIC XTOUCH_WAVESHARE=1)

target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:C>:-fcommon>)
if(XTOUCH_BOARD MATCHES "[Ss]3")
  target_sources(${COMPONENT_LIB} PRIVATE "${DEPS}/JPEGDEC/src/s3_simd_420.S" "${DEPS}/JPEGDEC/src/s3_simd_444.S" "${DEPS}/JPEGDEC/src/s3_simd_dequant.S")
  if(XTOUCH_FEATURE_PRINT_PREVIEW)
   target_sources(${COMPONENT_LIB} PRIVATE "${ROOT}/lib/PNGdec/src/s3_simd_rgb565.S")
  endif()
endif()

file(READ "${ROOT}/version.json" VERSION_JSON)
string(JSON XTOUCH_VERSION GET "${VERSION_JSON}" version)
target_compile_definitions(${COMPONENT_LIB} PUBLIC XTOUCH_FIRMWARE_VERSION="${XTOUCH_VERSION}")
set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=misleading-indentation)
file(GLOB PNG_ZLIB_SOURCES "${ROOT}/lib/PNGdec/src/*.c")
set_source_files_properties(${PNG_ZLIB_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wno-error=implicit-fallthrough")

idf_component_get_property(ARDUINO_LIB espressif__arduino-esp32 COMPONENT_LIB)
target_compile_options(${ARDUINO_LIB} PRIVATE -Wno-error=missing-field-initializers)

target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>)

if(XTOUCH_BOARD MATCHES "[Pp]4")
  target_compile_options(${ARDUINO_LIB} PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-error=discarded-qualifiers>)
endif()

if(TRUE) # The shared adapter is a no-op at the original design resolution.
  set(P4_HEADER "${ROOT}/boards/common/display_scale.h")
  foreach(UI_FILE ${UI})
    if(NOT UI_FILE MATCHES "/fonts/" AND NOT UI_FILE MATCHES "/images/" AND NOT UI_FILE MATCHES "/text_fit.c$" AND NOT UI_FILE MATCHES "/resources/ui-assets/")
      set_property(SOURCE ${UI_FILE} APPEND PROPERTY COMPILE_OPTIONS "-include" "${P4_HEADER}")
    endif()
  endforeach()
endif()

if(XTOUCH_BOARD MATCHES "[Pp]4")
 target_compile_definitions(${LVGL_LIB} PUBLIC XTOUCH_BOARD_P4=1)
endif()

if(XTOUCH_UI_NATIVE_CANVAS)
 target_compile_definitions(${COMPONENT_LIB} PRIVATE XTOUCH_UI_NATIVE_CANVAS=1)
endif()
if(XTOUCH_UI_HIDPI_ASSETS)
 target_compile_definitions(${COMPONENT_LIB} PRIVATE XTOUCH_UI_HIDPI_ASSETS=1)
 target_compile_definitions(${LVGL_LIB} PUBLIC XTOUCH_UI_HIDPI_ASSETS=1)
endif()

include("${ROOT}/boards/common/sdmmc_transport.cmake")

# Optimize rendering and codecs while leaving configuration/network parsing on
# the SDK defaults: ArduinoJson 6.21.6 is not clean under GCC 15 -O2 diagnostics.
target_compile_options(${LVGL_LIB} PRIVATE -O2)
set_property(SOURCE ${UI} "${ROOT}/boards/${XTOUCH_BOARD}/display.cpp"
 "${ROOT}/src/features/camera/camera.cpp" "${ROOT}/src/features/camera/camera_rtsp.cpp"
 "${DEPS}/JPEGDEC/src/JPEGDEC.cpp" "${ROOT}/lib/PNGdec/src/PNGdec.cpp"
 ${PNG_ZLIB_SOURCES} APPEND PROPERTY COMPILE_OPTIONS -O2)

include("${ROOT}/boards/common/ui_build_options.cmake")
xtouch_ui_build_options(${COMPONENT_LIB})

# esp-modbus 2.1.4 uses strncpy(..., strlen(source)); GCC 15 diagnoses it at -O2.
# Keep the upstream warning visible without failing unrelated firmware builds.
idf_component_get_property(MODBUS_LIB espressif__esp-modbus COMPONENT_LIB)
target_compile_options(${MODBUS_LIB} PRIVATE -Wno-error=stringop-truncation)

# ArduinoJson 6.21.6 rebases its pool after realloc and triggers GCC 15 -O2
# diagnostics in inline headers. Retain diagnostics while allowing this pinned library.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=use-after-free -Wno-error=maybe-uninitialized)

include("${ROOT}/boards/common/camera_decoder.cmake")
