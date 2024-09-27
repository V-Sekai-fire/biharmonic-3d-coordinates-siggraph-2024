#include "include/BHC.h"
#include "include/BasicIO.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h> // Include glfw3.h after our OpenGL definitions

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

int main(int argc, char const *argv[]) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui - BHC Example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

	std::cout << " Coordinates " << std::endl;
	// Coordinates:
	std::vector<std::vector<double>> BHC_h_phi, BHC_h_psi, BHC_bh_phi, BHC_bh_psi; // unconstrained
	std::vector<std::vector<double>> BHConstrainedC_13_phi, BHConstrainedC_13_psi; // (1,3) version

	std::cout << " Compute (1,3)-regularized matrices " << std::endl;
	// Compute (1,3)-regularized matrices (depends only on the cage, not on the mesh):
	Eigen::MatrixXd ConstrainedBiH_13_C11;
	Eigen::MatrixXd ConstrainedBiH_13_C12;
	Eigen::MatrixXd ConstrainedBiH_13_C21;
	Eigen::MatrixXd ConstrainedBiH_13_C22;

    float gamma_D_13BC = 1.0;
    std::vector<point3d> cage_vertices, cage_modified_vertices, mesh_vertices, mesh_modified_vertices;
    std::vector<std::vector<unsigned int>> cage_triangles, mesh_triangles;
    std::vector<point3d> cage_triangle_normals;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Biharmonic Coordinates Control Panel");

        ImGui::Text("Adjust parameters and load/save operations:");
        ImGui::SliderFloat("Gamma D 13BC", &gamma_D_13BC, 0.0, 10.0);

        if (ImGui::Button("Load Cage")) {
            OBJIO::open("models/cage.obj", cage_vertices, cage_triangles, true);
        }
        if (ImGui::Button("Load Mesh")) {
            OBJIO::open("models/mesh.obj", mesh_vertices, mesh_triangles, true);
        }
        if (ImGui::Button("Deform Cage")) {
            OBJIO::open("models/cage_deformed.obj", cage_modified_vertices);
            cage_triangle_normals.resize(cage_triangles.size());
            for (unsigned int tIt = 0; tIt < cage_triangles.size(); ++tIt) {
                auto &t = cage_triangles[tIt];
                cage_triangle_normals[tIt] = point3d::cross(cage_modified_vertices[t[1]] - cage_modified_vertices[t[0]], cage_modified_vertices[t[2]] - cage_modified_vertices[t[0]]).direction();
            }
        }
        if (ImGui::Button("Update Mesh Deformation")) {
            std::cout << " Compute cage triangle normals " << std::endl;
            std::vector<point3d> cage_triangle_normals(cage_triangles.size(), point3d(0, 0, 0));
            for (unsigned int tIt = 0; tIt < cage_triangles.size(); ++tIt) {
                auto &t = cage_triangles[tIt];
                cage_triangle_normals[tIt] = point3d::cross(cage_modified_vertices[t[1]] - cage_modified_vertices[t[0]], cage_modified_vertices[t[2]] - cage_modified_vertices[t[0]]).direction();
            }
            std::cout << " Update the mesh deformation, from the cage deformation " << std::endl;
            std::vector<point3d> mesh_modified_vertices(mesh_vertices.size());
            #pragma omp parallel for
            for (int v = 0; v < mesh_vertices.size(); ++v) {
                point3d pos(0, 0, 0);
                for (unsigned int vc = 0; vc < cage_modified_vertices.size(); ++vc)
                    pos += BHConstrainedC_13_phi[v][vc] * cage_modified_vertices[vc];
                for (unsigned int tc = 0; tc < cage_triangles.size(); ++tc) {
                    pos += BHConstrainedC_13_psi[v][tc] * cage_triangle_normals[tc];
                }
                mesh_modified_vertices[v] = pos;
            }
        }
        if (ImGui::Button("Save Deformed Mesh")) {
            OBJIO::save("models/mesh_deformed.obj", mesh_modified_vertices, mesh_triangles);
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}