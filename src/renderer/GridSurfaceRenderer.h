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
    };
}


#endif //MOLUMES_GRIDSURFACERENDERER_H
