cmake_minimum_required(VERSION 3.16)
project(Blockchain_DNS)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(blockchain)
add_subdirectory(cache)

#include_directories(blockchain/include)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH $ORIGIN)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexec-charset=GBK")

find_package(Threads REQUIRED)

add_executable(Blockchain_DNS main.cpp config.cpp config.h server.cpp server.h global.cpp global.h dns.cpp dns.h taskmanager.cpp taskmanager.h blockchainmanager.cpp blockchainmanager.h forward.cpp forward.h)

target_link_libraries(Blockchain_DNS Blockchain Cache Threads::Threads libjemalloc.a dl)
