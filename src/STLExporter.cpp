#include "STLExporter.h"

#include <iostream>
#include <iomanip>
#include <array>
#include <filesystem>
#include <format>
#include <chrono>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <portable-file-dialogs.h>
#include <glm/glm.hpp>

#include "renderer/CrystalRenderer.h"

using namespace molumes;
using namespace glm;
namespace fs = std::filesystem;
namespace chr = std::chrono;

// Utility functions:
auto getCurrentDate() {
    /// New c++20 calendar library is weird...
    return chr::year_month_day{chr::floor<chr::days>(chr::current_zone()->to_local(chr::system_clock::now()))};
}

auto getCurrentTime(auto currentDate) {
    return chr::hh_mm_ss{chr::current_zone()->to_local(chr::system_clock::now()) - chr::local_days{currentDate}};
}


STLExporter::STLExporter(Viewer *viewer, CrystalRenderer* crystalRenderer) : m_renderer{crystalRenderer}, Interactor(viewer) {

}

void STLExporter::keyEvent(int key, int scancode, int action, int mods) {
    if (m_renderer == nullptr || m_renderer->getVertices().empty())
        return;

    if ((key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) && action == GLFW_PRESS)
        m_ctrl = true;
    else if (m_ctrl && key == GLFW_KEY_S && action == GLFW_PRESS)
        exportFile();
}

void STLExporter::display() {
    Interactor::display();

    if (m_renderer == nullptr || m_renderer->getVertices().empty())
        return;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Export file...", "Ctrl+S"))
            exportFile();

        ImGui::EndMenu();
    }
}

void STLExporter::exportFile() {
    static const char* filePatterns[] = {"*.stl", "*.stl-ascii"};
    const auto rootPath = fs::current_path() / fs::path{defaultFileName};
    auto fileDialog = pfd::save_file("Export model as ...", rootPath.string(), {
        "Binary STL (.stl, *.stl-binary)", "*.stl *.stl-binary",
        "ASCII STL (.stl-ascii)", "*.stl-ascii",
        "All files", "*"}, pfd::opt::none);

    auto filepath = fs::path{fileDialog.result()};

    // If no file path was supplied, assume saving was cancelled. Abort
    if (filepath.empty())
        return;

#ifndef NDEBUG
    try {
#endif
        if (filepath.extension() == ".stl-ascii") {
            std::ofstream ofs{filepath, std::ofstream::trunc | std::ofstream::out};
            exportAscii(std::move(ofs), filepath.stem() == fs::path{defaultFileName}.stem() ? defaultModelName : filepath.stem().string());
        } else {
            // Fix extension if no extension was supplied:
            if (filepath.extension().empty())
                filepath.replace_extension(".stl");

            std::ofstream ofs{filepath, std::ofstream::trunc | std::ofstream::out | std::ofstream::binary};
            exportBinary(std::move(ofs));
        }
#ifndef NDEBUG
    }
    catch (...) {
        std::cout << "Failed to write file to " << filepath << std::endl;
        return;
    }
#endif

    if (validateBinaryFile(filepath.string()))
        std::cout << "Successfully exported model as " << filepath << std::endl;
    else
        std::cout << "Error while exporting model: " << filepath << std::endl;
}

void STLExporter::exportAscii(std::ofstream&& ofs, const std::string& modelName) {
    if (m_renderer == nullptr)
        return;
    const auto vertices = m_renderer->getVertices();
    if (vertices.empty())
        return;

    const auto normalVertexPairs = zipNormalsAndVertices(vertices);

    ofs << "solid " << modelName << std::endl;

    for (const auto& [n, v1, v2, v3] : normalVertexPairs) {
        ofs << std::format("facet normal {:e} {:e} {:e}", n.x, n.y, n.z) << std::endl;
        ofs << "    outer loop" << std::endl;
        ofs << std::format("        vertex {:e} {:e} {:e}", v1.x, v1.y, v1.z) << std::endl;
        ofs << std::format("        vertex {:e} {:e} {:e}", v2.x, v2.y, v2.z) << std::endl;
        ofs << std::format("        vertex {:e} {:e} {:e}", v3.x, v3.y, v3.z) << std::endl;
        ofs << "    endloop" << std::endl;
        ofs << "endfacet" << std::endl;
    }

    ofs << "endsolid " << modelName;
}

void STLExporter::exportBinary(std::ofstream&& ofs) {
    if (m_renderer == nullptr)
        return;
    const auto vertices = m_renderer->getVertices();
    if (vertices.empty())
        return;

    const auto normalVertexPairs = zipNormalsAndVertices(vertices);

    const auto date = getCurrentDate();
    std::string header = std::format("UNITS=mm. STL file generated by Molumes software at {:%H:%M} {:%d/%m/%y} :)", getCurrentTime(date), date);
    header.resize(binaryHeaderSize, ' ');
    ofs.write(header.data(), binaryHeaderSize);

    const auto byteWrite = [&ofs]<typename T>(T data, std::streamsize size = sizeof(T)) {
        assert(size <= sizeof(T));
        ofs.write(reinterpret_cast<const char*>(&data), size);
    };

    const auto triangleCount = static_cast<uint32_t>(normalVertexPairs.size());
    std::cout << "Exported triangle count: " << normalVertexPairs.size() << std::endl;
    byteWrite(triangleCount, 4);

    for (const auto& [n, v1, v2, v3] : normalVertexPairs) {
        BinaryTriangleStruct triangle{
            .normal={n.x, n.y, n.z},
            .vertex1={v1.x, v1.y, v1.z},
            .vertex2={v2.x, v2.y, v2.z},
            .vertex3={v3.x, v3.y, v3.z}
        };

        byteWrite(triangle, 50); // Struct is 52 bytes because of machine alignment, so manually just write the 50 first of them
    }
}

std::vector<glm::vec3> STLExporter::normalizeRange(const std::vector<glm::vec4> &vertices, float size) {
    std::vector<glm::vec3> out;
    out.reserve(vertices.size());
    for (const auto& v : vertices)
        // Vertices from crystalrenderer are in normalized range [-1,1], same as bounding box renderer, so scaling them in the editor will scale the file output.
        out.push_back((glm::vec3{v} * 0.5f + 0.5f) * size);

    return out;
}

std::vector<glm::vec3> STLExporter::calculateNormals(const std::vector<glm::vec3>& vertices) {
    std::vector<glm::vec3> normals;
    normals.reserve(vertices.size() / 3);
    for (std::size_t i{0}; i +2 < vertices.size(); i+=3) {
        const auto a = vertices.at(i+2) - vertices.at(i+1);
        const auto b = vertices.at(i) - vertices.at(i+1);
        const auto normal = glm::normalize(glm::cross(a, b));
        normals.emplace_back(glm::any(glm::isnan(normal)) ? glm::vec3{0.f} : normal);
    }

    return normals;
}

std::vector<STLExporter::Plane> STLExporter::zipNormalsAndVertices(const std::vector<glm::vec4> &vertices) {
    const auto normalized = normalizeRange(vertices);
    const auto normals = calculateNormals(normalized);
    std::vector<Plane> planes;
    planes.reserve(normals.size());
    for (std::size_t i{0}; i < normals.size(); ++i)
        planes.emplace_back(normals.at(i), normalized.at(i*3), normalized.at(i*3+1), normalized.at(i*3+2));
    return planes;
}

bool STLExporter::validateBinaryFile(const std::string& filename) {
    const auto path = fs::path{filename};
    if (path.empty())
        return false;

    std::ifstream ifs{path, std::ifstream::in | std::ifstream::binary};
    if (!ifs)
        return false;

    ifs.seekg(80, std::ios::beg);
    uint32_t triangleCount;
    ifs.read(reinterpret_cast<char*>(&triangleCount), 4);
    ifs.seekg(0, std::ios::end);
    return ifs.tellg() == binaryHeaderSize + 4 + triangleCount * 50; // 84 is header(80) + triangle count(4)
}

