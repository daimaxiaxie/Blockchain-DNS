cmake_minimum_required(VERSION 3.16)
project(Blockchain)

set(LIBRARY_OUTPUT_PATH ../)

add_library(${PROJECT_NAME} SHARED Blockchain.h Blockchain.cpp)