cmake_minimum_required(VERSION 3.22)
project(libcurl_client)

set(CMAKE_CXX_STANDARD 14)

include(FindPkgConfig)
pkg_check_modules(LIBCURL REQUIRED libcurl)

add_executable(libcurl_client main.cpp)

target_link_directories("${PROJECT_NAME}" PRIVATE "${LIBCURL_INCLUDE_DIRS}")
target_link_directories("${PROJECT_NAME}" PRIVATE "${LIBCURL_LIBRARY_DIRS}")
target_link_libraries("${PROJECT_NAME}" PRIVATE "${LIBCURL_LIBRARIES}")