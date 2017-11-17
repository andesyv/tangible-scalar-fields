#pragma once
#include <list>
#include <globjects/base/File.h>

namespace molumes
{
	class Viewer;

	class Renderer
	{
	public:
		Renderer(Viewer* viewer);
		Viewer * viewer();
		virtual void display() = 0;
		virtual std::list<globjects::File*> shaderFiles() const;

	private:
		Viewer* m_viewer;
	};

}