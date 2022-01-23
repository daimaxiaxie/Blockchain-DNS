cmake_minimum_required(VERSION 3.16)
project(Cache)

set(LIBRARY_OUTPUT_PATH ../)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-fPIC)
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

add_library(${PROJECT_NAME} SHARED Cache.h Cache.cpp list.cpp list.h)

target_link_libraries(${PROJECT_NAME} libjemalloc.a dl Threads::Threads)