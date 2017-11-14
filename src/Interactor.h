#pragma once

namespace molumes
{
	class Viewer;

	class Interactor
	{
	public:
		Interactor(Viewer* viewer);
		Viewer * viewer();

		virtual void keyEvent(int key, int scancode, int action, int mods);
		virtual void mouseButtonEvent(int button, int action, int mods);
		virtual void cursorPosEvent(double xpos, double ypos);

	private:
		Viewer* m_viewer;
	};

}