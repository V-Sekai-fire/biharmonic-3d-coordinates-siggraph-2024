#ifndef MESH_MORPH_3D_H
#define MESH_MORPH_3D_H

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include "point3.h"

namespace godot {

class MeshMorph3D : public Node3D {
    GDCLASS(MeshMorph3D, Node3D)
private:
    Ref<ArrayMesh> cage_mesh;
    std::vector<point3d> original_vertices;
    std::vector<point3d> modified_vertices;
    float gamma_D_13BC = 1.0;

    std::vector<point3d> convert_godot_array_to_vector(const Array& godot_array) {
        std::vector<point3d> vec;
        vec.reserve(godot_array.size());
        for (int i = 0; i < godot_array.size(); ++i) {
            Vector3 v = godot_array[i];
            vec.emplace_back(point3d(v.x, v.y, v.z));
        }
        return vec;
    }

    std::vector<point3d> extract_vertices(Ref<ArrayMesh> mesh) {
        std::vector<point3d> vertices;
        // Assuming mesh has only one surface
        Array arrays = mesh->surface_get_arrays(0);
        PackedVector3Array vertex_array = arrays[Mesh::ARRAY_VERTEX];
        for (int i = 0; i < vertex_array.size(); ++i) {
            vertices.push_back(vertex_array[i]);
        }
        return vertices;
    }

    const std::vector<std::vector<unsigned int>> extract_triangles(Ref<ArrayMesh> mesh) {
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

    std::vector<point3d> calculate_normals(Ref<ArrayMesh> mesh) {
        std::vector<point3d> normals;
        int surface_count = mesh->get_surface_count();
        for (int i = 0; i < surface_count; ++i) {
            Ref<SurfaceTool> st;
            st.instantiate();
            st->create_from(mesh, i);
            st->generate_normals();
            Ref<ArrayMesh> temp_mesh = st->commit();
            Array arrays = temp_mesh->surface_get_arrays(0);
            PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];
            PackedVector3Array computed_normals = arrays[Mesh::ARRAY_NORMAL];
            for (int j = 0; j < computed_normals.size(); ++j) {
                normals.push_back(computed_normals[j]);
            }
        }
        return normals;
    }
	std::vector<point3d> apply_deformation(const std::vector<point3d> &original_vertices, const std::vector<std::vector<unsigned int>> &cage_triangles, const std::vector<point3d> &cage_vertices, const std::vector<point3d> &cage_modified_vertices, std::vector<point3d> &cage_triangle_normals);
	void update_mesh(Ref<ArrayMesh> mesh, const std::vector<point3d>& vertices) {
        Ref<SurfaceTool> st = memnew(SurfaceTool);
        st->begin(Mesh::PRIMITIVE_TRIANGLES);
        for (const point3d& p : vertices) {
            Vector3 vertex(p[0], p[1], p[2]);
            st->add_vertex(vertex);
        }
        mesh = st->commit(mesh);
    }
protected:
	static void _bind_methods();
public:

    MeshMorph3D();
    ~MeshMorph3D();

    void _init();

    void set_cage_mesh(Ref<ArrayMesh> p_mesh);
    Ref<ArrayMesh> get_cage_mesh() const;

    void update_vertex_positions(const PackedVector3Array &new_positions);
    void apply_deformation_to_children();

    void set_gamma_D_13BC(float value) { gamma_D_13BC = value; }
    float get_gamma_D_13BC() const { return gamma_D_13BC; }
};
}

#endif // MESH_MORPH_3D_H
