cmake_minimum_required(VERSION 3.16)
project(opt-iter-test VERSION 0.0.0)

include(cmake/prelude.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt REQUIRED)
find_package(ut REQUIRED)

add_subdirectory(lib/opt-iter)

option(OPT_ITER_TEST_RUN_AFTER_BUILD "immediately run test after build" ON)

function(make_test name)
    add_executable(${name} source/${name}.cpp)
    target_include_directories(${name} PRIVATE source)
    target_link_libraries(${name} PRIVATE opt-iter fmt::fmt Boost::ut)
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wconversion)

    # sanitizer
    target_compile_options(${name} PRIVATE -fsanitize=address,leak,undefined)
    target_link_options(${name} PRIVATE -fsanitize=address,leak,undefined)

    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
    if(OPT_ITER_TEST_RUN_AFTER_BUILD)
        add_custom_command(
            TARGET ${name}
            POST_BUILD
            COMMAND $<TARGET_FILE:${name}>
        )
    endif()
endfunction()

enable_testing()

make_test(opt_iter_test)
