# The app descriptor, UI and release metadata must report the same version.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../../version.json")
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../version.json" XTOUCH_PROJECT_VERSION_JSON)
string(JSON PROJECT_VER GET "${XTOUCH_PROJECT_VERSION_JSON}" version)
if(NOT PROJECT_VER MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
 message(FATAL_ERROR "version.json must contain a release version such as 1.0.0")
endif()
