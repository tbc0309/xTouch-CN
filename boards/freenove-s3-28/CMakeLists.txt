cmake_minimum_required(VERSION 3.22)
set(XTOUCH_BOARD "freenove-s3-28" CACHE STRING "Board target")
include("${CMAKE_CURRENT_LIST_DIR}/../common/port_project.cmake")
project(${PORT_project})
