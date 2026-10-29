cmake_minimum_required(VERSION 3.22)
set(XTOUCH_BOARD "waveshare-s3-43c" CACHE STRING "Board target")
include("${CMAKE_CURRENT_LIST_DIR}/../common/port_project.cmake")
project(${PORT_project})
