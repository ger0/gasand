cmake_minimum_required(VERSION 3.7)

project(gasand)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(gasand main.cpp)
target_link_libraries(gasand ${SDL2_LIBRARIES})
