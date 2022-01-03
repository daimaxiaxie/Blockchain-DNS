cmake_minimum_required(VERSION 3.16)
project(Blockchain)

set(LIBRARY_OUTPUT_PATH ../)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

add_library(${PROJECT_NAME} SHARED Blockchain.h Blockchain.cpp)

target_link_libraries(Blockchain ${CURL_LIBRARIES})