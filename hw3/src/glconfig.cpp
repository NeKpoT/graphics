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

void Material::load(std::string texture_filename) {
    load_image(texture, texture_filename.c_str());
}

void Material::load(tinyobj::material_t mat, std::string path_to_textures) {
    load(path_to_textures + mat.diffuse_texname);
}

GLuint Material::get_texture() const {
    return texture;
}

Mesh::Mesh(std::vector<float> vertices, std::vector<unsigned int> indices, Material material, std::vector<size_t> attribs)
    : mat(material) {
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

Mesh::Mesh(GLuint vbo, GLuint vao, GLuint ebo, Material material, int vertex_count)
    : vbo(vbo)
    , vao(vao)
    , ebo(ebo)
    , mat(material)
    , vertex_count(vertex_count) {}

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
    float R = 3, r = 1;

    std::vector<float> vertices;
    for (unsigned int xi = 0; xi < width; xi++) {
        for (unsigned int yi = 0; yi < heigth; yi++) {
            float xf = (1.0f * xi) / width;
            float yf = (1.0f * yi) / heigth;
            float xt = (1.0f * (xi + 1)) / width;
            float yt = (1.0f * (yi + 1)) / heigth;
            std::vector<glm::vec2> vs = {
                { xf, yf },
                { xf, yt },
                { xt, yt },
                { xf, yf },
                { xt, yf },
                { xt, yt }
            };
            for (const glm::vec2 &p : vs) {
                glm::vec3 out_normal;
                glm::vec3 out_position;
                glm::vec2 out_texcoord;

                out_texcoord = p;

                glm::vec3 position = glm::vec3(p, 0.0);
                out_position = to_tor(position, R, r);
                out_normal = glm::normalize(to_tor(position + glm::vec3(0, 0, 1), R, r) - out_position);

                std::vector<float> v = {
                    out_position.x,
                    out_position.y,
                    out_position.z,
                    out_normal.x,
                    out_normal.y,
                    out_normal.z,
                    out_texcoord.x,
                    out_texcoord.y,
                };
                std::copy(v.begin(), v.end(), std::back_insert_iterator<std::vector<float>>(vertices));
            }
        }
    }

    std::vector<unsigned int> indices;
    for (size_t i = 0; i * 8 < vertices.size(); i++) {
        indices.push_back(i);
    }

    Material mat;
    mat.load("assets/test.png");
    return Mesh(vertices, indices, mat, { 3, 3, 2 });
}

Mesh genTriangulation2(unsigned int width, unsigned int heigth) {
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
                xt, yf,
                xt, yt
            };
            std::copy(v.begin(), v.end(), std::back_insert_iterator<std::vector<float>>(vertices));
        }
    }

    std::vector<unsigned int> indices;
    for (size_t i = 0; i * 2 < vertices.size(); i++) {
        indices.push_back(i);
    }

    Material mat;
    mat.load("assets/test.png");
    return Mesh(vertices, indices, mat, { 2 });
}
