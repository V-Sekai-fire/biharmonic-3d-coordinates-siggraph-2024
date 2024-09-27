#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include "include/BHC.h"
#include "include/BasicIO.h"
#include <Eigen/Dense>
#include <vector>

void updateDeformation(const std::vector<point3d>& cage_vertices, const std::vector<std::vector<unsigned int>>& cage_triangles,
                                 const std::vector<point3d>& cage_modified_vertices, std::vector<point3d>& mesh_vertices,
                                 std::vector<point3d>& mesh_modified_vertices, Eigen::MatrixXd& ConstrainedBiH_13_C11,
                                 Eigen::MatrixXd& ConstrainedBiH_13_C12, Eigen::MatrixXd& ConstrainedBiH_13_C21, Eigen::MatrixXd& ConstrainedBiH_13_C22) {
    // Compute unconstrained biharmonic coordinates
    std::vector<std::vector<double>> BHC_h_phi(mesh_vertices.size()), BHC_h_psi(mesh_vertices.size()),
                                      BHC_bh_phi(mesh_vertices.size()), BHC_bh_psi(mesh_vertices.size());

    #pragma omp parallel for
    for (int p_idx = 0; p_idx < mesh_vertices.size(); ++p_idx) {
        BiharmonicCoordinates3D::computeCoordinates(mesh_vertices[p_idx], cage_triangles, cage_vertices,
                                                    BHC_h_phi[p_idx], BHC_h_psi[p_idx], BHC_bh_phi[p_idx], BHC_bh_psi[p_idx]);
    }

    // Compute deformations based on modified cage
    std::vector<point3d> cage_triangle_normals(cage_triangles.size(), point3d(0, 0, 0));
    for (unsigned int tIt = 0; tIt < cage_triangles.size(); ++tIt) {
        auto &t = cage_triangles[tIt];
        cage_triangle_normals[tIt] = point3d::cross(cage_modified_vertices[t[1]] - cage_modified_vertices[t[0]], 
                                                    cage_modified_vertices[t[2]] - cage_modified_vertices[t[0]]).direction();
    }

    // Update mesh vertices based on computed biharmonic coordinates and modified cage
    #pragma omp parallel for
    for (int v = 0; v < mesh_vertices.size(); ++v) {
        point3d pos(0, 0, 0);
        for (unsigned int vc = 0; vc < cage_modified_vertices.size(); ++vc)
            pos += BHC_h_phi[v][vc] * cage_modified_vertices[vc];
        for (unsigned int tc = 0; tc < cage_triangles.size(); ++tc) {
            pos += BHC_h_psi[v][tc] * cage_triangle_normals[tc];
        }
        mesh_modified_vertices[v] = pos;
    }
}

int main(int argc, char const *argv[]) {
    polyscope::init();

    Eigen::MatrixXd ConstrainedBiH_13_C11, ConstrainedBiH_13_C12, ConstrainedBiH_13_C21, ConstrainedBiH_13_C22;
    std::vector<point3d> cage_vertices, cage_modified_vertices, mesh_vertices, mesh_modified_vertices;
    std::vector<std::vector<unsigned int>> cage_triangles, mesh_triangles;

    // Load initial cage and mesh
    if (!OBJIO::open("models/cage.obj", cage_vertices, cage_triangles, true)) {
        std::cerr << "Failed to load cage model" << std::endl;
        return -1;
    }
    if (!OBJIO::open("models/mesh.obj", mesh_vertices, mesh_triangles, true)) {
        std::cerr << "Failed to load mesh model" << std::endl;
        return -1;
    }

    // Ensure mesh_modified_vertices is ready before registration
    mesh_modified_vertices.resize(mesh_vertices.size(), point3d(0, 0, 0)); // Initialize with default positions

    // Initial deformation update call
    updateDeformation(cage_vertices, cage_triangles, cage_modified_vertices, mesh_vertices, mesh_modified_vertices, ConstrainedBiH_13_C11, ConstrainedBiH_13_C12, ConstrainedBiH_13_C21, ConstrainedBiH_13_C22);

    // Register meshes with Polyscope
    polyscope::registerSurfaceMesh("Original Mesh", mesh_vertices, mesh_triangles);
    polyscope::registerSurfaceMesh("Deformed Mesh", mesh_modified_vertices, mesh_triangles);

    while (!polyscope::windowRequestsClose()) {
        if (ImGui::Begin("Biharmonic Coordinates Control Panel")) {
            ImGui::Text("Adjust parameters and load/save operations:");

            if (ImGui::Button("Load Cage")) {
                if (!OBJIO::open("models/cage.obj", cage_vertices, cage_triangles, true)) {
                    std::cerr << "Failed to reload cage model" << std::endl;
                }
            }
            if (ImGui::Button("Load Mesh")) {
                if (!OBJIO::open("models/mesh.obj", mesh_vertices, mesh_triangles, true)) {
                    std::cerr << "Failed to reload mesh model" << std::endl;
                }
            }
            if (ImGui::Button("Deform Cage")) {
                if (!OBJIO::open("models/cage_deformed.obj", cage_modified_vertices)) {
                    std::cerr << "Failed to load deformed cage model" << std::endl;
                } else {
                    updateDeformation(cage_vertices, cage_triangles, cage_modified_vertices, mesh_vertices, mesh_modified_vertices, ConstrainedBiH_13_C11, ConstrainedBiH_13_C12, ConstrainedBiH_13_C21, ConstrainedBiH_13_C22);
                    polyscope::getSurfaceMesh("Deformed Mesh")->updateVertexPositions(mesh_modified_vertices);
                }
            }
            if (ImGui::Button("Save Deformed Mesh")) {
                OBJIO::save("models/mesh_deformed.obj", mesh_modified_vertices, mesh_triangles);
            }
            ImGui::End();
        }
        polyscope::requestRedraw();
    }

    polyscope::shutdown();
    return 0;
}

