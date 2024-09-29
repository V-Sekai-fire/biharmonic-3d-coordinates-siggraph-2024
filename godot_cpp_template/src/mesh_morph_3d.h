#ifndef MESH_MORPH_3D_H
#define MESH_MORPH_3D_H

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "point3.h"

namespace godot {

class MeshMorph3D : public MeshInstance3D {
    GDCLASS(MeshMorph3D, MeshInstance3D)
private:
    Ref<ArrayMesh> deform_mesh;
    std::vector<point3d> original_cage_vertices;
    std::vector<point3d> modified_cage_vertices;
    float gamma_D_13BC = 1.0;

	std::vector<point3d> convert_godot_array_to_vector(const Array &godot_array);
	std::vector<point3d> extract_vertices(Ref<ArrayMesh> mesh);
	const std::vector<std::vector<unsigned int>> extract_triangles(Ref<ArrayMesh> mesh);
	std::vector<point3d> calculate_normals(Ref<ArrayMesh> mesh);
	std::vector<point3d> apply_deformation(const std::vector<point3d> &original_vertices, const std::vector<std::vector<unsigned int>> &cage_triangles, const std::vector<point3d> &cage_vertices, const std::vector<point3d> &cage_modified_vertices, std::vector<point3d> &cage_triangle_normals);
	bool open_obj_file(const String &filename, std::vector<point3d> &vertices, std::vector<std::vector<unsigned int>> &faces);

protected:
	static void _bind_methods();
public:

    MeshMorph3D();
    ~MeshMorph3D();

    void _init();

    void set_deform_mesh(Ref<ArrayMesh> p_mesh) {
        deform_mesh = p_mesh;
        apply_deformation_to_children();
    }

    Ref<ArrayMesh> get_deform_mesh() const {
        return deform_mesh;
    }

    void apply_deformation_to_children();

    void set_gamma_D_13BC(float value) { gamma_D_13BC = value; }
    float get_gamma_D_13BC() const { return gamma_D_13BC; }
};
}

#endif // MESH_MORPH_3D_H
