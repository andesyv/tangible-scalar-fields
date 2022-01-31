#pragma once

#include <memory>
#include <future>
#include <optional>
#include <tuple>

#include <glm/mat4x4.hpp>

#include "Renderer.h"
#include "../WorkerThread.h"
#include "../GeometryUtils.h"

namespace globjects {
    class VertexArray;

    class Buffer;

    class Sync;
}

namespace molumes {
    class Viewer;

    /**
     * Renderer class for visualizing 3D printed objects before exporting as STL.
     * Also responsible for geometry calculation from 2D visualization data pipeline.
     *
     * @brief 3D-Printing renderer
     */
    class CrystalRenderer : public Renderer {
    private:
        /// ------------------- GUI Parameters -------------------------------------------
        bool m_wireframe = false;
        bool m_renderHull = true;
        bool m_tileNormalsEnabled = false;
        bool m_orientationNotchEnabled = false;
        int m_renderStyleOption = 0;
        int m_topRegressionPlaneAlignment{1}, m_bottomRegressionPlaneAlignment{1};
        int m_geometryMode = 0;
        float m_tileScale = 1.0f;
        float m_tileHeight = 0.5f;
        float m_tileNormalsFactor = 1.0;
        float m_extrusionFactor = 0.2f;
        float m_valueThreshold = 0.f;
        float m_cutValue = 0.5f;
        float m_cutWidth = 0.1f;
        float m_orientationNotchDepth = 1.f;
        float m_orientationNotchHeightAdjust = 0.f;
        float m_orientationNotchAngle = 45.f;

    public:
        explicit CrystalRenderer(Viewer *viewer);

        void setEnabled(bool enabled) override;

        void display() override;

        auto getVertices() const { return m_vertices; }

        void fileLoaded(const std::string &) override;

#ifndef NDEBUG

        // Override to rebuild geometry on shader reloading (only enabled in debug builds)
        void reloadShaders() override;

#endif

    private:
        /**
         * Geometry type enum to use in local functions and shader invocations.
         * Different geometry types are handled a bit differently throughout the C++ and GLSL code. For instance
         * when geometry is mirrored, the geometry buffers needs to be twice the size compared to buffers for normal
         * geometry.
         * @brief Local geometry type enum
         */
        enum GeometryMode : int {
            Normal = 0, /**<
                         * Geometry is extruded "upwards" from flat surface / baseline
                         */
            Mirror, /**<
                     * Geometry is directly mirrored across the bottom surface / baseline
                     */
            Cut, /**<
                  * Baseline is customizable. Geometry below flat surface / baseline is inverted and otherwise
                  * clamped.
                  */
            Concave /**<
                     * Geometry below flat surface / baseline is negated such that high points are low and vice versa.
                     * Baseline is extruded the minimal amount to ensure it doesn't self-intersect.
                     */
        };

        /// ------------------- Local variables -------------------------------------------
        std::unique_ptr<globjects::VertexArray> m_vao;
        // m_vertexBuffer is either m_computeBuffer or a smaller separate buffer subset
        // Using shared_ptr to check if shared resource is same
        std::shared_ptr<globjects::Buffer> m_computeBuffer, m_computeBuffer2, m_vertexBuffer;
        std::unique_ptr<globjects::Buffer> m_hullBuffer;
        std::unique_ptr<globjects::Buffer> m_maxValDiff;

        using WorkerThreadT = decltype(worker_manager_from_functions(getHexagonConvexHull, geometryPostProcessing));
        WorkerThreadT::ResultTypes m_workerResults;
        WorkerThreadT m_worker{};
        // Seems weird to have a pointer to a bool, but I'm only using this as a signal to the worker thread because
        // weak_ptr::expired() is thread-safe.
        std::shared_ptr<bool> m_workerControlFlag;

        std::vector<glm::vec4> m_vertices;
        std::vector<unsigned int> m_vertexHull;
        glm::mat4 m_modelMatrix{1.f};
        gl::GLsizei m_drawingCount = 0;
        gl::GLsizei m_hullSize = 0;
        bool m_hexagonsUpdated = true;
        bool m_hexagonsSecondPartUpdated = false;

        void drawGUI(bool &bufferNeedsResize);

        /**
         * Runs the first compute shader which generates the base geometry
         * @return A OpenGL sync object referring to the geometry pass shader invocation
         */
        [[nodiscard]] std::unique_ptr<globjects::Sync>
        generateBaseGeometry(std::shared_ptr<globjects::Texture> &&accumulateTexture,
                             std::shared_ptr<globjects::Buffer> &&accumulateMax,
                             const std::weak_ptr<globjects::Buffer> &tileNormalsRef, int tile_max_y, int count,
                             int num_cols, int num_rows, float tile_scale, glm::mat4 disp_mat);

        /**
         * Runs the second and third compute shader which respectively: culls and extrudes the geometry, and scales and translates the geometry.
         * @return A OpenGL sync object referring to the transformation pass shader invocation
         */
        [[nodiscard]] std::unique_ptr<globjects::Sync>
        cullAndExtrude(std::shared_ptr<globjects::Texture> &&accumulateTexture,
                       const std::weak_ptr<globjects::Buffer> &tileNormalsRef,
                       int tile_max_y, int count, int num_cols, int num_rows, float tile_scale, glm::mat4 disp_mat);

        void resizeVertexBuffer(int hexCount);

        auto getGeometryMode() const { return static_cast<GeometryMode>(m_geometryMode); }

        /// Returns the vertex count to be drawn to screen
        static auto getDrawingCount(int count) {
            return count * 6 * 2 * 3;
        } // count * hex_size * (inner+outer triangle) * triangle_size

        /// Returns the vertex count of the geometry (same as getDrawingCount() but multiplied by 2 if geometry type != normal)
        auto getVertexCountMainGeometry(int count) const {
            return getDrawingCount(count) * (getGeometryMode() != Normal ? 2 : 1);
        }

        /// Same as getVertexCountMainGeometry() but multiplied by 3 to make room for extrusions + extra geometry
        auto getVertexCountFull(int count) const {
            return getVertexCountMainGeometry(count) * 3;
        }

        /// Returns the size in bytes required for the geometry
        auto getBufferSize(int count) const {
            return static_cast<gl::GLsizeiptr>(getVertexCountFull(count) * sizeof(glm::vec4));
        }

        /**
         * Calculates the matrix that converts from local geometry space (from geometry shader) to world space
         * @param mirrorFlip - Whether the local space is flipped or not
         * @return The matrix that transforms from local geometry space (from geometry shader) to world space
         */
        glm::mat4 getModelMatrix(bool mirrorFlip = false) const;
    };
}