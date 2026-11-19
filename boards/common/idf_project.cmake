# Shared module defaults apply to every standard and contributed board project.
include("${CMAKE_CURRENT_LIST_DIR}/interactive_modules.cmake")
if(XTOUCH_FEATURE_PIXEL_SCREEN)
 list(APPEND SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.pixel_screen.defaults")
 list(REMOVE_DUPLICATES SDKCONFIG_DEFAULTS)
endif()
# Initialize IDF, then export module/profile choices before project() scans dependencies.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/project_options.cmake")
