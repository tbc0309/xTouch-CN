# Filename coverage follows the MCU, not a particular board or resource size.
function(xtouch_filename_font_profile MCU REQUEST RESULT)
 if(NOT REQUEST MATCHES "^(auto|common-zh|ui)$")
  message(FATAL_ERROR "XTOUCH_FILENAME_FONT must be auto, common-zh or ui")
 endif()
 if(REQUEST STREQUAL "auto")
  if(MCU STREQUAL "esp32s3" OR MCU STREQUAL "esp32p4")
   set(SELECTED common-zh)
  else()
   set(SELECTED ui)
  endif()
 else()
  set(SELECTED "${REQUEST}")
 endif()
 set(${RESULT} "${SELECTED}" PARENT_SCOPE)
endfunction()

function(xtouch_filename_font_options TARGET)
 set(XTOUCH_FILENAME_FONT auto CACHE STRING "Filename coverage: auto, common-zh or ui")
 set_property(CACHE XTOUCH_FILENAME_FONT PROPERTY STRINGS auto common-zh ui)
 option(XTOUCH_FILENAME_PUNCTUATION "Supplement common filename punctuation" ON)
 xtouch_filename_font_profile("${IDF_TARGET}" "${XTOUCH_FILENAME_FONT}" PROFILE)
 set(COMMON_ENABLED OFF)
 set(PUNCTUATION_ENABLED OFF)
 if(XTOUCH_FEATURE_GCODE_BROWSER)
  if(PROFILE STREQUAL "common-zh")
   set(COMMON_ENABLED ON)
   target_sources(${TARGET} PRIVATE
    "${ROOT}/resources/fonts/file-names/common_zh_14.c"
    "${ROOT}/resources/fonts/file-names/common_zh_28.c"
    "${ROOT}/resources/fonts/file-names/common_zh_28_small.c")
  endif()
  if(XTOUCH_FILENAME_PUNCTUATION)
   set(PUNCTUATION_ENABLED ON)
   if(NOT COMMON_ENABLED)
    target_sources(${TARGET} PRIVATE
     "${ROOT}/resources/fonts/file-names/ui_captions_14.c"
     "${ROOT}/resources/fonts/file-names/ui_captions_28.c"
     "${ROOT}/resources/fonts/file-names/ui_captions_28_small.c")
   endif()
   target_sources(${TARGET} PRIVATE
    "${ROOT}/resources/fonts/file-names/punctuation_14.c"
    "${ROOT}/resources/fonts/file-names/punctuation_28.c"
    "${ROOT}/resources/fonts/file-names/punctuation_28_small.c")
  endif()
 endif()
 target_compile_definitions(${TARGET} PUBLIC
  XTOUCH_FILENAME_COMMON_ZH=$<BOOL:${COMMON_ENABLED}>
  XTOUCH_FILENAME_PUNCTUATION=$<BOOL:${PUNCTUATION_ENABLED}>)
 if(COMMON_ENABLED OR PUNCTUATION_ENABLED)
  target_sources(${TARGET} PRIVATE "${ROOT}/resources/fonts/file-names/filename_font.c")
  if(NOT TARGET xtouch_filename_fonts_check)
   if(PYTHON)
    set(FONT_CHECK_PYTHON "${PYTHON}")
   else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(FONT_CHECK_PYTHON "${Python3_EXECUTABLE}")
   endif()
   add_custom_target(xtouch_filename_fonts_check
    COMMAND "${FONT_CHECK_PYTHON}" "${ROOT}/scripts/filename_font_coverage.py" --check
    VERBATIM COMMENT "Checking filename supplement/UI coverage")
  endif()
  add_dependencies(${TARGET} xtouch_filename_fonts_check)
 endif()
endfunction()
