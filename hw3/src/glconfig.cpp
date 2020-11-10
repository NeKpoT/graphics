#include "glconfig.h"

#include <iostream>
#include <numeric>

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
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char *image = stbi_load(
        filename,
        //  "assets/test.png",
        &width,
        &height,
        &channels,
        STBI_rgb);

    std::cout << "Loading " << filename << ' ' << width << ' ' << height << ' ' << channels << std::endl;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image);
}

Material::Material(std::string texture_filename, GLfloat texture_a, GLfloat prism_n) : texture_a(texture_a), prism_n(prism_n) {
    glGenTextures(1, &texture);
    load_image(texture, texture_filename.c_str());
}
Material::Material(float color[], GLfloat texture_a, GLfloat prism_n)
    : texture_a(texture_a)
    , prism_n(prism_n) {
    color_[0] = color[0];
    color_[1] = color[1];
    color_[2] = color[2];
    texture = -1;
}

GLuint Material::get_texture() const {
    return texture;
}

glm::vec3 Material::get_color() const {
    return glm::vec3(color_[0], color_[1], color_[2]);
}

Mesh::Mesh(std::vector<float> vertices, std::vector<unsigned int> indices, std::vector<Material> materials, std::vector<size_t> attribs)
    : mats(materials) {
    vertex_count = vertices.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);

    size_t all_attr_len = std::accumulate(attribs.begin(), attribs.end(), 0);
    size_t prev_attr_len = 0;
    for (size_t attr_i = 0; attr_i < attribs.size(); attr_i++) {
        glVertexAttribPointer(attr_i, attribs[attr_i], GL_FLOAT, GL_FALSE, all_attr_len * sizeof(float), (void *)(prev_attr_len * sizeof(float)));
        glEnableVertexAttribArray(attr_i);
        prev_attr_len += attribs[attr_i];
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Mesh::Mesh(GLuint vbo, GLuint vao, GLuint ebo, std::vector<Material> materials, int vertex_count)
    : vbo(vbo)
    , vao(vao)
    , ebo(ebo)
    , mats(materials)
    , vertex_count(vertex_count) {}

void Mesh::draw(shader_t &shader) {
    for (int i = 0; i < mats.size(); i++) {
        const Material &mat = mats[i];

        glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_2D, mat.get_texture());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

        const std::string i_str = std::to_string(i);
        shader.set_uniform<int>("u_tex" + i_str, i + 1);
        shader.set_uniform<float>("u_texture_a" + i_str, mat.texture_a);
        shader.set_uniform<float>("u_prism_n" + i_str, mat.prism_n);

        shader.set_uniform<float>("u_color" + i_str, mat.get_color().x, mat.get_color().y, mat.get_color().z);
    }
    draw();
}

void Mesh::draw() {
    // Bind vertex array = buffers + indices
    glBindVertexArray(vao);
    // Execute draw call
    glDrawElements(GL_TRIANGLES, vertex_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

glm::vec3 to_tor(glm::vec3 pos, float R, float r) {
    float long_a = pos.x * 2 * M_PI;
    float lat_a = pos.y * 2 * M_PI;

    r += pos.z;
    float radius = R - r * cos(lat_a);
    return glm::vec3(
        cos(long_a) * radius,
        sin(long_a) * radius,
        r * sin(lat_a));
}

Mesh genTriangulation(unsigned int width, unsigned int heigth) {
    std::vector<float> vertices;
    for (unsigned int xi = 0; xi < width; xi++) {
        for (unsigned int yi = 0; yi < heigth; yi++) {
            float xf = (1.0f * xi) / width;
            float yf = (1.0f * yi) / heigth;
            float xt = (1.0f * (xi + 1)) / width;
            float yt = (1.0f * (yi + 1)) / heigth;
            std::vector<float> v = {
                xf, yf,
                xf, yt,
                xt, yt,
                xf, yf,
                xt, yt,
                xt, yf
            };
            std::copy(v.begin(), v.end(), std::back_insert_iterator<std::vector<float>>(vertices));
        }
    }

    std::vector<unsigned int> indices;
    for (size_t i = 0; i * 2 < vertices.size(); i++) {
        indices.push_back(i);
    }

    return Mesh(vertices, indices, std::vector<Material>(), { 2 });
}

Shadow::Shadow(size_t width, size_t height)
    : width(width)
    , height(height) {

    glGenFramebuffers(1, &depth_buffer);

    glGenTextures(1, &shadow_map);
    glBindTexture(GL_TEXTURE_2D, shadow_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, depth_buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Shadow::set_shadow(glm::mat4 light_view) {
    view = light_view;

    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, depth_buffer);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Shadow::unset_shadow() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Shadow::get_shadow_map() {
    return shadow_map;
}