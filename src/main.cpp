
#include <iostream>

#include <glbinding/gl/gl.h>
#include <glbinding/ContextInfo.h>
#include <glbinding/Version.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <globjects/globjects.h>
#include <globjects/logging.h>

using namespace gl;

void error(int errnum, const char * errmsg)
{
	globjects::critical() << errnum << ": " << errmsg << std::endl;
}

int main(int /*argc*/, char * /*argv*/[])
{
	// Initialize GLFW
	if (!glfwInit())
		return 1;

	glfwSetErrorCallback(error);
	glfwWindowHint(GLFW_VISIBLE, true);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

	// Create a context and, if valid, make it current
	GLFWwindow * window = glfwCreateWindow(768, 768, "globjects Command Line Output", NULL, NULL);

	if (window == nullptr)
	{
		globjects::critical() << "Context creation failed. Terminate execution.";

		glfwTerminate();
		return 1;
	}

	// Make context current
	glfwMakeContextCurrent(window);

	// Initialize globjects (internally initializes glbinding, and registers the current context)
	globjects::init();

	std::cout
		<< "OpenGL Version:  " << glbinding::ContextInfo::version() << std::endl
		<< "OpenGL Vendor:   " << glbinding::ContextInfo::vendor() << std::endl
		<< "OpenGL Renderer: " << glbinding::ContextInfo::renderer() << std::endl << std::endl;

	globjects::DebugMessage::enable();

	glBegin(GL_FRACTIONAL_EVEN);
	glEnd();

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Destroy window
	glfwDestroyWindow(window);

	// Properly shutdown GLFW
	glfwTerminate();

	return 0;
}
