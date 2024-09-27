#include "imgui.h"
#include "include/BHC.h"
#include "include/BasicIO.h"
#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include <ImGuizmo.h>
#include <Eigen/Dense>
#include <iostream>
#include <vector>

std::vector<std::vector<double>> BHC_h_phi, BHC_h_psi, BHC_bh_phi, BHC_bh_psi; // unconstrained
std::vector<std::vector<double>> BHConstrainedC_13_phi, BHConstrainedC_13_psi; // (1,3) version

Eigen::MatrixXd ConstrainedBiH_13_C11;
Eigen::MatrixXd ConstrainedBiH_13_C12;
Eigen::MatrixXd ConstrainedBiH_13_C21;
Eigen::MatrixXd ConstrainedBiH_13_C22;

float gamma_D_13BC = 1.0;
std::vector<point3d> cage_vertices, cage_modified_vertices, mesh_vertices, mesh_modified_vertices;
std::vector<std::vector<unsigned int>> cage_triangles, mesh_triangles;
std::vector<point3d> cage_triangle_normals;

void myCallback() {
	ImGui::Text("Adjust parameters and load/save operations:");
	ImGui::SliderFloat("Gamma D 13BC", &gamma_D_13BC, 0.0, 10.0);
	static std::vector<Eigen::Matrix4f> gizmoTransforms(cage_vertices.size(), Eigen::Matrix4f::Identity());
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::Begin("Gizmo Window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();

	float viewMatrix[16], projectionMatrix[16];
	polyscope::view::getCameraViewMatrix(viewMatrix);
	polyscope::view::getCameraProjectionMatrix(projectionMatrix);

	for (size_t i = 0; i < cage_vertices.size(); ++i) {
		ImGuizmo::Manipulate(viewMatrix, projectionMatrix, ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, gizmoTransforms[i].data());
		// Update cage vertices from gizmo output
		cage_modified_vertices[i] = point3d(gizmoTransforms[i](0, 3), gizmoTransforms[i](1, 3), gizmoTransforms[i](2, 3));
	}

// 	std::vector<std::vector<double>> BHC_h_phi(mesh_vertices.size()), BHC_h_psi(mesh_vertices.size()),
// 			BHC_bh_phi(mesh_vertices.size()), BHC_bh_psi(mesh_vertices.size());

// #pragma omp parallel for
// 	for (int p_idx = 0; p_idx < mesh_vertices.size(); ++p_idx) {
// 		BiharmonicCoordinates3D::computeCoordinates(mesh_vertices[p_idx], cage_triangles, cage_vertices,
// 				BHC_h_phi[p_idx], BHC_h_psi[p_idx], BHC_bh_phi[p_idx], BHC_bh_psi[p_idx]);
// 	}

// 	std::vector<point3d> cage_triangle_normals(cage_triangles.size(), point3d(0, 0, 0));
// 	for (unsigned int tIt = 0; tIt < cage_triangles.size(); ++tIt) {
// 		auto &t = cage_triangles[tIt];
// 		cage_triangle_normals[tIt] = point3d::cross(cage_modified_vertices[t[1]] - cage_modified_vertices[t[0]],
// 				cage_modified_vertices[t[2]] - cage_modified_vertices[t[0]])
// 											 .direction();
// 	}

// #pragma omp parallel for
// 	for (int v = 0; v < mesh_vertices.size(); ++v) {
// 		point3d pos(0, 0, 0);
// 		for (unsigned int vc = 0; vc < cage_modified_vertices.size(); ++vc)
// 			pos += BHC_h_phi[v][vc] * cage_modified_vertices[vc];
// 		for (unsigned int tc = 0; tc < cage_triangles.size(); ++tc) {
// 			pos += BHC_h_psi[v][tc] * cage_triangle_normals[tc];
// 		}
// 		mesh_modified_vertices[v] = pos;
// 	}

// 	std::cout << " Compute cage triangle normals " << std::endl;
// 	// std::vector<point3d> cage_triangle_normals(cage_triangles.size(), point3d(0, 0, 0));
// 	for (unsigned int tIt = 0; tIt < cage_triangles.size(); ++tIt) {
// 		auto &t = cage_triangles[tIt];
// 		cage_triangle_normals[tIt] = point3d::cross(cage_modified_vertices[t[1]] - cage_modified_vertices[t[0]], cage_modified_vertices[t[2]] - cage_modified_vertices[t[0]]).direction();
// 	}
// 	std::cout << " Update the mesh deformation, from the cage deformation " << std::endl;
// 	std::vector<point3d> mesh_modified_vertices(mesh_vertices.size());
// #pragma omp parallel for
// 	for (int v = 0; v < mesh_vertices.size(); ++v) {
// 		point3d pos(0, 0, 0);
// 		for (unsigned int vc = 0; vc < cage_modified_vertices.size(); ++vc)
// 			pos += BHConstrainedC_13_phi[v][vc] * cage_modified_vertices[vc];
// 		for (unsigned int tc = 0; tc < cage_triangles.size(); ++tc) {
// 			pos += BHConstrainedC_13_psi[v][tc] * cage_triangle_normals[tc];
// 		}
// 		mesh_modified_vertices[v] = pos;
// 	}
	if (ImGui::Button("Save Deformed Mesh")) {
		OBJIO::save("models/mesh_deformed.obj", mesh_modified_vertices, mesh_triangles);
	}
}

int main(int argc, char const *argv[]) {
	polyscope::init();
	if (!OBJIO::open("models/cage.obj", cage_vertices, cage_triangles, true)) {
		std::cerr << "Failed to load cage model" << std::endl;
		return -1;
	}
	if (!OBJIO::open("models/mesh.obj", mesh_vertices, mesh_triangles, true)) {
		std::cerr << "Failed to load mesh model" << std::endl;
		return -1;
	}
	mesh_modified_vertices.resize(mesh_vertices.size(), point3d(0, 0, 0));
	polyscope::registerSurfaceMesh("Original Mesh", mesh_vertices, mesh_triangles);
	polyscope::SurfaceMesh *cageMesh = polyscope::registerSurfaceMesh("Cage", cage_vertices, cage_triangles);
	cageMesh->setTransparency(0.5);
	polyscope::registerSurfaceMesh("Deformed Mesh", mesh_modified_vertices, mesh_triangles);
	polyscope::state::userCallback = myCallback;
	polyscope::show();
	return EXIT_SUCCESS;
}
