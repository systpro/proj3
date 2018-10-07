cmake_minimum_required(VERSION 3.10)
project(proj_3 C)

set(CMAKE_C_STANDARD 99)

add_executable(proj_3 main.c floppy.h)
target_link_libraries(proj_3 m)