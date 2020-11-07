#include "glconfig.h"

#include <iostream>

#include <fmt/format.h>

// Imgui + bindings
#include "imgui.h"

#include "../bindings/imgui_impl_glfw.h"
#include "../bindings/imgui_impl_opengl3.h"

#include "tiny_obj_loader.h"

static void glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

GLFWwindow* init_window() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return NULL;
    std::cerr << "glfw init\n";

    // GL 3.3 + GLSL 330
    const char *glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    std::cerr << "glfw window hint\n";

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Dear ImGui - Conan", NULL, NULL);
    std::cerr << "glfw create window\n";
    if (window == NULL) {
        std::cerr << "failed to create window\n";
        return NULL;
    }
    glfwMakeContextCurrent(window);

    std::cerr << "glfwMakeContextCurrent\n";
    glfwSwapInterval(1); // Enable vsync

    std::cerr << "glfw window\n";

    // Initialize GLEW, i.e. fill all possible function pointers for current OpenGL context
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize OpenGL loader!\n";
        return NULL;
    }
    std::cerr << "glew init\n";

    return window;
}

void setup_imgui(GLFWwindow *window) {
    // Setup GUI context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char *glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();
}

void load_image(GLuint &texture, const char* filename) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *image = stbi_load(
        filename,
        //  "assets/test.png",
        &width,
        &height,
        &channels,
        STBI_rgb);

    std::cout << width << height << channels;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image);
}


Material::Material() {
    glGenTextures(1, &texture);
}

bool Material::load(tinyobj::material_t mat, std::string path_to_textures) {
    load_image(texture, (path_to_textures + mat.diffuse_texname).c_str());
}

GLuint Material::get_texture() const {
    return texture;
}


Mesh::Mesh(std::vector<float> vertices, std::vector<unsigned int> indices, Material material) : mat(material) {
    vertiex_count = vertices.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::draw() {

    // Bind vertex array = buffers + indices
    glBindVertexArray(vao);
    // Execute draw call
    glDrawElements(GL_TRIANGLES, vertiex_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
