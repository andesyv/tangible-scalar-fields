# Custom find module that attempts to find the Force Dimension Haptic SDK (DHD)
# To work, the environment variable DHD_HOME should be set to the installation directory of the sdk
# (The installation directory of the Haptic SDK is typically the same as the Robotic one)

# (mostly copied from glbinding/FindKHR.cmake and glbinding/FindGLEW.cmake)
# Also from https://izzys.casa/2020/12/how-to-find-packages-with-cmake-the-basics/

include(FindPackageHandleStandardArgs)

# Determine platform (used by dhd library)
if ("${CMAKE_GENERATOR_PLATFORM}" MATCHES "(Win64|IA64)" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(X64 TRUE)
endif()
if (WIN32)
    if (DEFINED X64)
        set(DHD_DEFINITIONS "WIN64")
    else ()
        set(DHD_DEFINITIONS "WIN32")
    endif ()
elseif (APPLE)
    set(DHD_DEFINITIONS "MACOSX")
else ()
    set(DHD_DEFINITIONS "LINUX")
endif ()


# Include files
find_path(DHD_INCLUDE_DIR dhdc.h
        PATHS $ENV{DHD_HOME}
        PATH_SUFFIXES /include
        DOC "The directory where include/dhdc.h resides"
        )

if(DEFINED X64)
    find_library(DHD_LIBRARY dhdms64
            PATHS $ENV{DHD_HOME}
            PATH_SUFFIXES /lib /lib64
            )
else()
    find_library(DHD_LIBRARY dhdms
            PATHS $ENV{DHD_HOME}
            PATH_SUFFIXES /lib
            )
endif()

find_package_handle_standard_args(DHD
        "Failed to find Force Dimension Haptic SDK. Please set the DHD_HOME environment variable to the Force Dimension (Haptic) SDK installation root"
        DHD_INCLUDE_DIR DHD_LIBRARY DHD_DEFINITIONS)
if (DHD_FOUND)
    mark_as_advanced(DHD_INCLUDE_DIR)
    mark_as_advanced(DHD_LIBRARY)
    mark_as_advanced(DHD_DEFINITIONS)
endif()

if (DHD_FOUND AND NOT TARGET DHD::DHD)
    add_library(DHD::DHD STATIC IMPORTED)
    set_property(TARGET DHD::DHD PROPERTY IMPORTED_LOCATION ${DHD_LIBRARY})
    target_include_directories(DHD::DHD INTERFACE ${DHD_INCLUDE_DIR})
    target_compile_definitions(DHD::DHD INTERFACE ${DHD_DEFINITIONS})
endif()