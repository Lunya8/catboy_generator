cmake_minimum_required(VERSION 3.22)
project(daily_catboy C)

set(CMAKE_C_STANDARD 99)

add_executable(daily_catboy main.c)
target_link_libraries(daily_catboy curl SDL2 SDL2_image ${CMAKE_SOURCE_DIR}/lib/libjansson.a)
target_include_directories(daily_catboy PUBLIC inc)
