cmake_minimum_required(VERSION 3.16)
project(Cache)

set(LIBRARY_OUTPUT_PATH ../)

add_library(${PROJECT_NAME} SHARED Cache.h Cache.cpp)