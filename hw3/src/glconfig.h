#pragma once


#include <GL/glew.h>

// Imgui + bindings
// #include "imgui.h"

// #include "../bindings/imgui_impl_glfw.h"
// #include "../bindings/imgui_impl_opengl3.h"

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// STB, load images
#include <stb_image.h>

// Math constant and routines for OpenGL interop
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>


// obj loader
// #define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "opengl_shader.h"

GLFWwindow* init_window();

void setup_imgui(GLFWwindow* window);

void load_image(GLuint& texture, const char* filename);

template <typename T>
void crop_interval(T& x, T minv, T maxv) {
    x = std::max(minv, std::min(maxv, x));
}

template <typename T>
void crop_abs(T& x, T max_abs) {
    crop_interval(x, -max_abs, max_abs);
}

class Material {
  public:
    Material();

    void load(std::string texture_filename);
    void load(tinyobj::material_t mat, std::string path_to_textures = "");

    GLuint get_texture() const;

  private:
    GLuint texture;
};

class Mesh {
  public:
    Mesh(std::vector<float> vertices, std::vector<unsigned int> indices, Material material, std::vector<size_t> attribs);
    Mesh(GLuint vbo, GLuint vao, GLuint ebo, Material material, int vertex_count);

    void draw();

    const Material mat;
  private:
    GLuint vbo, vao, ebo;

    int vertex_count;
};

Mesh genTriangulation(unsigned int width, unsigned int heigth);

Mesh genTorLandscape(const unsigned char* image, unsigned int width, unsigned int heigth);