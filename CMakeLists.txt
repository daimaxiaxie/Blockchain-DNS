cmake_minimum_required(VERSION 3.16)
project(Blockchain_DNS)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(blockchain)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH $ORIGIN)

add_executable(Blockchain_DNS main.cpp)

target_link_libraries(Blockchain_DNS Blockchain)
