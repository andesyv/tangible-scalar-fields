#include "STLExporter.h"

#include <iostream>
#include <array>

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <portable-file-dialogs.h>

using namespace molumes;

STLExporter::STLExporter(Viewer *viewer) : Interactor(viewer) {

}

void STLExporter::keyEvent(int key, int scancode, int action, int mods) {
    if ((key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) && action == GLFW_PRESS)
        m_ctrl = true;
    else if (m_ctrl && key == GLFW_KEY_S && action == GLFW_PRESS)
        exportFile();
}

void STLExporter::display() {
    Interactor::display();

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Export file...", "Ctrl+S"))
            exportFile();

        ImGui::EndMenu();
    }
}

void STLExporter::exportFile() {
    pfd::settings::verbose(true);

    static const char* filePatterns[] = {"*.stl", "*.stl-ascii"};
    auto fileDialog = pfd::save_file("Export model as ...", "./", {
        "Binary STL (.stl)", "*.stl",
        "ASCII STL (.stl-ascii)", "*.stl-ascii",
        "All files", "*"}, pfd::opt::none);

    const auto fileName = fileDialog.result();

//    if (fileName) {
        std::cout << "Filename: " << fileName << std::endl;
//    }
}
