#include "HexTileRenderer.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
#include <omp.h>
#include "../Viewer.h"
#include "../Scene.h"
#include "../Table.h"
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

struct Statistics
{
	uint intersectionCount = 0;
	uint totalPixelCount = 0;
	uint totalEntryCount = 0;
	uint maximumEntryCount = 0;
};

//#define BENCHMARK
//#define STATISTICS

HexTileRenderer::HexTileRenderer(Viewer* viewer) : Renderer(viewer)
{
	Shader::hintIncludeImplementation(Shader::IncludeImplementation::Fallback);

	m_verticesQuad->setStorage(std::array<vec3, 1>({ vec3(0.0f, 0.0f, 0.0f) }), gl::GL_NONE_BIT);
	auto vertexBindingQuad = m_vaoQuad->binding(0);
	vertexBindingQuad->setBuffer(m_verticesQuad.get(), 0, sizeof(vec3));
	vertexBindingQuad->setFormat(3, GL_FLOAT);
	m_vaoQuad->enable(0);
	m_vaoQuad->unbind();

	// shader storage buffer object for current maximum accumulated value
	m_valueMaxBuffer->setStorage(sizeof(uint), nullptr, gl::GL_NONE_BIT);

	m_shaderSourceDefines = StaticStringSource::create("");
	m_shaderDefines = NamedString::create("/defines.glsl", m_shaderSourceDefines.get());

	m_shaderSourceGlobals = File::create("./res/hexagon/globals.glsl");
	m_shaderGlobals = NamedString::create("/globals.glsl", m_shaderSourceGlobals.get());

	// create shader programs
	createShaderProgram("points", {
		{GL_VERTEX_SHADER,"./res/hexagon/point-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/point-fs.glsl"}
		});

	createShaderProgram("point-circle", {
		{GL_VERTEX_SHADER,"./res/hexagon/point-circle-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/hexagon/point-circle-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/point-circle-fs.glsl"}
		});

	createShaderProgram("tiles-disc", {
		{GL_VERTEX_SHADER,"./res/hexagon/discrepancy-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/discrepancy-fs.glsl"}
		});

	createShaderProgram("square-acc", {
		{GL_VERTEX_SHADER,"./res/hexagon/square/square-acc-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/square/square-acc-fs.glsl"}
		});

	createShaderProgram("square-acc-count", {
		{GL_VERTEX_SHADER,"./res/hexagon/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/hexagon/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/square/max-acc-val-fs.glsl"}
		});

	createShaderProgram("square-tiles", {
		{GL_VERTEX_SHADER,"./res/hexagon/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/hexagon/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/square/square-tiles-fs.glsl"}
		});

	createShaderProgram("square-grid", {
		{GL_VERTEX_SHADER,"./res/hexagon/square/square-grid-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/hexagon/square/square-grid-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/square/square-grid-fs.glsl"}
		});

	/*createShaderProgram("hexagon-grid", {
		{GL_VERTEX_SHADER,"./res/hexagon/hexagon-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/hexagon/hexagon-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/hexagon-fs.glsl"}
		});*/

	createShaderProgram("shade", {
		{GL_VERTEX_SHADER,"./res/hexagon/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/hexagon/image-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/shade-fs.glsl"}
		});

	m_framebufferSize = viewer->viewportSize();

	// init textures
	m_pointChartTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_pointCircleTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_hexTilesTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// the size of this textures is set dynamicly depending on the grid granularity
	m_tilesDiscrepanciesTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, ivec2(0, 0), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	m_squareAccumulateTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, ivec2(0, 0), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_squareTilesTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_squareGridTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

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
	m_pointCircleFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_pointCircleFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_hexFramebuffer = Framebuffer::create();
	m_hexFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_hexTilesTexture.get());
	m_hexFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_hexFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_tilesDiscrepanciesFramebuffer = Framebuffer::create();
	m_tilesDiscrepanciesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tilesDiscrepanciesTexture.get());
	m_tilesDiscrepanciesFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_tilesDiscrepanciesFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_squareAccumulateFramebuffer = Framebuffer::create();
	m_squareAccumulateFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_squareAccumulateTexture.get());
	m_squareAccumulateFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_squareAccumulateFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_squareTilesFramebuffer = Framebuffer::create();
	m_squareTilesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_squareTilesTexture.get());
	m_squareTilesFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_squareTilesFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_squareGridFramebuffer = Framebuffer::create();
	m_squareGridFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_squareGridTexture.get());
	m_squareGridFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_squareGridFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_shadeFramebuffer = Framebuffer::create();
	m_shadeFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_colorTexture.get());
	m_shadeFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	m_shadeFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
}


void HexTileRenderer::display()
{
	auto currentState = State::currentState();

	if (viewer()->viewportSize() != m_framebufferSize)
	{
		m_framebufferSize = viewer()->viewportSize();
		m_pointChartTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_hexTilesTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_squareTilesTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}


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

	if (m_squareSize != m_squareSize_tmp) {
		calculateSquareTextureSize(inverseModelViewProjectionMatrix);
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

	// Scatterplot GUI -----------------------------------------------------------------------------------------------------------
	renderGUI();

	setShaderDefines();
	// ---------------------------------------------------------------------------------------------------------------------------

	// do not render if either the dataset was not loaded or the window is minimized 
	if (viewer()->scene()->table()->activeTableData().size() == 0 || viewer()->viewportSize().x == 0 || viewer()->viewportSize().y == 0)
	{
		return;
	}

	int vertexCount = int(viewer()->scene()->table()->activeTableData()[0].size());

	// ====================================================================================== POINTS RENDER PASS =======================================================================================

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

	shaderProgram_points->setUniform("pointColor", viewer()->samplePointColor());

	shaderProgram_points->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	if (m_colorMapLoaded)
	{
		m_colorMapTexture->bindActive(5);
		shaderProgram_points->setUniform("colorMapTexture", 5);
		shaderProgram_points->setUniform("textureWidth", m_ColorMapWidth);
		shaderProgram_points->setUniform("viewportX", float(viewportSize.x));
	}

	m_vao->bind();
	shaderProgram_points->use();

	m_vao->drawArrays(GL_POINTS, 0, vertexCount);

	shaderProgram_points->release();
	m_vao->unbind();

	if (m_colorMapLoaded)
	{
		m_colorMapTexture->unbindActive(5);
	}

	// disable blending for draw buffer 0 (classical scatter plot)
	glDisablei(GL_BLEND, 0);

	m_pointFramebuffer->unbind();

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// ====================================================================================== POINT CIRCLES RENDER PASS =======================================================================================

	m_pointCircleFramebuffer->bind();
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

	auto shaderProgram_pointCircles = shaderProgram("point-circle");

	//set correct radius
	float scaleAdjustedRadius = m_pointCircleRadius / pointCircleRadiusDiv * viewer()->scaleFactor();;

	//geometry shader
	shaderProgram_pointCircles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	shaderProgram_pointCircles->setUniform("radius", scaleAdjustedRadius);
	shaderProgram_pointCircles->setUniform("aspectRatio", viewer()->m_windowHeight / viewer()->m_windowWidth);

	//fragment shader
	shaderProgram_pointCircles->setUniform("pointColor", viewer()->samplePointColor());

	m_vao->bind();
	shaderProgram_pointCircles->use();

	m_vao->drawArrays(GL_POINTS, 0, vertexCount);

	shaderProgram_pointCircles->release();
	m_vao->unbind();

	// disable blending for draw buffer 0 (classical scatter plot)
	glDisablei(GL_BLEND, 0);

	m_pointCircleFramebuffer->unbind();

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// ====================================================================================== DISCREPANCIES RENDER PASS ======================================================================================
	// Accumulate Points into squares

	m_tilesDiscrepanciesFramebuffer->bind();

	// set viewport to size of accumulation texture
	glViewport(0, 0, m_squareNumCols, m_squareNumRows);

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

	shaderProgram_discrepancies->setUniform("numCols", m_squareNumCols);
	shaderProgram_discrepancies->setUniform("numRows", m_squareNumRows);
	shaderProgram_discrepancies->setUniform("discrepancyDiv", m_discrepancyDiv);

	m_vaoTiles->bind();
	shaderProgram_discrepancies->use();

	m_vaoTiles->drawArrays(GL_POINTS, 0, numSquares);

	shaderProgram_discrepancies->release();
	m_vaoTiles->unbind();

	// disable blending
	glDisablei(GL_BLEND, 0);

	//reset Viewport
	glViewport(0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y);

	m_tilesDiscrepanciesFramebuffer->unbind();

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// ====================================================================================== ACCUMULATE RENDER PASS ======================================================================================
	// Accumulate Points into squares

	m_squareAccumulateFramebuffer->bind();

	// set viewport to size of accumulation texture
	glViewport(0, 0, m_squareNumCols, m_squareNumRows);

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

	auto shaderProgram_squares = shaderProgram("square-acc");

	shaderProgram_squares->setUniform("maxBounds_Off", maxBounds_Offset);
	shaderProgram_squares->setUniform("minBounds", minBounds);

	shaderProgram_squares->setUniform("maxTexCoordX", m_squareMaxX);
	shaderProgram_squares->setUniform("maxTexCoordY", m_squareMaxY);

	m_vao->bind();
	shaderProgram_squares->use();

	m_vao->drawArrays(GL_POINTS, 0, vertexCount);

	shaderProgram_squares->release();
	m_vao->unbind();

	if (m_colorMapLoaded)
	{
		m_colorMapTexture->unbindActive(5);
	}

	// disable blending
	glDisablei(GL_BLEND, 0);

	//reset Viewport
	glViewport(0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y);

	m_squareAccumulateFramebuffer->unbind();

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// ====================================================================================== MAX VAL & TILES RENDER PASS ======================================================================================
	// THIRD: Get maximum accumulated value (used for coloring) -  no framebuffer needed, because we don't render anything. we just save the max value into the storage buffer
	// FOURTH: Render Squares

	// SSBO --------------------------------------------------------------------------------------------------------------------------------------------------
	m_valueMaxBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 2);

	// max accumulated Value
	const uint maxValue = 0;
	m_valueMaxBuffer->clearSubData(GL_R32UI, 0, sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &maxValue);
	// -------------------------------------------------------------------------------------------------

	m_squareAccumulateTexture->bindActive(1);

	auto shaderProgram_accumulate_count = shaderProgram("square-acc-count");

	//geometry shader
	shaderProgram_accumulate_count->setUniform("maxBounds_Off", maxBounds_Offset);
	shaderProgram_accumulate_count->setUniform("maxBounds", maxBounds);
	shaderProgram_accumulate_count->setUniform("minBounds", minBounds);

	//geometry & fragment shader
	shaderProgram_accumulate_count->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	shaderProgram_accumulate_count->setUniform("windowWidth", viewer()->viewportSize()[0]);
	shaderProgram_accumulate_count->setUniform("windowHeight", viewer()->viewportSize()[1]);

	//fragment Shader
	shaderProgram_accumulate_count->setUniform("maxTexCoordX", m_squareMaxX);
	shaderProgram_accumulate_count->setUniform("maxTexCoordY", m_squareMaxY);

	shaderProgram_accumulate_count->setUniform("squareAccumulateTexture", 1);

	m_vaoQuad->bind();

	shaderProgram_accumulate_count->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	shaderProgram_accumulate_count->release();

	m_vaoQuad->unbind();

	m_squareAccumulateTexture->unbindActive(1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// -------------------------------------------------------------------------------------------------
	// Render squares
	m_squareTilesFramebuffer->bind();

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

	m_squareAccumulateTexture->bindActive(1);
	m_tilesDiscrepanciesTexture->bindActive(2);

	auto shaderProgram_square_tiles = shaderProgram("square-tiles");

	//geometry shader
	shaderProgram_square_tiles->setUniform("maxBounds_Off", maxBounds_Offset);
	shaderProgram_square_tiles->setUniform("maxBounds", maxBounds);
	shaderProgram_square_tiles->setUniform("minBounds", minBounds);

	//geometry & fragment shader
	shaderProgram_square_tiles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	shaderProgram_square_tiles->setUniform("windowWidth", viewer()->viewportSize()[0]);
	shaderProgram_square_tiles->setUniform("windowHeight", viewer()->viewportSize()[1]);

	//fragment Shader
	shaderProgram_square_tiles->setUniform("maxTexCoordX", m_squareMaxX);
	shaderProgram_square_tiles->setUniform("maxTexCoordY", m_squareMaxY);

	shaderProgram_square_tiles->setUniform("squareAccumulateTexture", 1);
	shaderProgram_square_tiles->setUniform("tilesDiscrepancyTexture", 2);

	if (m_colorMapLoaded)
	{
		m_colorMapTexture->bindActive(5);
		shaderProgram_square_tiles->setUniform("colorMapTexture", 5);
		shaderProgram_square_tiles->setUniform("textureWidth", m_ColorMapWidth);
	}

	m_vaoQuad->bind();

	shaderProgram_square_tiles->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	shaderProgram_square_tiles->release();

	m_vaoQuad->unbind();

	if (m_colorMapLoaded)
	{
		m_colorMapTexture->unbindActive(5);
	}

	m_squareAccumulateTexture->unbindActive(1);
	m_tilesDiscrepanciesTexture->unbindActive(2);

	// disable blending
	glDisablei(GL_BLEND, 0);

	m_squareTilesFramebuffer->unbind();

	//unbind shader storage buffer
	m_valueMaxBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// ====================================================================================== GRID RENDER PASS ======================================================================================
	// render square grid into texture

	m_squareGridFramebuffer->bind();

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

	m_squareGridTexture->bindActive(0);
	//used to check if we actually need to draw the grid for a given square
	m_squareAccumulateTexture->bindActive(1);

	auto shaderProgram_square_grid = shaderProgram("square-grid");

	//set uniforms

	//vertex shader
	shaderProgram_square_grid->setUniform("numCols", m_squareNumCols);
	shaderProgram_square_grid->setUniform("numRows", m_squareNumRows);

	//geometry shader
	shaderProgram_square_grid->setUniform("squareSize", squareSizeWS);
	shaderProgram_square_grid->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	shaderProgram_square_grid->setUniform("windowWidth", viewer()->viewportSize()[0]);
	shaderProgram_square_grid->setUniform("windowHeight", viewer()->viewportSize()[1]);
	shaderProgram_square_grid->setUniform("maxBounds_Off", maxBounds_Offset);
	shaderProgram_square_grid->setUniform("maxBounds", maxBounds);
	shaderProgram_square_grid->setUniform("minBounds", minBounds);
	shaderProgram_square_grid->setUniform("squareAccumulateTexture", 1);

	//fragment Shader
	shaderProgram_square_grid->setUniform("squareBorderColor", vec3(1.0f, 1.0f, 1.0f));
	shaderProgram_square_grid->setUniform("squareGridTexture", 0);

	m_vaoTiles->bind();

	shaderProgram_square_grid->use();
	m_vaoTiles->drawArrays(GL_POINTS, 0, numSquares);
	shaderProgram_square_grid->release();

	m_vaoTiles->unbind();

	m_squareGridTexture->unbindActive(0);
	m_squareAccumulateTexture->unbindActive(1);


	// disable blending
	glDisablei(GL_BLEND, 0);

	m_squareGridFramebuffer->unbind();

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// ====================================================================================== SHADE/BLEND RENDER PASS ======================================================================================
	// blend everything together and draw to screen
	m_shadeFramebuffer->bind();

	m_pointChartTexture->bindActive(0);
	m_squareTilesTexture->bindActive(1);
	m_squareAccumulateTexture->bindActive(2);
	m_squareGridTexture->bindActive(3);
	m_pointCircleTexture->bindActive(4);

	auto shaderProgram_shade = shaderProgram("shade");

	shaderProgram_shade->setUniform("pointChartTexture", 0);

	if (m_renderPointCircles) {
		shaderProgram_shade->setUniform("pointCircleTexture", 4);
	}

	if (m_renderSquares) {
		shaderProgram_shade->setUniform("tilesTexture", 1);
		shaderProgram_shade->setUniform("gridTexture", 3);
	}

	if (m_renderAccumulatePoints) {
		shaderProgram_shade->setUniform("accPointTexture", 2);
	}

	m_vaoQuad->bind();

	shaderProgram_shade->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	shaderProgram_shade->release();

	m_vaoQuad->unbind();

	m_pointChartTexture->unbindActive(0);
	m_squareTilesTexture->unbindActive(1);
	m_squareAccumulateTexture->unbindActive(2);
	m_squareGridTexture->unbindActive(3);
	m_pointCircleTexture->unbindActive(4);

	m_shadeFramebuffer->unbind();

	m_shadeFramebuffer->blit(GL_COLOR_ATTACHMENT0, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	currentState->apply();
}

// --------------------------------------------------------------------------------------
// ###########################  SQUARE CALC ############################################
// --------------------------------------------------------------------------------------

void HexTileRenderer::calculateSquareTextureSize(const mat4 inverseModelViewProjectionMatrix) {

	// set new size
	m_squareSize = m_squareSize_tmp;

	squareSizeWS = (inverseModelViewProjectionMatrix * vec4(m_squareSize_tmp / squareSizeDiv, 0, 0, 0)).x;
	squareSizeWS *= viewer()->scaleFactor();

	vec3 maxBounds = viewer()->scene()->table()->maximumBounds();
	vec3 minBounds = viewer()->scene()->table()->minimumBounds();

	vec3 boundingBoxSize = maxBounds - minBounds;

	// get maximum value of X,Y in accumulateTexture-Space
	m_squareNumCols = ceil(boundingBoxSize.x / squareSizeWS);
	m_squareNumRows = ceil(boundingBoxSize.y / squareSizeWS);
	numSquares = m_squareNumRows * m_squareNumCols;
	m_squareMaxX = m_squareNumCols - 1;
	m_squareMaxY = m_squareNumRows - 1;

	//The squares on the maximum sides of the bounding box, will not fit into the box perfectly most of the time
	//therefore we calculate new maximum bounds that fit them perfectly
	//this way we can perform a mapping using the set square size
	maxBounds_Offset = vec2(m_squareNumCols * squareSizeWS + minBounds.x, m_squareNumRows * squareSizeWS + minBounds.y);


	//set texture size
	m_tilesDiscrepanciesTexture->image2D(0, GL_RGBA32F, ivec2(m_squareNumCols, m_squareNumRows), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	m_squareAccumulateTexture->image2D(0, GL_RGBA32F, ivec2(m_squareNumCols, m_squareNumRows), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	//calculate viewport settings
	/*vec4 testPoint = vec4(1.0f, 2.0f, 0.0f, 1.0f);

	vec2 testPointNDC = vec2((testPoint.x * 2 / float(m_squareMaxX)) - 1, (testPoint.y * 2 / float(m_squareMaxY)) - 1);
	vec2 testPointSS = vec2((testPointNDC.x + 1) * (m_squareMaxX / 2), (testPointNDC.y + 1) * (m_squareMaxY / 2));
	*/


	//setup grid vertices
	// create m_squareNumCols*m_squareNumRows vertices with position = 0.0f
	// we generate the correct position in the vertex shader
	m_verticesTiles->setData(std::vector<float>(m_squareNumCols*m_squareNumRows, 0.0f), GL_STATIC_DRAW);

	auto vertexBinding = m_vaoTiles->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_verticesTiles.get(), 0, sizeof(float));
	vertexBinding->setFormat(1, GL_FLOAT);
	m_vaoTiles->enable(0);


	//calc2D discrepancy and setup discrepancy buffer
	std::vector<float> tilesDiscrepancies;

	tilesDiscrepancies = CalculateDiscrepancy2D(viewer()->scene()->table()->activeXColumn(), viewer()->scene()->table()->activeYColumn(),
		viewer()->scene()->table()->maximumBounds(), viewer()->scene()->table()->minimumBounds());

	m_discrepanciesBuffer->setData(tilesDiscrepancies, GL_STATIC_DRAW);

	vertexBinding = m_vaoTiles->binding(1);
	vertexBinding->setAttribute(1);
	vertexBinding->setBuffer(m_discrepanciesBuffer.get(), 0, sizeof(float));
	vertexBinding->setFormat(1, GL_FLOAT);
	m_vaoTiles->enable(1);
}

// --------------------------------------------------------------------------------------
// ###########################  HEXAGON CALC ############################################
// --------------------------------------------------------------------------------------

void HexTileRenderer::renderHexagonGrid(mat4 modelViewProjectionMatrix) {
	// RENDER EMPTY HEXAGONS
	// TODO
	// think about, if we can maybe omit duplicate line rendering
	// fix bug with wrong line starts

	m_hexFramebuffer->bind();
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnablei(GL_BLEND, 0);
	glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquationi(0, GL_MAX);

	if (hexSize != m_hexSize_tmp) {
		calculateNumberOfHexagons();
	}
	if (hexRot != m_hexRot_tmp) {
		setRotationMatrix();
	}

	auto shaderProgram_hexagonGrid = shaderProgram("hexagon-grid");
	shaderProgram_hexagonGrid->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	shaderProgram_hexagonGrid->setUniform("hexBorderColor", vec3(1.0f, 1.0f, 1.0f));

	shaderProgram_hexagonGrid->setUniform("horizontal_space", horizontal_space);
	// divide by 2 because we use double-height coordinates in vertex shader
	shaderProgram_hexagonGrid->setUniform("vertical_space", vertical_space / 2.0f);
	shaderProgram_hexagonGrid->setUniform("num_cols", m_hexCols);
	shaderProgram_hexagonGrid->setUniform("data_offset", vec2(viewer()->scene()->table()->minimumBounds()));

	shaderProgram_hexagonGrid->setUniform("hexSize", hexSize);
	shaderProgram_hexagonGrid->setUniform("rotation", hexRotMat);

	m_vaoHex->bind();

	shaderProgram_hexagonGrid->use();

	m_vaoHex->drawArrays(GL_POINTS, 0, m_hexCount);

	shaderProgram_hexagonGrid->release();
	m_vaoHex->unbind();


	// disable blending for draw buffer 0 (classical scatter plot)
	glDisablei(GL_BLEND, 0);

	m_hexFramebuffer->unbind();

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}


void HexTileRenderer::calculateNumberOfHexagons() {

	// set new size
	hexSize = m_hexSize_tmp;

	// calculations derived from: https://www.redblobgames.com/grids/hexagons/
	// we assume flat topped hexagons
	// we use "Offset Coordinates"
	vec3 boundingBoxSize = viewer()->scene()->table()->maximumBounds() - viewer()->scene()->table()->minimumBounds();
	horizontal_space = hexSize * 1.5f;
	vertical_space = sqrt(3)*hexSize;

	//+1 because else the floor operation could return 0
	float cols_tmp = 1 + (boundingBoxSize.x / horizontal_space);
	m_hexCols = floor(cols_tmp);
	if ((cols_tmp - m_hexCols) * horizontal_space >= hexSize / 2.0f) {
		m_hexCols += 1;
	}

	float rows_tmp = 1 + (boundingBoxSize.y / vertical_space);
	m_hexRows = floor(rows_tmp);
	if ((rows_tmp - m_hexRows) * vertical_space >= vertical_space / 2) {
		m_hexRows += 1;
	}

	m_hexCount = m_hexRows * m_hexCols;

	// create m_hexCount vertices with position = 0.0f
	// we generate the correct position in the vertex shader
	m_verticesHex->setData(std::vector<float>(m_hexCount, 0.0f), GL_STATIC_DRAW);

	auto vertexBinding = m_vaoHex->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_verticesHex.get(), 0, sizeof(float));
	vertexBinding->setFormat(1, GL_FLOAT);
	m_vaoHex->enable(0);
}

void HexTileRenderer::setRotationMatrix() {
	// TODO: rotation
	// when rotating, we cannot longer use a rectangular grid, because points will fall outside
	// talk with thomas about possible solution
	hexRot = m_hexRot_tmp;

	vec3 minPos = viewer()->scene()->table()->minimumBounds();
	hexRotMat = glm::mat4(1);
	hexRotMat = glm::translate(hexRotMat, minPos);
	hexRotMat = glm::rotate(hexRotMat, glm::pi<float>() / 180 * hexRot, glm::vec3(0.0f, 0.0f, 1.0f));
	hexRotMat = glm::translate(hexRotMat, -minPos);
}

// --------------------------------------------------------------------------------------
// ###########################  GUI ##################################################### 
// --------------------------------------------------------------------------------------
/*
Renders the User interface
*/
void HexTileRenderer::renderGUI() {

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
			glm::mat4 newLightTransform = glm::inverse(viewTransform)*glm::translate(mat4(1.0f), (-5 * viewer()->m_scatterPlotDiameter*initLightDir))*viewTransform;
			viewer()->setLightTransform(newLightTransform);
			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

			// calculate accumulate texture settings - needs to be last step here ------------------------------
			calculateSquareTextureSize(inverse(viewer()->modelViewProjectionTransform()));
			calculateNumberOfHexagons();
			setRotationMatrix();
			// -------------------------------------------------------------------------------

		}

		if (ImGui::CollapsingHeader("Hexagonal Tiles"), ImGuiTreeNodeFlags_DefaultOpen)
		{
			ImGui::SliderFloat("Size", &m_hexSize_tmp, 5.0f, 200.0f);
			ImGui::SliderFloat("Rotation", &m_hexRot_tmp, 0.0f, 60.0f);
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

		if (ImGui::CollapsingHeader("Discrepancy"), ImGuiTreeNodeFlags_DefaultOpen)
		{
			ImGui::Checkbox("Render Point Circles", &m_renderPointCircles);
			ImGui::SliderFloat("Point Circle Radius", &m_pointCircleRadius, 1.0f, 100.0f);
			ImGui::SliderFloat("Discrepancy Divisor", &m_discrepancyDiv, 1.0f, 3.0f);
		}

		if (ImGui::CollapsingHeader("Square Tiles"), ImGuiTreeNodeFlags_DefaultOpen)
		{
			ImGui::Checkbox("Render Squares", &m_renderSquares);
			ImGui::Checkbox("Render Grid", &m_renderGrid);
			ImGui::Checkbox("Render Acc Points", &m_renderAccumulatePoints);
			ImGui::SliderFloat("Square Size ", &m_squareSize_tmp, 1.0f, 100.0f);

		}


		// update status
		dataChanged = false;
	}
	ImGui::End();
}


/*
Gathers all #define, reset the defines files and reload the shaders
*/
void HexTileRenderer::setShaderDefines() {
	std::string defines = "";


	if (m_colorMapLoaded)
		defines += "#define COLORMAP\n";

	if (m_renderPointCircles) {
		defines += "#define RENDER_POINT_CIRCLES\n";
	}

	if (m_renderSquares)
	{
		defines += "#define RENDER_SQUARES\n";
	}
	if (m_renderGrid) {
		defines += "#define RENDER_SQUARE_GRID\n";
	}
	if (m_renderAccumulatePoints)
		defines += "#define RENDER_ACC_POINTS\n";


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
float molumes::HexTileRenderer::mapInterval(float x, float a, float b, int c)
{
	return (x - a)*c / (b - a);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DISCREPANCY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<float> HexTileRenderer::CalculateDiscrepancy2D(const std::vector<float>& samplesX, const std::vector<float>& samplesY, vec3 maxBounds, vec3 minBounds)
{

	// Calculates the discrepancy of this data.
	// Assumes the data is [0,1) for valid sample range.
	// Assmues samplesX.size() == samplesY.size()
	int numSamples = samplesX.size();

	std::vector<float> sortedX(numSamples, 0.0f);
	std::vector<float> sortedY(numSamples, 0.0f);

	std::vector<float> pointsInTilesCount(numSquares, 0.0f);
	std::vector<float> tilesMaxBoundX(numSquares, -INFINITY);
	std::vector<float> tilesMaxBoundY(numSquares, -INFINITY);
	std::vector<float> tilesMinBoundX(numSquares, INFINITY);
	std::vector<float> tilesMinBoundY(numSquares, INFINITY);

	std::vector<float> discrepancies(numSquares, 0.0f);

	std::cout << "Discrepancy: " << numSamples << " Samples; " << numSquares << " Tiles" << '\n';

	//timing
	std::clock_t start;
	double duration;

	start = std::clock();

	//Step 1: Count how many elements belong to each square
	for (int i = 0; i < numSamples; i++) {

		float sampleX = samplesX[i];
		float sampleY = samplesY[i];

		// to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
		// If the current value = maxValue, we take the maxTexCoord instead
		int squareX = min(m_squareMaxX, int(mapInterval(sampleX, minBounds[0], maxBounds_Offset[0], m_squareMaxX + 1)));
		int squareY = min(m_squareMaxY, int(mapInterval(sampleY, minBounds[1], maxBounds_Offset[1], m_squareMaxY + 1)));

		pointsInTilesCount[squareX + m_squareNumCols * squareY]++;
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Step1: " << duration << '\n';

	start = clock();

	//Step 2: calc prefix Sum
	std::vector<float> pointsInTilesPrefixSum(pointsInTilesCount);
	int prefixSum = 0;
	for (int i = 0; i < numSquares; i++) {
		int sCount = pointsInTilesPrefixSum[i];
		pointsInTilesPrefixSum[i] = prefixSum;
		prefixSum += sCount;
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Step2: " << duration << '\n';

	start = clock();

	//Step 3: sort points according to squares
	// 1D array with "buckets" according to prefix Sum of squares
	// also set the bounding box of points inside tile
	std::vector<float> pointsInTilesRunningPrefixSum(pointsInTilesPrefixSum);
	for (int i = 0; i < numSamples; i++) {

		float sampleX = samplesX[i];
		float sampleY = samplesY[i];

		// to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
		// If the current value = maxValue, we take the maxTexCoord instead
		int squareX = min(m_squareMaxX, int(mapInterval(sampleX, minBounds[0], maxBounds_Offset[0], m_squareMaxX + 1)));
		int squareY = min(m_squareMaxY, int(mapInterval(sampleY, minBounds[1], maxBounds_Offset[1], m_squareMaxY + 1)));

		int squareIndex1D = squareX + m_squareNumCols * squareY;

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
	std::cout << "Step3: " << duration << '\n';

	start = clock();

	//Step 4: calculate discrepancy for each tile
#pragma omp parallel
	{
#pragma omp for
		for (int i = 0; i < numSquares; i++) {

			float maxDifference = 0.0f;
			//use the addition and not PrefixSum[i+1] so we can easily get the last boundary
			for (int j = pointsInTilesPrefixSum[i]; j < pointsInTilesPrefixSum[i] + pointsInTilesCount[i]; j++)
			{
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

			// set tile discrepancy
			discrepancies[i] = maxDifference;
		}
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Step4: " << duration << '\n' << '\n';

	return discrepancies;
}