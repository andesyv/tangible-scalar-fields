#include "TileRenderer.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
#include <omp.h>
#include "../../Viewer.h"
#include "../../Scene.h"
#include "../../Table.h"
#include <lodepng.h>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <ctime>


#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

TileRenderer::TileRenderer(Viewer* viewer) : Renderer(viewer)
{
	tile_processors["square"] = new SquareTile(this);
	tile_processors["hexagon"] = new HexTile(this);

	Shader::hintIncludeImplementation(Shader::IncludeImplementation::Fallback);

	m_verticesQuad->setStorage(std::array<vec3, 1>({ vec3(0.0f, 0.0f, 0.0f) }), gl::GL_NONE_BIT);
	auto vertexBindingQuad = m_vaoQuad->binding(0);
	vertexBindingQuad->setBuffer(m_verticesQuad.get(), 0, sizeof(vec3));
	vertexBindingQuad->setFormat(3, GL_FLOAT);
	m_vaoQuad->enable(0);
	m_vaoQuad->unbind();

	// shader storage buffer object for current maximum accumulated value and maximum discrepancy
	m_valueMaxBuffer->setStorage(sizeof(uint) * 2, nullptr, gl::GL_NONE_BIT);

	m_shaderSourceDefines = StaticStringSource::create("");
	m_shaderDefines = NamedString::create("/defines.glsl", m_shaderSourceDefines.get());

	m_shaderSourceGlobals = File::create("./res/tiles/globals.glsl");
	m_shaderGlobals = NamedString::create("/globals.glsl", m_shaderSourceGlobals.get());

	m_shaderSourceGlobalsHexagon = File::create("./res/tiles/hexagon/globals.glsl");
	m_shaderGlobalsHexagon = NamedString::create("/hex/globals.glsl", m_shaderSourceGlobalsHexagon.get());

	// create shader programs
	createShaderProgram("points", {
		{GL_VERTEX_SHADER,"./res/tiles/point-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/point-fs.glsl"}
		});

	createShaderProgram("point-circle", {
		{GL_VERTEX_SHADER,"./res/tiles/point-circle-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/point-circle-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/point-circle-fs.glsl"}
		});

	createShaderProgram("tiles-disc", {
		{GL_VERTEX_SHADER,"./res/tiles/discrepancy-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/discrepancy-fs.glsl"}
		});

	createShaderProgram("max-val", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/max-val-fs.glsl"}
		});

	createShaderProgram("density-normals", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/image-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/density-normals-fs.glsl"}
		});

	createShaderProgram("shade", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/image-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/shade-fs.glsl"}
		});

	m_framebufferSize = viewer->viewportSize();

	// init textures
	m_pointChartTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_pointCircleTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// the size of this textures is set dynamicly depending on the grid granularity
	m_tilesDiscrepanciesTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, ivec2(1, 1), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	m_tileAccumulateTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, ivec2(1, 1), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_tilesTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_gridTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_kdeTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_densityNormalsTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_colorTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	//colorMap - 1D Texture
	m_colorMapTexture = Texture::create(GL_TEXTURE_1D);
	m_colorMapTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorMapTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (auto& d : std::filesystem::directory_iterator("./dat"))
	{
		std::filesystem::path csvPath(d);

		if (csvPath.extension().string() == ".csv")
		{
			// log CSV files that were found in the "./dat" folder
			globjects::debug() << "Found CSV file: " << csvPath.string() << " ...";
			std::string filename = csvPath.filename().string();

			// update collections containing all current CSV files
			m_guiFileNames += filename + '\0';
			m_fileNames.push_back(filename);
		}
	}

	// create Framebuffer
	m_pointFramebuffer = Framebuffer::create();
	m_pointFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_pointChartTexture.get());
	m_pointFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_pointFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_pointCircleFramebuffer = Framebuffer::create();
	m_pointCircleFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_pointCircleTexture.get());
	m_pointCircleFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_kdeTexture.get());
	m_pointCircleFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_pointCircleFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

	m_tilesDiscrepanciesFramebuffer = Framebuffer::create();
	m_tilesDiscrepanciesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tilesDiscrepanciesTexture.get());
	m_tilesDiscrepanciesFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_tilesDiscrepanciesFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_tileAccumulateFramebuffer = Framebuffer::create();
	m_tileAccumulateFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tileAccumulateTexture.get());
	m_tileAccumulateFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_tileAccumulateFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_tilesFramebuffer = Framebuffer::create();
	m_tilesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tilesTexture.get());
	m_tilesFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_tilesFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_gridFramebuffer = Framebuffer::create();
	m_gridFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_gridTexture.get());
	m_gridFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_gridFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_densityNormalsFramebuffer = Framebuffer::create();
	m_densityNormalsFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_densityNormalsTexture.get());
	m_densityNormalsFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_densityNormalsFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_shadeFramebuffer = Framebuffer::create();
	m_shadeFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_colorTexture.get());
	m_shadeFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	m_shadeFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
}

molumes::TileRenderer::~TileRenderer()
{
	if (tile_processors.count("square") > 0) {
		delete tile_processors["square"];
	}
	if (tile_processors.count("hexagon") > 0) {
		delete tile_processors["hexagon"];
	}
}


void TileRenderer::display()
{
	auto currentState = State::currentState();

	// Scatterplot GUI -----------------------------------------------------------------------------------------------------------
	// needs to be first to set all the new values before rendering
	renderGUI();

	setShaderDefines();
	// ---------------------------------------------------------------------------------------------------------------------------

	// do not render if either the dataset was not loaded or the window is minimized 
	if (viewer()->scene()->table()->activeTableData().size() == 0 || viewer()->viewportSize().x == 0 || viewer()->viewportSize().y == 0)
	{
		return;
	}

	int vertexCount = int(viewer()->scene()->table()->activeTableData()[0].size());

	// retrieve/compute all necessary matrices and related properties
	const mat4 viewMatrix = viewer()->viewTransform();
	const mat4 inverseViewMatrix = inverse(viewMatrix);
	const mat4 modelViewMatrix = viewer()->modelViewTransform();
	const mat4 inverseModelViewMatrix = inverse(modelViewMatrix);
	const mat4 modelLightMatrix = viewer()->modelLightTransform();
	const mat4 inverseModelLightMatrix = inverse(modelLightMatrix);
	const mat4 modelViewProjectionMatrix = viewer()->modelViewProjectionTransform();
	const mat4 inverseModelViewProjectionMatrix = inverse(modelViewProjectionMatrix);
	const mat4 projectionMatrix = viewer()->projectionTransform();
	const mat4 inverseProjectionMatrix = inverse(projectionMatrix);
	const mat3 normalMatrix = mat3(transpose(inverseModelViewMatrix));
	const mat3 inverseNormalMatrix = inverse(normalMatrix);
	const ivec2 viewportSize = viewer()->viewportSize();
	const vec2 maxBounds = viewer()->scene()->table()->maximumBounds();
	const vec2 minBounds = viewer()->scene()->table()->minimumBounds();

	//TODO: show thomas
	// projection matrix so we are in screen space and not view space
	vec4 viewLightPosition = projectionMatrix * viewer()->lightTransform() * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	//viewLightPosition.z *= viewer()->scaleFactor();

	if (m_selected_tile_style != m_selected_tile_style_tmp || m_tileSize != m_tileSize_tmp || m_renderDiscrepancy != m_renderDiscrepancy_tmp
		|| m_discrepancy_easeIn != m_discrepancy_easeIn_tmp || m_discrepancy_lowCount != m_discrepancy_lowCount_tmp || viewer()->viewportSize() != m_framebufferSize) {

		// set new values
		m_selected_tile_style = m_selected_tile_style_tmp;
		m_tileSize = m_tileSize_tmp;
		m_renderDiscrepancy = m_renderDiscrepancy_tmp;
		m_discrepancy_easeIn = m_discrepancy_easeIn_tmp;
		m_discrepancy_lowCount = m_discrepancy_lowCount_tmp;

		//set tile processor
		switch (m_selected_tile_style) {
		case 1:
			tile = tile_processors["square"];
			break;
		case 2:
			tile = tile_processors["hexagon"];
			break;
		default:
			tile = nullptr;
			break;
		}

		if (viewer()->viewportSize() != m_framebufferSize)
		{
			m_framebufferSize = viewportSize;
			m_pointChartTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_pointCircleTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_tilesTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_gridTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_kdeTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_densityNormalsTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}

		calculateTileTextureSize(inverseModelViewProjectionMatrix);
	}

	double mouseX, mouseY;
	glfwGetCursorPos(viewer()->window(), &mouseX, &mouseY);
	vec2 focusPosition = vec2(2.0f*float(mouseX) / float(viewportSize.x) - 1.0f, -2.0f*float(mouseY) / float(viewportSize.y) + 1.0f);

	vec4 projectionInfo(float(-2.0 / (viewportSize.x * projectionMatrix[0][0])),
		float(-2.0 / (viewportSize.y * projectionMatrix[1][1])),
		float((1.0 - (double)projectionMatrix[0][2]) / projectionMatrix[0][0]),
		float((1.0 + (double)projectionMatrix[1][2]) / projectionMatrix[1][1]));

	float projectionScale = float(viewportSize.y) / fabs(2.0f / projectionMatrix[1][1]);

	//LIGHTING -------------------------------------------------------------------------------------------------------------------
	// adapted version of "pearl" from teapots.c File Reference
	// Mark J. Kilgard, Silicon Graphics, Inc. 1994 - http://web.archive.org/web/20100725103839/http://www.cs.utk.edu/~kuck/materials_ogl.htm
	ambientMaterial = vec3(0.20725f, 0.20725f, 0.20725f);
	diffuseMaterial = vec3(1, 1, 1);// 0.829f, 0.829f, 0.829f);
	specularMaterial = vec3(0.296648f, 0.296648f, 0.296648f);
	shininess = 11.264f;
	//----------------------------------------------------------------------------------------------------------------------------

	vec4 nearPlane = inverseProjectionMatrix * vec4(0.0, 0.0, -1.0, 1.0);
	nearPlane /= nearPlane.w;

	// ====================================================================================== POINTS RENDER PASS =======================================================================================
	// ONLY USED TO SHOW INITIAL POINTS

	if (m_selected_tile_style == 0 && !m_renderPointCircles) {

		m_pointFramebuffer->bind();
		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		// allow blending for the classical point chart color-attachment (0) of the point frame-buffer
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_MAX);

		// -------------------------------------------------------------------------------------------------

		auto shaderProgram_points = shaderProgram("points");

		shaderProgram_points->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

		m_vao->bind();
		shaderProgram_points->use();

		m_vao->drawArrays(GL_POINTS, 0, vertexCount);

		shaderProgram_points->release();
		m_vao->unbind();

		// disable blending for draw buffer 0 (classical scatter plot)
		glDisablei(GL_BLEND, 0);

		m_pointFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
	// ====================================================================================== DISCREPANCIES RENDER PASS ======================================================================================
	// Accumulate Points into tiles

	if (m_renderDiscrepancy && tile != nullptr) {
		m_tilesDiscrepanciesFramebuffer->bind();

		// set viewport to size of accumulation texture
		glViewport(0, 0, tile->m_tile_cols, tile->m_tile_rows);

		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		// allow blending for the classical point chart color-attachment (0) of the point frame-buffer
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_FUNC_ADD);

		// -------------------------------------------------------------------------------------------------

		auto shaderProgram_discrepancies = shaderProgram("tiles-disc");

		shaderProgram_discrepancies->setUniform("numCols", tile->m_tile_cols);
		shaderProgram_discrepancies->setUniform("numRows", tile->m_tile_rows);
		shaderProgram_discrepancies->setUniform("discrepancyDiv", m_discrepancyDiv);

		m_vaoTiles->bind();
		shaderProgram_discrepancies->use();

		m_vaoTiles->drawArrays(GL_POINTS, 0, tile->numTiles);

		shaderProgram_discrepancies->release();
		m_vaoTiles->unbind();

		// disable blending
		glDisablei(GL_BLEND, 0);

		//reset Viewport
		glViewport(0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y);

		m_tilesDiscrepanciesFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== POINT CIRCLES  & KDE RENDER PASS =======================================================================================
	// render Point Circles into texture
	// render Kernel Density Estimation into texture

	if (m_renderPointCircles || m_renderKDE || m_renderTileNormals) {
		m_pointCircleFramebuffer->bind();
		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		// additive blending
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendEquationi(0, GL_FUNC_ADD);
		glEnablei(GL_BLEND, 1);
		glBlendFunci(1, GL_ONE, GL_ONE);
		glBlendEquationi(1, GL_FUNC_ADD);
		// -------------------------------------------------------------------------------------------------

		auto shaderProgram_pointCircles = shaderProgram("point-circle");

		//set correct radius
		float scaleAdjustedRadius = m_pointCircleRadius / pointCircleRadiusDiv * viewer()->scaleFactor();
		float scaleAdjustedRadiusMult = gaussSampleRadiusMult / viewer()->scaleFactor();

		//geometry shader
		shaderProgram_pointCircles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
		shaderProgram_pointCircles->setUniform("radius", scaleAdjustedRadius);
		shaderProgram_pointCircles->setUniform("aspectRatio", viewer()->m_windowHeight / viewer()->m_windowWidth);

		//fragment shader
		shaderProgram_pointCircles->setUniform("pointColor", viewer()->samplePointColor());
		shaderProgram_pointCircles->setUniform("sigma2", m_sigma);
		shaderProgram_pointCircles->setUniform("radiusMult", scaleAdjustedRadiusMult);
		shaderProgram_pointCircles->setUniform("densityMult", m_densityMult);

		m_vao->bind();
		shaderProgram_pointCircles->use();

		m_vao->drawArrays(GL_POINTS, 0, vertexCount);

		shaderProgram_pointCircles->release();
		m_vao->unbind();

		// disable blending for draw buffer 0 (classical scatter plot)
		glDisablei(GL_BLEND, 0);
		glDisablei(GL_BLEND, 1);

		m_pointCircleFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== ACCUMULATE RENDER PASS ======================================================================================
	// Accumulate Points into tiles
	if (tile != nullptr) {
		m_tileAccumulateFramebuffer->bind();

		// set viewport to size of accumulation texture
		glViewport(0, 0, tile->m_tile_cols, tile->m_tile_rows);

		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		//ADDITIVE Blending GL_COLOR_ATTACHMENT0
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendEquationi(0, GL_FUNC_ADD);

		// -------------------------------------------------------------------------------------------------

		auto shaderProgram_tile_acc = tile->getAccumulationProgram();

		m_vao->bind();
		shaderProgram_tile_acc->use();

		m_vao->drawArrays(GL_POINTS, 0, vertexCount);

		shaderProgram_tile_acc->release();
		m_vao->unbind();

		// disable blending
		glDisablei(GL_BLEND, 0);

		//reset Viewport
		glViewport(0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y);

		m_tileAccumulateFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== MAX VAL RENDER PASS ======================================================================================
	// MAX VAL: Get maximum accumulated value (used for coloring) -  no framebuffer needed, because we don't render anything. we just save the max value into the storage buffer

	// SSBO --------------------------------------------------------------------------------------------------------------------------------------------------
	m_valueMaxBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 0);

	// max accumulated Value
	const uint initialMaxValue = 0;
	m_valueMaxBuffer->clearSubData(GL_R32UI, 0, sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &initialMaxValue);
	m_valueMaxBuffer->clearSubData(GL_R32UI, sizeof(uint), sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &initialMaxValue);
	// -------------------------------------------------------------------------------------------------

	m_tileAccumulateTexture->bindActive(1);
	m_pointCircleTexture->bindActive(2);

	//can use the same shader for hexagon and square tiles
	auto shaderProgram_max_val = shaderProgram("max-val");

	//geometry & fragment shader
	shaderProgram_max_val->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	//fragment shader
	shaderProgram_max_val->setUniform("pointCircleTexture", 2);

	if (tile == nullptr) {
		//geometry shader
		shaderProgram_max_val->setUniform("maxBounds_acc", maxBounds);
		shaderProgram_max_val->setUniform("minBounds_acc", minBounds);
	}
	else {
		//geometry shader
		shaderProgram_max_val->setUniform("maxBounds_acc", tile->maxBounds_Offset);
		shaderProgram_max_val->setUniform("minBounds_acc", tile->minBounds_Offset);

		shaderProgram_max_val->setUniform("windowWidth", viewer()->viewportSize()[0]);
		shaderProgram_max_val->setUniform("windowHeight", viewer()->viewportSize()[1]);
		//fragment Shader
		shaderProgram_max_val->setUniform("maxTexCoordX", tile->m_tileMaxX);
		shaderProgram_max_val->setUniform("maxTexCoordY", tile->m_tileMaxY);

		shaderProgram_max_val->setUniform("accumulateTexture", 1);
	}

	m_vaoQuad->bind();

	shaderProgram_max_val->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	shaderProgram_max_val->release();

	m_vaoQuad->unbind();

	m_tileAccumulateTexture->unbindActive(1);
	m_pointCircleTexture->unbindActive(2);

	//unbind shader storage buffer
	m_valueMaxBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//TODO: density normals pass only debug to show normals per pixel
	if (m_renderDensityNormals || m_renderTileNormals) {
		// ====================================================================================== DENSITY NORMALS RENDER PASS ======================================================================================
		// render Density Normals into texture

		m_densityNormalsFramebuffer->bind();

		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		//Blending GL_COLOR_ATTACHMENT0
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_MAX);

		// -------------------------------------------------------------------------------------------------

		m_kdeTexture->bindActive(0);

		auto shaderProgram_density_normals = shaderProgram("density-normals");

		//fragment shader
		shaderProgram_density_normals->setUniform("kdeTexture", 0);

		m_vaoQuad->bind();

		shaderProgram_density_normals->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		shaderProgram_density_normals->release();

		m_vaoQuad->unbind();

		m_kdeTexture->unbindActive(0);

		// disable blending
		glDisablei(GL_BLEND, 0);

		m_densityNormalsFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		// ====================================================================================== TILE NORMALS RENDER PASS ======================================================================================
		// render Tile Normals into storage buffer
		if (tile != nullptr && m_renderTileNormals) {
			// SSBO --------------------------------------------------------------------------------------------------------------------------------------------------
			m_tileNormalsBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 0);

			// one Pixel fo data is enough to clear whole buffer
			// https://www.khronos.org/opengl/wiki/GLAPI/glClearBufferData
			const uint initialVal = 0;
			m_tileNormalsBuffer->clearData(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &initialVal);
			// -------------------------------------------------------------------------------------------------

			m_kdeTexture->bindActive(1);
			//TODO: remove
			m_densityNormalsTexture->bindActive(2);
			m_tileAccumulateTexture->bindActive(3);

			auto shaderProgram_tile_normals = tile->getTileNormalsProgram();

			//geometry shader
			shaderProgram_tile_normals->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

			shaderProgram_tile_normals->setUniform("windowWidth", viewportSize[0]);
			shaderProgram_tile_normals->setUniform("windowHeight", viewportSize[1]);

			shaderProgram_tile_normals->setUniform("kdeTexture", 1);
			shaderProgram_tile_normals->setUniform("densityNormalsTexture", 2);
			shaderProgram_tile_normals->setUniform("accumulateTexture",3 );

			m_vaoQuad->bind();

			shaderProgram_tile_normals->use();
			m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
			shaderProgram_tile_normals->release();

			m_vaoQuad->unbind();

			m_kdeTexture->unbindActive(1);
			m_densityNormalsTexture->unbindActive(2);
			m_tileAccumulateTexture->unbindActive(3);

			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			/*uint* arrayMain = new uint[4 * tile->m_tile_cols*tile->m_tile_rows]();
			m_tileNormalsBuffer->getSubData(0, sizeof(uint) * 4 * tile->m_tile_cols*tile->m_tile_rows, arrayMain);

			std::cout << "New Frame: " << std::endl;

			float a = 0.123456789f;
			uint t = floatBitsToUint(a);
			uint t_s = t / 10000;
			float t_f = uintBitsToFloat(t_s * 10000);

			uint t2 = a * 1000000;
			float t2_f = t2 / 1000000.0f;

			for (int i = 0; i < tile->m_tile_cols*tile->m_tile_rows; i++) {
				vec4 tileNormal = vec4(0);
				for (int j = 0; j < 4; j++) {
					uint y = arrayMain[4 * i + j];
					float z = y / 10000.0f;
					tileNormal[j] = z;
				}
				//tileNormal = vec4(normalize(vec3(tileNormal.x, tileNormal.y, 1.0f)), tileNormal.w);
				vec3 lightNormal = vec3(tileNormal.x/tileNormal.w, tileNormal.y / tileNormal.w, 0.01f);
				lightNormal = normalize(lightNormal);
				for (int j = 0; j < 3; j++) {

					std::cout << i << ": " << j << ": " << lightNormal[j] << std::endl;
				}
			}
			delete[] arrayMain;*/
		}
	}

	// ====================================================================================== TILES RENDER PASS ======================================================================================
	// Render tiles
	if (tile != nullptr) {

		m_valueMaxBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);

		m_tilesFramebuffer->bind();

		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		//Blending GL_COLOR_ATTACHMENT0
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_MAX);

		// -------------------------------------------------------------------------------------------------

		m_tileAccumulateTexture->bindActive(2);
		m_tilesDiscrepanciesTexture->bindActive(3);

		auto shaderProgram_tiles = tile->getTileProgram();
		
		//geometry shader
		shaderProgram_tiles->setUniform("windowWidth", viewportSize[0]);
		shaderProgram_tiles->setUniform("windowHeight", viewportSize[1]);

		shaderProgram_tiles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

		//fragment Shader
		shaderProgram_tiles->setUniform("tileHeightMult", m_tileHeightMult);
		shaderProgram_tiles->setUniform("borderWidth", m_borderWidth);
		shaderProgram_tiles->setUniform("invertPyramid", m_invertPyramid);

		//lighting
		shaderProgram_tiles->setUniform("lightPos", vec3(viewLightPosition));
		shaderProgram_tiles->setUniform("viewPos", vec3(0));
		shaderProgram_tiles->setUniform("lightColor", vec3(1));
		
		// textures
		shaderProgram_tiles->setUniform("accumulateTexture", 2);
		shaderProgram_tiles->setUniform("tilesDiscrepancyTexture", 3);

		if (m_colorMapLoaded)
		{
			m_colorMapTexture->bindActive(5);
			shaderProgram_tiles->setUniform("colorMapTexture", 5);
			shaderProgram_tiles->setUniform("textureWidth", m_ColorMapWidth);
		}

		m_vaoQuad->bind();

		shaderProgram_tiles->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		shaderProgram_tiles->release();

		m_vaoQuad->unbind();

		if (m_colorMapLoaded)
		{
			m_colorMapTexture->unbindActive(5);
		}

		m_tileAccumulateTexture->unbindActive(2);
		m_tilesDiscrepanciesTexture->unbindActive(3);

		// disable blending
		glDisablei(GL_BLEND, 0);

		m_tilesFramebuffer->unbind();

		//unbind shader storage buffer
		m_valueMaxBuffer->unbind(GL_SHADER_STORAGE_BUFFER);
		m_tileNormalsBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== GRID RENDER PASS ======================================================================================
	// render grid into texture

	if (m_renderGrid && tile != nullptr) {
		m_gridFramebuffer->bind();

		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		//Blending GL_COLOR_ATTACHMENT0
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_MAX);

		// -------------------------------------------------------------------------------------------------

		m_gridTexture->bindActive(0);
		//used to check if we actually need to draw the grid for a given square
		m_tileAccumulateTexture->bindActive(1);

		tile->renderGrid(m_vaoTiles, modelViewProjectionMatrix);

		m_gridTexture->unbindActive(0);
		m_tileAccumulateTexture->unbindActive(1);

		// disable blending
		glDisablei(GL_BLEND, 0);

		m_gridFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== SHADE/BLEND RENDER PASS ======================================================================================
	// blend everything together and draw to screen
	m_shadeFramebuffer->bind();

	m_pointChartTexture->bindActive(0);
	m_tilesTexture->bindActive(1);
	m_tileAccumulateTexture->bindActive(2);
	m_gridTexture->bindActive(3);
	m_pointCircleTexture->bindActive(4);
	//debug
	m_kdeTexture->bindActive(5);
	m_densityNormalsTexture->bindActive(7);
	//end debug

	m_valueMaxBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 6);

	auto shaderProgram_shade = shaderProgram("shade");

	if (!m_renderPointCircles && m_selected_tile_style == 0) {
		shaderProgram_shade->setUniform("pointChartTexture", 0);
	}

	if (m_renderPointCircles) {
		shaderProgram_shade->setUniform("pointCircleTexture", 4);
	}

	if (m_selected_tile_style != 0) {
		shaderProgram_shade->setUniform("tilesTexture", 1);
	}

	if (m_renderGrid) {
		shaderProgram_shade->setUniform("gridTexture", 3);
	}

	if (m_renderAccumulatePoints) {
		shaderProgram_shade->setUniform("accPointTexture", 2);
	}

	if (m_renderKDE) {
		shaderProgram_shade->setUniform("kdeTexture", 5);
	}
	if (m_renderDensityNormals) {
		shaderProgram_shade->setUniform("densityNormalsTexture", 7);
	}

	m_vaoQuad->bind();

	shaderProgram_shade->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	shaderProgram_shade->release();

	m_vaoQuad->unbind();

	m_pointChartTexture->unbindActive(0);
	m_tilesTexture->unbindActive(1);
	m_tileAccumulateTexture->unbindActive(2);
	m_gridTexture->unbindActive(3);
	m_pointCircleTexture->unbindActive(4);
	m_kdeTexture->unbindActive(5);
	m_densityNormalsTexture->unbindActive(7);

	//unbind shader storage buffer
	m_valueMaxBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	m_shadeFramebuffer->unbind();

	m_shadeFramebuffer->blit(GL_COLOR_ATTACHMENT0, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	currentState->apply();
}

// --------------------------------------------------------------------------------------
// ###########################  TILES ############################################
// --------------------------------------------------------------------------------------
void TileRenderer::calculateTileTextureSize(const mat4 inverseModelViewProjectionMatrix) {

	// if we only render points, we do not need to calculate tile sizes
	if (tile != nullptr) {

		vec3 maxBounds = viewer()->scene()->table()->maximumBounds();
		vec3 minBounds = viewer()->scene()->table()->minimumBounds();

		tile->tileSizeWS = (inverseModelViewProjectionMatrix * vec4(m_tileSize_tmp / tile->tileSizeDiv, 0, 0, 0)).x;
		tile->tileSizeWS *= viewer()->scaleFactor();

		vec3 boundingBoxSize = maxBounds - minBounds;

		tile->calculateNumberOfTiles(boundingBoxSize, minBounds);

		//set texture size
		m_tilesDiscrepanciesTexture->image2D(0, GL_RGBA32F, ivec2(tile->m_tile_cols, tile->m_tile_rows), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_tileAccumulateTexture->image2D(0, GL_RGBA32F, ivec2(tile->m_tile_cols, tile->m_tile_rows), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		//allocate tile normals buffer storage
		m_tileNormalsBuffer->bind(GL_SHADER_STORAGE_BUFFER);
		// we safe each value of the normal (vec4) seperately
		m_tileNormalsBuffer->setData(sizeof(uint) * 4 * tile->m_tile_cols*tile->m_tile_rows, nullptr, GL_STREAM_DRAW);
		m_tileNormalsBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

		//calculate viewport settings
		/*vec4 testPoint = vec4(1.0f, 2.0f, 0.0f, 1.0f);

		vec2 testPointNDC = vec2((testPoint.x * 2 / float(m_squareMaxX)) - 1, (testPoint.y * 2 / float(m_squareMaxY)) - 1);
		vec2 testPointSS = vec2((testPointNDC.x + 1) * (m_squareMaxX / 2), (testPointNDC.y + 1) * (m_squareMaxY / 2));
		*/

		//setup grid vertices
		// create m_squareNumCols*m_squareNumRows vertices with position = 0.0f
		// we generate the correct position in the vertex shader
		m_verticesTiles->setData(std::vector<float>(tile->m_tile_cols*tile->m_tile_rows, 0.0f), GL_STATIC_DRAW);

		auto vertexBinding = m_vaoTiles->binding(0);
		vertexBinding->setAttribute(0);
		vertexBinding->setBuffer(m_verticesTiles.get(), 0, sizeof(float));
		vertexBinding->setFormat(1, GL_FLOAT);
		m_vaoTiles->enable(0);

		//calc2D discrepancy and setup discrepancy buffer
		std::vector<float> tilesDiscrepancies(tile->numTiles, 0.0f);

		if (m_renderDiscrepancy) {
			tilesDiscrepancies = calculateDiscrepancy2D(viewer()->scene()->table()->activeXColumn(), viewer()->scene()->table()->activeYColumn(),
				viewer()->scene()->table()->maximumBounds(), viewer()->scene()->table()->minimumBounds());
		}

		m_discrepanciesBuffer->setData(tilesDiscrepancies, GL_STATIC_DRAW);

		vertexBinding = m_vaoTiles->binding(1);
		vertexBinding->setAttribute(1);
		vertexBinding->setBuffer(m_discrepanciesBuffer.get(), 0, sizeof(float));
		vertexBinding->setFormat(1, GL_FLOAT);
		m_vaoTiles->enable(1);
	}
}

// --------------------------------------------------------------------------------------
// ###########################  GUI ##################################################### 
// --------------------------------------------------------------------------------------
/*
Renders the User interface
*/
void TileRenderer::renderGUI() {

	// boolean variable used to automatically update the data
	static bool dataChanged = false;
	ImGui::Begin("Illuminated Point Plots");

	if (ImGui::CollapsingHeader("CSV-Files"), ImGuiTreeNodeFlags_DefaultOpen)
	{

		ImGui::Combo("Files", &m_fileDataID, m_guiFileNames.c_str());

		if (m_fileDataID != m_oldFileDataID)
		{
			//std::cout << "File selection event - " << "File: " << m_fileDataID << "\n";

			// reset column names
			m_guiColumnNames = { 'N', 'o', 'n', 'e', '\0' };

			if (m_fileDataID != 0) {

				// initialize table
				viewer()->scene()->table()->load("./dat/" + m_fileNames[m_fileDataID]);

				// extract column names and prepare GUI
				std::vector<std::string> tempNames = viewer()->scene()->table()->getColumnNames();

				for (std::vector<std::string>::iterator it = tempNames.begin(); it != tempNames.end(); ++it) {
					m_guiColumnNames += *it + '\0';
				}

				// provide default selections assuming
				m_xAxisDataID = 1;		// contains the X-values
				m_yAxisDataID = 2;		// contains the Y-values
				m_radiusDataID = 3;		// contains the radii
				m_colorDataID = 4;		// contains the colors
			}

			// update status
			m_oldFileDataID = m_fileDataID;
			dataChanged = true;
		}

		// show all column names from selected CSV file
		ImGui::Text("Selected Columns:");
		ImGui::Combo("X-axis", &m_xAxisDataID, m_guiColumnNames.c_str());
		ImGui::Combo("Y-axis", &m_yAxisDataID, m_guiColumnNames.c_str());

		if (ImGui::Button("Update") || dataChanged)
		{
			// update buffers according to recent changes -> since combo also contains 'None" we need to subtract 1 from ID
			viewer()->scene()->table()->updateBuffers(m_xAxisDataID - 1, m_yAxisDataID - 1, m_radiusDataID - 1, m_colorDataID - 1);

			// update VBOs for all four columns
			m_xColumnBuffer->setData(viewer()->scene()->table()->activeXColumn(), GL_STATIC_DRAW);
			m_yColumnBuffer->setData(viewer()->scene()->table()->activeYColumn(), GL_STATIC_DRAW);
			m_radiusColumnBuffer->setData(viewer()->scene()->table()->activeRadiusColumn(), GL_STATIC_DRAW);
			m_colorColumnBuffer->setData(viewer()->scene()->table()->activeColorColumn(), GL_STATIC_DRAW);


			// update VAO for all buffers ----------------------------------------------------
			auto vertexBinding = m_vao->binding(0);
			vertexBinding->setAttribute(0);
			vertexBinding->setBuffer(m_xColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(0);

			vertexBinding = m_vao->binding(1);
			vertexBinding->setAttribute(1);
			vertexBinding->setBuffer(m_yColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(1);

			// -------------------------------------------------------------------------------

			// Scaling the model's bounding box to the canonical view volume
			vec3 boundingBoxSize = viewer()->scene()->table()->maximumBounds() - viewer()->scene()->table()->minimumBounds();
			float maximumSize = std::max({ boundingBoxSize.x, boundingBoxSize.y, boundingBoxSize.z });
			mat4 modelTransform = scale(vec3(2.0f) / vec3(maximumSize));
			modelTransform = modelTransform * translate(-0.5f*(viewer()->scene()->table()->minimumBounds() + viewer()->scene()->table()->maximumBounds()));
			viewer()->setModelTransform(modelTransform);

			// store diameter of current scatter plot and initialize light position
			viewer()->m_scatterPlotDiameter = sqrt(pow(boundingBoxSize.x, 2) + pow(boundingBoxSize.y, 2));

			// initial position of the light source (azimuth 120 degrees, elevation 45 degrees, 5 times the distance to the object in center) ---------------------------------------------------------
			glm::mat4 viewTransform = viewer()->viewTransform();
			glm::vec3 initLightDir = normalize(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
			glm::mat4 newLightTransform = glm::inverse(viewTransform)*glm::translate(mat4(1.0f), (5 * viewer()->m_scatterPlotDiameter*initLightDir))*viewTransform;
			viewer()->setLightTransform(newLightTransform);
			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

			// calculate accumulate texture settings - needs to be last step here ------------------------------
			calculateTileTextureSize(inverse(viewer()->modelViewProjectionTransform()));
			// -------------------------------------------------------------------------------

		}

		if (ImGui::CollapsingHeader("Color Maps"), ImGuiTreeNodeFlags_DefaultOpen)
		{
			// show all available color-maps
			ImGui::Combo("Maps", &m_colorMap, "None\0Bone\0Cubehelix\0GistEart\0GnuPlot2\0Grey\0Inferno\0Magma\0Plasma\0PuBuGn\0Rainbow\0Summer\0Virdis\0Winter\0Wista\0YlGnBu\0YlOrRd\0");

			// allow the user to load a discrete version of the color map
			ImGui::Checkbox("Discrete Colors (7)", &m_discreteMap);

			// load new texture if either the texture has changed or the type has changed from discrete to continuous or vice versa
			if (m_colorMap != m_oldColorMap || m_discreteMap != m_oldDiscreteMap)
			{
				if (m_colorMap > 0) {
					std::vector<std::string> colorMapFilenames = { "./dat/colormaps/bone_1D.png", "./dat/colormaps/cubehelix_1D.png", "./dat/colormaps/gist_earth_1D.png",  "./dat/colormaps/gnuplot2_1D.png" ,
						"./dat/colormaps/grey_1D.png", "./dat/colormaps/inferno_1D.png", "./dat/colormaps/magma_1D.png", "./dat/colormaps/plasma_1D.png", "./dat/colormaps/PuBuGn_1D.png",
						"./dat/colormaps/rainbow_1D.png", "./dat/colormaps/summer_1D.png", "./dat/colormaps/virdis_1D.png", "./dat/colormaps/winter_1D.png", "./dat/colormaps/wista_1D.png", "./dat/colormaps/YlGnBu_1D.png",
						"./dat/colormaps/YlOrRd_1D.png" };

					uint colorMapWidth, colorMapHeight;
					std::vector<unsigned char> colorMapImage;

					std::string textureName = colorMapFilenames[m_colorMap - 1];

					if (m_discreteMap)
					{
						std::string fileExtension = "_discrete7";

						// insert the discrete identifier "_discrete7" before the file extension ".png"
						textureName.insert(textureName.length() - 4, fileExtension);
					}

					uint error = lodepng::decode(colorMapImage, colorMapWidth, colorMapHeight, textureName);

					if (error)
						globjects::debug() << "Could not load " << colorMapFilenames[m_colorMap - 1] << "!";
					else
					{
						m_colorMapTexture->image1D(0, GL_RGBA, colorMapWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&colorMapImage.front());
						m_colorMapTexture->generateMipmap();

						// store width of texture and mark as loaded
						m_ColorMapWidth = colorMapWidth;
						m_colorMapLoaded = true;
					}
				}
				else
				{
					// disable color map
					m_colorMapLoaded = false;
				}

				// update status
				m_oldColorMap = m_colorMap;
				m_oldDiscreteMap = m_discreteMap;
			}
		}

		if (ImGui::CollapsingHeader("Discrepancy"))
		{
			ImGui::Checkbox("Render Point Circles", &m_renderPointCircles);
			ImGui::SliderFloat("Point Circle Radius", &m_pointCircleRadius, 1.0f, 100.0f);
			ImGui::Checkbox("Show Discrepancy", &m_renderDiscrepancy_tmp);
			ImGui::SliderFloat("Ease In", &m_discrepancy_easeIn_tmp, 1.0f, 5.0f);
			ImGui::SliderFloat("Low Point Count", &m_discrepancy_lowCount_tmp, 0.0f, 1.0f);
			ImGui::SliderFloat("Discrepancy Divisor", &m_discrepancyDiv, 1.0f, 3.0f);
		}
		if (ImGui::CollapsingHeader("Tiles"))
		{
			const char* tile_styles[]{ "none", "square", "hexagon" };
			ImGui::Combo("Tile Rendering", &m_selected_tile_style_tmp, tile_styles, IM_ARRAYSIZE(tile_styles));
			ImGui::Checkbox("Render Grid", &m_renderGrid);
			ImGui::SliderFloat("Tile Size ", &m_tileSize_tmp, 1.0f, 100.0f);
		}

		if (ImGui::CollapsingHeader("Regression Plane"))
		{
			ImGui::Checkbox("Render KDE", &m_renderKDE);
			ImGui::Checkbox("Render Density Normals", &m_renderDensityNormals);
			ImGui::Checkbox("Render Tile Normals", &m_renderTileNormals);
			ImGui::SliderFloat("Sigma", &m_sigma, 0.1f, 10.0f);
			ImGui::SliderFloat("Sample Radius", &m_pointCircleRadius, 1.0f, 100.0f);
			ImGui::SliderFloat("Density Multiply", &m_densityMult, 1.0f, 100.0f);
			ImGui::SliderFloat("Tile Height Mult", &m_tileHeightMult, 0.1f, 2.0f);
			ImGui::SliderFloat("Border Width", &m_borderWidth, 0.0f, 1.0f);
			ImGui::Checkbox("Invert Pyramid", &m_invertPyramid);
		}

		ImGui::Checkbox("Render Acc Points", &m_renderAccumulatePoints);

		// update status
		dataChanged = false;
	}
	ImGui::End();
}


/*
Gathers all #define, reset the defines files and reload the shaders
*/
void TileRenderer::setShaderDefines() {
	std::string defines = "";


	if (m_colorMapLoaded)
		defines += "#define COLORMAP\n";

	if (m_renderPointCircles) {
		defines += "#define RENDER_POINT_CIRCLES\n";
	}

	if (m_renderDiscrepancy) {
		defines += "#define RENDER_DISCREPANCY\n";
	}

	if (m_selected_tile_style != 0)
	{
		defines += "#define RENDER_TILES\n";
	}

	if (m_selected_tile_style == 2)
	{
		defines += "#define RENDER_HEXAGONS\n";
	}

	if (m_renderGrid) {
		defines += "#define RENDER_GRID\n";
	}

	if (m_renderAccumulatePoints)
		defines += "#define RENDER_ACC_POINTS\n";

	if (m_renderKDE)
		defines += "#define RENDER_KDE\n";

	if (m_renderDensityNormals)
		defines += "#define RENDER_DENSITY_NORMALS\n";

	if (m_renderTileNormals)
		defines += "#define RENDER_TILE_NORMALS\n";

	if (defines != m_shaderSourceDefines->string())
	{
		m_shaderSourceDefines->setString(defines);

		reloadShaders();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//INTERVAL MAPPING
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//maps value x from [a,b] --> [0,c]
float molumes::TileRenderer::mapInterval(float x, float a, float b, int c)
{
	return (x - a)*c / (b - a);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DISCREPANCY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//PRECONDITION: tile != nullptr
std::vector<float> TileRenderer::calculateDiscrepancy2D(const std::vector<float>& samplesX, const std::vector<float>& samplesY, vec3 maxBounds, vec3 minBounds)
{

	// Calculates the discrepancy of this data.
	// Assumes the data is [0,1) for valid sample range.
	// Assmues samplesX.size() == samplesY.size()
	int numSamples = samplesX.size();

	std::vector<float> sortedX(numSamples, 0.0f);
	std::vector<float> sortedY(numSamples, 0.0f);

	std::vector<float> pointsInTilesCount(tile->numTiles, 0.0f);
	std::vector<float> tilesMaxBoundX(tile->numTiles, -INFINITY);
	std::vector<float> tilesMaxBoundY(tile->numTiles, -INFINITY);
	std::vector<float> tilesMinBoundX(tile->numTiles, INFINITY);
	std::vector<float> tilesMinBoundY(tile->numTiles, INFINITY);

	std::vector<float> discrepancies(tile->numTiles, 0.0f);

	float eps = 0.05;

	//std::cout << "Discrepancy: " << numSamples << " Samples; " << numSquares << " Tiles" << '\n';

	//timing
	std::clock_t start;
	double duration;

	start = std::clock();

	//Step 1: Count how many elements belong to each square
	for (int i = 0; i < numSamples; i++) {

		float sampleX = samplesX[i];
		float sampleY = samplesY[i];

		pointsInTilesCount[tile->mapPointToTile(vec2(sampleX, sampleY))]++;
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	//std::cout << "Step1: " << duration << '\n';

	start = clock();

	//Step 2: calc prefix Sum
	std::vector<float> pointsInTilesPrefixSum(pointsInTilesCount);
	int prefixSum = 0;
	int sampleCount = 0;
	int maxSampleCount = 0;
	for (int i = 0; i < tile->numTiles; i++) {
		sampleCount = pointsInTilesPrefixSum[i];
		pointsInTilesPrefixSum[i] = prefixSum;
		prefixSum += sampleCount;
		maxSampleCount = max(maxSampleCount, sampleCount);
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	//std::cout << "Step2: " << duration << '\n';

	start = clock();

	//Step 3: sort points according to squares
	// 1D array with "buckets" according to prefix Sum of squares
	// also set the bounding box of points inside tile
	std::vector<float> pointsInTilesRunningPrefixSum(pointsInTilesPrefixSum);
	for (int i = 0; i < numSamples; i++) {

		float sampleX = samplesX[i];
		float sampleY = samplesY[i];

		int squareIndex1D = tile->mapPointToTile(vec2(sampleX, sampleY));

		// put sample in correct position and increment prefix sum
		int sampleIndex = pointsInTilesRunningPrefixSum[squareIndex1D]++;
		sortedX[sampleIndex] = sampleX;
		sortedY[sampleIndex] = sampleY;

		//set bounding box of samples inside tile
		tilesMaxBoundX[squareIndex1D] = max(tilesMaxBoundX[squareIndex1D], sampleX);
		tilesMaxBoundY[squareIndex1D] = max(tilesMaxBoundY[squareIndex1D], sampleY);

		tilesMinBoundX[squareIndex1D] = min(tilesMinBoundX[squareIndex1D], sampleX);
		tilesMinBoundY[squareIndex1D] = min(tilesMinBoundY[squareIndex1D], sampleY);
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	//std::cout << "Step3: " << duration << '\n';

	start = clock();

	//Step 4: calculate discrepancy for each tile
	omp_set_num_threads(4);
#pragma omp parallel
	{
		/*
#pragma omp master
		{
			std::cout << "NumThreads: " << omp_get_num_threads() << '\n';
		}
#pragma omp barrier

		std::cout << "Thread: " << omp_get_thread_num() << '\n';
#pragma omp barrier
*/

#pragma omp for
		for (int i = 0; i < tile->numTiles; i++) {

			float maxDifference = 0.0f;
			int startPoint = pointsInTilesPrefixSum[i];
			int stopPoint = pointsInTilesPrefixSum[i] + pointsInTilesCount[i];

			//use the addition and not PrefixSum[i+1] so we can easily get the last boundary
			for (int j = startPoint; j < stopPoint; j++) {
				//normalize sample inside its square
				float stopValueXNorm = mapInterval(sortedX[j], tilesMinBoundX[i], tilesMaxBoundX[i], 1);
				float stopValueYNorm = mapInterval(sortedY[j], tilesMinBoundY[i], tilesMaxBoundY[i], 1);

				// calculate area
				float area = stopValueXNorm * stopValueYNorm;

				// closed interval [startValue, stopValue]
				float countInside = 0.0f;
				for (int k = pointsInTilesPrefixSum[i]; k < pointsInTilesPrefixSum[i] + pointsInTilesCount[i]; k++)
				{
					//normalize sample inside its square
					float sampleXNorm = mapInterval(sortedX[k], tilesMinBoundX[i], tilesMaxBoundX[i], 1);
					float sampleYNorm = mapInterval(sortedY[k], tilesMinBoundY[i], tilesMaxBoundY[i], 1);


					if (sampleXNorm <= stopValueXNorm &&
						sampleYNorm <= stopValueYNorm)
					{
						countInside++;
					}
				}
				float density = countInside / float(numSamples);
				float difference = std::abs(density - area);

				if (difference > maxDifference)
				{
					maxDifference = difference;
				}
			}

			maxDifference = max(eps, maxDifference);

			// Ease In
			maxDifference = pow(maxDifference, m_discrepancy_easeIn);

			// account for tiles with few points
			//maxDifference = (1 - m_discrepancy_lowCount) * maxDifference + m_discrepancy_lowCount * (1 - pow((pointsInTilesCount[i] / float(maxSampleCount)), 2));
			maxDifference = min(maxDifference + m_discrepancy_lowCount * (1 - pow((pointsInTilesCount[i] / float(maxSampleCount)), 2)), 1.0f);


			discrepancies[i] = maxDifference;
		}
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	//std::cout << "Step4: " << duration << '\n' << '\n';

	return discrepancies;
}