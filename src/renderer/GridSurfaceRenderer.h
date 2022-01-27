#ifndef MOLUMES_GRIDSURFACERENDERER_H
#define MOLUMES_GRIDSURFACERENDERER_H

#include "Renderer.h"

namespace molumes {
class GridSurfaceRenderer : public Renderer {
public:
    explicit GridSurfaceRenderer(Viewer *viewer);
//    void setEnabled(bool enabled) override;

    void display() override;
};
}


#endif //MOLUMES_GRIDSURFACERENDERER_H
