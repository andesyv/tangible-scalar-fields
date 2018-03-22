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

#include "SSAO.h"

namespace molumes
{
	class Viewer;

	class SphereRenderer : public Renderer
	{
	public:
		SphereRenderer(Viewer *viewer);
		virtual void display();
		virtual std::list<globjects::File*> shaderFiles() const;

	private:
		
		std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_vertices = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_atomData = std::make_unique<globjects::Buffer>();

		std::unique_ptr<globjects::Buffer> m_intersectionBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Texture> m_offsetTexture = nullptr;
		
		std::unique_ptr<globjects::VertexArray> m_vaoQuad = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesQuad = std::make_unique<globjects::Buffer>();
		
		std::unique_ptr<globjects::Program> m_programSphere = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programSpawn = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programShade = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programBlend = std::make_unique<globjects::Program>();

		std::unique_ptr<globjects::File> m_vertexShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_geometryShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceSphere = nullptr;	
		std::unique_ptr<globjects::Shader> m_fragmentShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_vertexShaderSourceImage = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderImage = nullptr;

		std::unique_ptr<globjects::File> m_geometryShaderSourceImage = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderImage = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceSpawn = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderSpawn = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceShade = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderShade = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceBlend = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderBlend = nullptr;

		std::array< std::unique_ptr<globjects::Framebuffer>, 2 > m_frameBuffers;
		std::array< std::unique_ptr<globjects::Texture>, 2 > m_positionTextures;
		std::array< std::unique_ptr<globjects::Texture>, 2 > m_normalTextures;
		std::array< std::unique_ptr<globjects::Texture>, 2 > m_depthTextures;
		std::unique_ptr<SSAO> m_ssao = nullptr;

		gl::GLsizei m_size;
		glm::ivec2 m_framebufferSize;
	};

}