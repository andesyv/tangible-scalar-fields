#pragma once
#define GLSL(A) #A

namespace molumes
{
	class Viewer;

	class Renderer
	{
	public:
		Renderer(Viewer* viewer);
		Viewer * viewer();
		virtual void display() = 0;

	private:
		Viewer* m_viewer;
	};

}