# Custom find module that attempts to find the Force Dimension Robotic SDK (DRD)
# To work, the environment variable DRD_HOME should be set to the installation directory of the sdk
# (The installation directory of the Robotic SDK is typically the same as the Haptic one)

# (mostly copied from glbinding/FindKHR.cmake and glbinding/FindGLEW.cmake)
# Also from https://izzys.casa/2020/12/how-to-find-packages-with-cmake-the-basics/

include(FindPackageHandleStandardArgs)

# Determine platform (used by drd library)
if ("${CMAKE_GENERATOR_PLATFORM}" MATCHES "(Win64|IA64)" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(X64 1)
endif()
if (WIN32)
    if (DEFINED X64)
        set(DRD_DEFINITIONS "WIN64")
    else ()
        set(DRD_DEFINITIONS "WIN32")
    endif ()
elseif (APPLE)
    set(DRD_DEFINITIONS "MACOSX")
else ()
    set(DRD_DEFINITIONS "LINUX")
endif ()


# Include files
find_path(DRD_INCLUDE_DIR drdc.h
        PATHS $ENV{DRD_HOME}
        PATH_SUFFIXES /include
        DOC "The directory where include/drdc.h resides"
        )

if(DEFINED X64)
    find_library(DRD_LIBRARY drdms64
            PATHS $ENV{DRD_HOME}
            PATH_SUFFIXES /lib
            )
else()
    find_library(DRD_LIBRARY drdms
            PATHS $ENV{DRD_HOME}
            PATH_SUFFIXES /lib /lib64
            )
endif()

find_package_handle_standard_args(DRD
        "Failed to find Force Dimension Haptic SDK. Please set the DRD_HOME environment variable to the Force Dimension (Haptic) SDK installation root"
        DRD_INCLUDE_DIR DRD_LIBRARY DRD_DEFINITIONS)
if (DRD_FOUND)
    mark_as_advanced(DRD_INCLUDE_DIR)
    mark_as_advanced(DRD_LIBRARY)
    mark_as_advanced(DRD_DEFINITIONS)
endif()

if (DRD_FOUND AND NOT TARGET DRD::DRD)
    add_library(DRD::DRD STATIC IMPORTED)
    set_property(TARGET DRD::DRD PROPERTY IMPORTED_LOCATION ${DRD_LIBRARY})
    target_include_directories(DRD::DRD INTERFACE ${DRD_INCLUDE_DIR})
    target_compile_definitions(DRD::DRD INTERFACE ${DRD_DEFINITIONS})
endif()