#include <iostream>

#include <glbinding/gl/gl.h>
#include <glbinding/ContextInfo.h>
#include <glbinding/Version.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <globjects/globjects.h>
#include <globjects/logging.h>

#include "Scene.h"
#include "Protein.h"
#include "Viewer.h"
#include "Interactor.h"
#include "Renderer.h"

using namespace gl;
using namespace glm;
using namespace globjects;
using namespace molumes;

void error_callback(int errnum, const char * errmsg)
{
	globjects::critical() << errnum << ": " << errmsg << std::endl;
}

int main(int /*argc*/, char * /*argv*/[])
{
	// Initialize GLFW
	if (!glfwInit())
		return 1;

	glfwSetErrorCallback(error_callback);

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 8);

	// Create a context and, if valid, make it current
	GLFWwindow * window = glfwCreateWindow(768, 768, "molumes", NULL, NULL);

	if (window == nullptr)
	{
		globjects::critical() << "Context creation failed - terminating execution.";

		glfwTerminate();
		return 1;
	}

	// Make context current
	glfwMakeContextCurrent(window);

	// Initialize globjects (internally initializes glbinding, and registers the current context)
	globjects::init();

	// Enable debug logging
	globjects::DebugMessage::enable();
	
	globjects::debug()
		<< "OpenGL Version:  " << glbinding::ContextInfo::version() << std::endl
		<< "OpenGL Vendor:   " << glbinding::ContextInfo::vendor() << std::endl
		<< "OpenGL Renderer: " << glbinding::ContextInfo::renderer() << std::endl;

	auto scene = std::make_unique<Scene>();
	scene->protein()->load("./dat/5ldw.pdb");
	auto viewer = std::make_unique<Viewer>(window, scene.get());

	// Scaling the model's bounding box to the canonical view volume
	vec3 boundingBoxSize = scene->protein()->maximumBounds() - scene->protein()->minimumBounds();
	float maximumSize = std::max({ boundingBoxSize.x, boundingBoxSize.y, boundingBoxSize.z });
	mat4 modelTransform =  scale(vec3(2.0f) / vec3(maximumSize)); 
	modelTransform = modelTransform * translate(-0.5f*(scene->protein()->minimumBounds() + scene->protein()->maximumBounds()));
	viewer->setModelTransform(modelTransform);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		viewer->display();
		glfwSwapBuffers(window);
		glfwWaitEvents();
	}

	// Destroy window
	glfwDestroyWindow(window);

	// Properly shutdown GLFW
	glfwTerminate();

	return 0;
}
