cmake_minimum_required(VERSION 3.21)
project(PcapPlPl_snifer)

set(CMAKE_CXX_STANDARD 11)

include(FindPkgConfig)
pkg_check_modules(PCAPPP REQUIRED PcapPlusPlus)

add_executable(PcapPlPl_snifer main.cpp)

target_include_directories("${PROJECT_NAME}" PRIVATE "${PCAPPP_INCLUDE_DIRS}")
target_link_directories("${PROJECT_NAME}" PRIVATE "${PCAPPP_LIBRARY_DIRS}")
target_link_libraries("${PROJECT_NAME}" PRIVATE "${PCAPPP_LIBRARIES}")


