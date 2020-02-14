#include "HexTileRenderer.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
#include "../Viewer.h"
#include "../Scene.h"
#include "../Table.h"
#include <lodepng.h>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


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

	
	m_shaderSourceDefines = StaticStringSource::create("");
	m_shaderDefines = NamedString::create("/defines.glsl", m_shaderSourceDefines.get());

	m_shaderSourceGlobals = File::create("./res/hexagon/globals.glsl");
	m_shaderGlobals = NamedString::create("/globals.glsl", m_shaderSourceGlobals.get());
	
	// create shader programs
	createShaderProgram("points", {
		{GL_VERTEX_SHADER,"./res/hexagon/point-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/point-fs.glsl"}
		});

	createShaderProgram("hexagon-grid", { 
		{GL_VERTEX_SHADER,"./res/hexagon/hexagon-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/hexagon/hexagon-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/hexagon-fs.glsl"} 
		});

	createShaderProgram("shade", {
		{GL_VERTEX_SHADER,"./res/hexagon/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/hexagon/image-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/hexagon/shade-fs.glsl"}
		});

	m_framebufferSize = viewer->viewportSize();

	// init textures
	m_pointChartTexture = Texture::create(GL_TEXTURE_2D);
	m_pointChartTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_pointChartTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_pointChartTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_pointChartTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_pointChartTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_hexTilesTexture = Texture::create(GL_TEXTURE_2D);
	m_hexTilesTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_hexTilesTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_hexTilesTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_hexTilesTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_hexTilesTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_squareTilesTexture = Texture::create(GL_TEXTURE_2D);
	m_squareTilesTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_squareTilesTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_squareTilesTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_squareTilesTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//TODO: set dynamic size
	m_squareTilesTexture->image2D(0, GL_R16, ivec2(4, 4), 0, GL_R, GL_UNSIGNED_BYTE, nullptr);

	m_colorTexture = Texture::create(GL_TEXTURE_2D);
	m_colorTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	//colorMap
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

	m_hexFramebuffer = Framebuffer::create();
	m_hexFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_hexTilesTexture.get());
	m_hexFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_hexFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_squareFramebuffer = Framebuffer::create();
	m_squareFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_hexTilesTexture.get());
	m_squareFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_squareFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

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
		//TODO: set dynamic size
		m_squareTilesTexture->image2D(0, GL_R16, ivec2(4,4), 0, GL_R, GL_UNSIGNED_BYTE, nullptr);
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

	// ====================================================================================== FIRST RENDER PASS =======================================================================================

	m_pointFramebuffer->bind();
	glClearDepth(1.0f);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// make sure points are drawn on top of each other
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

	// test different blending options interactively: --------------------------------------------------
	// https://andersriggelsen.dk/glblendfunc.php

	// allow blending for the classical point chart color-attachment (0) of the point frame-buffer
	glEnablei(GL_BLEND, 0);
	glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquationi(0, GL_MAX);

	// -------------------------------------------------------------------------------------------------

	auto shaderProgram_points = shaderProgram("points");
	shaderProgram_points->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	shaderProgram_points->setUniform("pointColor", viewer()->samplePointColor());

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
	// ====================================================================================== SECOND RENDER PASS ======================================================================================
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

	// ====================================================================================== THIRD RENDER PASS ======================================================================================
	m_shadeFramebuffer->bind();

	m_pointChartTexture->bindActive(0);
	m_hexTilesTexture->bindActive(1);

	auto shaderProgram_shade = shaderProgram("shade");
	shaderProgram_shade->setUniform("pointChartTexture", 0);
	shaderProgram_shade->setUniform("hexTilesTexture", 1);

	m_vaoQuad->bind();

	shaderProgram_shade->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	shaderProgram_shade->release();

	m_vaoQuad->unbind();

	m_pointChartTexture->unbindActive(0);
	m_hexTilesTexture->unbindActive(1);

	m_shadeFramebuffer->unbind();

	m_shadeFramebuffer->blit(GL_COLOR_ATTACHMENT0, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	currentState->apply();
}

// --------------------------------------------------------------------------------------
// ###########################  HEXAGON CALC ############################################
// --------------------------------------------------------------------------------------

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

			calculateNumberOfHexagons();
			setRotationMatrix();
			// -------------------------------------------------------------------------------


			// Scaling the model's bounding box to the canonical view volume
			vec3 boundingBoxSize = viewer()->scene()->table()->maximumBounds() - viewer()->scene()->table()->minimumBounds();
			float maximumSize = std::max({ boundingBoxSize.x, boundingBoxSize.y, boundingBoxSize.z });
			mat4 modelTransform = scale(vec3(2.0f) / vec3(maximumSize));
			modelTransform = modelTransform * translate(-0.5f*(viewer()->scene()->table()->minimumBounds() + viewer()->scene()->table()->maximumBounds()));
			viewer()->setModelTransform(modelTransform);

			// store diameter of current scatter plot and initialize light position
			viewer()->m_scatterPlotDiameter = sqrt(pow(boundingBoxSize.x, 2) + pow(boundingBoxSize.y, 2));

			// initial position of the light source (azimuth 120 degrees, elevation 45 degrees, 5 times the distance to the object in center) ---------------------------------------------------------------------------------------------------------
			glm::mat4 viewTransform = viewer()->viewTransform();
			glm::vec3 initLightDir = normalize(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
			glm::mat4 newLightTransform = glm::inverse(viewTransform)*glm::translate(mat4(1.0f), (-5 * viewer()->m_scatterPlotDiameter*initLightDir))*viewTransform;
			viewer()->setLightTransform(newLightTransform);
			//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
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

		// update status
		dataChanged = false;
	}
	ImGui::End();
}


/*
Gathers all #define, sets them in the shaders an reloads them
*/
void HexTileRenderer::setShaderDefines() {
	std::string defines = "";


	if (m_colorMapLoaded)
		defines += "#define COLORMAP\n";

	if (defines != m_shaderSourceDefines->string())
	{
		m_shaderSourceDefines->setString(defines);

		reloadShaders();
	}
}