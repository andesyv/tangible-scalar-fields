#include "Interactor.h"
using namespace molumes;

Interactor::Interactor(Viewer* viewer) : m_viewer(viewer)
{

}

Viewer * Interactor::viewer()
{
	return m_viewer;
}

void Interactor::keyEvent(int key, int scancode, int action, int mods)
{
}

void Interactor::mouseButtonEvent(int button, int action, int mods)
{
}

void Interactor::cursorPosEvent(double xpos, double ypos)
{
}
