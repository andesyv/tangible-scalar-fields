#pragma once

#include <memory>
#include "Renderer.h"

namespace globjects
{
	class VertexArray;
	class Buffer;
}

namespace molumes
{
	class Viewer;

	class CrystalRenderer : public Renderer
	{
	public:
		CrystalRenderer(Viewer *viewer);
		void setEnabled(bool enabled) override;
		virtual void display() override;

	private:
		std::unique_ptr<globjects::VertexArray> m_vao;
		std::unique_ptr<globjects::Buffer> m_vertexBuffer;
	};
}