# SPDX-License-Identifier: BSL-1.0

add_executable(tare_tests
    optional_test.cpp
)
 
target_link_libraries(tare_tests PRIVATE tare::tare)
target_compile_options(tare_tests PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-fconstexpr-steps=100000000>
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=100000000>
    $<$<CXX_COMPILER_ID:MSVC>:/constexpr:steps100000000>
) # stupid shit
 
add_test(NAME tare_tests COMMAND tare_tests)
 
