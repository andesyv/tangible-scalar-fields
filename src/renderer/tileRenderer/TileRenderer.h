#pragma once

#include <future>

#include "../Renderer.h"
#include "../../Channel.h"
#include "../../Constants.h"

#include <glm/glm.hpp>

namespace globjects {
    class StaticStringSource;

    class VertexArray;

    class Buffer;

    class NamedString;

    class Framebuffer;

    class Sync;
}

namespace molumes {
    class Viewer;

    class Tile;

    class TileRenderer : public Renderer {
    public:
        TileRenderer();
        explicit TileRenderer(Viewer *viewer,
                              WriterChannel<std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, HapticMipMapLevels>> &&normal_channel);

        void setEnabled(bool enabled) override;

        void display() override;

        bool offscreen_render() override;

        void fileLoaded(const std::string &filename) override;

        static constexpr std::size_t ROUND_ROBIN_SIZE = 3;
    private:
        Tile *tile = nullptr;
        std::unordered_map<std::string, std::shared_ptr<Tile>> tile_processors;

        // INITIAL POINT DATA ----------------------------------------------------------------------
        std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();
        std::unique_ptr<globjects::Buffer> m_xColumnBuffer = std::make_unique<globjects::Buffer>();
        std::unique_ptr<globjects::Buffer> m_yColumnBuffer = std::make_unique<globjects::Buffer>();
        std::unique_ptr<globjects::Buffer> m_radiusColumnBuffer = std::make_unique<globjects::Buffer>();
        std::unique_ptr<globjects::Buffer> m_colorColumnBuffer = std::make_unique<globjects::Buffer>();


        // TILES GRID VERTEX DATA------------------------------------------------------------
        std::unique_ptr<globjects::VertexArray> m_vaoTiles = std::make_unique<globjects::VertexArray>();
        std::unique_ptr<globjects::Buffer> m_verticesTiles = std::make_unique<globjects::Buffer>();
        std::unique_ptr<globjects::Buffer> m_discrepanciesBuffer = std::make_unique<globjects::Buffer>();


        // QUAD VERTEX DATA -------------------------------------------------------------------------------
        std::unique_ptr<globjects::VertexArray> m_vaoQuad = std::make_unique<globjects::VertexArray>();
        std::unique_ptr<globjects::Buffer> m_verticesQuad = std::make_unique<globjects::Buffer>();


        // SHADER GLOBALS / DEFINES -------------------------------------------------------------------------------
        std::unique_ptr<globjects::StaticStringSource> m_shaderSourceDefines = nullptr;
        std::unique_ptr<globjects::NamedString> m_shaderDefines = nullptr;

        std::unique_ptr<globjects::File> m_shaderSourceGlobals = nullptr;
        std::unique_ptr<globjects::NamedString> m_shaderGlobals = nullptr;

        std::unique_ptr<globjects::File> m_shaderSourceGlobalsHexagon = nullptr;
        std::unique_ptr<globjects::NamedString> m_shaderGlobalsHexagon = nullptr;

        // TEXTURES -------------------------------------------------------------------------
        std::unique_ptr<globjects::Texture> m_depthTexture = nullptr;
        std::unique_ptr<globjects::Texture> m_colorTexture = nullptr;

        std::unique_ptr<globjects::Texture> m_pointChartTexture = nullptr;
        std::unique_ptr<globjects::Texture> m_pointCircleTexture = nullptr;
        std::unique_ptr<globjects::Texture> m_tilesDiscrepanciesTexture = nullptr;
        std::shared_ptr<globjects::Texture> m_tileAccumulateTexture = nullptr;
        std::unique_ptr<globjects::Texture> m_tilesTexture = nullptr;
        std::unique_ptr<globjects::Texture> m_normalsAndDepthTexture = nullptr;
        std::unique_ptr<globjects::Texture> m_gridTexture = nullptr;
        std::unique_ptr<globjects::Texture> m_kdeTexture = nullptr;

        int m_ColorMapWidth = 0;
        std::unique_ptr<globjects::Texture> m_colorMapTexture = nullptr;

        std::unique_ptr<globjects::Texture> m_tileTextureArray = nullptr;

        //---------------------------------------------------------------------------------------

        //SSBO
        std::shared_ptr<globjects::Buffer> m_valueMaxBuffer;
        std::shared_ptr<globjects::Buffer> m_tileNormalsBuffer;

        // FRAMEBUFFER -------------------------------------------------------------------------
        std::unique_ptr<globjects::Framebuffer> m_pointFramebuffer = nullptr;
        std::unique_ptr<globjects::Framebuffer> m_pointCircleFramebuffer = nullptr;
        std::unique_ptr<globjects::Framebuffer> m_hexFramebuffer = nullptr;
        std::unique_ptr<globjects::Framebuffer> m_tilesDiscrepanciesFramebuffer = nullptr;
        std::unique_ptr<globjects::Framebuffer> m_tileAccumulateFramebuffer = nullptr;
        std::unique_ptr<globjects::Framebuffer> m_tilesFramebuffer = nullptr;
        std::unique_ptr<globjects::Framebuffer> m_gridFramebuffer = nullptr;
        std::unique_ptr<globjects::Framebuffer> m_shadeFramebuffer = nullptr;

        glm::ivec2 m_framebufferSize{};


        // LIGHTING -----------------------------------------------------------------------
        glm::vec3 ambientMaterial{};
        glm::vec3 diffuseMaterial{};
        glm::vec3 specularMaterial{};
        float shininess{};

        // RENDER PASSES --------------------------------------------------------------------
        void pointsRenderPass(int vertexCount, const glm::mat4 &modelViewProjectionMatrix);

        void discrepanciesRenderPass();

        void kdeRenderPass(int vertexCount, const glm::mat4 &modelViewProjectionMatrix);

        void accumulateRenderPass(int vertexCount);

        void maxValRenderPass(const glm::mat4 &modelViewProjectionMatrix, const glm::vec2 &maxBounds,
                              const glm::vec2 &minBounds);

        void normalRenderPass(const glm::mat4 &modelViewProjectionMatrix, const glm::ivec2 &viewportSize);

        void tileRenderPass(const glm::mat4 &modelViewProjectionMatrix, const glm::ivec2 &viewportSize,
                            const glm::vec4 &viewLightPosition);

        void gridRenderPass(const glm::mat4 &modelViewProjectionMatrix);

        void shadeRenderPass();


        // TILES CALC----------------------------------------------------------------
        void calculateTileTextureSize(const glm::mat4 &inverseModelViewProjectionMatrix);


        // GUI ----------------------------------------------------------------------------

        void renderGUI();

        void setShaderDefines();

        void updateData();

        // items for ImGui Combo
        std::string m_currentFileName = "None";

        std::string m_guiColumnNames = "None";

        // store combo ID of selected file
        int m_fileDataID = 0;
        int m_oldFileDataID = 0;

        // store combo ID of selected columns
        int m_xAxisDataID = 0, m_yAxisDataID = 0, m_radiusDataID = 0, m_colorDataID = 0;

        // selection of color maps
        int m_colorMap = 12;                        // use "virdis" as default heatmap
        bool m_colorMapLoaded = false;
        bool m_discreteMap = false;
        bool m_normals_parameters_changed = true;
        // ---------------------------------

        float m_aaoScaling = 1.0f;

        // Tiles Parameters
        // [none=0, square=1, hexagon=2]
        int m_selected_tile_style = -1;
        int m_selected_tile_style_tmp = 2;

        // tileSize adjustable by user
        float m_tileSize = 20.0f;
        float m_tileSize_tmp = m_tileSize;
        // gridWidth adjustable by user
        float m_gridWidth = 1.5f;

        //point circle parameters
        // circle radius adjustable by user
        float m_pointCircleRadius = 6.5f;
        // kde radius adjustable by user
        float m_kdeRadius = 70.0f;
        // divisor of radius that is set by the user when calculation radius in WorldSpace
        const float pointCircleRadiusDiv = 5000.0f;

        //discrepancy parameters
        // divisor of discrpenacy values adjustable by user
        float m_discrepancyDiv = 1.0f;
        // ease function modyfier adjustable by user
        float m_discrepancy_easeIn = 1.0f;
        float m_discrepancy_easeIn_tmp = m_discrepancy_easeIn;
        // adjustment rate for tiles with few points adjustable by user
        float m_discrepancy_lowCount = 0.0f;
        float m_discrepancy_lowCount_tmp = m_discrepancy_lowCount;

        //regression parameters
        // sigma of gauss kernel for KDE adjustable by user
        float m_sigma = 2.6f;
        // divisor of radius that is set by the user when calculation radius in WorldSpace
        const float gaussSampleRadiusMult = 400.0f;
        // multiplyer for KDE height field adjustable by user
        float m_densityMult = 13.0f;
        // multiplyer for pyramid height adjustable by user
        float m_tileHeightMult = 10.0f;
        // defines how much of pyramid border is shown = moves regression plane up and down inside pyramid adjustable by user
        float m_borderWidth = 0.2f; // width in percent to size
        // render pyramid yes,no adjustable by user
        bool m_showBorder = true;
        // invert pyramid adjustable by user
        bool m_invertPyramid = false;
        // blend range for anti-aliasing of pyramid edges
        float blendRange = 1.0f;

        // GUI parameters for Fresnel factor computation (empirically chosen)
        float m_fresnelBias = 0.1f;
        float m_fresnelPow = 0.3f;

        // textured tiles default values according to google maps thumbnails
        glm::uint m_tileTextureWidth = 96;
        glm::uint m_tileTextureHeight = 64;
        int m_numberTextureTiles = 360;            // total number of tile we want to load

        // define booleans
        bool m_renderPointCircles = true;
        bool m_renderDiscrepancy = false;
        bool m_renderDiscrepancy_tmp = m_renderDiscrepancy;
        bool m_renderGrid = false;
        bool m_renderKDE = false;
        bool m_renderTileNormals = true;
        bool m_smoothTileNormals = true;
        bool m_renderNormalBuffer = false;
        bool m_renderDepthBuffer = false;
        bool m_renderAnalyticalAO = true;
        bool m_renderMomochromeTiles = false;
        bool m_renderFresnelReflectance = true;
        bool m_sobelEdgeColoring = false;
        bool m_tileTexturing = false;

        // ------------------------------------------------------------------------------------------

        // INTERVAL MAPPING--------------------------------------------------------------------------
        //maps value x from [a,b] --> [0,c]
        static float mapInterval(float x, float a, float b, int c);

        // DISCREPANCY------------------------------------------------------------------------------

        std::vector<float>
        calculateDiscrepancy2D(const std::vector<float> &samplesX, const std::vector<float> &samplesY,
                               glm::vec3 maxBounds, glm::vec3 minBounds);


        using NormalTexType = std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, HapticMipMapLevels>;
        struct NormalFrameData {
            std::shared_ptr<globjects::Texture> texture{};
            std::unique_ptr<globjects::Buffer> transfer_buffer{};
            std::unique_ptr<globjects::Framebuffer> framebuffer{};
            glm::ivec2 size;
            std::unique_ptr<globjects::Sync> pass_sync{};
            std::future<NormalTexType> tile_normal_async_task{};
            unsigned int step = 3;
        };
        std::array<NormalFrameData, ROUND_ROBIN_SIZE> m_normal_frame_data{};
        unsigned int round_robin_fb_index = 0;

    public:

        WriterChannel<NormalTexType> m_normal_tex_channel;

        bool updateColorMap();
        bool m_debug_heightmap{false};
    };

}

