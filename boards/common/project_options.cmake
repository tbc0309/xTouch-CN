# IDF expands component requirements in an isolated CMake script.
# Only explicit xTouch module/profile values are exported; SDK defaults stay SDK-owned.
get_cmake_property(XTOUCH_PROJECT_VARIABLES VARIABLES)
foreach(name IN LISTS XTOUCH_PROJECT_VARIABLES)
 if(name MATCHES "^XTOUCH_(FEATURE_[A-Z_]+|PORT_PROFILE|BSP_PROFILE|BOARD)$")
  idf_build_set_property(${name} "${${name}}")
 endif()
endforeach()
