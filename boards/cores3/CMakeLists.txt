cmake_minimum_required(VERSION 3.22)
set(XTOUCH_BOARD "cores3" CACHE STRING "Board target")
include("${CMAKE_CURRENT_LIST_DIR}/../common/port_project.cmake")
project(${PORT_project})
