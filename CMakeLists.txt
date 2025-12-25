cmake_minimum_required(VERSION 3.10)
project(perf_counter LANGUAGES C CXX VERSION 0.1.0)

#
# CMake options
#
option(PERF_COUNTER_BUILD_EXAMPLES "Build example programs" OFF)
message(STATUS "PERF_COUNTER_BUILD_EXAMPLES: ${PERF_COUNTER_BUILD_EXAMPLES}")

#
# Build type configuration
#
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (default Release)" FORCE)
endif()

#
# C11/C++11 standards
#
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#
# Link-time optimization (LTO)
#
include(CheckIPOSupported)
check_ipo_supported(RESULT is_lto_supported)

if(is_lto_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    message(WARNING "Link-time optimization (LTO) is not supported by the compiler.")
endif()

#
# Target definition: perf_counter
#
add_library(perf_counter
    src/perf_counter.c
    include/perf_counter.h
)
add_library(perf_counter::perf_counter ALIAS perf_counter)

set_target_properties(perf_counter PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(perf_counter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_definitions(perf_counter PRIVATE
    PROJECT_VERSION="${PROJECT_VERSION}"
    _GNU_SOURCE
)

#
# Compiler warnings
#
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

#
# Dependencies: libpfm (Optional)
#
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBPFM QUIET libpfm)
endif()

if(NOT LIBPFM_FOUND)
    find_path(LIBPFM_INCLUDE_DIRS perfmon/pfmlib.h)
    find_library(LIBPFM_LIBRARIES pfm)
    if(LIBPFM_INCLUDE_DIRS AND LIBPFM_LIBRARIES)
        set(LIBPFM_FOUND TRUE)
    endif()
endif()

if(LIBPFM_FOUND)
    message(STATUS "Found libpfm: ${LIBPFM_LIBRARIES}")
    target_compile_definitions(perf_counter PUBLIC PERF_COUNTER_WITH_LIBPFM)
    target_include_directories(perf_counter SYSTEM PRIVATE ${LIBPFM_INCLUDE_DIRS})
    target_link_libraries(perf_counter PRIVATE ${LIBPFM_LIBRARIES})
else()
    message(WARNING "Package 'libpfm' not found")
endif()

#
# Subdirectories
#
if (PERF_COUNTER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
