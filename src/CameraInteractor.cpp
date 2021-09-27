#include "CameraInteractor.h"
#include "Viewer.h"

#include <iostream>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <globjects/logging.h>
#include <imgui.h>

using namespace molumes;
using namespace glm;
using namespace gl;

CameraInteractor::CameraInteractor(Viewer * viewer) : Interactor(viewer)
{
	resetProjectionTransform();
	resetViewTransform();

	globjects::debug() << "Camera interactor usage:" << std::endl;
	globjects::debug() << "  Left mouse - rotate" << std::endl;
	globjects::debug() << "  Middle mouse - pan" << std::endl;
	globjects::debug() << "  Right mouse - zoom" << std::endl;
	globjects::debug() << "  R - reset view" << std::endl;
	globjects::debug() << "  Cursor left - rotate negative around current y-axis" << std::endl;
	globjects::debug() << "  Cursor right - rotate positive around current y-axis" << std::endl;
	globjects::debug() << "  Cursor up - rotate negative around current x-axis" << std::endl;
	globjects::debug() << "  Cursor right - rotate positive around current x-axis" << std::endl << std::endl;
}

void CameraInteractor::framebufferSizeEvent(int width, int height)
{
	float aspect = float(width) / float(height);

	if (m_perspective)
		viewer()->setProjectionTransform(perspective(m_fov, aspect, m_near, m_far));
	else
		viewer()->setProjectionTransform(ortho(-1.0f*aspect, 1.0f*aspect, -1.0f, 1.0f, m_near, m_far));
}

void CameraInteractor::keyEvent(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
	{
		m_light = true;
		m_mousePrevious = m_mouseCurrent;
		cursorPosEvent(m_mouseCurrent.x, m_mouseCurrent.x);
	}
	else if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
	{
		m_light = false;
	}
	else if (key == GLFW_KEY_R && action == GLFW_RELEASE)
	{
		resetViewTransform();
	}
	else if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
	{
		mat4 viewTransform = viewer()->viewTransform();
		mat4 inverseViewTransform = inverse(viewTransform);
		vec4 transformedAxis = inverseViewTransform * vec4(0.0, 1.0, 0.0, 0.0);

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
	else if (key == GLFW_KEY_B && action == GLFW_RELEASE)
	{
		std::cout << "Starting benchmark" << std::endl;

		m_benchmark = true;
		m_startTime = glfwGetTime();
		m_frameCount = 0;
	}
}

void CameraInteractor::mouseButtonEvent(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		m_rotating = true;
		m_mousePrevious = m_mouseCurrent;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		m_scaling = true;
		m_mousePrevious = m_mouseCurrent;
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
	{
		m_panning = true;
		m_mousePrevious = m_mouseCurrent;
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
	m_mouseCurrent.x = xpos;
	m_mouseCurrent.y = ypos;

	if (m_light)
	{
		vec3 v = arcballVector(m_mouseCurrent.x, m_mouseCurrent.y);
		mat4 viewTransform = viewer()->viewTransform();

		// 5 times the distance to the object in center) -----------------------------------------------------------------------
		mat4 lightTransform = inverse(viewTransform)*translate(mat4(1.0f), (5.0f*viewer()->m_scatterPlotDiameter)*v)*viewTransform;
		viewer()->setLightTransform(lightTransform);
		//----------------------------------------------------------------------------------------------------------------------	
	}

	const bool nequal = any(notEqual(m_mouseCurrent, m_mousePrevious));

	if (m_rotating)
	{
		if (nequal)
		{
			vec3 va = arcballVector(m_mousePrevious.x, m_mousePrevious.y);
			vec3 vb = arcballVector(m_mouseCurrent.x, m_mouseCurrent.y);
	
			if (va != vb)
			{
				float angle = acos(max(-1.0f, min(1.0f, dot(va, vb))));
				vec3 axis = cross(va, vb);
	
				mat4 viewTransform = viewer()->viewTransform();
				mat4 inverseViewTransform = inverse(viewTransform);
				vec4 transformedAxis = inverseViewTransform * vec4(axis, 0.0);
	
				mat4 newViewTransform = rotate(viewTransform, angle, vec3(transformedAxis));
				viewer()->setViewTransform(newViewTransform);
			}
		}
	
	}

	if (m_scaling)
	{
		if (nequal)
		{
			ivec2 viewportSize = viewer()->viewportSize();
			vec2 va = vec2{2.f, -2.f}*vec2{m_mousePrevious} / vec2{viewportSize} + vec2{-1.f, 1.f};
			vec2 vb = vec2{2.f, -2.f}*vec2{m_mouseCurrent} / vec2{viewportSize} + vec2{-1.f, 1.f};
			vec2 d = vb - va;

			float l = std::abs(d.x) > std::abs(d.y) ? d.x : d.y;
			float s = 1.0;

			//set the local and global scale factor
			if (l > 0.0f)
			{
				s += std::min(0.5f, length(d));
			}
			else
			{
				s -= std::min(0.5f, length(d));
			}

			viewer()->setScaleFactor(viewer()->scaleFactor()*s);
			mat4 viewTransform = viewer()->viewTransform();
			mat4 newViewTransform = scale(viewTransform, vec3(s, s, s));
			viewer()->setViewTransform(newViewTransform);
		}
	}

	if (m_panning)
	{
		if (nequal)
		{
			ivec2 viewportSize = viewer()->viewportSize();
			float aspect = float(viewportSize.x) / float(viewportSize.y);
			vec2 va = vec2{2.f, -2.f}*vec2{m_mousePrevious} / vec2{viewportSize} + vec2{-1.f, 1.f};
			vec2 vb = vec2{2.f, -2.f}*vec2{m_mouseCurrent} / vec2{viewportSize} + vec2{-1.f, 1.f};
			vec2 d = vb - va;

			mat4 viewTransform = viewer()->viewTransform();
			mat4 newViewTransform = translate(mat4(1.0), vec3(aspect*d.x, d.y, 0.0f))*viewTransform;
			viewer()->setViewTransform(newViewTransform);
		}
	}

	m_mousePrevious = m_mouseCurrent;

}

void CameraInteractor::scrollEvent(double xoffset, double yoffset) {
	m_scrollCurrent += dvec2{xoffset, yoffset};

	const auto deltaScroll = m_scrollCurrent.x + m_scrollCurrent.y - m_scrollPrevious.x - m_scrollPrevious.y;
	if (0.001 < std::abs(deltaScroll))
	{
		const auto newView = translate(mat4(1.0), vec3(0.f, 0.f, deltaScroll))*viewer()->viewTransform();
		viewer()->setViewTransform(newView);
		m_scrollPrevious = m_scrollCurrent;
	}
}

void CameraInteractor::display()
{
	if (m_benchmark)
	{
		m_frameCount++;

		mat4 viewTransform = viewer()->viewTransform();
		mat4 inverseViewTransform = inverse(viewTransform);
		vec4 transformedAxis = inverseViewTransform * vec4(0.0, 1.0, 0.0, 0.0);

		mat4 newViewTransform = rotate(viewTransform, pi<float>() / 180.0f, vec3(transformedAxis));
		viewer()->setViewTransform(newViewTransform);


		if (m_frameCount >= 360)
		{
			double currentTime = glfwGetTime();

			std::cout << "Benchmark finished." << std::endl;
			std::cout << "Rendered " << m_frameCount << " frames in " << (currentTime - m_startTime) << " seconds." << std::endl;
			std::cout << "Average frames/second: " << double(m_frameCount) / (currentTime - m_startTime) << std::endl;

			m_benchmark = false;
		}


	}

	bool globalPerspective = viewer()->m_perspective;
	int projectionOption = static_cast<int>(globalPerspective);

	if (ImGui::BeginMenu("Camera"))
	{
		ImGui::RadioButton("Orthographic", &projectionOption,0);
		ImGui::RadioButton("Perspective", &projectionOption,1);

		ImGui::EndMenu();
	}

	globalPerspective = static_cast<bool>(projectionOption);

	if (m_perspective != globalPerspective) {
		m_perspective = viewer()->m_perspective = globalPerspective;

		resetProjectionTransform();
	}
	

	/*
	if (m_light)
	{
		glDepthFunc(GL_ALWAYS);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(value_ptr(viewer()->projectionTransform()));

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(value_ptr(viewer()->modelViewTransform()));

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glPointSize(7.0);
		glBegin(GL_POINTS);
		glVertex3fv(value_ptr(viewer()->worldLightPosition()));
		glEnd();
	}*/

}

void CameraInteractor::resetProjectionTransform()
{
	vec2 viewportSize = viewer()->viewportSize();
	framebufferSizeEvent(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
}

void CameraInteractor::resetViewTransform()
{
	viewer()->setViewTransform(lookAt(vec3(0.0f, 0.0f, m_distance), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));

	// necessary to perform the bounding-box dependent adaption-------------------------------------------------------
	// --> making sure the resulting visualization has the same dimensions as the screenshot used in the user study
	mat4 newViewTransform = scale(viewer()->viewTransform(), vec3(0.5f, 0.5f, 0.5f));
	viewer()->setViewTransform(newViewTransform);
	//----------------------------------------------------------------------------------------------------------------
	
	setLightPosition();

	viewer()->setScaleFactor(1.0f);
}

void molumes::CameraInteractor::setLightPosition()
{
	// initial position of the light source (azimuth 120 degrees, elevation 45 degrees, 5 times the distance to the object in center) ---------------------------------------------------------------------------------------------------------
	glm::mat4 viewTransform = viewer()->viewTransform();
	glm::vec3 initLightDir = normalize(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	glm::mat4 newLightTransform = glm::inverse(viewTransform)*glm::translate(mat4(1.0f), ( 5 * viewer()->m_scatterPlotDiameter*initLightDir))*viewTransform;
	viewer()->setLightTransform(newLightTransform);
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

}

vec3 CameraInteractor::arcballVector(double x, double y)
{
	ivec2 viewportSize = viewer()->viewportSize();
	vec3 p = vec3(2.0f*float(x) / float(viewportSize.x) - 1.0f, -2.0f*float(y) / float(viewportSize.y) + 1.0f, 0.0);

	float length2 = p.x*p.x + p.y*p.y;

	if (length2 < 1.0f)
		p.z = sqrt(1.0f - length2);
	else
		p = normalize(p);

	return p;
}
