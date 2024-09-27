#include "imgui.h"
#include "include/BHC.h"
#include "include/BasicIO.h"
#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include <Eigen/Dense>
#include <iostream>

bool showDeformedMesh = false;

float gamma_D_13BC = 1.0; // you can play with this
std::vector<point3d> cage_vertices;
std::vector<std::vector<unsigned int>> cage_triangles;
std::vector<point3d> mesh_vertices;
std::vector<point3d> mesh_modified_vertices;
std::vector<point3d> cage_modified_vertices;
std::vector<std::vector<unsigned int>> mesh_triangles;
std::vector<std::vector<double>> BHC_h_phi, BHC_h_psi, BHC_bh_phi, BHC_bh_psi; // unconstrained
std::vector<std::vector<double>> BHConstrainedC_13_phi, BHConstrainedC_13_psi; // (1,3) version
void myCallback() {
	ImGui::Text("Adjust parameters and load/save operations:");
	ImGui::SliderFloat("Gamma D 13BC", &gamma_D_13BC, 0.0f, 10.0f);

	if (ImGui::Button("Toggle Mesh View")) {
		showDeformedMesh = !showDeformedMesh;
		std::cout << "Toggle Mesh View clicked. showDeformedMesh is now " << (showDeformedMesh ? "true" : "false") << std::endl;

		if (showDeformedMesh) {
			std::cout << "Loading cage..." << std::endl;
			if (!OBJIO::open("models/cage.obj", cage_vertices, cage_triangles, true)) {
				std::cerr << "Failed to load cage model." << std::endl;
				return;
			}
			std::cout << "Cage loaded successfully." << std::endl;

			std::cout << "Loading mesh..." << std::endl;
			if (!OBJIO::open("models/mesh.obj", mesh_vertices, mesh_triangles, true)) {
				std::cerr << "Failed to load mesh model." << std::endl;
				return;
			}
			std::cout << "Mesh loaded successfully." << std::endl;

			std::cout << "Computing (1,3)-regularized matrices..." << std::endl;
			Eigen::MatrixXd ConstrainedBiH_13_C11;
			Eigen::MatrixXd ConstrainedBiH_13_C12;
			Eigen::MatrixXd ConstrainedBiH_13_C21;
			Eigen::MatrixXd ConstrainedBiH_13_C22;

            BiharmonicCoordinates3D::computeConstrainedBiharmonicMatrices_13(
                    cage_triangles, cage_vertices,
                    ConstrainedBiH_13_C11, ConstrainedBiH_13_C12, ConstrainedBiH_13_C21, ConstrainedBiH_13_C22,
                    gamma_D_13BC);
            std::cout << "(1,3)-regularized matrices computed successfully." << std::endl;

			std::cout << "Computing unconstrained coordinates..." << std::endl;
			BHC_h_phi.resize(mesh_vertices.size());
			BHC_bh_phi.resize(mesh_vertices.size());
			BHC_h_psi.resize(mesh_vertices.size());
			BHC_bh_psi.resize(mesh_vertices.size());

#pragma omp parallel for
			for (int p_idx = 0; p_idx < mesh_vertices.size(); ++p_idx) {
				BiharmonicCoordinates3D::computeCoordinates(mesh_vertices[p_idx],
						cage_triangles,
						cage_vertices,
						BHC_h_phi[p_idx], BHC_h_psi[p_idx], BHC_bh_phi[p_idx], BHC_bh_psi[p_idx]);
			}
			std::cout << "Unconstrained coordinates computed." << std::endl;

            std::cout << " Compute (1,3)-regularized BHC " << std::endl;
            {
                BHConstrainedC_13_phi.resize(mesh_vertices.size());
                BHConstrainedC_13_psi.resize(mesh_vertices.size());
                for (int p_idx = 0; p_idx < mesh_vertices.size(); ++p_idx)
                {
                    BiharmonicCoordinates3D::compute_13_blending_from_unconstrained_biharmonics(
                        BHC_h_phi[p_idx], BHC_h_psi[p_idx], BHC_bh_phi[p_idx], BHC_bh_psi[p_idx],
                        ConstrainedBiH_13_C11, ConstrainedBiH_13_C12, ConstrainedBiH_13_C21, ConstrainedBiH_13_C22,
                        BHConstrainedC_13_phi[p_idx], BHConstrainedC_13_psi[p_idx]
                    );
                }
            }
            OBJIO::open("models/cage_deformed.obj", cage_modified_vertices);
			std::vector<point3d> cage_triangle_normals(cage_triangles.size(), point3d(0, 0, 0));
			for (unsigned int tIt = 0; tIt < cage_triangles.size(); ++tIt) {
				auto &t = cage_triangles[tIt];
				cage_triangle_normals[tIt] = point3d::cross(cage_modified_vertices[t[1]] - cage_modified_vertices[t[0]],
						cage_modified_vertices[t[2]] - cage_modified_vertices[t[0]]).direction();
			}
			std::cout << "Cage triangle normals computed." << std::endl;

#pragma omp parallel for
			for (int v = 0; v < mesh_vertices.size(); ++v) {
				point3d pos(0, 0, 0);
				for (unsigned int vc = 0; vc < cage_modified_vertices.size(); ++vc) {
					pos += BHConstrainedC_13_phi[v][vc] * cage_modified_vertices[vc];
				}
				for (unsigned int tc = 0; tc < cage_triangles.size(); ++tc) {
					pos += BHConstrainedC_13_psi[v][tc] * cage_triangle_normals[tc];
				}
				mesh_modified_vertices[v] = pos;
			}
			std::cout << "Mesh deformation updated from cage deformation." << std::endl;
		} else {
			std::cout << " Reload original mesh " << std::endl;
			OBJIO::open("models/mesh.obj", mesh_vertices, mesh_triangles, true);
			mesh_modified_vertices = mesh_vertices;
		}
		polyscope::getSurfaceMesh("Deformed Mesh")->updateVertexPositions(mesh_modified_vertices);
		polyscope::getSurfaceMesh(showDeformedMesh ? "Deformed Mesh" : "Original Mesh")->setEnabled(true);
		polyscope::getSurfaceMesh(showDeformedMesh ? "Original Mesh" : "Deformed Mesh")->setEnabled(false);
	}

	if (ImGui::Button("Save Deformed Mesh")) {
		OBJIO::save("models/mesh_deformed.obj", mesh_modified_vertices, mesh_triangles);
	}
}

int main(int argc, char const *argv[]) {
	polyscope::init();
	if (!OBJIO::open("models/cage.obj", cage_vertices, cage_triangles, true)) {
		std::cerr << "Failed to load cage model" << std::endl;
	}
	if (!OBJIO::open("models/mesh.obj", mesh_vertices, mesh_triangles, true)) {
		std::cerr << "Failed to load mesh model" << std::endl;
	}
	mesh_modified_vertices.resize(mesh_vertices.size(), point3d(0, 0, 0));
	polyscope::registerSurfaceMesh("Original Mesh", mesh_vertices, mesh_triangles);
	polyscope::registerSurfaceMesh("Deformed Mesh", mesh_modified_vertices, mesh_triangles)->setEnabled(false);
	polyscope::state::userCallback = myCallback;
	polyscope::show();
	return EXIT_SUCCESS;
}
