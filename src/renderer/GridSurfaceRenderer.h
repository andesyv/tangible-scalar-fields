#ifndef MOLUMES_GRIDSURFACERENDERER_H
#define MOLUMES_GRIDSURFACERENDERER_H

#include <memory>
#include <vector>

#include "Renderer.h"

namespace globjects {
    class VertexArray;

    class Buffer;
}

namespace molumes {
    class GridSurfaceRenderer : public Renderer {
    public:
        explicit GridSurfaceRenderer(Viewer *viewer);
//    void setEnabled(bool enabled) override;

        void display() override;

    private:
        std::unique_ptr<globjects::Buffer> m_planeBounds, m_screenSpacedBuffer;
        std::unique_ptr<globjects::VertexArray> m_planeVAO, m_screenSpacedVAO;
        unsigned int m_mip_map_level{0};
        float m_height_multiplier{1.f};
    };
}


#endif //MOLUMES_GRIDSURFACERENDERER_H
