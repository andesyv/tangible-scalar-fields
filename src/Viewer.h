#pragma once

#include <memory>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Scene.h"
#include "Interactor.h"
#include "Renderer.h"

namespace molumes
{

	class Viewer
	{
	public:
		Viewer(GLFWwindow* window, Scene* scene);
		void display();

		GLFWwindow * window();
		Scene* scene();

		glm::ivec2 viewportSize() const;

		glm::mat4 modelTransform() const;
		glm::mat4 viewTransform() const;
		glm::mat4 projectionTransform() const;

		void setViewTransform(const glm::mat4& m);
		void setModelTransform(const glm::mat4& m);
		void setProjectionTransform(const glm::mat4& m);

		glm::mat4 modelViewTransform() const;
		glm::mat4 modelViewProjectionTransform() const;

	private:

		static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

		GLFWwindow* m_window;
		Scene *m_scene;

		std::vector<std::unique_ptr<Interactor>> m_interactors;
		std::vector<std::unique_ptr<Renderer>> m_renderers;

		glm::mat4 m_modelTransform = glm::mat4(1.0);
		glm::mat4 m_viewTransform = glm::mat4(1.0);
		glm::mat4 m_projectionTransform = glm::mat4(1.0);
	};


}