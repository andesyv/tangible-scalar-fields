#include "MolumeRenderer.h"
#include <globjects/base/File.h>
#include <iostream>
#include "Viewer.h"
#include "Protein.h"
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>


using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

MolumeRenderer::MolumeRenderer(Viewer* viewer) : Renderer(viewer)
{
	static ivec3 pageSize = Molume::pageSize();
	static float offset = 0.0f;// 2.0f*sqrt(3.0f);
	static std::array<vec3, 8> vertices {{
		// front
		{ -offset, -offset, offset+pageSize.z },
		{ offset+pageSize.x, -offset, offset+pageSize.z },
		{ offset+pageSize.x, offset+pageSize.y, offset+pageSize.z },
		{ -offset, offset+pageSize.y, offset+pageSize.z },
		// back
		{ -offset, -offset, -offset },
		{ offset+pageSize.x, -offset, -offset },
		{ offset+pageSize.x, offset+pageSize.y, -offset },
		{ -offset, offset+pageSize.y, -offset }
	}};
	
	static std::array< std::array<GLushort, 4>, 6> indices{ {
		// front
		{0,1,2,3},
		// top
		{1,5,6,2},
		// back
		{7,6,5,4},
		// bottom
		{4,0,3,7},
		// left
		{4,5,1,0},
		// right
		{3,2,6,7}
	}};


	m_molume.insert(viewer->scene()->protein()->atoms());

	m_indices->setData(indices, GL_STATIC_DRAW);
	m_vertices->setData(vertices, GL_STATIC_DRAW);
	m_pagePositions->setData(m_molume.pagePositions(), GL_STATIC_DRAW);

	m_size = static_cast<GLsizei>(indices.size()*indices.front().size());
	m_vao->bindElementBuffer(m_indices.get());

	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices.get(), 0, sizeof(vec3));
	vertexBinding->setFormat(3, GL_FLOAT);
	vertexBinding->setDivisor(0);
	m_vao->enable(0);

	auto pagePositionBinding = m_vao->binding(1);
	pagePositionBinding->setAttribute(1);
	pagePositionBinding->setBuffer(m_pagePositions.get(), 0, sizeof(vec3));
	pagePositionBinding->setFormat(3, GL_FLOAT);
	pagePositionBinding->setDivisor(1);
	m_vao->enable(1);

	m_vao->unbind();

	m_verticesQuad->setData(std::array<vec3, 1>({ vec3(0.0f, 0.0f, 0.0f) }), GL_STATIC_DRAW);
	auto vertexBindingQuad = m_vaoQuad->binding(0);
	vertexBindingQuad->setBuffer(m_verticesQuad.get(), 0, sizeof(vec3));
	vertexBindingQuad->setFormat(3, GL_FLOAT);
	m_vaoQuad->enable(0);
	m_vaoQuad->unbind();

	m_vertexShaderSource = Shader::sourceFromFile("./res/molume/molumepage-vs.glsl");
	m_tesselationControlShaderSource = Shader::sourceFromFile("./res/molume/molumepage-tcs.glsl");
	m_tesselationEvaluationShaderSource = Shader::sourceFromFile("./res/molume/molumepage-tes.glsl");
	m_geometryShaderSource = Shader::sourceFromFile("./res/molume/molumepage-gs.glsl");
	m_fragmentShaderSource = Shader::sourceFromFile("./res/molume/molumepage-fs.glsl");

	m_vertexShader = Shader::create(GL_VERTEX_SHADER, m_vertexShaderSource.get());
	m_tesselationControlShader = Shader::create(GL_TESS_CONTROL_SHADER, m_tesselationControlShaderSource.get());
	m_tesselationEvaluationShader = Shader::create(GL_TESS_EVALUATION_SHADER, m_tesselationEvaluationShaderSource.get());
	m_geometryShader = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderSource.get());
	m_fragmentShader = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSource.get());

	m_program->attach(m_vertexShader.get(), m_tesselationControlShader.get(), m_tesselationEvaluationShader.get(), m_geometryShader.get(), m_fragmentShader.get());

	m_vertexShaderSourceCast = Shader::sourceFromFile("./res/molume/molumecast-vs.glsl");
	m_geometryShaderSourceCast = Shader::sourceFromFile("./res/molume/molumecast-gs.glsl");
	m_fragmentShaderSourceCast = Shader::sourceFromFile("./res/molume/molumecast-fs.glsl");

	m_vertexShaderCast = Shader::create(GL_VERTEX_SHADER, m_vertexShaderSourceCast.get());
	m_geometryShaderCast = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderSourceCast.get());
	m_fragmentShaderCast = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSourceCast.get());

	m_programShade->attach(m_vertexShaderCast.get(), m_geometryShaderCast.get(), m_fragmentShaderCast.get());
	m_framebufferSize = viewer->viewportSize();

	m_colorTexture = Texture::create(GL_TEXTURE_2D);
	m_colorTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_entryTexture = Texture::create(GL_TEXTURE_2D);
	m_entryTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_entryTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_entryTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_entryTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_entryTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_exitTexture = Texture::create(GL_TEXTURE_2D);
	m_exitTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_exitTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_exitTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_exitTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_exitTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_depthTexture = Texture::create(GL_TEXTURE_2D);
	m_depthTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_depthTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_depthTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_depthTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_depthTexture->image2D(0, GL_DEPTH_COMPONENT32F, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

	m_frameBuffer = Framebuffer::create();
	m_frameBuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_colorTexture.get());
	m_frameBuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_entryTexture.get());
	m_frameBuffer->attachTexture(GL_COLOR_ATTACHMENT2, m_exitTexture.get());
	m_frameBuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_frameBuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2});

	m_frameBuffer->printStatus();

	m_molumeTexture = Texture::create(GL_TEXTURE_3D);

	m_molumeTexture->setParameter(GL_TEXTURE_SPARSE_ARB, true);
	m_molumeTexture->setParameter(GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, 0);

	m_molumeTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_molumeTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_molumeTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_molumeTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_molumeTexture->setParameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	m_molumeTexture->storage3D(1, GL_RGBA8, m_molume.cellCount());


	for (int z = 0; z < m_molume.pageCount().z; z++)
	{
		for (int y = 0; y < m_molume.pageCount().y; y++)
		{
			for (int x = 0; x < m_molume.pageCount().x; x++)
			{
				size_t pageIndex = m_molume.pageIndex(ivec3(x, y, z));
				const glm::u8vec4 * pageData = m_molume.pageData(pageIndex);
				
				if (pageData != nullptr)
				{
					m_molumeTexture->pageCommitment(0, ivec3(x, y, z)*m_molume.pageSize(), m_molume.pageSize(), true);
					m_molumeTexture->subImage3D(0, ivec3(x, y, z)*m_molume.pageSize(), m_molume.pageSize(), GL_RGBA, GL_UNSIGNED_BYTE, pageData);
				}

			}
		}

	}

}

std::list<globjects::File*> MolumeRenderer::shaderFiles() const
{
	return std::list<globjects::File*>({
		m_vertexShaderSource.get(),
		m_tesselationControlShaderSource.get(),
		m_tesselationEvaluationShaderSource.get(),
		m_geometryShaderSource.get(),
		m_fragmentShaderSource.get(),
		m_vertexShaderSourceCast.get(),
		m_geometryShaderSourceCast.get(),
		m_fragmentShaderSourceCast.get(),
	});
}

void MolumeRenderer::display()
{
	if (viewer()->viewportSize() != m_framebufferSize)
	{
		m_framebufferSize = viewer()->viewportSize();
		m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_entryTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_exitTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_depthTexture->image2D(0, GL_DEPTH_COMPONENT32F, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
	}

	m_program->setUniform("projection", viewer()->projectionTransform());
	m_program->setUniform("modelView", viewer()->modelViewTransform()*m_molume.transform());
/*
	mat4 modelview = viewer()->modelViewTransform()*m_molume.transform();
	vec4 eye = inverse(modelview)*vec4(0.0,0.0,0.0,1.0);
	std::cout << to_string(eye) << std::endl;
*/
	glDisable(GL_BLEND);

	m_program->use();

	m_vao->bind();
	glPatchParameteri(GL_PATCH_VERTICES, 4);

	m_frameBuffer->bind();
	glEnable(GL_DEPTH_CLAMP);

	m_frameBuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT1 });
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	m_vao->drawElementsInstanced(GL_PATCHES, m_size, GL_UNSIGNED_SHORT, nullptr, m_molume.pagePositions().size());

	m_frameBuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT2 });
	glClearDepth(0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	m_vao->drawElementsInstanced(GL_PATCHES, m_size, GL_UNSIGNED_SHORT, nullptr, m_molume.pagePositions().size());

	m_vao->unbind();
	m_program->release();

	m_frameBuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	glClear(GL_COLOR_BUFFER_BIT);
	glDepthFunc(GL_LESS);

	m_entryTexture->bindActive(0);
	m_exitTexture->bindActive(1);
	m_molumeTexture->bindActive(2);

	m_programShade->setUniform("enrtyTexture", 0);
	m_programShade->setUniform("exitTexture", 1);
	m_programShade->setUniform("molumeTexture", 2);

	m_programShade->use();

	m_vaoQuad->bind();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_vaoQuad->unbind();

	m_programShade->release();

	m_molumeTexture->unbindActive(2);
	m_exitTexture->unbindActive(1);
	m_entryTexture->unbindActive(0);

	m_frameBuffer->unbind();

	Framebuffer::defaultFBO()->bind();
	m_frameBuffer->blit(GL_COLOR_ATTACHMENT0, { 0,0,m_framebufferSize.x,m_framebufferSize.y }, Framebuffer::defaultFBO().get(), GL_BACK_LEFT, { 0,0,m_framebufferSize.x,m_framebufferSize.y }, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glDisable(GL_DEPTH_CLAMP);
	glClearDepth(1.0);
}