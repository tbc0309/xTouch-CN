cmake_minimum_required(VERSION 3.22)
set(XTOUCH_BOARD "esp32-p4x-wifi6-touch-lcd-3.5" CACHE STRING "Board target")
include("${CMAKE_CURRENT_LIST_DIR}/../common/port_project.cmake")
project(${PORT_project})
