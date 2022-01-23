cmake_minimum_required(VERSION 3.16)
project(Blockchain)

set(LIBRARY_OUTPUT_PATH ../)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

find_library(BLAKE2_LIBRARY b2 ${PROJECT_SOURCE_DIR}/lib)

add_library(${PROJECT_NAME} SHARED Blockchain.h Blockchain.cpp scale.cpp scale.h)

target_include_directories(Blockchain PUBLIC include)

target_link_libraries(Blockchain ${CURL_LIBRARIES} jsoncpp xxhash ${BLAKE2_LIBRARY})