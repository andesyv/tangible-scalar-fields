find_package(glm QUIET)
find_package(glfw3 QUIET)
find_package(glbinding QUIET)
find_package(globjects QUIET)

if (NOT ${glm_FOUND} OR NOT ${glfw3_FOUND} OR NOT ${glbinding_FOUND} OR NOT ${globjects_FOUND})
    message("Missing required package. Fetching and rebuilding dependencies.")
    execute_process(
            COMMAND ${CMAKE_SOURCE_DIR}/config/fetch-libs.cmd
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE EXT_BUILD_RESULT
    )
    if (NOT EXT_BUILD_RESULT EQUAL "0")
        message(ERROR "Failed to fetch external packages.")
    endif()
    if (WIN32)
        execute_process(
                COMMAND ${CMAKE_SOURCE_DIR}/config/build-libs.cmd
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                RESULT_VARIABLE EXT_BUILD_RESULT
        )
        if (NOT EXT_BUILD_RESULT EQUAL "0")
            message(ERROR "Failed to build external packages.")
        endif()
        execute_process(
                COMMAND ${CMAKE_SOURCE_DIR}/config/copy-libs.cmd
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                RESULT_VARIABLE EXT_BUILD_RESULT
        )
        if (NOT EXT_BUILD_RESULT EQUAL "0")
            message(WARNING "Failed to copy dll's.")
        endif()
    else()
        execute_process(
                COMMAND ${CMAKE_SOURCE_DIR}/config/linux-build-libs.sh
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                RESULT_VARIABLE EXT_BUILD_RESULT
        )
        if (NOT EXT_BUILD_RESULT EQUAL "0")
            message(ERROR "Failed to build external packages.")
        endif()
    endif()
endif()