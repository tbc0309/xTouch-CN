# Scope the PSRAM allocation policy to the pinned TinyH264 decoder archive.
# Do not wrap global SDK allocators: MQTT, Wi-Fi and display DMA keep their policies.
if(XTOUCH_FEATURE_CAMERA AND (IDF_TARGET STREQUAL "esp32s3" OR IDF_TARGET STREQUAL "esp32p4"))
 get_target_property(XTOUCH_DECODER_ARCHIVE tinyh264 IMPORTED_LOCATION)
 if(NOT XTOUCH_DECODER_ARCHIVE)
  message(FATAL_ERROR "The pinned TinyH264 decoder archive is missing")
 endif()
 set(XTOUCH_PSRAM_DECODER "${CMAKE_CURRENT_BINARY_DIR}/camera_decoder/libtinyh264.a")
 add_custom_command(OUTPUT "${XTOUCH_PSRAM_DECODER}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/camera_decoder"
  COMMAND "${CMAKE_OBJCOPY}"
   --redefine-sym heap_caps_malloc_prefer=xtouch_camera_decoder_malloc_prefer
   --redefine-sym heap_caps_calloc_prefer=xtouch_camera_decoder_calloc_prefer
   --redefine-sym malloc=xtouch_camera_decoder_malloc
   "${XTOUCH_DECODER_ARCHIVE}" "${XTOUCH_PSRAM_DECODER}"
  DEPENDS "${XTOUCH_DECODER_ARCHIVE}" VERBATIM)
 add_custom_target(xtouch_camera_decoder DEPENDS "${XTOUCH_PSRAM_DECODER}")
 add_dependencies(${COMPONENT_LIB} xtouch_camera_decoder)
 set_target_properties(tinyh264 PROPERTIES IMPORTED_LOCATION "${XTOUCH_PSRAM_DECODER}")
 # The imported decoder is linked after the main archive; keep its callbacks reachable.
 target_link_options(${COMPONENT_LIB} INTERFACE
  "LINKER:--undefined=xtouch_camera_decoder_malloc"
  "LINKER:--undefined=xtouch_camera_decoder_malloc_prefer"
  "LINKER:--undefined=xtouch_camera_decoder_calloc_prefer")
 target_sources(${COMPONENT_LIB} PRIVATE "${ROOT}/boards/common/camera_allocator.cpp")
endif()
