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

#include "droplet.h"
#include "glconfig.h"
#include "opengl_shader.h"

// STB, load images
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// obj loader implementation
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

const double FPS_CAP = 144;
const double FRAME_DURATION_NSECONDS = 1e9 / FPS_CAP;
const int FRAMES_MEASURE = 10;

int fps = -1;

void wait_fps_cap() {
    using namespace std::chrono;

    static steady_clock::time_point last_frames_times[FRAMES_MEASURE];
    static int last_frame_pos = -2;
    static int current_frame_pos = -1;

    if (last_frame_pos == -1) {
        last_frame_pos = 0;
        current_frame_pos = 1;
    } else {
        ;
        steady_clock::duration frame_duration = std::chrono::steady_clock::now() - last_frames_times[last_frame_pos];
        steady_clock::duration frame_min_duration = nanoseconds((int)FRAME_DURATION_NSECONDS);

        if (frame_duration < frame_min_duration) {
            std::this_thread::sleep_for(frame_min_duration - frame_duration);
        }
    }

    auto time_now = std::chrono::steady_clock::now();
    fps = 1e9 / nanoseconds(time_now - last_frames_times[current_frame_pos]).count() * FRAMES_MEASURE;

    last_frames_times[current_frame_pos] = time_now;
    last_frame_pos = current_frame_pos;
    current_frame_pos = (current_frame_pos + 1) % FRAMES_MEASURE;
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
    for (auto &c : skybox_vertices)
        c *= 30;

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
std::vector<Mesh> create_object(const tinyobj::attrib_t &attrib, const std::vector<tinyobj::shape_t> &shapes, const std::vector<Material> &materials) {

    std::vector<Mesh> meshes;
    meshes.reserve(shapes.size());

    for (const tinyobj::shape_t &shape : shapes) {
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

            glm::vec3 vert[3];
            for (size_t vi = 0; vi < 3; vi++) {
                size_t pos = 3 * shape.mesh.indices[index + vi].vertex_index;
                vert[vi].x = attrib.vertices[pos + 0];
                vert[vi].y = attrib.vertices[pos + 1];
                vert[vi].z = attrib.vertices[pos + 2];
            }
            glm::vec3 def_normal = glm::normalize(glm::cross(vert[2] - vert[0], vert[1] - vert[0]));

            for (size_t vi = 0; vi < face_size; vi++, index++) {
                indices.push_back(vertex_count++);
                tinyobj::index_t idx = shape.mesh.indices[index];

                auto start = attrib.vertices.begin() + 3 * shape.mesh.indices[index].vertex_index;
                std::copy(start, start + 3, std::back_insert_iterator<std::vector<float>>(vertices));
                if (!attrib.normals.empty()) {
                    auto start2 = attrib.normals.begin() + 3 * shape.mesh.indices[index].normal_index;
                    std::copy(start2, start2 + 3, std::back_insert_iterator<std::vector<float>>(vertices));
                } else {
                    // use default normal
                    vertices.push_back(def_normal.x);
                    vertices.push_back(def_normal.y);
                    vertices.push_back(def_normal.z);
                }

                tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                vertices.push_back(tx);
                vertices.push_back(ty);
            }
        }

        meshes.emplace_back(vertices, indices, std::vector<Material> { materials[shape.mesh.material_ids[0]] }, std::vector<size_t> { 3, 3, 2 });
    }

    return meshes;
}

void load_cubemap(GLuint &texture) {
    std::string filenames[] = {
        "assets/Bridge/posx.jpg",
        "assets/Bridge/negx.jpg",
        "assets/Bridge/posy.jpg",
        "assets/Bridge/negy.jpg",
        "assets/Bridge/posz.jpg",
        "assets/Bridge/negz.jpg"

        // "assets/SaintLazarusChurch/posx.jpg",
        // "assets/SaintLazarusChurch/negx.jpg",
        // "assets/SaintLazarusChurch/posy.jpg",
        // "assets/SaintLazarusChurch/negy.jpg",
        // "assets/SaintLazarusChurch/posz.jpg",
        // "assets/SaintLazarusChurch/negz.jpg"

        // "assets/sky/px.png",
        // "assets/sky/nx.png",
        // "assets/sky/py.png",
        // "assets/sky/ny.png",
        // "assets/sky/pz.png",
        // "assets/sky/nz.png"

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

std::vector<Mesh> load_object(std::string path, std::string filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn, err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) { throw "failed loading"; }

    std::vector<Material> mats;
    for (size_t i = 0; i < materials.size(); i++) {
        auto &material = materials[i];
        if (material.diffuse_texname != "") {
            mats.emplace_back(std::string(path) + material.diffuse_texname);
        } else {
            mats.emplace_back(material.diffuse);
        }
    }

    return create_object(attrib, shapes, mats);
}

const float SCROLL_STEP = 0.05;
const float MOUSE_SENS = 0.005;

float rotation = 0;
float pitch = 0;
glm::vec3 player_pos = glm::vec3(0, 0.7, 0);
glm::vec2 player_flat_dir(1.0, 0.0);
glm::vec3 player_look_dir(1, 0, 0);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    player_pos.x += yoffset * SCROLL_STEP * player_flat_dir.x;
    player_pos.z += yoffset * SCROLL_STEP * player_flat_dir.y;

    crop_abs(player_pos.x, 20.0f);
    crop_abs(player_pos.y, 20.0f);
}

void mouse_moved(GLFWwindow *window, double xoffset, double yoffset) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {

        rotation -= xoffset * MOUSE_SENS;
        pitch += yoffset * MOUSE_SENS;
        crop_interval(pitch, -0.499f, 0.499f);
        while (rotation > 2)
            rotation -= 2;
        while (rotation < 0)
            rotation += 2;

        player_flat_dir.x = cosf(rotation * M_PI);
        player_flat_dir.y = sinf(rotation * M_PI);
        player_look_dir = glm::vec3(player_flat_dir * cosf(pitch * M_PI), sinf(pitch * M_PI));
        std::swap(player_look_dir.y, player_look_dir.z);
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    static float cursor_position[] = { 0.0, 0.0 };
    mouse_moved(window, xpos - cursor_position[0], ypos - cursor_position[1]);
    cursor_position[0] = xpos;
    cursor_position[1] = ypos;
}

Mesh ground() {
    float dist = 30;
    std::vector<float> v;
    for (int dx = -1; dx <= 1; dx += 2) {
        for (int dy = -1; dy <= 1; dy += 2) {
            v.push_back(dx * dist);
            v.push_back(0);
            v.push_back(dy * dist);

            v.push_back(0);
            v.push_back(1);
            v.push_back(0);

            v.push_back((dx + 1) * dist * 5);
            v.push_back((dy + 1) * dist * 5);
        }
    }

    std::vector<unsigned int> indices { 0, 1, 2, 2, 3, 1 };
    std::reverse(indices.begin(), indices.end());
    std::vector<Material> mats = { Material("assets/bonsai/soil.jpg") };

    return Mesh(v, indices, mats, std::vector<size_t> { 3, 3, 2 });
}

int main(int, char **) {
    GLFWwindow *window = init_window();

    // std::vector<Mesh> tent_meshes = load_object("assets/tent/", "Market.obj");
    std::vector<Mesh> tent_meshes = load_object("assets/black_smith/", "black_smith.obj");
    glm::vec3 tent_position(0, 1.3, 0);

    // std::vector<Mesh> all_ground_meshes22 = load_object("assets/bonsai/", "bonsai-tree.obj");
    std::vector<Mesh> all_ground_meshes = { ground() };
    std::vector<Mesh *> ground_meshes = { &all_ground_meshes[0] };

    GLuint cubemap_texture;
    load_cubemap(cubemap_texture);

    GLuint droplet_tex;
    load_image(droplet_tex, "assets/droplet.png");

    // create our geometries
    GLuint vbo, vao, ebo;
    create_skybox(vbo, vao, ebo);

    // init shaders
    shader_t skybox_shader("assets/skybox.vs", "assets/skybox.fs");
    shader_t droplet_shader("assets/droplets.vs", "assets/droplets.fs", "assets/droplets.gs");
    shader_t object_shader("assets/object.vs", "assets/object.fs");
    shader_t id_shader("assets/id.vs", "assets/id.fs");

    std::cerr << "shaders done\n";

    setup_imgui(window);

    glfwSetScrollCallback(window, &scroll_callback);
    glfwSetCursorPosCallback(window, &mouse_callback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto const start_time = std::chrono::steady_clock::now();

    float r = 0.7, R = 5, height_mult = 0.8, badrock_height = 0.3;

    Shadow sun_shadow = Shadow(2048 * 4, 2048 * 4);

    Shadow height_map = Shadow(512, 512);

    float max_radius = 50;

    // controls
    static float fovy = 90;
    static float sun_speed_log = -20;
    static float sun_start_a = 3.4;
    static float camera_radius_mult = 1;
    static float droplet_speed = 7.0;

    float rain_tile_size = 7;
    float rain_height = 5;
    Droplets droplets(700 * 4, rain_tile_size, rain_height);

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        // Get windows size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        float const time_from_start = (float)(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count() / 1000.0);

        glm::mat4 sun_rotation = glm::rotate(
            sun_start_a + time_from_start * 2 * float(M_PI) * powf(2, sun_speed_log),
            glm::vec3(-1.0f, 1.0f, 1.0f));

        glm::vec3 sun_position = glm::vec3(sun_rotation * glm::normalize(-glm::vec4(1.0f, 1.0f, 1.0f, 0)));
        // glm::vec3 sun_position = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));

        glm::vec3 camera_position = player_pos;
        glm::vec3 camera_forward = player_look_dir;
        glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 camera_lookat = camera_position + camera_forward;

        glm::mat4 tent_model = glm::translate(tent_position) * glm::scale(glm::vec3(1.0f, 1.0f, 1.0f) * 1.5f);
        tent_model[3][3] = 1;

        glm::mat4 ground_model = glm::translate(glm::vec3(0, 0, 0)) * glm::scale(glm::vec3(1.0f, 1.0f, 1.0f) * 10.0f);

        auto model
            = glm::mat4(1);
        auto view = glm::lookAt<float>(
            camera_position,
            camera_lookat,
            camera_up);
        auto projection = glm::perspective<float>(glm::radians(fovy), float(display_w) / display_h, 0.1, 100);
        // std::cerr << fovy << std::endl;
        auto mvp = projection * view * model;
        auto vp = projection * view;
        auto mvp_no_translation = projection * glm::mat4(glm::mat3(view * model));

        auto tent_mvp = projection * view * tent_model;
        auto ground_mvp = projection * view * ground_model;

        // get sun shadow
        sun_shadow.set_shadow(
            glm::ortho(-max_radius, max_radius, -max_radius, max_radius, -max_radius, max_radius) * glm::lookAt(sun_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)));
        
        id_shader.use();
        {
            glm::mat4 shadow_mvp = sun_shadow.view * tent_model;
            id_shader.set_uniform("u_mvp", glm::value_ptr(shadow_mvp));
            for (Mesh &mesh : tent_meshes) {
                mesh.draw();
            }
            shadow_mvp = sun_shadow.view * ground_model;
            id_shader.set_uniform("u_mvp", glm::value_ptr(shadow_mvp));
            for (Mesh *mesh : ground_meshes) {
                mesh->draw();
            }
        }
        sun_shadow.unset_shadow();

        height_map.set_shadow(
            glm::ortho(-max_radius, max_radius, -max_radius, max_radius, -max_radius, max_radius) * glm::lookAt(glm::vec3(0, 10, 0), glm::vec3(0, 0, 0), glm::vec3(1, 0, 0)));

        id_shader.use();
        {
            glm::mat4 shadow_mvp = height_map.view * tent_model;
            id_shader.set_uniform("u_mvp", glm::value_ptr(shadow_mvp));
            for (Mesh &mesh : tent_meshes) {
                mesh.draw();
            }
            shadow_mvp = height_map.view * ground_model;
            id_shader.set_uniform("u_mvp", glm::value_ptr(shadow_mvp));
            for (Mesh *mesh : ground_meshes) {
                mesh->draw();
            }
        }
        height_map.unset_shadow();

        // start actual drawing

        // Set viewport to fill the whole window area
        glViewport(0, 0, display_w, display_h);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        // Gui start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // GUI
        ImGui::Begin("GUI");
        ImGui::Text("FPS: %d", fps);
        ImGui::SliderFloat("fovy", &fovy, 1, 180);
        ImGui::SliderFloat("sun start a", &sun_start_a, 0, 2 * M_PI);
        ImGui::SliderFloat("sun speed log", &sun_speed_log, -20, 0);
        ImGui::SliderFloat("camera radius", &camera_radius_mult, 0.5, 5);

        ImGui::SliderFloat("droplet speed", &droplet_speed, 0.1, 30);
        droplets.speed = droplet_speed;

        static bool texture_gamma_correction = true;
        ImGui::Checkbox("texture gamma correction", &texture_gamma_correction);

        static bool blend_gamma_correction = true;
        ImGui::Checkbox("blend gamma correction", &blend_gamma_correction);

        ImGui::End();

        skybox_shader.use();
        skybox_shader.set_uniform("u_mvp", glm::value_ptr(mvp_no_translation));
        skybox_shader.set_uniform("u_cube", int(0));
        skybox_shader.set_uniform("dl_num", 1);
        skybox_shader.set_uniform("dl_dir", sun_position);
        skybox_shader.set_uniform("dl_light", glm::vec3(1.0f, 1.0f, 1.0f));

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

        // draw tent

        sun_shadow.bind_shadow_texture(10);

        auto pass_everything_lambda = [&](shader_t &shader) {
            shader.set_uniform("u_cam", camera_position.x, camera_position.y, camera_position.z);

            shader.set_uniform("u_cube", int(0));

            shader.set_uniform("u_tex_gamma_correct", texture_gamma_correction);
            shader.set_uniform("u_blend_gamma_correct", blend_gamma_correction);

            shader.set_uniform("dl_num", 1);
            shader.set_uniform("dl_dir", sun_position);
            shader.set_uniform("dl_light", glm::vec3(1.0f, 1.0f, 1.0f));
            shader.set_uniform("dl_depth", 10);
            shader.set_uniform("dl_vp", glm::value_ptr(sun_shadow.view));

            shader.set_uniform("pd_num", 0);
            // shader.set_uniform("pd_dir", torch_dir);
            // shader.set_uniform("pd_pos", torch_pos);
            // shader.set_uniform("pd_light", glm::vec3(1.0f, 1.0f, 0.5f) * 0.5f);
            // shader.set_uniform("pd_angle", 0.6f);
            // shader.set_uniform("pd_depth", 11);
            // shader.set_uniform("pd_vp", glm::value_ptr(torch_shadow.view));

            shader.set_uniform("background_light", glm::vec3(1, 1, 1) * 0.8f);
        };

        object_shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        pass_everything_lambda(object_shader);

        object_shader.set_uniform("u_m", glm::value_ptr(tent_model));
        object_shader.set_uniform("u_mvp", glm::value_ptr(tent_mvp));
        object_shader.set_uniform<float>("u_tile", 1, 1);
        // object_shader.set_uniform("background_light", 0.7f, 0.7f, 0.7f);

        for (Mesh &mesh : tent_meshes) {
            mesh.draw(object_shader);
        }

        object_shader.set_uniform("u_m", glm::value_ptr(ground_model));
        object_shader.set_uniform("u_mvp", glm::value_ptr(ground_mvp));
        for (Mesh *mesh : ground_meshes) {
            mesh->draw(object_shader);
        }

        // draw droplets

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, droplet_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

        droplet_shader.use();

        
        height_map.bind_shadow_texture(11);
        droplet_shader.set_uniform("u_height_tex", 11);
        droplet_shader.set_uniform("u_height_view", glm::value_ptr(height_map.view));
        sun_shadow.bind_shadow_texture(12);
        droplet_shader.set_uniform("u_shadow_tex", 12);
        droplet_shader.set_uniform("u_shadow_view", glm::value_ptr(sun_shadow.view));

        droplet_shader.set_uniform("u_mvp", glm::value_ptr(mvp));
        droplet_shader.set_uniform("tile_size", rain_tile_size);
        droplet_shader.set_uniform("camera_position", camera_position);
        droplet_shader.set_uniform("camera_forward", camera_forward);
        droplet_shader.set_uniform("u_droplet_tex", 1);
        
        droplets.draw(droplet_shader, time_from_start);

        // Generate gui render commands
        ImGui::Render();

        // Execute gui render commands using OpenGL backend
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap the backbuffer with the frontbuffer that is used for screen display
        glfwSwapBuffers(window);

        wait_fps_cap();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
