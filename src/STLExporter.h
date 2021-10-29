#ifndef MOLUMES_STLEXPORTER_H
#define MOLUMES_STLEXPORTER_H

#include "Interactor.h"

#include <fstream>
#include <vector>
#include <array>

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace molumes {
class CrystalRenderer;

/**
 * @brief Custom STL Exporter utility class
 * Made with file specs from Wikipedia https://en.wikipedia.org/wiki/STL_(file_format)
 */
class STLExporter : public Interactor{
private: // Type data:
    struct Plane {
        glm::vec3 normal;
        glm::vec3 v1, v2, v3;
    };

    struct BinaryTriangleStruct {
        std::array<glm::float32_t, 3> normal{};
        std::array<glm::float32_t, 3> vertex1{};
        std::array<glm::float32_t, 3> vertex2{};
        std::array<glm::float32_t, 3> vertex3{};
        uint16_t attributeByteCount = 0;
    };

    static constexpr auto binaryHeaderSize = 80u;
    static constexpr auto boundingSize = 100.f; // Arbitrary scale number. 100 worked fine in PrusaSlicer

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

    static std::vector<glm::vec3> normalizeRange(const std::vector<glm::vec4>& vertices, float size = boundingSize);
    static std::vector<glm::vec3> calculateNormals(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices);
    static std::vector<Plane> zipNormalsAndVertices(const std::vector<glm::vec4>& vertices);

    static bool validateBinaryFile(const std::string& filename);
};
}


#endif //MOLUMES_STLEXPORTER_H
