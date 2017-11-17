#include "CameraInteractor.h"

#include <iostream>
#include <algorithm>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "Viewer.h"

using namespace molumes;
using namespace glm;

CameraInteractor::CameraInteractor(Viewer * viewer) : Interactor(viewer)
{
	resetProjectionTransform();
	resetViewTransform();

	std::cout << "Camera interactor usage:" << std::endl;
	std::cout << "  Left mouse - rotate" << std::endl;
	std::cout << "  Middle mouse - pan" << std::endl;
	std::cout << "  Right mouse - zoom" << std::endl;
	std::cout << "  Home - reset view" << std::endl;
	std::cout << "  Cursor left - rotate negative around current y-axis" << std::endl;
	std::cout << "  Cursor right - rotate positive around current y-axis" << std::endl;
	std::cout << "  Cursor up - rotate negative around current x-axis" << std::endl;
	std::cout << "  Cursor right - rotate positive around current x-axis" << std::endl << std::endl;
}

void CameraInteractor::framebufferSizeEvent(int width, int height)
{
	float aspect = float(width) / float(height);
	viewer()->setProjectionTransform(perspective(m_fov, aspect, m_near, m_far));
}

void CameraInteractor::keyEvent(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_HOME && action == GLFW_RELEASE)
	{
		resetViewTransform();
	}
	else if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
	{
		mat4 viewTransform = viewer()->viewTransform();
		mat4 inverseViewTransform = inverse(viewTransform);
		vec4 transformedAxis = inverseViewTransform * vec4(0.0,1.0,0.0,0.0);

		mat4 newViewTransform = rotate(viewTransform, -0.5f*quarter_pi<float>(), vec3(transformedAxis));
		viewer()->setViewTransform(newViewTransform);
	}
	else if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
	{
		mat4 viewTransform = viewer()->viewTransform();
		mat4 inverseViewTransform = inverse(viewTransform);
		vec4 transformedAxis = inverseViewTransform * vec4(0.0, 1.0, 0.0, 0.0);

		mat4 newViewTransform = rotate(viewTransform, 0.5f*quarter_pi<float>(), vec3(transformedAxis));
		viewer()->setViewTransform(newViewTransform);
	}
	else if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
	{
		mat4 viewTransform = viewer()->viewTransform();
		mat4 inverseViewTransform = inverse(viewTransform);
		vec4 transformedAxis = inverseViewTransform * vec4(1.0, 0.0, 0.0, 0.0);

		mat4 newViewTransform = rotate(viewTransform, -0.5f*quarter_pi<float>(), vec3(transformedAxis));
		viewer()->setViewTransform(newViewTransform);
	}
	else if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
	{
		mat4 viewTransform = viewer()->viewTransform();
		mat4 inverseViewTransform = inverse(viewTransform);
		vec4 transformedAxis = inverseViewTransform * vec4(1.0, 0.0, 0.0, 0.0);

		mat4 newViewTransform = rotate(viewTransform, 0.5f*quarter_pi<float>(), vec3(transformedAxis));
		viewer()->setViewTransform(newViewTransform);
	}
}

void CameraInteractor::mouseButtonEvent(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		m_rotating = true;
		m_xPrevious = m_xCurrent;
		m_yPrevious = m_yCurrent;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		m_scaling = true;
		m_xPrevious = m_xCurrent;
		m_yPrevious = m_yCurrent;
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
	{
		m_panning = true;
		m_xPrevious = m_xCurrent;
		m_yPrevious = m_yCurrent;
	}
	else
	{
		m_rotating = false;
		m_scaling = false;
		m_panning = false;
	}
}

void CameraInteractor::cursorPosEvent(double xpos, double ypos)
{
	m_xCurrent = xpos;
	m_yCurrent = ypos;

	if (m_rotating)
	{
		if (m_xCurrent != m_xPrevious && m_yCurrent != m_yPrevious)
		{
			vec3 va = arcballVector(m_xPrevious, m_yPrevious);
			vec3 vb = arcballVector(m_xCurrent, m_yCurrent);
			float angle = acos(min(1.0f, dot(va, vb)));
			vec3 axis = cross(va, vb);

			mat4 viewTransform = viewer()->viewTransform();
			mat4 inverseViewTransform = inverse(viewTransform);
			vec4 transformedAxis = inverseViewTransform * vec4(axis, 0.0);

			mat4 newViewTransform = rotate(viewTransform, angle, vec3(transformedAxis));
			viewer()->setViewTransform(newViewTransform);

			m_xPrevious = m_xCurrent;
			m_yPrevious = m_yCurrent;
		}
	}

	if (m_scaling)
	{
		if (m_xCurrent != m_xPrevious && m_yCurrent != m_yPrevious)
		{
			ivec2 viewportSize = viewer()->viewportSize();
			vec2 va = vec2(2.0f*float(m_xPrevious) / float(viewportSize.x) - 1.0f, -2.0f*float(m_yPrevious) / float(viewportSize.y) + 1.0f);
			vec2 vb = vec2(2.0f*float(m_xCurrent) / float(viewportSize.x) - 1.0f, -2.0f*float(m_yCurrent) / float(viewportSize.y) + 1.0f);
			vec2 d = vb - va;

			float l = std::abs(d.x) > std::abs(d.y) ? d.x : d.y;
			float s = 1.0;

			if (l > 0.0f)
			{
				s += std::min(0.5f, 2.0f*length(d));
			}
			else
			{
				s -= std::min(0.5f, 2.0f*length(d));
			}

			mat4 viewTransform = viewer()->viewTransform();
			mat4 newViewTransform = scale(viewTransform, vec3(s, s, s));
			viewer()->setViewTransform(newViewTransform);

			m_xPrevious = m_xCurrent;
			m_yPrevious = m_yCurrent;
		}
	}

	if (m_panning)
	{
		if (m_xCurrent != m_xPrevious && m_yCurrent != m_yPrevious)
		{
			ivec2 viewportSize = viewer()->viewportSize();
			vec2 va = vec2(2.0f*float(m_xPrevious) / float(viewportSize.x) - 1.0f, -2.0f*float(m_yPrevious) / float(viewportSize.y) + 1.0f);
			vec2 vb = vec2(2.0f*float(m_xCurrent) / float(viewportSize.x) - 1.0f, -2.0f*float(m_yCurrent) / float(viewportSize.y) + 1.0f);
			vec2 d = vb - va;

			mat4 viewTransform = viewer()->viewTransform();
			mat4 newViewTransform = translate(mat4(1.0),vec3(2.0f*d.x,2.0f*d.y,0.0f))*viewTransform;
			viewer()->setViewTransform(newViewTransform);

			m_xPrevious = m_xCurrent;
			m_yPrevious = m_yCurrent;
		}
	}
}

void CameraInteractor::resetProjectionTransform()
{
	vec2 viewportSize = viewer()->viewportSize();
	framebufferSizeEvent(viewportSize.x, viewportSize.y);
}

void CameraInteractor::resetViewTransform()
{
	viewer()->setViewTransform(lookAt(vec3(0.0f, 0.0f, -m_distance), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));
}

vec3 CameraInteractor::arcballVector(double x, double y)
{
	ivec2 viewportSize = viewer()->viewportSize();
	vec3 p = vec3(2.0f*float(x) / float(viewportSize.x)-1.0f, -2.0f*float(y) / float(viewportSize.y)+1.0f, 0.0);

	float length2 = p.x*p.x + p.y*p.y;

	if (length2 < 1.0f)
		p.z = sqrt(1.0f - length2);
	else
		p = normalize(p);

	return p;
}
