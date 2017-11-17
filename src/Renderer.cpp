#include "Renderer.h"
using namespace molumes;

Renderer::Renderer(Viewer* viewer) : m_viewer(viewer)
{

}

Viewer * Renderer::viewer()
{
	return m_viewer;
}

std::list<globjects::File*> Renderer::shaderFiles() const
{
	return std::list<globjects::File*>();
}