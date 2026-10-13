# SPDX-License-Identifier: BSL-1.0

option(TESTS_SANITIZE "Build tests with asan and ubsan" OFF)

add_executable(tare_tests
    main.cpp
    optional_test.cpp
    result_test.cpp
    span_test.cpp
)

if (TESTS_SANITIZE)
    target_compile_options(tare_tests PRIVATE
        -fsanitize=address,undefined
        -fno-sanitize-recover=all
        -fno-omit-frame-pointer
    )

    target_link_options(tare_tests PRIVATE -fsanitize=address,undefined)

    if (WIN32)
        set_target_properties(tare_tests PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
    endif ()
endif ()
 
target_link_libraries(tare_tests PRIVATE tare::tare)
target_compile_options(tare_tests PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-fconstexpr-steps=100000000>
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=100000000>
    $<$<CXX_COMPILER_ID:MSVC>:/constexpr:steps100000000>
) # stupid shit
 
add_test(NAME tare_tests COMMAND tare_tests)
 
