#pragma optimize("", off)

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <fmt/format.h>

#include <GL/glew.h>

// Imgui + bindings
#include "imgui.h"

#include "../bindings/imgui_impl_glfw.h"
#include "../bindings/imgui_impl_opengl3.h"

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// Math constant and routines for OpenGL interop
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>


#include "opengl_shader.h"
#include "glconfig.h"

// STB, load images
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// obj loader implementation
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

const double FPS_CAP = 60;
const double FRAME_TIME_NSECONDS = 1e9 / FPS_CAP;

void wait_fps_cap(std::chrono::steady_clock::time_point start_time) {
    using namespace std::chrono;
    steady_clock::duration frame_time = steady_clock::now() - start_time;
    steady_clock::duration frame_min_time = nanoseconds((int)FRAME_TIME_NSECONDS);

    if (frame_time < frame_min_time) {
        std::this_thread::sleep_for(frame_min_time - frame_time);
    }
}

static void glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

void create_skybox(GLuint &vbo, GLuint &vao, GLuint &ebo) {
    // create the triangle
    float skybox_vertices[8 * 3];
    for (int xi = 0; xi <= 1; xi++) {
        for (int yi = 0; yi <= 1; yi++) {
            for (int zi = 0; zi <= 1; zi++) {
                float *vertex = skybox_vertices + (xi * 4 + yi * 2 + zi) * 3;
                vertex[0] = xi * 2 - 1;
                vertex[1] = yi * 2 - 1;
                vertex[2] = zi * 2 - 1;
            }
        }
    }

    std::vector<unsigned int> indices;
    unsigned int mults[] = { 1, 2, 4 };
    for (int ax = 0; ax < 3; ax++) {
        unsigned int m[3];
        for (int i = 0; i < 3; i++) m[i] = mults[(ax + i) % 3];

        for (int add = 0; add <= 1; add++) {
            indices.push_back(add * m[0]);
            indices.push_back(add * m[0] + m[1]);
            indices.push_back(add * m[0] + m[2]);

            indices.push_back(add * m[0] + m[1] + m[2]);
            indices.push_back(add * m[0] + m[2]);
            indices.push_back(add * m[0] + m[1]);
        }
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

/// @returns number of triangles
std::vector<Mesh> create_object
    ( const tinyobj::attrib_t &attrib
    , const std::vector<tinyobj::shape_t> &shapes
    , const std::vector<Material> &materials) {

    std::vector<Mesh> meshes;

    for (const tinyobj::shape_t& shape : shapes) {
        size_t vertex_count = 0;
        size_t face_count = 0;
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        
        size_t index = 0;
        for (auto face_size : shape.mesh.num_face_vertices) {
            if (face_size != 3) {
                std::cerr << "!!! face size = " << face_size << " !!!\n";
                exit(1);
            }
            face_count++;
            for (size_t vi = 0; vi < face_size; vi++, index++) {
                indices.push_back(vertex_count++);
                tinyobj::index_t idx = shape.mesh.indices[index];

                auto start = attrib.vertices.begin() + 3 * shape.mesh.indices[index].vertex_index;
                std::copy(start, start + 3, std::back_insert_iterator<std::vector<float>>(vertices));
                auto start2 = attrib.normals.begin() + 3 * shape.mesh.indices[index].normal_index;
                std::copy(start2, start2 + 3, std::back_insert_iterator<std::vector<float>>(vertices));

                tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                vertices.push_back(tx);
                vertices.push_back(ty);
            }
        }

        meshes.emplace_back(vertices, indices, materials[shape.mesh.material_ids[0]]);
    }

    return meshes;
}

void load_cubemap(GLuint &texture) {
    std::string filenames[] = {
        // "assets/Bridge/posx.jpg",
        // "assets/Bridge/negx.jpg",
        // "assets/Bridge/posy.jpg",
        // "assets/Bridge/negy.jpg",
        // "assets/Bridge/posz.jpg",
        // "assets/Bridge/negz.jpg"

        "assets/SaintLazarusChurch/posx.jpg",
        "assets/SaintLazarusChurch/negx.jpg",
        "assets/SaintLazarusChurch/posy.jpg",
        "assets/SaintLazarusChurch/negy.jpg",
        "assets/SaintLazarusChurch/posz.jpg",
        "assets/SaintLazarusChurch/negz.jpg"

        // "assets/testcube/positive-x.tga",
        // "assets/testcube/negative-x.tga",
        // "assets/testcube/positive-y.tga",
        // "assets/testcube/negative-y.tga",
        // "assets/testcube/positive-z.tga",
        // "assets/testcube/negative-z.tga"
    };

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    stbi_set_flip_vertically_on_load(false);

    int width, height, channels;
    for (int i = 0; i < 6; i++) {
        unsigned char *image = stbi_load(filenames[i].c_str(),
                                         &width,
                                         &height,
                                         &channels,
                                         STBI_rgb);

        std::cout << "loaded " << width << " * " << height << " cubemap face" << std::endl;

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        stbi_image_free(image);
    }
}

bool load_object(tinyobj::attrib_t &attrib, std::vector<tinyobj::shape_t> &shapes, std::vector<tinyobj::material_t> &materials) {
    std::string warn, err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "reflex_camera.obj");

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    return ret;
}


static float pitch;
static float rotation;

const float SCROLL_STEP = 0.05;
const float MOUSE_SENS = 0.005;
static float radius = 1;

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    radius -= yoffset * SCROLL_STEP;
    crop_interval(radius, 0.1f, 10.0f);
}

void mouse_moved(GLFWwindow *window, double xoffset, double yoffset) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
        rotation += xoffset * MOUSE_SENS;
        pitch += yoffset * MOUSE_SENS;
        crop_abs(pitch, 0.5f);
        if (rotation > 2)
            rotation -= 2;
        if (rotation < 0)
            rotation += 2;
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    static float cursor_position[] = { 0.0, 0.0 };
    mouse_moved(window, xpos - cursor_position[0], ypos - cursor_position[1]);
    cursor_position[0] = xpos;
    cursor_position[1] = ypos;
}

int main(int, char **) {
    GLFWwindow *window = init_window();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    if (!load_object(attrib, shapes, materials)) {
        return 0;
    }

    std::vector<Material> mats(materials.size());
    for (size_t i = 0; i < materials.size(); i++) {
        mats[i].load(materials[i], "assets/reflex_camera/");
    }

    std::vector<Mesh> meshes = create_object(attrib, shapes, mats);

    GLuint cubemap_texture;
    load_cubemap(cubemap_texture);

    // create our geometries
    GLuint vbo, vao, ebo;
    create_skybox( vbo, vao, ebo);

    // init shaders
    shader_t skybox_shader("assets/skybox.vs", "assets/skybox.fs");
    shader_t object_shader("assets/object.vs", "assets/object.fs");

    std::cerr << "shaders done\n";

    setup_imgui(window);

    glfwSetScrollCallback(window, &scroll_callback);
    glfwSetCursorPosCallback(window, &mouse_callback);

    auto const start_time = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        // Get windows size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Set viewport to fill the whole window area
        glViewport(0, 0, display_w, display_h);

        // Fill background with solid color
        // glClearColor(0.30f, 0.55f, 0.60f, 1.00f);
        // glEnable(GL_CULL_FACE);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // Gui start new frame
        auto const frame_start_time = std::chrono::steady_clock::now();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // GUI
        ImGui::Begin("Triangle Position/Color");
        static float fovy = 90;
        ImGui::SliderFloat("fovy", &fovy, 10, 180);

        static float prism_n = 0.8;
        ImGui::SliderFloat("prism_n", &prism_n, 0.1, 1);

        static float texture_a = 0.2;
        ImGui::SliderFloat("texture alpha", &texture_a, 0, 1);

        static float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        ImGui::ColorEdit3("color", color);
        ImGui::End();

        // Pass the parameters to the shader as uniforms
        float const time_from_start = (float)(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count() / 1000.0);

        auto model = glm::rotate(
            glm::rotate(
                glm::mat4(1), 
                pitch * glm::pi<float>(), 
                glm::vec3(1, 0, 0)) * glm::scale(glm::vec3(1, 1, 1)),
            rotation * glm::pi<float>(), 
            glm::vec3(0, 1, 0)
        );
        auto view = glm::lookAt<float>(glm::vec3(0, 0, radius), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        auto projection = glm::perspective<float>(glm::radians(fovy), float(display_w) / display_h, 0.1, 100);
        auto mvp = projection * view * model;
        auto mvp_no_translation = projection * glm::mat4(glm::mat3(view * model));

        
        glm::vec3 camera_position = (glm::inverse(view * model) * glm::vec4(0, 0, 0, 1));

        skybox_shader.use();
        skybox_shader.set_uniform("u_mvp", glm::value_ptr(mvp_no_translation));
        skybox_shader.set_uniform("u_cube", int(0));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // Bind vertex array = buffers + indices
        glBindVertexArray(vao);
        // Execute draw call
        glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        
        for (Mesh& mesh : meshes) {

            object_shader.use();

            object_shader.set_uniform("u_mvp", glm::value_ptr(mvp));
            object_shader.set_uniform("u_cam", camera_position.x, camera_position.y, camera_position.z);

            object_shader.set_uniform("u_cube", int(1));
            object_shader.set_uniform("u_tex", int(0));

            object_shader.set_uniform("u_texture_a", texture_a);

            object_shader.set_uniform("u_prism_n", prism_n);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.mat.get_texture());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            mesh.draw();
        }

        // Generate gui render commands
        ImGui::Render();

        // Execute gui render commands using OpenGL backend
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap the backbuffer with the frontbuffer that is used for screen display
        glfwSwapBuffers(window);

        wait_fps_cap(frame_start_time);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
