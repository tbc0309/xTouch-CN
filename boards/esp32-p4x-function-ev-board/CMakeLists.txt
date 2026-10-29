cmake_minimum_required(VERSION 3.22)
set(XTOUCH_BOARD "esp32-p4x-function-ev-board" CACHE STRING "Board target")
include("${CMAKE_CURRENT_LIST_DIR}/../common/port_project.cmake")
project(${PORT_project})
