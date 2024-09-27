#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include "imgui.h"
#include <iostream>

int main(int argc, char const *argv[]) {
    polyscope::init();

    // Register a dummy mesh to ensure Polyscope has something to render
    std::vector<std::array<double, 3>> vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    std::vector<std::array<size_t, 3>> faces = {{0, 1, 2}};
    polyscope::registerSurfaceMesh("Dummy Mesh", vertices, faces);

    while (!polyscope::windowRequestsClose()) {
        polyscope::show();  // Use show instead of manual redraw requests

        if (ImGui::Begin("Simple Test Window")) {
            ImGui::Text("Hello, world!");
            ImGui::End();
        }
    }

    polyscope::shutdown();
    return 0;
}
