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
        original_vertices = cage_mesh->surface_get_arrays(0)[Mesh::ARRAY_VERTEX];
    }
}

Ref<ArrayMesh> MeshMorph3D::get_cage_mesh() const {
    return cage_mesh;
}

void MeshMorph3D::update_vertex_positions(const PackedVector3Array &new_positions) {
    modified_vertices = new_positions;
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

    std::vector<point3d> cage_triangle_normals = calculate_normals(cage_deformed_mesh);

    std::cout << "Loading mesh..." << std::endl;
    mesh = ResourceLoader::get_singleton()->load("res://models/mesh.obj");
    if (mesh.is_null()) {
        std::cerr << "Failed to load mesh model" << std::endl;
        return;
    }

    std::vector<point3d> mesh_vertices = extract_vertices(mesh);
    std::vector<point3d> mesh_modified_vertices = apply_deformation(mesh_vertices, cage_vertices, cage_modified_vertices, cage_triangle_normals);

    update_mesh(mesh, mesh_modified_vertices);
}