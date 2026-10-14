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
            src/tare/optional.hpp
            src/tare/result.hpp
            src/tare/panic.hpp
            src/tare/span.hpp
            src/tare/traits.hpp
            src/tare/types.hpp
)

set_target_properties(tare PROPERTIES CXX_EXTENSIONS OFF)

target_compile_features(tare INTERFACE cxx_std_26)
target_compile_options(tare INTERFACE
    $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wextra -Wpedantic -Wconversion -fcolor-diagnostics>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

option(TARE_BUILD_TESTS "Build tare tests" ${PROJECT_IS_TOP_LEVEL})
if(TARE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(TARE_BUILD_DOCS "Build tare documentation" OFF)
if(TARE_BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    set(DOXYGEN_PROJECT_NAME "tare")
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${PROJECT_SOURCE_DIR}/docs/index.md")
    set(DOXYGEN_FILE_PATTERNS *.hpp *.md)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXCLUDE_SYMBOLS tare::detail tare::detail::*)
    set(DOXYGEN_EXCLUDE "${PROJECT_SOURCE_DIR}/docs/doxygen-awesome-css")
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_DISABLE_INDEX NO)
    set(DOXYGEN_FULL_SIDEBAR NO)
    set(DOXYGEN_HTML_COLORSTYLE LIGHT)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET "${PROJECT_SOURCE_DIR}/docs/doxygen-awesome-css/doxygen-awesome.css")
    set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
    set(DOXYGEN_SHOW_FILES NO)
    set(DOXYGEN_SHOW_USED_FILES NO)
    set(DOXYGEN_VERBATIM_HEADERS NO)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)

    doxygen_add_docs(tare_docs
        "${PROJECT_SOURCE_DIR}/src/tare"
        "${PROJECT_SOURCE_DIR}/docs"
        COMMENT "Generating tare docs"
    )
endif()