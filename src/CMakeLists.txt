file(GLOB_RECURSE molumes_sources *.cpp *.h)
file(GLOB imgui_sources ${CMAKE_SOURCE_DIR}/lib/imgui/*.cpp ${CMAKE_SOURCE_DIR}/lib/imgui/*.h)

add_executable(molumes ${molumes_sources} ${imgui_sources})

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glm)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glbinding)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/globjects)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glfw)

find_package(glm REQUIRED)
find_package(glbinding REQUIRED)
find_package(globjects REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/lib/HBAOPlus/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/imgui/)

target_link_libraries(molumes PUBLIC glfw)
target_link_libraries(molumes PUBLIC glbinding::glbinding)
target_link_libraries(molumes PUBLIC globjects::globjects)
target_link_libraries(molumes PUBLIC ${CMAKE_SOURCE_DIR}/lib/HBAOPlus/lib/GFSDK_SSAO_GL.win64.lib)

set_target_properties(molumes PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
