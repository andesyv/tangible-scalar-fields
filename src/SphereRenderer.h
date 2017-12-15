#pragma once
#include "Renderer.h"
#include <memory>

#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glm/glm.hpp>

#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/Framebuffer.h>
#include <globjects/Renderbuffer.h>
#include <globjects/Texture.h>
#include <globjects/base/File.h>
#include <globjects/TextureHandle.h>

//#include "SSAO.h"

namespace molumes
{
	class Viewer;
	class SSAO;

	class SphereRenderer : public Renderer
	{
	public:
		SphereRenderer(Viewer *viewer);
		virtual void display();
		virtual std::list<globjects::File*> shaderFiles() const;

	private:
		
		std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_vertices = std::make_unique<globjects::Buffer>();

		std::unique_ptr<globjects::VertexArray> m_vaoQuad = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesQuad = std::make_unique<globjects::Buffer>();


		std::unique_ptr<globjects::Program> m_programSphere = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programShade = std::make_unique<globjects::Program>();

		std::unique_ptr<globjects::File> m_vertexShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::File> m_geometryShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::File> m_fragmentShaderSourceSphere = nullptr;
		
		std::unique_ptr<globjects::Shader> m_vertexShaderSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_vertexShaderSourceShade = nullptr;
		std::unique_ptr<globjects::File> m_geometryShaderSourceShade = nullptr;
		std::unique_ptr<globjects::File> m_fragmentShaderSourceShade = nullptr;

		std::unique_ptr<globjects::Shader> m_vertexShaderShade = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderShade = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderShade = nullptr;

		std::unique_ptr<globjects::Framebuffer> m_frameBuffer = nullptr;
		std::unique_ptr<globjects::Texture> m_colorTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_normalTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_depthTexture = nullptr;
		//std::unique_ptr<SSAO> m_ssao = nullptr;

		gl::GLsizei m_size;
		glm::ivec2 m_framebufferSize;
	};

}