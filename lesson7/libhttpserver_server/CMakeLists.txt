cmake_minimum_required(VERSION 3.22)
project(libhttpserver_server)

set(CMAKE_CXX_STANDARD 14)

include(FindPkgConfig)
pkg_check_modules(LIBHTTPSERVER REQUIRED libhttpserver)

add_executable(libhttpserver_server main.cpp)

target_link_directories("${PROJECT_NAME}" PRIVATE "${LIBHTTPSERVER_INCLUDE_DIRS}")
target_link_directories("${PROJECT_NAME}" PRIVATE "${LIBHTTPSERVER_LIBRARY_DIRS}")
target_link_libraries("${PROJECT_NAME}" PRIVATE "${LIBHTTPSERVER_LIBRARIES}")