cmake_minimum_required(VERSION 3.10)

project(perf_counter LANGUAGES C VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(perf_counter
    src/perf_counter.c
    include/perf_counter.h
)
add_library(perf_counter::perf_counter ALIAS perf_counter)

set_target_properties(perf_counter PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(perf_counter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_definitions(perf_counter PRIVATE
    PROJECT_VERSION="${PROJECT_VERSION}"
    _GNU_SOURCE
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(perf_counter PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wcast-align
        -Wwrite-strings
        -Wconversion
        -Wsign-conversion
        -Wstrict-prototypes
        -Wmissing-prototypes
        -Wundef
        -Wno-unused-parameter
    )
endif()

