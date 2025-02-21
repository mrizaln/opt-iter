cmake_minimum_required(VERSION 3.16)
project(opt-iter-test VERSION 0.0.0)

include(cmake/prelude.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(lib/opt-iter)

add_executable(main source/main.cpp)
target_link_libraries(main PRIVATE opt-iter)
target_compile_options(main PRIVATE -Wall -Wextra -Wconversion)
