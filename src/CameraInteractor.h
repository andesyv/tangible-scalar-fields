#pragma once
#include "Interactor.h"
#include <glm/glm.hpp>

namespace molumes
{
	class Viewer;

	class CameraInteractor : public Interactor
	{
	public:
		CameraInteractor(Viewer * viewer);
		virtual void mouseButtonEvent(int button, int action, int mods);
		virtual void cursorPosEvent(double xpos, double ypos);

	private:

		glm::vec3 arcballVector(double x, double y);

		bool m_active = false;
		double m_xPrevious = 0.0, m_yPrevious = 0.0;
		double m_xCurrent = 0.0, m_yCurrent = 0.0;
	};

}