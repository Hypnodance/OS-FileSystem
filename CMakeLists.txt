cmake_minimum_required(VERSION 3.16)
project(OS_FileSystem C)

set(CMAKE_C_STANDARD 99)

add_executable(OS_FileSystem main.c header.h file_system.c)