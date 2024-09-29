#include "mesh_morph_3d.h"
#include "BHC.h"
#include "BasicIO.h"
#include "point3.h"
#include <Eigen/Dense>
#include <iostream>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void MeshMorph3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("apply_deformation_to_children"), &MeshMorph3D::apply_deformation_to_children);
	ClassDB::bind_method(D_METHOD("set_deform_mesh", "mesh"), &MeshMorph3D::set_deform_mesh);
	ClassDB::bind_method(D_METHOD("get_deform_mesh"), &MeshMorph3D::get_deform_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "deform_mesh"), "set_deform_mesh", "get_deform_mesh");
	ClassDB::bind_method(D_METHOD("set_gamma_D_13BC", "gamma"), &MeshMorph3D::set_gamma_D_13BC);
	ClassDB::bind_method(D_METHOD("get_gamma_D_13BC"), &MeshMorph3D::get_gamma_D_13BC);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gamma"), "set_gamma_D_13BC", "get_gamma_D_13BC");
}

MeshMorph3D::MeshMorph3D() {
}

MeshMorph3D::~MeshMorph3D() {
}
void MeshMorph3D::apply_deformation_to_children() {
    // Load original cage and mesh data
    std::vector<point3d> cage_vertices;
    std::vector<std::vector<unsigned int>> cage_triangles;
    if (!OBJIO::open("triangle_3d_cage/art/cage.obj", cage_vertices, cage_triangles, true)) {
        std::cerr << "Failed to load cage model." << std::endl;
        return;
    }

    std::vector<point3d> mesh_vertices;
    std::vector<std::vector<unsigned int>> mesh_triangles;
    if (!OBJIO::open("triangle_3d_cage/art/mesh.obj", mesh_vertices, mesh_triangles, true)) {
        std::cerr << "Failed to load mesh model." << std::endl;
        return;
    }
    UtilityFunctions::print(String("Vertex count in original mesh: ") + String::num_int64(mesh_vertices.size()));

    // Compute (1,3)-regularized matrices
    Eigen::MatrixXd ConstrainedBiH_13_C11;
    Eigen::MatrixXd ConstrainedBiH_13_C12;
    Eigen::MatrixXd ConstrainedBiH_13_C21;
    Eigen::MatrixXd ConstrainedBiH_13_C22;
	std::vector<std::vector<double>> BHConstrainedC_13_phi, BHConstrainedC_13_psi; // (1,3) version
	std::vector<std::vector<double>> BHC_h_phi, BHC_h_psi, BHC_bh_phi, BHC_bh_psi; // unconstrained
    BiharmonicCoordinates3D::computeConstrainedBiharmonicMatrices_13(
        cage_triangles, cage_vertices,
        ConstrainedBiH_13_C11, ConstrainedBiH_13_C12, ConstrainedBiH_13_C21, ConstrainedBiH_13_C22,
        gamma_D_13BC);
    UtilityFunctions::print("(1,3)-regularized matrices computed successfully.");
    BHC_h_phi.resize(mesh_vertices.size());
    BHC_bh_phi.resize(mesh_vertices.size());
    BHC_h_psi.resize(mesh_vertices.size());
    BHC_bh_psi.resize(mesh_vertices.size());

    // Compute unconstrained coordinates
    UtilityFunctions::print("Compute (1,3)-regularized BHC ");
    BHConstrainedC_13_phi.resize(mesh_vertices.size());
    BHConstrainedC_13_psi.resize(mesh_vertices.size());

#pragma omp parallel for
    for (int p_idx = 0; p_idx < mesh_vertices.size(); ++p_idx) {
        BiharmonicCoordinates3D::computeCoordinates(mesh_vertices[p_idx],
            cage_triangles, cage_vertices,
            BHC_h_phi[p_idx], BHC_h_psi[p_idx], BHC_bh_phi[p_idx], BHC_bh_psi[p_idx]);
    }
    UtilityFunctions::print("Unconstrained coordinates computed.");

    // Compute (1,3)-regularized blending from unconstrained biharmonics
    for (int p_idx = 0; p_idx < mesh_vertices.size(); ++p_idx) {
        BiharmonicCoordinates3D::compute_13_blending_from_unconstrained_biharmonics(
            BHC_h_phi[p_idx], BHC_h_psi[p_idx], BHC_bh_phi[p_idx], BHC_bh_psi[p_idx],
            ConstrainedBiH_13_C11, ConstrainedBiH_13_C12, ConstrainedBiH_13_C21, ConstrainedBiH_13_C22,
            BHConstrainedC_13_phi[p_idx], BHConstrainedC_13_psi[p_idx]);
    }

    std::vector<point3d> cage_modified_vertices;
    OBJIO::open("triangle_3d_cage/art/cage_deformed.obj", cage_modified_vertices);
    // Compute cage triangle normals
    std::vector<point3d> cage_triangle_normals(cage_triangles.size(), point3d(0, 0, 0));
    for (unsigned int tIt = 0; tIt < cage_triangles.size(); ++tIt) {
        auto &t = cage_triangles[tIt];
        cage_triangle_normals[tIt] = point3d::cross(cage_modified_vertices[t[1]] - cage_modified_vertices[t[0]],
                                                    cage_modified_vertices[t[2]] - cage_modified_vertices[t[0]]).direction();
    }
    UtilityFunctions::print("Cage triangle normals computed.");

    // Apply deformation to mesh vertices based on computed coordinates and normals
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
    UtilityFunctions::print("Mesh deformation updated from cage deformation.");
    UtilityFunctions::print(String("Vertex count in deformed mesh: ") + String::num_int64(mesh_vertices.size()));

    Ref<SurfaceTool> st = memnew(SurfaceTool);
    st->begin(Mesh::PRIMITIVE_TRIANGLES);
    for (const auto& triangle : mesh_triangles) {
        st->add_vertex(godot::Vector3(mesh_vertices[triangle[0]][0], mesh_vertices[triangle[0]][1], -mesh_vertices[triangle[0]][2]));
        st->add_vertex(godot::Vector3(mesh_vertices[triangle[1]][0], mesh_vertices[triangle[1]][1], -mesh_vertices[triangle[1]][2]));
        st->add_vertex(godot::Vector3(mesh_vertices[triangle[2]][0], mesh_vertices[triangle[2]][1], -mesh_vertices[triangle[2]][2]));
    }
    st->index();
    st->generate_normals();
    set_mesh(st->commit());
}

std::vector<point3d> godot::MeshMorph3D::convert_godot_array_to_vector(const Array &godot_array) {
	std::vector<point3d> vec;
	vec.reserve(godot_array.size());
	for (int i = 0; i < godot_array.size(); ++i) {
		Vector3 v = godot_array[i];
		vec.emplace_back(point3d(v.x, v.y, v.z));
	}
	return vec;
}

std::vector<point3d> godot::MeshMorph3D::extract_vertices(Ref<ArrayMesh> mesh) {
    std::vector<point3d> vertices;
    Array arrays = mesh->surface_get_arrays(0);
    PackedVector3Array vertex_array = arrays[Mesh::ARRAY_VERTEX];
    PackedInt32Array index_array = arrays[Mesh::ARRAY_INDEX];
    if (index_array.is_empty()) {
        for (int i = 0; i < vertex_array.size(); ++i) {
            Vector3 godot_vertex = vertex_array[i];
            // Swapping Y and Z coordinates, and negating the new Y
            point3d converted_vertex(godot_vertex.x, -godot_vertex.z, godot_vertex.y);
            vertices.push_back(converted_vertex);
        }
    } else {
        for (int i = 0; i < index_array.size(); ++i) {
            int index = index_array[i];
            if (index >= 0 && index < vertex_array.size()) {
                Vector3 godot_vertex = vertex_array[index];
                // Swapping Y and Z coordinates, and negating the new Y
                point3d converted_vertex(godot_vertex.x, -godot_vertex.z, godot_vertex.y);
                vertices.push_back(converted_vertex);
            } else {
                UtilityFunctions::printerr("ERROR: Index out of bounds in the vertex array.");
                return std::vector<point3d>();
            }
        }
    }
    return vertices;
}

const std::vector<std::vector<unsigned int>> godot::MeshMorph3D::extract_triangles(Ref<ArrayMesh> mesh) {
	std::vector<std::vector<unsigned int>> triangles;
	// Assuming mesh has only one surface
	Array arrays = mesh->surface_get_arrays(0);
	PackedInt32Array index_array = arrays[Mesh::ARRAY_INDEX];
	for (int i = 0; i < index_array.size(); i += 3) {
		if (i + 2 < index_array.size()) {
			std::vector<unsigned int> triangle;
			triangle.push_back(index_array[i]);
			triangle.push_back(index_array[i + 1]);
			triangle.push_back(index_array[i + 2]);
			triangles.push_back(triangle);
		}
	}
	return triangles;
}
bool godot::MeshMorph3D::open_obj_file(const String &filename, std::vector<point3d> &vertices, std::vector<std::vector<unsigned int>> &faces) {
    Ref<FileAccess> file = FileAccess::open(filename, FileAccess::READ);
    if (file.is_null()) {
        UtilityFunctions::print(String("Cannot open file: ") + filename);
        return false;
    }

    // Read the entire file content into a string
    String file_content = file->get_as_text();
    PackedStringArray lines = file_content.split("\n");
    UtilityFunctions::print(String("Lines of file: " + itos(lines.size()) + "\n"));
    for (int i = 0; i < lines.size(); ++i) {
        String line = lines[i].strip_edges();
        if (line.begins_with("v ")) {
            // Parse vertex line
            PackedStringArray vertex_parts = line.substr(2).split(" ");
            double x = String(vertex_parts[0]).to_float();
            double y = String(vertex_parts[1]).to_float();
            double z = String(vertex_parts[2]).to_float();
            vertices.push_back(point3d(x, y, z));
        } else if (line.begins_with("f ")) {
            // Parse face line
            PackedStringArray face_parts = line.substr(2).split(" ");
            std::vector<unsigned int> face_indices;
            for (int j = 0; j < face_parts.size(); ++j) {
                int index = face_parts[j].to_int() - 1; // OBJ indices are 1-based
                face_indices.push_back(index);
            }
            // Triangulate n-gon
            for (size_t k = 1; k < face_indices.size() - 1; ++k) {
                std::vector<unsigned int> triangle = {face_indices[0], face_indices[k], face_indices[k + 1]};
                faces.push_back(triangle);
            }
        }
    }

    file->close();
    return true;
}
