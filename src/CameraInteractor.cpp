#include "CameraInteractor.h"

#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"

using namespace molumes;
using namespace glm;

CameraInteractor::CameraInteractor(Viewer * viewer) : Interactor(viewer)
{
	vec2 viewportSize = viewer->viewportSize();
	float aspect = float(viewportSize.x) / float(viewportSize.y);

	viewer->setProjectionTransform(perspective(45.0f, aspect, 0.0125f, 16.0f));
	viewer->setViewTransform(lookAt(vec3(0.0f, 0.0f, -2.0f*sqrt(3.0f)), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));

}

void CameraInteractor::mouseButtonEvent(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		m_active = true;
		m_xPrevious = m_xCurrent;
		m_yPrevious = m_yCurrent;
	}
	else
	{
		m_active = false;
	}
}

void CameraInteractor::cursorPosEvent(double xpos, double ypos)
{
	m_xCurrent = xpos;
	m_yCurrent = ypos;

	if (m_active)
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
