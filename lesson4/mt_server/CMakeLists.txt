cmake_minimum_required(VERSION 3.10)

project(mt-server C CXX)

set(CMAKE_CXX_STANDARD 17)

set(${PROJECT_NAME}_SRC main.cpp)

find_package(Boost REQUIRED COMPONENTS system) # asio

source_group(source FILES ${${PROJECT_NAME}_SRC}})

add_executable("${PROJECT_NAME}" "${${PROJECT_NAME}_SRC}")

if(WIN32)
  target_link_libraries("${PROJECT_NAME}" wsock32 ws2_32)
else()
  target_include_directories("${PROJECT_NAME}" PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries("${PROJECT_NAME}" PRIVATE ${Boost_LIBRARIES} pthread)
endif()

