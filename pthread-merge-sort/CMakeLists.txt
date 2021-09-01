cmake_minimum_required(VERSION 3.19)
project(pthread_merge_sort C)

set(CMAKE_C_STANDARD 11)

add_executable(pthread_merge_sort main.c sort.h sort.c readFile.c readFile.h)