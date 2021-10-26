file(GLOB_RECURSE molumes_sources *.cpp *.h)
file(GLOB imgui_sources ${CMAKE_SOURCE_DIR}/lib/imgui/*.cpp ${CMAKE_SOURCE_DIR}/lib/imgui/*.h)
file(GLOB lodepng_sources ${CMAKE_SOURCE_DIR}/lib/lodepng/lodepng.cpp ${CMAKE_SOURCE_DIR}/lib/lodepng/lodepng.h)

target_sources(molumes PRIVATE ${molumes_sources})
target_sources(ext PRIVATE
		${imgui_sources}
		${lodepng_sources}
)

list(APPEND CMAKE_PREFIX_PATH
		${CMAKE_SOURCE_DIR}/lib/glm
		${CMAKE_SOURCE_DIR}/lib/glbinding
		${CMAKE_SOURCE_DIR}/lib/globjects
		${CMAKE_SOURCE_DIR}/lib/glfw
)
 
find_package(glm REQUIRED)
find_package(glbinding REQUIRED)
find_package(globjects REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Matlab COMPONENTS ENG_LIBRARY)
find_package(OpenMP)

target_include_directories(molumes PRIVATE
		${CMAKE_SOURCE_DIR}/lib/HBAOPlus/include
		${CMAKE_SOURCE_DIR}/lib/imgui/
		${CMAKE_SOURCE_DIR}/lib/lodepng/
		${CMAKE_SOURCE_DIR}/lib/tinyfd/
		${CMAKE_SOURCE_DIR}/lib/portable-file-dialogs/
)

target_link_libraries(molumes PUBLIC
		glfw
		glbinding::glbinding
		glbinding::glbinding-aux
		globjects::globjects
)
if(OpenMP_CXX_FOUND)
    target_link_libraries(molumes PUBLIC OpenMP::OpenMP_CXX)
endif()
target_link_libraries(molumes PUBLIC ext)

if (Matlab_FOUND)
	target_include_directories(molumes PRIVATE ${Matlab_INCLUDE_DIRS})
	target_link_libraries(molumes PUBLIC ${MATLAB_LIBRARIES})
	target_compile_definitions(molumes INTERFACE HAVE_MATLAB)
endif()

set_target_properties(molumes PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
