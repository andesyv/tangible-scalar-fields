cmake_minimum_required(VERSION 3.13)
project(molumes)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

add_library(ext)
add_executable(molumes)

if (MSVC)
    target_compile_options(ext PRIVATE /w)
    # /experimental:external only needed for MSVC < 16.10
    target_compile_options(molumes PRIVATE /experimental:external /external:anglebrackets /external:I ./lib /external:W0 /WX)
endif()

set(CMAKE_BUILD_PARALLEL_LEVEL 8)

add_subdirectory(src)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT molumes)


