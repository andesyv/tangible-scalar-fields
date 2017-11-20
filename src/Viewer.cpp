#include "Viewer.h"

#include <glbinding/gl/gl.h>
#include <iostream>
#include "CameraInteractor.h"
#include "BoundingBoxRenderer.h"
#include "SphereRenderer.h"

using namespace molumes;
using namespace gl;
using namespace glm;

Viewer::Viewer(GLFWwindow *window, Scene *scene) : m_window(window), m_scene(scene)
{
	glfwSetWindowUserPointer(window, static_cast<void*>(this));
	glfwSetFramebufferSizeCallback(window, &Viewer::framebufferSizeCallback);
	glfwSetKeyCallback(window, &Viewer::keyCallback);
	glfwSetMouseButtonCallback(window, &Viewer::mouseButtonCallback);
	glfwSetCursorPosCallback(window, &Viewer::cursorPosCallback);

	m_interactors.emplace_back(std::make_unique<CameraInteractor>(this));
	m_renderers.emplace_back(std::make_unique<BoundingBoxRenderer>(this));
	m_renderers.emplace_back(std::make_unique<SphereRenderer>(this));

	int i = 1;

	std::cout << "Available renderers (use the number keys to toggle):" << std::endl;

	for (auto& r : m_renderers)
	{
		std::cout << "  " << i << " - " << typeid(*r.get()).name() << std::endl;
		++i;
	}

	std::cout << std::endl;

}

void Viewer::display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, viewportSize().x, viewportSize().y);

	for (auto& r : m_renderers)
	{
		if (r->isEnabled())
			r->display();
	}
}

GLFWwindow * Viewer::window()
{
	return m_window;
}

Scene* Viewer::scene()
{
	return m_scene;
}

ivec2 Viewer::viewportSize() const
{
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	return ivec2(width,height);
}

mat4 Viewer::modelTransform() const
{
	return m_modelTransform;
}

mat4 Viewer::viewTransform() const
{
	return m_viewTransform;
}

void Viewer::setModelTransform(const glm::mat4& m)
{
	m_modelTransform = m;
}

void Viewer::setViewTransform(const glm::mat4& m)
{
	m_viewTransform = m;
}

void Viewer::setProjectionTransform(const glm::mat4& m)
{
	m_projectionTransform = m;
}

mat4 Viewer::projectionTransform() const
{
	return m_projectionTransform;
}

mat4 Viewer::modelViewTransform() const
{
	return viewTransform()*modelTransform();
}

mat4 Viewer::modelViewProjectionTransform() const
{
	return projectionTransform()*modelViewTransform();
}

void Viewer::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		for (auto& i : viewer->m_interactors)
		{
			i->framebufferSizeEvent(width, height);
		}
	}
}

void Viewer::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (key == GLFW_KEY_F5 && action == GLFW_RELEASE)
		{
			for (auto& r : viewer->m_renderers)
			{
				std::cout << "Reloading shaders for instance of " << typeid(*r.get()).name() << " ... " << std::endl;

				for (auto& s : r->shaderFiles())
				{
					std::cout << "  " << s->shortInfo() << std::endl;
					s->reload();
				}
				std::cout << r->shaderFiles().size() << " shaders reloaded." << std::endl << std::endl;

			}
		}
		else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && action == GLFW_RELEASE)		
		{
			int index = key - GLFW_KEY_1;

			if (index < viewer->m_renderers.size())
			{
				bool enabled = viewer->m_renderers[index]->isEnabled();

				if (enabled)
					std::cout << "Renderer " << index + 1 << " of type " << typeid(*viewer->m_renderers[index].get()).name() << " is now disabled." << std::endl << std::endl;
				else
					std::cout << "Renderer " << index + 1 << " of type " << typeid(*viewer->m_renderers[index].get()).name() << " is now enabled." << std::endl << std::endl;

				viewer->m_renderers[index]->setEnabled(!enabled);
			}
		}


		for (auto& i : viewer->m_interactors)
		{
			i->keyEvent(key, scancode, action, mods);
		}
	}
}

void Viewer::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		for (auto& i : viewer->m_interactors)
		{
			i->mouseButtonEvent(button, action, mods);
		}
	}
}

void Viewer::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		for (auto& i : viewer->m_interactors)
		{
			i->cursorPosEvent(xpos, ypos);
		}
	}
}