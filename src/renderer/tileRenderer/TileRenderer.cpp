#include "TileRenderer.h"
#include "../../Utils.h"

#include <imgui.h>
#include <omp.h>
#include <lodepng.h>
#include <ctime>
#include <memory>
#include <format>
#include <algorithm>
#include <chrono>

#include <glbinding/gl/gl.h>

#include <globjects/State.h>
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
#include <globjects/NamedString.h>
#include <globjects/base/StaticStringSource.h>
#include <globjects/Sync.h>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/transform.hpp>
#include <chrono>

#endif

#define GLM_ENABLE_EXPERIMENTAL

#include "Tile.h"
#include "SquareTile.h"
#include "HexTile.h"
#include "../../Viewer.h"
#include "../../Scene.h"
#include "../../CSV/Table.h"
#include "../../interactors/HapticInteractor.h"

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

auto get_index_offset(uint current_index, int offset) {
    const auto index = (static_cast<int>(current_index) + offset) % static_cast<int>(TileRenderer::ROUND_ROBIN_SIZE);
    return index < 0 ? TileRenderer::ROUND_ROBIN_SIZE + index : index;
}

TileRenderer::TileRenderer(Viewer *viewer,
                           WriterChannel<std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, HapticMipMapLevels>> &&normal_channel)
        : Renderer(viewer), m_normal_tex_channel{normal_channel} {
    m_verticesQuad->setStorage(std::array<vec3, 1>({vec3(0.0f, 0.0f, 0.0f)}), gl::GL_NONE_BIT);
    auto vertexBindingQuad = m_vaoQuad->binding(0);
    vertexBindingQuad->setBuffer(m_verticesQuad.get(), 0, sizeof(vec3));
    vertexBindingQuad->setFormat(3, GL_FLOAT);
    m_vaoQuad->enable(0);
    m_vaoQuad->unbind();

    // shader storage buffer object for current maximum accumulated value and maximum alpha value of point circle blending
    m_valueMaxBuffer = std::move(std::make_shared<globjects::Buffer>());
    viewer->m_sharedResources.tileAccumulateMax = m_valueMaxBuffer;
    m_valueMaxBuffer->setStorage(sizeof(uint) * 2, nullptr, gl::GL_NONE_BIT);

    m_shaderSourceDefines = StaticStringSource::create("");
    m_shaderDefines = NamedString::create("/defines.glsl", m_shaderSourceDefines.get());

    m_shaderSourceGlobals = File::create("./res/tiles/globals.glsl");
    m_shaderGlobals = NamedString::create("/globals.glsl", m_shaderSourceGlobals.get());

    m_shaderSourceGlobalsHexagon = File::create("./res/tiles/hexagon/globals.glsl");
    m_shaderGlobalsHexagon = NamedString::create("/hex/globals.glsl", m_shaderSourceGlobalsHexagon.get());

    // initialise tile objects
    tile_processors["square"] = std::move(std::make_shared<SquareTile>(this));
    tile_processors["hexagon"] = std::move(std::make_shared<HexTile>(this));

    // create shader programs
    createShaderProgram("points", {
            {GL_VERTEX_SHADER,   "./res/tiles/point-vs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/tiles/point-fs.glsl"}
    });

    createShaderProgram("point-circle", {
            {GL_VERTEX_SHADER,   "./res/tiles/point-circle-vs.glsl"},
            {GL_GEOMETRY_SHADER, "./res/tiles/point-circle-gs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/tiles/point-circle-fs.glsl"}
    });

    createShaderProgram("tiles-disc", {
            {GL_VERTEX_SHADER,   "./res/tiles/discrepancy-vs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/tiles/discrepancy-fs.glsl"}
    });

    createShaderProgram("max-val", {
            {GL_VERTEX_SHADER,   "./res/tiles/image-vs.glsl"},
            {GL_GEOMETRY_SHADER, "./res/tiles/bounding-quad-gs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/tiles/max-val-fs.glsl"}
    });

    createShaderProgram("shade", {
            {GL_VERTEX_SHADER,   "./res/tiles/image-vs.glsl"},
            {GL_GEOMETRY_SHADER, "./res/tiles/image-gs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/tiles/shade-fs.glsl"}
    });

    m_framebufferSize = viewer->viewportSize();

    // init textures
    m_pointChartTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0,
                                          GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    m_pointCircleTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0,
                                           GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // the size of this textures is set dynamicly depending on the tile grid resolution
    m_tilesDiscrepanciesTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
                                                  GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, ivec2(1, 1), 0, GL_RGBA,
                                                  GL_UNSIGNED_BYTE, nullptr);
    // Convert to shared ptr so it can be shared across multiple renderers:
    auto tex = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                               0, GL_RGBA32F, ivec2(1, 1), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    m_tileAccumulateTexture = std::shared_ptr{std::move(tex)};
    viewer->m_sharedResources.tileAccumulateTexture = m_tileAccumulateTexture;

    m_tilesTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0,
                                     GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    m_normalsAndDepthTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                                               0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    m_gridTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0,
                                    GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    m_kdeTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0,
                                   GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    for (auto i = 0u; i < ROUND_ROBIN_SIZE; ++i) {
        auto texture = std::shared_ptr{std::move(Texture::create(GL_TEXTURE_2D))};
        texture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        texture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        texture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        texture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        texture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        m_normal_frame_data.at(i).texture = std::move(texture);
    }
    // TODO: Switch around this one
    viewer->m_sharedResources.smoothNormalsTexture = m_normal_frame_data.front().texture;

    m_colorTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0,
                                     GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

//    m_normal_transfer_buffer->setStorage(1920 * 1080 * sizeof(glm::vec4), GL_MAP_READ_BIT);
//    m_normal_transfer_buffer->setData(1920 * 1080 * sizeof(glm::vec4), nullptr, GL_DYNAMIC_READ);

    //colorMap - 1D Texture
    m_colorMapTexture = Texture::create(GL_TEXTURE_1D);
    m_colorMapTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_colorMapTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //tile textures array
    m_tileTextureArray = Texture::create(GL_TEXTURE_2D_ARRAY);
    m_tileTextureArray->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_tileTextureArray->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_tileTextureArray->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_tileTextureArray->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // allocate storage according to default values
    m_tileTextureArray->storage3D(1, GL_RGBA32F, static_cast<GLsizei>(m_tileTextureWidth),
                                  static_cast<GLsizei>(m_tileTextureHeight), m_numberTextureTiles);

    for (int i = 1; i <= m_numberTextureTiles; i++) {

        std::vector<unsigned char> tileTextureImage;
        std::string filePath = "./dat/tileTextures/tileTexture_" + std::to_string(i) + ".png";

        uint error = lodepng::decode(tileTextureImage, m_tileTextureWidth, m_tileTextureHeight, filePath);

        if (error)
            globjects::debug() << "Could not load " << filePath << "!";
        else {
            m_tileTextureArray->subImage3D(0, 0, 0, /*Z-offset*/ i - 1, static_cast<GLsizei>(m_tileTextureWidth),
                                           static_cast<GLsizei>(m_tileTextureHeight), 1,
                                           GL_RGBA, GL_UNSIGNED_BYTE, (void *) &tileTextureImage.front());
        }
    }

    m_colorMapTexture->generateMipmap();

    // create Framebuffer
    m_pointFramebuffer = Framebuffer::create();
    m_pointFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_pointChartTexture.get());
    m_pointFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
    m_pointFramebuffer->setDrawBuffers({GL_COLOR_ATTACHMENT0});

    m_pointCircleFramebuffer = Framebuffer::create();
    m_pointCircleFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_pointCircleTexture.get());
    m_pointCircleFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_kdeTexture.get());
    m_pointCircleFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
    m_pointCircleFramebuffer->setDrawBuffers({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});

    m_tilesDiscrepanciesFramebuffer = Framebuffer::create();
    m_tilesDiscrepanciesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tilesDiscrepanciesTexture.get());
    m_tilesDiscrepanciesFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
    m_tilesDiscrepanciesFramebuffer->setDrawBuffers({GL_COLOR_ATTACHMENT0});

    m_tileAccumulateFramebuffer = Framebuffer::create();
    m_tileAccumulateFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tileAccumulateTexture.get());
    m_tileAccumulateFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
    m_tileAccumulateFramebuffer->setDrawBuffers({GL_COLOR_ATTACHMENT0});

    m_tilesFramebuffer = Framebuffer::create();
    m_tilesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tilesTexture.get());
    m_tilesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_normalsAndDepthTexture.get());
    m_tilesFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
    m_tilesFramebuffer->setDrawBuffers({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});

    m_gridFramebuffer = Framebuffer::create();
    m_gridFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_gridTexture.get());
    m_gridFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
    m_gridFramebuffer->setDrawBuffers({GL_COLOR_ATTACHMENT0});

    m_shadeFramebuffer = Framebuffer::create();
    m_shadeFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_colorTexture.get());
    m_shadeFramebuffer->setDrawBuffers({GL_COLOR_ATTACHMENT0});
    m_shadeFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());

    for (auto i = 0u; i < ROUND_ROBIN_SIZE; ++i) {
        auto &fb = m_normal_frame_data.at(i).framebuffer;
        fb = Framebuffer::create();
        fb->attachTexture(GL_COLOR_ATTACHMENT0, m_normal_frame_data.at(i).texture.get());
        fb->setDrawBuffers({GL_COLOR_ATTACHMENT0});
        fb->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
    }

    m_colorMapLoaded = updateColorMap();
}

void molumes::TileRenderer::setEnabled(bool enabled) {
    Renderer::setEnabled(enabled);

    if (enabled)
        viewer()->setPerspective(false);

    viewer()->m_cameraRotateAllowed = !enabled;
}


void TileRenderer::display() {
    auto mainState = stateGuard();

    // Scatterplot GUI -----------------------------------------------------------------------------------------------------------
    // needs to be first to set all the new values before rendering
    renderGUI();

    setShaderDefines();
    // ---------------------------------------------------------------------------------------------------------------------------

    // do not render if either the dataset was not loaded or the window is minimized
    if (viewer()->scene()->table()->activeTableData().empty() || viewer()->viewportSize().x == 0 ||
        viewer()->viewportSize().y == 0) {
        return;
    }

    // number of datapoints
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
    const vec4 backgroundColor = vec4(viewer()->backgroundColor(), 0);

    // transform light position, since we evaluate the illumination model in view space
    vec4 viewLightPosition = viewMatrix * viewer()->lightTransform() * vec4(0.0f, 0.0f, 0.0f, 1.0f);

    // if any of these values changed we need to:
    // recompute the resolution and positioning of the current tile grid
    // reset the texture resolutions
    // recompute the discrepancy
    if (m_selected_tile_style != m_selected_tile_style_tmp || m_tileSize != m_tileSize_tmp ||
        m_renderDiscrepancy != m_renderDiscrepancy_tmp
        || m_discrepancy_easeIn != m_discrepancy_easeIn_tmp || m_discrepancy_lowCount != m_discrepancy_lowCount_tmp ||
        viewer()->viewportSize() != m_framebufferSize) {

        // set new values
        m_selected_tile_style = m_selected_tile_style_tmp;
        m_tileSize = m_tileSize_tmp;
        m_renderDiscrepancy = m_renderDiscrepancy_tmp;
        m_discrepancy_easeIn = m_discrepancy_easeIn_tmp;
        m_discrepancy_lowCount = m_discrepancy_lowCount_tmp;

        //set tile processor
        switch (m_selected_tile_style) {
            case 1:
                tile = tile_processors["square"].get();
                viewer()->m_sharedResources.tile = tile_processors["square"];
                break;
            case 2:
                tile = tile_processors["hexagon"].get();
                viewer()->m_sharedResources.tile = tile_processors["hexagon"];
                break;
            default:
                viewer()->m_sharedResources.tile = {};
                tile = nullptr;
                break;
        }

        if (viewportSize != m_framebufferSize) {
            m_framebufferSize = viewportSize;
            for (auto *tex: std::to_array({m_pointChartTexture.get(), m_pointCircleTexture.get(), m_tilesTexture.get(),
                                           m_normalsAndDepthTexture.get(), m_gridTexture.get(), m_kdeTexture.get(),
                                           m_colorTexture.get(), m_normal_frame_data[0].texture.get(),
                                           m_normal_frame_data[1].texture.get(),
                                           m_normal_frame_data[2].texture.get()})) // Way too lazy to make this dynamic
                tex->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }

        calculateTileTextureSize(inverseModelViewProjectionMatrix);
    }

    double mouseX, mouseY;
    glfwGetCursorPos(viewer()->window(), &mouseX, &mouseY);
    vec2 focusPosition = vec2(2.0f * float(mouseX) / float(viewportSize.x) - 1.0f,
                              -2.0f * float(mouseY) / float(viewportSize.y) + 1.0f);

    vec4 projectionInfo(float(-2.0 / (static_cast<float>(viewportSize.x) * projectionMatrix[0][0])),
                        float(-2.0 / (static_cast<float>(viewportSize.y) * projectionMatrix[1][1])),
                        float((1.0 - (double) projectionMatrix[0][2]) / projectionMatrix[0][0]),
                        float((1.0 + (double) projectionMatrix[1][2]) / projectionMatrix[1][1]));

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
    if (m_selected_tile_style == 0 && !m_renderPointCircles)
        pointsRenderPass(vertexCount, modelViewProjectionMatrix);

    // ====================================================================================== DISCREPANCIES RENDER PASS ======================================================================================
    if (m_renderDiscrepancy && tile != nullptr)
        discrepanciesRenderPass();

    // ====================================================================================== POINT CIRCLES  & KDE RENDER PASS =======================================================================================
    if (m_renderPointCircles || m_renderKDE || m_renderTileNormals)
        kdeRenderPass(vertexCount, modelViewProjectionMatrix);

    // ====================================================================================== ACCUMULATE RENDER PASS ======================================================================================
    if (tile != nullptr)
        accumulateRenderPass(vertexCount);

    // ====================================================================================== MAX VAL RENDER PASS ======================================================================================
    maxValRenderPass(modelViewProjectionMatrix, maxBounds, minBounds);

    // ====================================================================================== TILE NORMALS RENDER PASS ======================================================================================
    const bool shouldRenderNormal = tile != nullptr && m_renderTileNormals;
    if (shouldRenderNormal)
        normalRenderPass(modelViewProjectionMatrix, viewportSize);


    // ====================================================================================== TILES RENDER PASS ======================================================================================
    if (tile != nullptr)
        tileRenderPass(modelViewProjectionMatrix, viewportSize, viewLightPosition);

    // ====================================================================================== GRID RENDER PASS ======================================================================================
    if (m_renderGrid && tile != nullptr)
        gridRenderPass(modelViewProjectionMatrix);

    // ====================================================================================== SHADE/BLEND RENDER PASS ======================================================================================
    shadeRenderPass();


    round_robin_fb_index = (round_robin_fb_index + 1) % ROUND_ROBIN_SIZE;
}

void TileRenderer::pointsRenderPass(int vertexCount, const mat4 &modelViewProjectionMatrix) {
    // renders data set as point primitives
    // ONLY USED TO SHOW INITIAL POINTS
    BindGuard _g{m_pointFramebuffer};
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

    // vertex shader
    shaderProgram_points->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

    shaderProgram_points->use();

    m_vao->drawArrays(GL_POINTS, 0, vertexCount);

    Program::release();

    // disable blending for draw buffer 0 (classical scatter plot)
    glDisablei(GL_BLEND, 0);

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void TileRenderer::discrepanciesRenderPass() {
    // write tile discrepancies into texture
    BindGuard _g1{m_tilesDiscrepanciesFramebuffer};

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

    //vertex shader
    shaderProgram_discrepancies->setUniform("numCols", tile->m_tile_cols);
    shaderProgram_discrepancies->setUniform("numRows", tile->m_tile_rows);
    shaderProgram_discrepancies->setUniform("discrepancyDiv", m_discrepancyDiv);

    m_vaoTiles->bind();
    shaderProgram_discrepancies->use();

    m_vaoTiles->drawArrays(GL_POINTS, 0, tile->numTiles);

    Program::release();
    m_vaoTiles->unbind();

    // disable blending
    glDisablei(GL_BLEND, 0);

    //reset Viewport
    glViewport(0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y);

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void TileRenderer::kdeRenderPass(int vertexCount, const mat4 &modelViewProjectionMatrix) {
    // render Point Circles into texture0
    // render Kernel Density Estimation into texture1
    BindGuard _g1{m_pointCircleFramebuffer};
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
    float scaleAdjustedPointCircleRadius = m_pointCircleRadius / pointCircleRadiusDiv * viewer()->scaleFactor();
    float scaleAdjustedKDERadius = m_kdeRadius / pointCircleRadiusDiv * viewer()->scaleFactor();
    float scaleAdjustedRadiusMult = gaussSampleRadiusMult / viewer()->scaleFactor();

    //geometry shader
    shaderProgram_pointCircles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
    shaderProgram_pointCircles->setUniform("aspectRatio", viewer()->m_windowHeight / viewer()->m_windowWidth);

    //geometry & fragment shader
    shaderProgram_pointCircles->setUniform("pointCircleRadius", scaleAdjustedPointCircleRadius);
    shaderProgram_pointCircles->setUniform("kdeRadius", scaleAdjustedKDERadius);

    //fragment shader
    shaderProgram_pointCircles->setUniform("pointColor", viewer()->samplePointColor());
    shaderProgram_pointCircles->setUniform("sigma2", m_sigma);
    shaderProgram_pointCircles->setUniform("radiusMult", scaleAdjustedRadiusMult);
    shaderProgram_pointCircles->setUniform("densityMult", m_densityMult);

    m_vao->bind();
    shaderProgram_pointCircles->use();

    m_vao->drawArrays(GL_POINTS, 0, vertexCount);

    Program::release();
    m_vao->unbind();

    // disable blending for draw buffer 0 (classical scatter plot)
    glDisablei(GL_BLEND, 0);
    glDisablei(GL_BLEND, 1);

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void TileRenderer::accumulateRenderPass(int vertexCount) {
    // Accumulate Points into tiles
    BindGuard _g1{m_tileAccumulateFramebuffer};

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

    // get shader program and uniforms from tile program
    auto shaderProgram_tile_acc = tile->getAccumulationProgram();

    m_vao->bind();
    shaderProgram_tile_acc->use();

    m_vao->drawArrays(GL_POINTS, 0, vertexCount);

    Program::release();
    m_vao->unbind();

    // disable blending
    glDisablei(GL_BLEND, 0);

    //reset Viewport
    glViewport(0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y);

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void TileRenderer::maxValRenderPass(const mat4 &modelViewProjectionMatrix, const vec2 &maxBounds,
                                    const vec2 &minBounds) {
    // Get maximum accumulated value (used for coloring)
    // Get maximum alpha of additive blended point circles (used for alpha normalization)
    // no framebuffer needed, because we don't render anything. we just save the max value into the storage buffer

    // SSBO --------------------------------------------------------------------------------------------------------------------------------------------------
    BindBaseGuard _g{m_valueMaxBuffer, GL_SHADER_STORAGE_BUFFER, 0};

    // max accumulated Value
    const uint initialMaxValue = 0;
    m_valueMaxBuffer->clearSubData(GL_R32UI, 0, sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &initialMaxValue);
    m_valueMaxBuffer->clearSubData(GL_R32UI, sizeof(uint), sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT,
                                   &initialMaxValue);
    // -------------------------------------------------------------------------------------------------

    BindActiveGuard _g2{m_tileAccumulateTexture, 1};
    BindActiveGuard _g3{m_pointCircleTexture, 2};

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
    } else {
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

    shaderProgram_max_val->use();
    m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
    Program::release();

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void TileRenderer::normalRenderPass(const mat4 &modelViewProjectionMatrix, const ivec2 &viewportSize) {
    // accumulate tile normals using KDE texture and save them into tileNormalsBuffer

    // Note: Basically calculates screen-space derivatives / gradient from the kdeTexture and additively combines them in a buffer per fragments hexagon coords
    // Final normal = accumulated_fragment_normal / data_points_within

    // Note: Maybe try additive blending for framebuffer? Don't think it should do much tho, as normal is calculated in fragment space per pixel.
    // render Tile Normals into storage buffer
    if (!m_tileNormalsBuffer) {
        m_tileNormalsBuffer = std::make_shared<Buffer>();
        viewer()->m_sharedResources.tileNormalsBuffer = m_tileNormalsBuffer;
        // we safe each value of the normal (vec4) seperately + accumulated kde height = 5 values
        m_tileNormalsBuffer->setData(static_cast<GLsizei>(sizeof(int) * 5 * tile->m_tile_cols * tile->m_tile_rows),
                                     nullptr, GL_STREAM_DRAW);
    }
    auto state = stateGuard();
    auto& frame_data = m_normal_frame_data.at(round_robin_fb_index);

    frame_data.framebuffer->bind();
    glDepthMask(GL_FALSE);
    // Normal gets converted from [-1,1] -> [0, 1] space, so the zero value is 0.5 (0.5 * 2 - 1 = 0)
    glClearColor(0.5f, 0.5f, 0.5f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // SSBO --------------------------------------------------------------------------------------------------------------------------------------------------
    BindBaseGuard _g{m_tileNormalsBuffer, GL_SHADER_STORAGE_BUFFER, 0};

    // one Pixel of data is enough to clear whole buffer
    // https://www.khronos.org/opengl/wiki/GLAPI/glClearBufferData
    const int initialVal = 0;
    m_tileNormalsBuffer->clearData(GL_R32I, GL_RED_INTEGER, GL_INT, &initialVal);
    // -------------------------------------------------------------------------------------------------

    BindActiveGuard _g2{m_kdeTexture, 1};
    BindActiveGuard _g3{m_tileAccumulateTexture, 2};

    auto shaderProgram_tile_normals = tile->getTileNormalsProgram();

    //geometry shader
    shaderProgram_tile_normals->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

    shaderProgram_tile_normals->setUniform("windowWidth", viewportSize[0]);
    shaderProgram_tile_normals->setUniform("windowHeight", viewportSize[1]);

    //fragment Shader
    shaderProgram_tile_normals->setUniform("maxTexCoordX", tile->m_tileMaxX);
    shaderProgram_tile_normals->setUniform("maxTexCoordY", tile->m_tileMaxY);

    shaderProgram_tile_normals->setUniform("bufferAccumulationFactor", tile->bufferAccumulationFactor);
    shaderProgram_tile_normals->setUniform("tileSize", tile->tileSizeWS);

    shaderProgram_tile_normals->setUniform("kdeTexture", 1);
    shaderProgram_tile_normals->setUniform("accumulateTexture", 2);

    shaderProgram_tile_normals->use();
    m_vaoQuad->drawArrays(GL_POINTS, 0, 1);

    m_normal_frame_data.at(round_robin_fb_index).size = m_framebufferSize;


    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    Framebuffer::unbind();

    frame_data.texture->generateMipmap();
    viewer()->m_sharedResources.smoothNormalsTexture = frame_data.texture;

    // Mark this "round-robin" pass available for the next frame
    if (!frame_data.transfer_buffer) {
        frame_data.transfer_buffer = Buffer::create();
        frame_data.processed = false;
    }
}

void TileRenderer::tileRenderPass(const mat4 &modelViewProjectionMatrix, const ivec2 &viewportSize,
                                  const vec4 &viewLightPosition) {
    // Render tiles
    BindBaseGuard _g{m_tileNormalsBuffer, GL_SHADER_STORAGE_BUFFER, 0}, _g2{m_valueMaxBuffer,
                                                                            GL_SHADER_STORAGE_BUFFER, 1};

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

    BindActiveGuard _g3{m_tileAccumulateTexture, 2};
    BindActiveGuard _g4{m_tilesDiscrepanciesTexture, 3};

    auto shaderProgram_tiles = tile->getTileProgram();

    //geometry shader
    shaderProgram_tiles->setUniform("windowWidth", viewportSize[0]);
    shaderProgram_tiles->setUniform("windowHeight", viewportSize[1]);

    shaderProgram_tiles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

    //geometry & fragment shader
    shaderProgram_tiles->setUniform("tileSize", tile->tileSizeWS);
    shaderProgram_tiles->setUniform("tileColor", viewer()->tileColor());

    //fragment Shader
    shaderProgram_tiles->setUniform("tileHeightMult", m_tileHeightMult);
    shaderProgram_tiles->setUniform("densityMult", m_densityMult);
    shaderProgram_tiles->setUniform("borderWidth", m_borderWidth);
    shaderProgram_tiles->setUniform("invertPyramid", m_invertPyramid);
    shaderProgram_tiles->setUniform("showBorder", m_showBorder);
    shaderProgram_tiles->setUniform("blendRange", blendRange * viewer()->scaleFactor());

    shaderProgram_tiles->setUniform("maxTexCoordX", tile->m_tileMaxX);
    shaderProgram_tiles->setUniform("maxTexCoordY", tile->m_tileMaxY);

    shaderProgram_tiles->setUniform("bufferAccumulationFactor", tile->bufferAccumulationFactor);

    //lighting
    shaderProgram_tiles->setUniform("lightPos", vec3(viewLightPosition));
    shaderProgram_tiles->setUniform("viewPos", vec3(0));
    shaderProgram_tiles->setUniform("lightColor", vec3(1));

    // textures
    shaderProgram_tiles->setUniform("accumulateTexture", 2);
    shaderProgram_tiles->setUniform("tilesDiscrepancyTexture", 3);

    // analytical ambient occlusion
    shaderProgram_tiles->setUniform("aaoScaling", m_aaoScaling);

    // fresnel factor
    shaderProgram_tiles->setUniform("fresnelBias", m_fresnelBias);
    shaderProgram_tiles->setUniform("fresnelPow", m_fresnelPow);

    if (m_colorMapLoaded) {
        m_colorMapTexture->bindActive(5);
        shaderProgram_tiles->setUniform("colorMapTexture", 5);
        shaderProgram_tiles->setUniform("textureWidth", m_ColorMapWidth);
    }

    if (m_tileTexturing) {
        m_tileTextureArray->bindActive(6);
        shaderProgram_tiles->setUniform("tileTextureArray", 6);
    }

    m_vaoQuad->bind();

    shaderProgram_tiles->use();
    m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
    Program::release();

    m_vaoQuad->unbind();

    if (m_tileTexturing) {
        m_tileTextureArray->unbindActive(6);
    }

    if (m_colorMapLoaded) {
        m_colorMapTexture->unbindActive(5);
    }

    // disable blending
    glDisablei(GL_BLEND, 0);

    Framebuffer::unbind();

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void TileRenderer::gridRenderPass(const mat4 &modelViewProjectionMatrix) {
    // render grid into texture
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

    auto shaderProgram_grid = tile->getGridProgram();

    shaderProgram_grid->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
    shaderProgram_grid->setUniform("windowWidth", viewer()->viewportSize()[0]);
    shaderProgram_grid->setUniform("windowHeight", viewer()->viewportSize()[1]);

    //fragment Shader
    shaderProgram_grid->setUniform("gridColor", viewer()->gridColor());
    shaderProgram_grid->setUniform("tileSize", tile->tileSizeWS);
    shaderProgram_grid->setUniform("gridWidth", m_gridWidth * viewer()->scaleFactor());
    shaderProgram_grid->setUniform("accumulateTexture", 1);

    //draw call
    m_vaoTiles->bind();

    shaderProgram_grid->use();
    m_vaoTiles->drawArrays(GL_POINTS, 0, tile->numTiles);
    Program::release();

    m_vaoTiles->unbind();

    m_gridTexture->unbindActive(0);
    m_tileAccumulateTexture->unbindActive(1);

    // disable blending
    glDisablei(GL_BLEND, 0);

    m_gridFramebuffer->unbind();

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void TileRenderer::shadeRenderPass() {
    // blend everything together and draw to screen
    BindGuard _g{m_shadeFramebuffer};

    std::vector<BindActiveGuard<std::unique_ptr<Texture>, unsigned int>> _texGuards{};
    _texGuards.emplace_back(m_pointChartTexture, 0);
    _texGuards.emplace_back(m_tilesTexture, 1);
    _texGuards.emplace_back(m_gridTexture, 3);
    _texGuards.emplace_back(m_pointCircleTexture, 4);
    //debug
    _texGuards.emplace_back(m_kdeTexture, 5);
    _texGuards.emplace_back(m_normalsAndDepthTexture, 2);
    //end debug

    BindBaseGuard _g2{m_valueMaxBuffer, GL_SHADER_STORAGE_BUFFER, 6};

    auto shaderProgram_shade = shaderProgram("shade");

    shaderProgram_shade->setUniform("backgroundColor", viewer()->backgroundColor());
    shaderProgram_shade->setUniform("sobelEdgeColor", viewer()->sobelEdgeColor());


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

    if (m_renderKDE) {
        shaderProgram_shade->setUniform("kdeTexture", 5);
    }

    if (m_renderNormalBuffer || m_renderDepthBuffer) {
        shaderProgram_shade->setUniform("normalsAndDepthTexture", 2);
    }

    m_vaoQuad->bind();

    shaderProgram_shade->use();
    m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
    Program::release();

    m_vaoQuad->unbind();

    m_shadeFramebuffer->blit(GL_COLOR_ATTACHMENT0, {0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y},
                             Framebuffer::defaultFBO().get(), GL_BACK,
                             {0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y},
                             GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

// --------------------------------------------------------------------------------------
// ###########################  TILES ############################################
// --------------------------------------------------------------------------------------
void TileRenderer::calculateTileTextureSize(const mat4 &inverseModelViewProjectionMatrix) {

    // if we only render points, we do not need to calculate tile sizes
    if (tile != nullptr) {

        vec3 maxBounds = viewer()->scene()->table()->maximumBounds();
        vec3 minBounds = viewer()->scene()->table()->minimumBounds();

        tile->tileSizeWS = (inverseModelViewProjectionMatrix * vec4(m_tileSize_tmp / tile->tileSizeDiv, 0, 0, 0)).x;
        tile->tileSizeWS *= viewer()->scaleFactor();

        vec3 boundingBoxSize = maxBounds - minBounds;

        tile->calculateNumberOfTiles(boundingBoxSize, minBounds);

        //set texture size
        m_tilesDiscrepanciesTexture->image2D(0, GL_RGBA32F, ivec2(tile->m_tile_cols, tile->m_tile_rows), 0, GL_RGBA,
                                             GL_UNSIGNED_BYTE, nullptr);
        m_tileAccumulateTexture->image2D(0, GL_RGBA32F, ivec2(tile->m_tile_cols, tile->m_tile_rows), 0, GL_RGBA,
                                         GL_UNSIGNED_BYTE, nullptr);

        //setup grid vertices
        // create m_squareNumCols*m_squareNumRows vertices with position = 0.0f
        // we generate the correct position in the vertex shader
        m_verticesTiles->setData(std::vector<float>(tile->m_tile_cols * tile->m_tile_rows, 0.0f), GL_STATIC_DRAW);

        auto vertexBinding = m_vaoTiles->binding(0);
        vertexBinding->setAttribute(0);
        vertexBinding->setBuffer(m_verticesTiles.get(), 0, sizeof(float));
        vertexBinding->setFormat(1, GL_FLOAT);
        m_vaoTiles->enable(0);

        //calc2D discrepancy and setup discrepancy buffer
        std::vector<float> tilesDiscrepancies(tile->numTiles, 0.0f);

        if (m_renderDiscrepancy) {
            tilesDiscrepancies = calculateDiscrepancy2D(viewer()->scene()->table()->activeXColumn(),
                                                        viewer()->scene()->table()->activeYColumn(),
                                                        viewer()->scene()->table()->maximumBounds(),
                                                        viewer()->scene()->table()->minimumBounds());
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
    if (!ImGui::BeginMenu("Illuminated Point Plots"))
        return;

    if (ImGui::CollapsingHeader("CSV-Files", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto formattedStr = std::format("File: {}", m_currentFileName);
        ImGui::Text("%s", formattedStr.c_str());

        // show all column names from selected CSV file
        ImGui::Text("Selected Columns:");
        ImGui::Combo("X-axis", &m_xAxisDataID, m_guiColumnNames.c_str());
        ImGui::Combo("Y-axis", &m_yAxisDataID, m_guiColumnNames.c_str());

        if (ImGui::Button("Update")) {
            updateData();
        }
    }

    // NB: "Fixing" this bug made it so that the 2D visualization isn't properly updated unless the menu is open
    if (ImGui::CollapsingHeader("Color Maps", ImGuiTreeNodeFlags_DefaultOpen)) {
        // show all available color-maps
        auto colorMapChanged = ImGui::Combo("Maps", &m_colorMap,
                     "None\0Bone\0Cubehelix\0GistEart\0GnuPlot2\0Grey\0Inferno\0Magma\0Plasma\0PuBuGn\0Rainbow\0Summer\0Virdis\0Winter\0Wista\0YlGnBu\0YlOrRd\0");

        // allow the user to load a discrete version of the color map
        colorMapChanged = ImGui::Checkbox("Discrete Colors (7)", &m_discreteMap) || colorMapChanged;

        // allow the user to load a discrete version of the color map
        ImGui::Checkbox("Monochrome-Tiles", &m_renderMomochromeTiles);

        // load new texture if either the texture has changed or the type has changed from discrete to continuous or vice versa
        if (colorMapChanged)
            m_colorMapLoaded = m_colorMap > 0 && updateColorMap();
    }

    if (ImGui::CollapsingHeader("Discrepancy",
                                ImGuiTreeNodeFlags_DefaultOpen)) { /// Stupidest bugfix: Flag was set outside function, making "if" always true...
        ImGui::Checkbox("Render Point Circles", &m_renderPointCircles);
        ImGui::SliderFloat("Point Circle Radius", &m_pointCircleRadius, 1.0f, 100.0f);
        ImGui::Checkbox("Show Discrepancy", &m_renderDiscrepancy_tmp);
        ImGui::SliderFloat("Ease In", &m_discrepancy_easeIn_tmp, 1.0f, 5.0f);
        ImGui::SliderFloat("Low Point Count", &m_discrepancy_lowCount_tmp, 0.0f, 1.0f);
        ImGui::SliderFloat("Discrepancy Divisor", &m_discrepancyDiv, 1.0f, 3.0f);
    }
    if (ImGui::CollapsingHeader("Tiles", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char *tile_styles[]{"none", "square", "hexagon"};
        ImGui::Combo("Tile Rendering", &m_selected_tile_style_tmp, tile_styles, IM_ARRAYSIZE(tile_styles));
        ImGui::Checkbox("Render Grid", &m_renderGrid);
        ImGui::SliderFloat("Grid Width", &m_gridWidth, 1.0f, 3.0f);
        ImGui::SliderFloat("Tile Size", &m_tileSize_tmp, 1.0f, 100.0f);

        // allow for (optional) analytical ambient occlusion of the hex-tiles
        ImGui::Checkbox("Analytical AO", &m_renderAnalyticalAO);
        ImGui::SliderFloat("AAO Scaling", &m_aaoScaling, 1.0f, 16.0f);

        // allow for modification of the fresnel factor calculation
        ImGui::Checkbox("Fresnel Reflectance", &m_renderFresnelReflectance);
        ImGui::SliderFloat("Fresnel Bias", &m_fresnelBias, 0.0f, 1.0f);
        ImGui::SliderFloat("Fresnel Power", &m_fresnelPow, 0.0f, 16.0f);
    }

    if (ImGui::CollapsingHeader("Regression Triangle", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Render KDE", &m_renderKDE);
        ImGui::Checkbox("Render Tile Normals", &m_renderTileNormals);
        ImGui::Checkbox("Smooth Normals", &m_smoothTileNormals);
        ImGui::SliderFloat("Sigma", &m_sigma, 0.1f, 10.0f);
        ImGui::SliderFloat("Sample Radius", &m_kdeRadius, 1.0f, 100.0f);
        ImGui::SliderFloat("Density Multiply", &m_densityMult, 1.0f, 20.0f);
        ImGui::SliderFloat("Tile Height Mult", &m_tileHeightMult, 1.0f, 20.0f);
        // the borderWidth cannot go from 0-1
        // if borderWidth == 1, all inside corner points are at the exact same position (tile center)
        // and we can not longer decide if a point is in the border or not
        // if borderWidth == 0, at least one of the inside corner points is at the exact same position as its corresponding outside corner point
        // then it can happen, that we can no longer compute the lighting normal for the corresponding border.
        ImGui::SliderFloat("Border Width", &m_borderWidth, 0.01f, 0.99f);
        ImGui::Checkbox("Show Border", &m_showBorder);
        ImGui::Checkbox("Sobel Edges", &m_sobelEdgeColoring);
        ImGui::Checkbox("Invert Pyramid", &m_invertPyramid);
        ImGui::Checkbox("Texture Tiles", &m_tileTexturing);
    }

    ImGui::Checkbox("Show Normal Buffer", &m_renderNormalBuffer);
    ImGui::Checkbox("Show Depth Buffer", &m_renderDepthBuffer);
    ImGui::EndMenu();
}

bool TileRenderer::updateColorMap() {
    const static auto colorMapFilenames = std::to_array<std::string>({
        "./dat/colormaps/bone_1D.png",
        "./dat/colormaps/cubehelix_1D.png",
        "./dat/colormaps/gist_earth_1D.png",
        "./dat/colormaps/gnuplot2_1D.png",
        "./dat/colormaps/grey_1D.png",
        "./dat/colormaps/inferno_1D.png",
        "./dat/colormaps/magma_1D.png",
        "./dat/colormaps/plasma_1D.png",
        "./dat/colormaps/PuBuGn_1D.png",
        "./dat/colormaps/rainbow_1D.png",
        "./dat/colormaps/summer_1D.png",
        "./dat/colormaps/virdis_1D.png",
        "./dat/colormaps/winter_1D.png",
        "./dat/colormaps/wista_1D.png",
        "./dat/colormaps/YlGnBu_1D.png",
        "./dat/colormaps/YlOrRd_1D.png"
    });

    uint colorMapWidth, colorMapHeight;
    std::vector<unsigned char> colorMapImage;

    std::string textureName = colorMapFilenames[m_colorMap - 1];

    if (m_discreteMap) {
        // insert the discrete identifier "_discrete7" before the file extension ".png"
        textureName.insert(textureName.length() - 4, "_discrete7");
    }

    const uint error = lodepng::decode(colorMapImage, colorMapWidth, colorMapHeight, textureName);

    if (!error) {
        m_colorMapTexture->image1D(0, GL_RGBA, static_cast<GLsizei>(colorMapWidth), 0, GL_RGBA,
                                   GL_UNSIGNED_BYTE,
                                   (void *) &colorMapImage.front());
        m_colorMapTexture->generateMipmap();

        // store width of texture and mark as loaded
        m_ColorMapWidth = static_cast<GLsizei>(colorMapWidth);
        return true;
    }

    debug() << std::format("Could not load {}! Error code: {}", colorMapFilenames[m_colorMap - 1], error);
    return false;
}


/*
Gathers all #define, reset the defines files and reload the shaders
*/
void TileRenderer::setShaderDefines() {
    std::vector<std::string> defines{};
    const auto conditions = std::to_array<std::pair<bool, const char *>>({
                                                                                 {m_colorMapLoaded,           "COLORMAP"},
                                                                                 {m_renderPointCircles,       "RENDER_POINT_CIRCLES"},
                                                                                 {m_renderDiscrepancy,        "RENDER_DISCREPANCY"},
                                                                                 {m_renderPointCircles,       "RENDER_POINT_CIRCLES"},
                                                                                 {m_selected_tile_style !=
                                                                                  0,                          "RENDER_TILES"},
                                                                                 {m_selected_tile_style ==
                                                                                  2,                          "RENDER_HEXAGONS"},
                                                                                 {m_renderGrid,               "RENDER_GRID"},
                                                                                 {m_renderKDE,                "RENDER_KDE"},
                                                                                 {m_renderTileNormals,        "RENDER_TILE_NORMALS"},
                                                                                 {m_smoothTileNormals,        "SMOOTH_TILE_NORMALS"},
                                                                                 {m_renderNormalBuffer,       "RENDER_NORMAL_BUFFER"},
                                                                                 {m_renderDepthBuffer,        "RENDER_DEPTH_BUFFER"},
                                                                                 {m_renderAnalyticalAO,       "RENDER_ANALYTICAL_AO"},
                                                                                 {m_renderMomochromeTiles,    "RENDER_MONOCHROME_TILES"},
                                                                                 {m_renderFresnelReflectance, "RENDER_FRESNEL_REFLECTANCE"},
                                                                                 {m_sobelEdgeColoring,        "RENDER_SOBEL_EDGE_COLORING"},
                                                                                 {m_tileTexturing,            "RENDER_TEXTURED_TILES"}

                                                                         });
    std::stringstream ss;
    for (const auto&[cond, val]: conditions) {
        if (cond)
            ss << std::format("#define {}\n", val);
    }

    if (ss.str() != m_shaderSourceDefines->string()) {
        m_shaderSourceDefines->setString(ss.str());

        reloadShaders();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//INTERVAL MAPPING
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//maps value x from [a,b] --> [0,c]
float molumes::TileRenderer::mapInterval(float x, float a, float b, int c) {
    return (x - a) * static_cast<float>(c) / (b - a);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DISCREPANCY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//PRECONDITION: tile != nullptr
std::vector<float>
TileRenderer::calculateDiscrepancy2D(const std::vector<float> &samplesX, const std::vector<float> &samplesY,
                                     vec3 maxBounds, vec3 minBounds) {

    // Calculates the discrepancy of this data.
    // Assumes the data is [0,1) for valid sample range.
    // Assmues samplesX.size() == samplesY.size()
    std::size_t numSamples = samplesX.size();

    std::vector<float> sortedX(numSamples, 0.0f);
    std::vector<float> sortedY(numSamples, 0.0f);

    std::vector<float> pointsInTilesCount(tile->numTiles, 0.0f);
    std::vector<float> tilesMaxBoundX(tile->numTiles, -INFINITY);
    std::vector<float> tilesMaxBoundY(tile->numTiles, -INFINITY);
    std::vector<float> tilesMinBoundX(tile->numTiles, INFINITY);
    std::vector<float> tilesMinBoundY(tile->numTiles, INFINITY);

    std::vector<float> discrepancies(tile->numTiles, 0.0f);

    float eps = 0.05f;

    //std::cout << "Discrepancy: " << numSamples << " Samples; " << numSquares << " Tiles" << '\n';

    //timing
    std::clock_t start;
    double duration;

    start = std::clock();

    //Step 1: Count how many elements belong to each tile
    for (std::size_t i = 0; i < numSamples; i++) {

        float sampleX = samplesX[i];
        float sampleY = samplesY[i];

        pointsInTilesCount[tile->mapPointToTile1D(vec2(sampleX, sampleY))]++;
    }

    duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
    //std::cout << "Step1: " << duration << '\n';

    start = clock();

    //Step 2: calc prefix Sum
    std::vector<float> pointsInTilesPrefixSum(pointsInTilesCount);
    int prefixSum = 0;
    int sampleCount = 0;
    int maxSampleCount = 0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(tile->numTiles); i++) {
        sampleCount = static_cast<int>(pointsInTilesPrefixSum[i]);
        pointsInTilesPrefixSum[i] = static_cast<float>(prefixSum);
        prefixSum += sampleCount;
        maxSampleCount = max(maxSampleCount, sampleCount);
    }

    duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
    //std::cout << "Step2: " << duration << '\n';

    start = clock();

    //Step 3: sort points according to tile
    // 1D array with "buckets" according to prefix Sum of tile
    // also set the bounding box of points inside tile
    std::vector<float> pointsInTilesRunningPrefixSum(pointsInTilesPrefixSum);
    for (std::size_t i = 0; i < numSamples; i++) {

        float sampleX = samplesX[i];
        float sampleY = samplesY[i];

        int squareIndex1D = tile->mapPointToTile1D(vec2(sampleX, sampleY));

        // put sample in correct position and increment prefix sum
        int sampleIndex = static_cast<int>(pointsInTilesRunningPrefixSum[squareIndex1D]++);
        sortedX[sampleIndex] = sampleX;
        sortedY[sampleIndex] = sampleY;

        //set bounding box of samples inside tile
        tilesMaxBoundX[squareIndex1D] = max(tilesMaxBoundX[squareIndex1D], sampleX);
        tilesMaxBoundY[squareIndex1D] = max(tilesMaxBoundY[squareIndex1D], sampleY);

        tilesMinBoundX[squareIndex1D] = min(tilesMinBoundX[squareIndex1D], sampleX);
        tilesMinBoundY[squareIndex1D] = min(tilesMinBoundY[squareIndex1D], sampleY);
    }

    duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
    //std::cout << "Step3: " << duration << '\n';

    start = clock();

    //Step 4: calculate discrepancy for each tile
    // this is done using a parallel for loop with the OpenMP Framework - need to compile with OpenMP support (add in CMakeList)
    // the computations of the discrpeancy of each seperate tile is independent
    // the number of threads used can be set to how many cores the respective PC has
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
            const int startPoint = static_cast<int>(pointsInTilesPrefixSum[i]);
            const int stopPoint = static_cast<int>(pointsInTilesPrefixSum[i] + pointsInTilesCount[i]);

            //use the addition and not PrefixSum[i+1] so we can easily get the last boundary
            for (int j = startPoint; j < stopPoint; j++) {
                //normalize sample inside its tile
                float stopValueXNorm = mapInterval(sortedX[j], tilesMinBoundX[i], tilesMaxBoundX[i], 1);
                float stopValueYNorm = mapInterval(sortedY[j], tilesMinBoundY[i], tilesMaxBoundY[i], 1);

                // calculate area
                float area = stopValueXNorm * stopValueYNorm;

                // closed interval [startValue, stopValue]
                float countInside = 0.0f;
                for (int k = startPoint; k < stopPoint; k++) {
                    //normalize sample inside its square
                    float sampleXNorm = mapInterval(sortedX[k], tilesMinBoundX[i], tilesMaxBoundX[i], 1);
                    float sampleYNorm = mapInterval(sortedY[k], tilesMinBoundY[i], tilesMaxBoundY[i], 1);


                    if (sampleXNorm <= stopValueXNorm &&
                        sampleYNorm <= stopValueYNorm) {
                        countInside++;
                    }
                }
                float density = countInside / float(pointsInTilesCount[i]);
                float difference = std::abs(density - area);

                if (difference > maxDifference) {
                    maxDifference = difference;
                }
            }

            maxDifference = max(eps, maxDifference);

            // Ease In
            maxDifference = pow(maxDifference, m_discrepancy_easeIn);

            // account for tiles with few points
            //maxDifference = (1 - m_discrepancy_lowCount) * maxDifference + m_discrepancy_lowCount * (1 - pow((pointsInTilesCount[i] / float(maxSampleCount)), 2));
            maxDifference = min(maxDifference + m_discrepancy_lowCount *
                                                (1 - pow((pointsInTilesCount[i] / float(maxSampleCount)), 2.f)), 1.0f);


            discrepancies[i] = maxDifference;
        }
    }

    duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
    //std::cout << "Step4: " << duration << '\n' << '\n';

    return discrepancies;
}

void TileRenderer::fileLoaded(const std::string &filename) {
    // reset column names
    m_guiColumnNames = "None";
    m_guiColumnNames += '\0'; /// Trailing null-terminators in string literals apparently gets truncated by STL

    // extract column names and prepare GUI
    std::vector<std::string> tempNames = viewer()->scene()->table()->getColumnNames();

    for (const auto &tempName: tempNames)
        m_guiColumnNames += tempName + '\0';

    m_currentFileName = filename;

    // provide default selections assuming
    m_xAxisDataID = 1;        // contains the X-values
    m_yAxisDataID = 2;        // contains the Y-values
    m_radiusDataID = 3;        // contains the radii
    m_colorDataID = 4;        // contains the colors

    updateData();
}

void TileRenderer::updateData() {
    // update buffers according to recent changes -> since combo also contains 'None" we need to subtract 1 from ID
    viewer()->scene()->table()->updateBuffers(m_xAxisDataID - 1, m_yAxisDataID - 1, m_radiusDataID - 1,
                                              m_colorDataID - 1);

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
    vec3 boundingBoxSize =
            viewer()->scene()->table()->maximumBounds() - viewer()->scene()->table()->minimumBounds();
    float maximumSize = std::max({boundingBoxSize.x, boundingBoxSize.y, boundingBoxSize.z});
    mat4 modelTransform = scale(vec3(2.0f) / vec3(maximumSize));
    modelTransform = modelTransform * translate(-0.5f * (viewer()->scene()->table()->minimumBounds() +
                                                         viewer()->scene()->table()->maximumBounds()));
    viewer()->setModelTransform(modelTransform);

    // store diameter of current scatter plot and initialize light position
    viewer()->m_scatterPlotDiameter = sqrt(pow(boundingBoxSize.x, 2.f) + pow(boundingBoxSize.y, 2.f));

    // initial position of the light source (azimuth 120 degrees, elevation 45 degrees, 5 times the distance to the object in center) ---------------------------------------------------------
    glm::mat4 viewTransform = viewer()->viewTransform();
    glm::vec3 initLightDir = normalize(
            glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f)) *
            glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    glm::mat4 newLightTransform = glm::inverse(viewTransform) * glm::translate(mat4(1.0f), (5 *
                                                                                            viewer()->m_scatterPlotDiameter *
                                                                                            initLightDir)) *
                                  viewTransform;
    viewer()->setLightTransform(newLightTransform);
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    // calculate accumulate texture settings - needs to be last step here ------------------------------
    calculateTileTextureSize(inverse(viewer()->modelViewProjectionTransform()));
    // -------------------------------------------------------------------------------

    m_tileNormalsBuffer.reset();
}

bool TileRenderer::offscreen_render() {
    Renderer::offscreen_render();

    // Copy framebuffer from last frame:

    const auto round_robin_last_index = get_index_offset(round_robin_fb_index, -1);
    // Should be safe to read without synchronization, because it hasn't been written to this frame:
    auto &frame_data = m_normal_frame_data.at(round_robin_last_index);
    // If there's no transfer buffer, we've already completed the work earlier. Mark as done
    if (!frame_data.transfer_buffer)
        return true;
    else if (!frame_data.processed) {
        BindTargetGuard _g{frame_data.transfer_buffer, GL_PIXEL_PACK_BUFFER};
        // Assign buffer here on runtime before render so buffer can be orphaned
        frame_data.transfer_buffer->setData(
                frame_data.size.x * frame_data.size.y * sizeof(vec4), nullptr, GL_STATIC_READ);
        BindActiveGuard _g2{frame_data.texture, 0};
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, nullptr);

        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        frame_data.pass_sync = Sync::fence(GL_SYNC_GPU_COMMANDS_COMPLETE);
        frame_data.processed = true;
    }


    // ====================================================================================== CPGPU Transfer ======================================================================================
    // After everything else is done, GPU is probably done with earlier tasks and is probably ready to transfer data over
    // But just to be sure, read from the last frame using a round-robin approach
    static constexpr auto MAX_SYNC_TIME = static_cast<GLuint64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::microseconds{10}).count());

    if (frame_data.pass_sync) {
        const auto sync_result = frame_data.pass_sync->clientWait(GL_SYNC_FLUSH_COMMANDS_BIT, MAX_SYNC_TIME);
        if (sync_result == GL_CONDITION_SATISFIED || sync_result == GL_ALREADY_SIGNALED) {
            frame_data.transfer_buffer->bind(GL_PIXEL_PACK_BUFFER);
            const auto vCount = frame_data.size.x * frame_data.size.y;
            const auto memPtr = reinterpret_cast<glm::vec4 *>(frame_data.transfer_buffer->mapRange(0, vCount *
                                                                                                      static_cast<GLsizeiptr>(sizeof(glm::vec4)),
                                                                                                   GL_MAP_READ_BIT));
            std::vector<glm::vec4> data{};
            if (memPtr != nullptr) {
                data = {memPtr, memPtr + vCount};
            }
            if (!frame_data.transfer_buffer->unmap())
                throw std::runtime_error{"Failed to unmap GPU buffer! (m_normal_transfer_buffer)"};
            frame_data.transfer_buffer->unbind(GL_PIXEL_PACK_BUFFER);

            m_tile_normal_async_task = std::async(std::launch::async, [&channel = this->m_normal_tex_channel, size = frame_data.size, data = std::move(data)](){
                channel.write(HapticInteractor::generateMipmaps(glm::uvec2{size}, data));
            });

            // Finish by releasing buffers:
            frame_data.transfer_buffer = {};
            frame_data.pass_sync = {};

            // We've finished all our work, mark as completed:
            return true;
        }
    }

    return false;
}
