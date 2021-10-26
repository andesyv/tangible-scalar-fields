#ifndef MOLUMES_STLEXPORTER_H
#define MOLUMES_STLEXPORTER_H

#include "Interactor.h"

#include <fstream>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace molumes {
class CrystalRenderer;

class STLExporter : public Interactor{
public:
    static constexpr auto defaultFileName = "model.stl";
    static constexpr auto defaultModelName = "crystal";

    explicit STLExporter(Viewer * viewer, CrystalRenderer* crystalRenderer = nullptr);

    void keyEvent(int key, int scancode, int action, int mods) override;
    void display() override;

    void exportFile();

private:
    bool m_ctrl = false;
    CrystalRenderer* m_renderer{nullptr};

    void exportAscii(std::ofstream&& ofs, const std::string& modelName = defaultModelName);
    void exportBinary(std::ofstream&& ofs);

    struct Plane {
        glm::vec3 normal;
        glm::vec3 v1, v2, v3;
    };

    static std::vector<glm::vec3> calculateNormals(const std::vector<glm::vec4>& vertices);
    static std::vector<Plane> zipNormalsAndVertices(const std::vector<glm::vec4>& vertices);
};
}


#endif //MOLUMES_STLEXPORTER_H
