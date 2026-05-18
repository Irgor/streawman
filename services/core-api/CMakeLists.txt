cmake_minimum_required(VERSION 3.16)
project(core-api VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/CPM.cmake)

CPMAddPackage("gh:drogonframework/drogon#v1.9.10")

add_executable(core-api src/main.cpp)
target_link_libraries(core-api PRIVATE drogon)
target_compile_options(core-api PRIVATE -Wall -Wextra)
