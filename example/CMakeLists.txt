cmake_minimum_required(VERSION 3.16)
project(opt-iter-test VERSION 0.0.0)

include(cmake/prelude.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(lib/opt-iter)

add_executable(bench source/bench.cpp)
target_link_libraries(bench PRIVATE opt-iter)
target_compile_options(bench PRIVATE -Wall -Wextra -Wconversion)

add_executable(line_splitter source/line_splitter.cpp)
target_link_libraries(line_splitter PRIVATE opt-iter)
target_compile_options(line_splitter PRIVATE -Wall -Wextra -Wconversion)

add_executable(int_gen source/int_gen.cpp)
target_link_libraries(int_gen PRIVATE opt-iter)
target_compile_options(int_gen PRIVATE -Wall -Wextra -Wconversion)
