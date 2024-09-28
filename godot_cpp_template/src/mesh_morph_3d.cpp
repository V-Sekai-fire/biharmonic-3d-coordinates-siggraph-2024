#include "mesh_morph_3d.h"
#include "BHC.h"
#include "point3.h"
#include <Eigen/Dense>
#include <iostream>
#include <godot_cpp/classes/array_mesh.hpp>

using namespace godot;

void MeshMorph3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("update_vertex_positions", "positions"), &MeshMorph3D::update_vertex_positions);
	ClassDB::bind_method(D_METHOD("apply_deformation_to_children", "mesh"), &MeshMorph3D::apply_deformation_to_children);

	ClassDB::bind_method(D_METHOD("set_cage_mesh", "mesh"), &MeshMorph3D::set_cage_mesh);
	ClassDB::bind_method(D_METHOD("get_cage_mesh"), &MeshMorph3D::get_cage_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "cage_mesh"), "set_cage_mesh", "get_cage_mesh");

	ClassDB::bind_method(D_METHOD("set_gamma_D_13BC", "gamma"), &MeshMorph3D::set_gamma_D_13BC);
	ClassDB::bind_method(D_METHOD("get_gamma_D_13BC"), &MeshMorph3D::get_gamma_D_13BC);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gamma"), "set_gamma_D_13BC", "get_gamma_D_13BC");
}

MeshMorph3D::MeshMorph3D() {
    cage_mesh = Ref<ArrayMesh>();
}

MeshMorph3D::~MeshMorph3D() {
}

void MeshMorph3D::set_cage_mesh(Ref<ArrayMesh> p_mesh) {
    cage_mesh = p_mesh;
    if (cage_mesh.is_valid()) {
        PackedVector3Array vertex_array = cage_mesh->surface_get_arrays(0)[Mesh::ARRAY_VERTEX];
        original_vertices.clear();
        original_vertices.reserve(vertex_array.size());
        for (int i = 0; i < vertex_array.size(); ++i) {
            Vector3 v = vertex_array[i];
            original_vertices.push_back(point3d(v.x, v.y, v.z));
        }
    }
}

Ref<ArrayMesh> MeshMorph3D::get_cage_mesh() const {
    return cage_mesh;
}

void MeshMorph3D::update_vertex_positions(const PackedVector3Array &new_positions) {
        modified_vertices.clear();
        modified_vertices.reserve(new_positions.size());
        for (int i = 0; i < new_positions.size(); ++i) {
            Vector3 v = new_positions[i];
            modified_vertices.push_back(point3d(v.x, v.y, v.z));
        }
}

void MeshMorph3D::apply_deformation_to_children() {
    Ref<SurfaceTool> surface_tool;
    surface_tool.instantiate();
    Ref<ArrayMesh> cage_mesh;
    Ref<ArrayMesh> cage_deformed_mesh;
    Ref<ArrayMesh> mesh;
    std::cout << "Loading cage..." << std::endl;
    cage_mesh = ResourceLoader::get_singleton()->load("res://models/cage.obj");
    if (cage_mesh.is_null()) {
        std::cerr << "Failed to load cage model" << std::endl;
        return;
    }

    cage_deformed_mesh = ResourceLoader::get_singleton()->load("res://models/cage_deformed.obj");
    if (cage_deformed_mesh.is_null()) {
        std::cerr << "Failed to load deformed cage model" << std::endl;
        return;
    }

    std::vector<point3d> cage_vertices = extract_vertices(cage_mesh);
    std::vector<point3d> cage_modified_vertices = extract_vertices(cage_deformed_mesh);
    const std::vector<std::vector<unsigned int>> cage_triangles = extract_triangles(cage_deformed_mesh);
    std::vector<point3d> cage_triangle_normals = calculate_normals(cage_deformed_mesh);
    std::cout << "Loading mesh..." << std::endl;
    mesh = ResourceLoader::get_singleton()->load("res://models/mesh.obj");
    if (mesh.is_null()) {
        std::cerr << "Failed to load mesh model" << std::endl;
        return;
    }

    std::vector<point3d> mesh_vertices = extract_vertices(mesh);
    std::vector<point3d> mesh_modified_vertices = apply_deformation(mesh_vertices, cage_triangles, cage_vertices, cage_modified_vertices, cage_triangle_normals);

    update_mesh(mesh, mesh_modified_vertices);
}

std::vector<point3d> godot::MeshMorph3D::apply_deformation(const std::vector<point3d> &p_original_vertices, const std::vector<std::vector<unsigned int>> &cage_triangles, const std::vector<point3d> &cage_vertices, const std::vector<point3d> &cage_modified_vertices, std::vector<point3d> &cage_triangle_normals) {
	std::vector<point3d> mesh_vertices = p_original_vertices;
	std::vector<std::vector<double>> BHConstrainedC_13_phi, BHConstrainedC_13_psi; // (1,3) version
	{
		std::cout << "Computing (1,3)-regularized matrices..." << std::endl;
		Eigen::MatrixXd ConstrainedBiH_13_C11;
		Eigen::MatrixXd ConstrainedBiH_13_C12;
		Eigen::MatrixXd ConstrainedBiH_13_C21;
		Eigen::MatrixXd ConstrainedBiH_13_C22;
		std::vector<std::vector<double>> BHC_h_phi, BHC_h_psi, BHC_bh_phi, BHC_bh_psi; // unconstrained
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

		std::cout << "Compute (1,3)-regularized BHC " << std::endl;
		
        BHConstrainedC_13_phi.resize(mesh_vertices.size());
        BHConstrainedC_13_psi.resize(mesh_vertices.size());
        for (int p_idx = 0; p_idx < mesh_vertices.size(); ++p_idx) {
            BiharmonicCoordinates3D::compute_13_blending_from_unconstrained_biharmonics(
                    BHC_h_phi[p_idx], BHC_h_psi[p_idx], BHC_bh_phi[p_idx], BHC_bh_psi[p_idx],
                    ConstrainedBiH_13_C11, ConstrainedBiH_13_C12, ConstrainedBiH_13_C21, ConstrainedBiH_13_C22,
                    BHConstrainedC_13_phi[p_idx], BHConstrainedC_13_psi[p_idx]);
        }
        for (unsigned int tIt = 0; tIt < cage_triangles.size(); ++tIt) {
            const auto &t = cage_triangles[tIt];
            point3d v1 = cage_modified_vertices[t[1]] - cage_modified_vertices[t[0]];
            point3d v2 = cage_modified_vertices[t[2]] - cage_modified_vertices[t[0]];
            point3d normal = point3d::cross(v1, v2).direction(); // Calculate the cross product and normalize it
            cage_triangle_normals[tIt] = normal;
        }
	}
	std::cout << "Cage triangle normals computed." << std::endl;
#pragma omp parallel for
    for (int v = 0; v < mesh_vertices.size(); ++v) {
        point3d pos(0, 0, 0);
        for (unsigned int vc = 0; vc < cage_vertices.size(); ++vc) {
            pos += BHConstrainedC_13_phi[v][vc] * cage_modified_vertices[vc];
        }
        for (unsigned int tc = 0; tc < cage_triangles.size(); ++tc) {
            pos += BHConstrainedC_13_psi[v][tc] * cage_triangle_normals[tc];
        }
        mesh_vertices[v] = pos;
    }
    std::cout << "Mesh deformation updated from cage deformation." << std::endl;
	return mesh_vertices;
}
