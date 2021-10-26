#include "STLExporter.h"

#include <iostream>
#include <iomanip>
#include <array>
#include <filesystem>
#include <format>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <portable-file-dialogs.h>
#include <glm/glm.hpp>

#include "renderer/CrystalRenderer.h"

using namespace molumes;
namespace fs = std::filesystem;

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
    std::cout << "Filename: " << filepath << std::endl;

    // If no file path was supplied, assume saving was cancelled. Abort
    if (filepath.empty())
        return;

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
}

std::string f_to_s(float num) {
    char buf[40]; // 40 is probably enough
    return std::string{buf, buf + std::sprintf(buf, "%e", num)};
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
        ofs << std::format("facet normal {} {} {}", f_to_s(n.x), f_to_s(n.y), f_to_s(n.z)) << std::endl;
        ofs << "    outer loop" << std::endl;
        ofs << std::format("        vertex {} {} {}", f_to_s(v1.x), f_to_s(v1.y), f_to_s(v1.z)) << std::endl;
        ofs << std::format("        vertex {} {} {}", f_to_s(v2.x), f_to_s(v2.y), f_to_s(v2.z)) << std::endl;
        ofs << std::format("        vertex {} {} {}", f_to_s(v3.x), f_to_s(v3.y), f_to_s(v3.z)) << std::endl;
        ofs << "    endloop" << std::endl;
        ofs << "endfacet" << std::endl;
    }

    ofs << "endsolid " << modelName;
}

void STLExporter::exportBinary(std::ofstream&& ofs) {
    
}

std::vector<glm::vec3> STLExporter::calculateNormals(const std::vector<glm::vec4>& vertices) {
    std::vector<glm::vec3> normals;
    normals.reserve(vertices.size() / 3);
    for (std::size_t i{0}; i +2 < vertices.size(); i+=3) {
        const auto a = glm::vec3{vertices.at(i+2) - vertices.at(i+1)};
        const auto b = glm::vec3{vertices.at(i) - vertices.at(i+1)};
        const auto normal = glm::normalize(glm::cross(a, b));
        normals.emplace_back(glm::any(glm::isnan(normal)) ? glm::vec3{0.f} : normal);
    }

    return normals;
}

std::vector<STLExporter::Plane> STLExporter::zipNormalsAndVertices(const std::vector<glm::vec4> &vertices) {
    const auto normals = calculateNormals(vertices);
    std::vector<Plane> planes;
    planes.reserve(normals.size());
    for (std::size_t i{0}; i < normals.size(); ++i)
        planes.emplace_back(normals.at(i), glm::vec3{vertices.at(i*3)}, glm::vec3{vertices.at(i*3+1)}, glm::vec3{vertices.at(i*3+2)});
    return planes;
}

