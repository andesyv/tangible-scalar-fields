#include "Interactor.h"
#include "../Viewer.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using namespace molumes;

Interactor::Interactor(Viewer* viewer) : m_viewer(viewer)
{

}

Viewer * Interactor::viewer()
{
	return m_viewer;
}

void Interactor::setEnabled(bool enabled)
{
	m_enabled = enabled;
}

bool Interactor::isEnabled() const
{
	return m_enabled;
}

void Interactor::framebufferSizeEvent(int width, int height)
{
}

void Interactor::keyEvent(int key, int scancode, int action, int mods)
{
    if ((key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) && action == GLFW_PRESS)
        m_ctrl = true;
    else if ((key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) && action == GLFW_RELEASE)
        m_ctrl = false;
    else if (m_ctrl && key == GLFW_KEY_O && action == GLFW_PRESS)
        viewer()->openFile();
}

void Interactor::mouseButtonEvent(int button, int action, int mods)
{
}

void Interactor::cursorPosEvent(double xpos, double ypos)
{
}

void Interactor::scrollEvent(double xoffset, double yoffset)
{
}

void Interactor::display()
{
}

