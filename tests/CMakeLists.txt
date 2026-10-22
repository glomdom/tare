# SPDX-License-Identifier: BSL-1.0

option(TARE_SANITIZE_TESTS "Build tests with asan and ubsan" OFF)

include(FetchContent)
include(GoogleTest)

if (TARE_SANITIZE_TESTS)
    add_compile_options(
        -fsanitize=address,undefined
        -fno-sanitize-recover=all
        -fno-omit-frame-pointer
    )

    add_link_options(-fsanitize=address,undefined)

    if (WIN32)
        set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
    endif ()
endif ()

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.18.0
    GIT_SHALLOW TRUE
    SYSTEM
)

FetchContent_MakeAvailable(googletest)

add_executable(tare_tests
    heap_allocator_test.cpp
    optional_test.cpp
    result_test.cpp
    span_test.cpp
)

target_link_libraries(tare_tests PRIVATE tare::tare GTest::gtest_main)
target_compile_features(tare_tests PRIVATE cxx_std_26)
target_compile_options(tare INTERFACE
    $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wextra -Wpedantic -Wconversion -fcolor-diagnostics>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

gtest_discover_tests(tare_tests)