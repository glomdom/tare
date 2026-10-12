# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.28)
project(tare VERSION 0.0.1 LANGUAGES CXX)

if(MSVC)
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(tare INTERFACE)
add_library(tare::tare ALIAS tare)

target_sources(tare
    INTERFACE
        FILE_SET HEADERS
        BASE_DIRS src
        FILES
            src/tare/traits.hpp
            src/tare/optional.hpp
            src/tare/result.hpp
)

target_compile_features(tare INTERFACE cxx_std_26)

set_target_properties(tare PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(tare INTERFACE
    $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wextra -Wconversion>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

option(TARE_BUILD_TESTS "Build tare tests" ${PROJECT_IS_TOP_LEVEL})
if(TARE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()