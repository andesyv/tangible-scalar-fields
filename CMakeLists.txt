cmake_minimum_required(VERSION 3.13)
project(molumes)

if (MSVC)
    # MSVC currently only supports some c++20 features in their "preview" branch, which is enabled in CMake by setting the standard to 23
    # See https://github.com/microsoft/STL/issues/1814
    set(CMAKE_CXX_STANDARD 23)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

add_library(ext)
add_executable(molumes)

# MSVC Specific options to silence warnings from external libraries
if (MSVC)
    target_compile_options(ext PRIVATE /w)
    target_compile_options(molumes PRIVATE /external:anglebrackets /external:I ./lib /external:W0 /WX)
    # For MSVC < 16.10:
    # target_compile_options(molumes PRIVATE /experimental:external /external:anglebrackets /external:I ./lib /external:W0 /WX)
endif()

add_compile_definitions($<$<CONFIG:RELEASE>:NDEBUG>)

set(CMAKE_BUILD_PARALLEL_LEVEL 8)

add_subdirectory(src)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT molumes)


