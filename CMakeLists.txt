cmake_minimum_required(VERSION 3.10)
project(MySDLGame)


find_package(SDL2 REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(SDL2_ttf REQUIRED)

add_executable(MySDLGame main.cpp)

target_link_libraries(MySDLGame SDL2::SDL2 SDL2_gfx SDL2_ttf)