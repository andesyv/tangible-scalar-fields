#ifndef MOLUMES_HAPTICRENDERER_H
#define MOLUMES_HAPTICRENDERER_H

#include "Renderer.h"

namespace globjects{
    class VertexArray;
    class Buffer;
}

namespace molumes {
    /**
     * Utility class that displays a glyph representing the haptic device
     */
    class HapticRenderer : public Renderer {
    private:
        std::unique_ptr<globjects::VertexArray> m_vao;
        std::unique_ptr<globjects::Buffer> m_point_buffer;
        float m_radius = 0.03f;

    public:
        explicit HapticRenderer(Viewer* viewer);
        void display() override;
    };
}


#endif //MOLUMES_HAPTICRENDERER_H
