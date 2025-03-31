cmake_minimum_required(VERSION 3.16)
project(opt-iter-test VERSION 0.0.0)

include(cmake/prelude.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(lib/opt-iter)

function(create_exe name)
    add_executable(${name} source/${name}.cpp)
    target_link_libraries(${name} PRIVATE opt-iter)
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wconversion)
endfunction()

create_exe(bench)
create_exe(line_splitter)
create_exe(int_gen)
create_exe(lambda)
