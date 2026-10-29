cmake_minimum_required(VERSION 3.22)
set(XTOUCH_BOARD "lcdwiki-es3c28p" CACHE STRING "Board target")
include("${CMAKE_CURRENT_LIST_DIR}/../common/port_project.cmake")
project(${PORT_project})
