cmake_minimum_required(VERSION 3.7)

project(gasand)
project(gasand-client)

set(CMAKE_CXX_STANDARD 17)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(gasand-server server.cpp)
add_executable(gasand client.cpp)
target_link_libraries(gasand-server ${SDL2_LIBRARIES})
target_link_libraries(gasand ${SDL2_LIBRARIES})
