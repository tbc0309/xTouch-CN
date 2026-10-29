cmake_minimum_required(VERSION 3.22)
set(XTOUCH_BOARD "espressif-s3-lcd-ev2" CACHE STRING "Board target")
include("${CMAKE_CURRENT_LIST_DIR}/../common/port_project.cmake")
project(${PORT_project})
