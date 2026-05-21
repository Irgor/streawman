cmake_minimum_required(VERSION 3.16)
project(core-api VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/CPM.cmake)

CPMAddPackage("gh:drogonframework/drogon#v1.9.10")

CPMAddPackage(
  NAME googletest
  GITHUB_REPOSITORY google/googletest
  GIT_TAG release-1.12.1
  VERSION 1.12.1
  OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
)

aux_source_directory(src/controllers/health CTL_SRC)

add_executable(core-api src/main.cpp ${CTL_SRC})
add_executable(core-api-tests tests/main.cpp ${CTL_SRC})

target_link_libraries(core-api PRIVATE drogon)
target_link_libraries(core-api-tests PRIVATE drogon gtest gtest_main gmock)

target_compile_options(core-api PRIVATE -Wall -Wextra)
target_compile_options(core-api-tests PRIVATE -Wall -Wextra)

enable_testing()
add_test(NAME core-api-tests COMMAND core-api-tests)
