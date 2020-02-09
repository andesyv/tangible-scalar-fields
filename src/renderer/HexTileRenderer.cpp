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

	//load source files
	m_vertexShaderSourcePoint = Shader::sourceFromFile("./res/hexagon/point-vs.glsl");
	m_fragmentShaderSourcePoint = Shader::sourceFromFile("./res/hexagon/point-fs.glsl");
	//
	m_vertexShaderSourceHex = Shader::sourceFromFile("./res/hexagon/hexagon-vs.glsl");
	m_geometryShaderSourceHex = Shader::sourceFromFile("./res/hexagon/hexagon-gs.glsl");
	m_fragmentShaderSourceHex = Shader::sourceFromFile("./res/hexagon/hexagon-fs.glsl");
	//
	m_vertexShaderSourceImage = Shader::sourceFromFile("./res/hexagon/image-vs.glsl");
	m_geometryShaderSourceImage = Shader::sourceFromFile("./res/hexagon/image-gs.glsl");
	m_fragmentShaderSourceShade = Shader::sourceFromFile("./res/hexagon/shade-fs.glsl");

	// create templates
	m_vertexShaderTemplatePoint = Shader::applyGlobalReplacements(m_vertexShaderSourcePoint.get());
	m_fragmentShaderTemplatePoint = Shader::applyGlobalReplacements(m_fragmentShaderSourcePoint.get());
	//
	m_vertexShaderTemplateHex = Shader::applyGlobalReplacements(m_vertexShaderSourceHex.get());
	m_geometryShaderTemplateHex = Shader::applyGlobalReplacements(m_geometryShaderSourceHex.get());
	m_fragmentShaderTemplateHex = Shader::applyGlobalReplacements(m_fragmentShaderSourceHex.get());
	//
	m_vertexShaderTemplateImage = Shader::applyGlobalReplacements(m_vertexShaderSourceImage.get());
	m_geometryShaderTemplateImage = Shader::applyGlobalReplacements(m_geometryShaderSourceImage.get());
	m_fragmentShaderTemplateShade = Shader::applyGlobalReplacements(m_fragmentShaderSourceShade.get());

	// create Shader
	m_vertexShaderPoint = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplatePoint.get());
	m_fragmentShaderPoint = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplatePoint.get());
	//
	m_vertexShaderHex = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateHex.get());
	m_geometryShaderHex = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateHex.get());
	m_fragmentShaderHex = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateHex.get());
	//
	m_vertexShaderImage = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateImage.get());
	m_geometryShaderImage = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateImage.get());
	m_fragmentShaderShade = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateShade.get());

	// attach shader to program
	m_programPoint->attach(m_vertexShaderPoint.get(), m_fragmentShaderPoint.get());
	m_programHex->attach(m_vertexShaderHex.get(), m_geometryShaderHex.get(), m_fragmentShaderHex.get());
	//m_programHex->attach(m_vertexShaderHex.get(), m_fragmentShaderHex.get());
	m_programShade->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderShade.get());

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

	m_colorTexture = Texture::create(GL_TEXTURE_2D);
	m_colorTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

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

	m_shadeFramebuffer = Framebuffer::create();
	m_shadeFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_colorTexture.get());
	m_shadeFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	m_shadeFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
}

std::list<globjects::File*> HexTileRenderer::shaderFiles() const
{
	return std::list<globjects::File*>({
		m_shaderSourceGlobals.get(),
		m_vertexShaderSourcePoint.get(),
		m_fragmentShaderSourcePoint.get(),
		m_vertexShaderSourceImage.get(),
		m_geometryShaderSourceImage.get(),
		m_fragmentShaderSourceShade.get()
		});
}


void HexTileRenderer::display()
{
	//TODO: ask thomas what that does
	auto currentState = State::currentState();

	if (viewer()->viewportSize() != m_framebufferSize)
	{
		m_framebufferSize = viewer()->viewportSize();
		m_pointChartTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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

	m_programPoint->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	m_programPoint->setUniform("pointColor", viewer()->samplePointColor());

	m_vao->bind();
	m_programPoint->use();

	m_vao->drawArrays(GL_POINTS, 0, vertexCount);

	m_programPoint->release();
	m_vao->unbind();

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

	if (m_hexSize != m_hexSize_tmp) {
		calculateNumberOfHexagons();
	}

	// create rotation matrix

	m_programHex->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programHex->setUniform("hexBorderColor", vec3(1.0f, 1.0f, 1.0f));

	m_programHex->setUniform("horizontal_space", horizontal_space);
	// divide by 2 because we use double-height coordinates in vertex shader
	m_programHex->setUniform("vertical_space", vertical_space / 2.0f);
	m_programHex->setUniform("num_cols", m_hexCols);
	m_programHex->setUniform("data_offset", vec2(viewer()->scene()->table()->minimumBounds()));
	
	m_programHex->setUniform("hexSize", m_hexSize);
	//m_programHex->setUniform("rotation", m_hexRot);

	m_vaoHex->bind();

	m_programHex->use();

	m_vaoHex->drawArrays(GL_POINTS, 0, m_hexCount);

	m_programHex->release();
	m_vaoHex->unbind();

	// disable blending for draw buffer 0 (classical scatter plot)
	glDisablei(GL_BLEND, 0);

	m_hexFramebuffer->unbind();

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// ====================================================================================== THIRD RENDER PASS ======================================================================================
	m_shadeFramebuffer->bind();

	m_pointChartTexture->bindActive(0);
	m_hexTilesTexture->bindActive(1);

	m_programShade->setUniform("pointChartTexture", 0);
	m_programShade->setUniform("hexTilesTexture", 1);

	m_vaoQuad->bind();

	m_programShade->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_programShade->release();

	m_vaoQuad->unbind();

	m_pointChartTexture->unbindActive(1);
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
	m_hexSize = m_hexSize_tmp;

	// calculations derived from: https://www.redblobgames.com/grids/hexagons/
	// we assume flat topped hexagons
	// we use "Offset Coordinates"
	vec3 boundingBoxSize = viewer()->scene()->table()->maximumBounds() - viewer()->scene()->table()->minimumBounds();
	horizontal_space = m_hexSize * 1.5f;
	vertical_space = sqrt(3)*m_hexSize;

	//+1 because else the floor operation could return 0
	float cols_tmp = 1 + (boundingBoxSize.x / horizontal_space);
	m_hexCols = floor(cols_tmp);
	if ((cols_tmp - m_hexCols) * horizontal_space >= m_hexSize / 2.0f) {
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
			ImGui::SliderFloat("Rotation", &m_hexRot, 0.0f, 60.0f);
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


	if (m_colorMapLoaded && m_heatMapGUI)
		defines += "#define HEATMAP_GUI\n";

	if (defines != m_shaderSourceDefines->string())
	{
		m_shaderSourceDefines->setString(defines);

		for (auto& s : shaderFiles())
			s->reload();
	}
}