file(GLOB_RECURSE sources *.cpp *.h)
add_executable(molumes ${sources})

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glm)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glbinding)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/globjects)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glfw)

find_package(glm REQUIRED)
find_package(glbinding REQUIRED)
find_package(globjects REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(molumes PUBLIC glfw)
target_link_libraries(molumes PUBLIC glbinding::glbinding)
target_link_libraries(molumes PUBLIC globjects::globjects)

set_target_properties(molumes PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
