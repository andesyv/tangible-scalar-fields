#ifndef MOLUMES_STLEXPORTER_H
#define MOLUMES_STLEXPORTER_H

#include "Interactor.h"

#include <string>
#include <iosfwd>
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
private:
    /**
     * Intermediate triangle representation
     */
    struct Triangle {
        glm::vec3 normal;
        glm::vec3 v1, v2, v3;
    };

    /**
     * Structured binary data for STL export
     *
     * Size is 50 bytes, but on a 64 bit compiler, the struct gets compiled as 52 bytes due to machine alignment
     */
    struct BinaryTriangleStruct {
        std::array<glm::float32_t, 3> normal{}; // 4 * 3 bytes
        std::array<glm::float32_t, 3> vertex1{}; // 4 * 3 bytes
        std::array<glm::float32_t, 3> vertex2{}; // 4 * 3 bytes
        std::array<glm::float32_t, 3> vertex3{}; // 4 * 3 bytes
        uint16_t attributeByteCount = 0; // 2 bytes (aligned as 4)
    };

    static constexpr auto binaryHeaderSize = 80u;
    // Arbitrary scale number with no standard. 100 worked fine in PrusaSlicer 2.3.3
    static constexpr auto boundingSize = 100.f;

public:
    static constexpr auto defaultFileName = "model.stl";
    static constexpr auto defaultModelName = "crystal";

    explicit STLExporter(Viewer * viewer, CrystalRenderer* crystalRenderer = nullptr);

    void keyEvent(int key, int scancode, int action, int mods) override;
    void display() override;

    void exportFile();

private:
    CrystalRenderer* m_renderer{nullptr};

    void exportAscii(std::ofstream&& ofs, const std::string& modelName = defaultModelName);
    void exportBinary(std::ofstream&& ofs);

    static std::vector<glm::vec3> normalizeRange(const std::vector<glm::vec4>& vertices, float size = boundingSize);
    static std::vector<glm::vec3> calculateNormals(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices);
    /// Calculates normals and pairs them together with triangles as normal-triangle pairs.
    static std::vector<Triangle> zipNormalsAndVertices(const std::vector<glm::vec4>& vertices);

    /// Checks that the written size of the file is the same as what's specified in the header of the file
    static bool validateBinaryFile(const std::string& filename);
};
}


#endif //MOLUMES_STLEXPORTER_H
