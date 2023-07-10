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

#include "movement.h"

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
    for (auto& c : skybox_vertices)
        c *= 16;

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
    meshes.reserve(shapes.size());

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

        meshes.emplace_back(vertices, indices, std::vector<Material>{ materials[shape.mesh.material_ids[0]] }, std::vector<size_t>{3, 3, 2});
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

        // "assets/SaintLazarusChurch/posx.jpg",
        // "assets/SaintLazarusChurch/negx.jpg",
        // "assets/SaintLazarusChurch/posy.jpg",
        // "assets/SaintLazarusChurch/negy.jpg",
        // "assets/SaintLazarusChurch/posz.jpg",
        // "assets/SaintLazarusChurch/negz.jpg"

        "assets/sky/px.png",
        "assets/sky/nx.png",
        "assets/sky/py.png",
        "assets/sky/ny.png",
        "assets/sky/pz.png",
        "assets/sky/nz.png"

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

static float pitch = 0.2;
static float rotation;
static TorMovementModel *model_p = nullptr;

const float SCROLL_STEP = 0.05;
const float MOUSE_SENS = 0.005;
static float radius = 1;

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    radius -= yoffset * SCROLL_STEP;
    crop_interval(radius, 0.1f, 10.0f);

    if (model_p != nullptr) {
        model_p->move_forvard(yoffset > 0 ? 1 : -1);
    }
}

void mouse_moved(GLFWwindow *window, double xoffset, double yoffset) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
        if (model_p != nullptr) {
            model_p->rotate(xoffset * MOUSE_SENS);
        }

        rotation += xoffset * MOUSE_SENS;
        pitch += yoffset * MOUSE_SENS;
        crop_interval(pitch, -0.1f, 0.4f);
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

std::pair<Mesh, TorMovementModel> make_torus(
    unsigned int longitude_size, 
    unsigned int latitude_size, 
    float r, 
    float R, 
    float height_mult, 
    float badrock_height) {

    GLenum error_id;
    const size_t vertex_size = 9;

    // READ FILE
    std::stringstream file_stream;
    try {
        std::ifstream file("assets/landscape_gen.vs");
        file_stream << file.rdbuf();
    } catch (std::exception const &e) {
        std::cerr << "Error reading shader file: " << e.what() << std::endl;
    }

    std::string transformer_string = file_stream.str();
    const char *transformer_text = transformer_string.c_str();

    // MAKE A PROGRAM
    GLuint shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shader, 1, &transformer_text, nullptr);
    glCompileShader(shader);

    // check error
    int success;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "Error compiling Vertex shader_t:\n"
                  << infoLog << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);

    // CREATE BUFFERS
    const GLchar *result_name[] = { "out_position", "out_normal", "out_texcoord", "out_height" };
    glTransformFeedbackVaryings(program, 4, result_name, GL_INTERLEAVED_ATTRIBS);

    glLinkProgram(program);

    // GET RESOURCES
    Material moon_mat("assets/moon.jpg");
    Material steel_mat("assets/metal dec.jpg", 0.7);

    GLuint height_map, normal_map;
    glGenTextures(1, &height_map);
    load_image(height_map, "assets/testheight2.png");
    glGenTextures(1, &normal_map);
    load_image(normal_map, "assets/normal_map.png");

    // CREATE BUFFERS
    Mesh plane = genTriangulation(longitude_size, latitude_size);
    GLuint outbuff;
    glGenBuffers(1, &outbuff);
    glBindBuffer(GL_ARRAY_BUFFER, outbuff);
    size_t result_vertex_count = latitude_size * longitude_size * 6;
    size_t result_size = result_vertex_count * vertex_size;
    glBufferData(GL_ARRAY_BUFFER, result_size * sizeof(float), nullptr, GL_STATIC_READ);

    // SET INPUTS
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, height_map);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_map);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(glGetUniformLocation(program, "height_map"), 0);
    glUniform1i(glGetUniformLocation(program, "normal_map"), 1); // unused
    glUniform1f(glGetUniformLocation(program, "R"), R);
    glUniform1f(glGetUniformLocation(program, "r"), r);
    glUniform1f(glGetUniformLocation(program, "badrock_height"), badrock_height);
    glUniform1f(glGetUniformLocation(program, "height_mult"), height_mult);

    // RUN PROGRAM
    GLuint query;
    glGenQueries(1, &query);

    glEnable(GL_RASTERIZER_DISCARD);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, outbuff); // SET INPUT

    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
    glBeginTransformFeedback(GL_TRIANGLES);
    plane.draw();
    glEndTransformFeedback();
    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

    glDisable(GL_RASTERIZER_DISCARD);

    glFlush();

    // CREATE MESH ARRAYS

    GLuint primitives;
    glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives);

    std::cerr << "Transformed " << primitives << " triangles" << std::endl;

    std::vector<GLfloat> result_data(result_size);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, result_data.size() * sizeof(GLfloat), &result_data[0]);

    std::vector<unsigned int> indices;
    indices.reserve(result_vertex_count);
    for (int i = 0; i < result_vertex_count; i++) {
        indices.push_back(i);
    }

    // FIX NORMALS
    // for (size_t fi = 0; fi < result_size; fi += 8 * 3) {
    //     std::vector<glm::vec3> pos;
    //     for (int vi = 0; vi < 3; vi++) {
    //         size_t start = fi + vi * 8;
    //         pos.emplace_back(result_data[start], result_data[start + 1], result_data[start + 2]);
    //     }
    //     glm::vec3 norm = glm::normalize(glm::cross(pos[2] - pos[0], pos[1] - pos[0]));
    //     for (int vi = 0; vi < 3; vi++) {
    //         size_t start_o = fi + vi * 8 + 3;
    //         result_data[start_o] = norm.x;
    //         result_data[start_o + 1] = norm.y;
    //         result_data[start_o + 2] = norm.z;
    //     }
    // }

    // INIT RETURNED VALUES
    TorMovementModel model = TorMovementModel(
        result_data,
        r, R,
        vertex_size,
        0, 6, 8, 3,
        longitude_size, latitude_size);

    Mesh tor_mesh = Mesh(result_data, indices, std::vector<Material> { moon_mat, steel_mat }, { 3, 3, 2, 1 });

    return { tor_mesh, model };
}

int main(int, char **) {
    GLFWwindow *window = init_window();

    std::vector<Mesh> car_meshes = load_object("assets/reflex_camera/", "reflex_camera.obj");

    GLuint cubemap_texture;
    load_cubemap(cubemap_texture);

    // create our geometries
    GLuint vbo, vao, ebo;
    create_skybox( vbo, vao, ebo);

    // init shaders
    shader_t skybox_shader("assets/skybox.vs", "assets/skybox.fs");
    shader_t moon_shader("assets/moon.vs", "assets/moon.fs");
    shader_t object_shader("assets/object.vs", "assets/object.fs");
    shader_t id_shader("assets/id.vs", "assets/id.fs");

    std::cerr << "shaders done\n";

    setup_imgui(window);

    glfwSetScrollCallback(window, &scroll_callback);
    glfwSetCursorPosCallback(window, &mouse_callback);

    auto const start_time = std::chrono::steady_clock::now();

    float r = 0.7, R = 5, height_mult = 0.8, badrock_height = 0.3;
    auto tmp = make_torus(300, std::max(5, int(300 * r / R)), r, R, height_mult, badrock_height);
    Mesh tor = tmp.first;
    TorMovementModel mmodel = tmp.second;
    model_p = &mmodel;

    int tile_x = 8;
    int tile_y = std::max(1, int(tile_x * r / R));

    std::vector<Mesh> meshes = std::vector<Mesh>(1, tor);

    glm::vec3 camera_offset_old;
    glm::vec3 camera_center_old;
    glm::vec3 camera_up_old;
    bool camera_position_new = true;
    float camera_smooth_coeff = 0.95;

    const float max_radius = R + r + height_mult + 1.5 + 10;

    Shadow sun_shadow = Shadow(2048 * 4, 2048 * 4);
    Shadow torch_shadow = Shadow(512, 512);

    // controls
    static float fovy = 90;
    static float sun_speed_log = -20;
    static float sun_start_a = 3.4;
    static float camera_radius_mult = 1;

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        // Get windows size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        auto const frame_start_time = std::chrono::steady_clock::now();
        float const time_from_start = (float)(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count() / 1000.0);

        glm::mat4 sun_rotation = glm::rotate(
            sun_start_a + time_from_start * 2 * float(M_PI) * powf(2, sun_speed_log),
            glm::vec3(-1.0f, 1.0f, 1.0f));

        glm::vec3 sun_position = glm::vec3(sun_rotation * glm::normalize(glm::vec4(1.0f, 1.0f, 1.0f, 0)));

        // Pass the parameters to the shader as uniforms
        glm::vec3 model_pos = mmodel.get_pos();
        glm::vec3 forward = glm::normalize(mmodel.get_forward());
        glm::vec3 model_up = mmodel.get_up();
        glm::vec3 tor_up = mmodel.get_tor_normal();

        glm::vec3 camera_up_desired = glm::normalize(model_up + tor_up);
        glm::vec3 camera_offset_desired = 
            0.3f * camera_up_desired + 0.2f * forward +
            0.4f * glm::mat3(glm::rotate((-pitch + 0.5f) * float(M_PI), glm::normalize(glm::cross(forward, camera_up_desired)))) * camera_up_desired;
        camera_offset_desired *= camera_radius_mult;
        glm::vec3 camera_center_desired = model_pos + 0.3f * forward;

        if (camera_position_new) {
            camera_offset_old = camera_offset_desired;
            camera_center_old = camera_center_desired;
            camera_up_old = camera_up_desired;
            camera_position_new = false;
        }

        glm::vec3 camera_offset = glm::mix(camera_offset_desired, camera_offset_old, camera_smooth_coeff);
        glm::vec3 camera_center = glm::mix(camera_center_desired, camera_center_old, camera_smooth_coeff);
        glm::vec3 camera_up = glm::mix(camera_up_desired, camera_up_old, camera_smooth_coeff);

        camera_offset_old = camera_offset;
        camera_offset_old = camera_offset;
        camera_center_old = camera_center;
        camera_up_old = camera_up;

        glm::mat4 car_model = glm::translate(model_pos) * glm::scale(glm::vec3(1.0f, 1.0f, 1.0f) * 0.09f);
        car_model = car_model * glm::mat4(glm::mat3(glm::normalize(glm::cross(forward, model_up)), model_up, forward - model_up * glm::dot(model_up, forward)));
        car_model[3][3] = 1;

        glm::vec3 camera_position = model_pos + camera_offset;
        auto model = glm::mat4(1);
        auto view = glm::lookAt<float>(
            camera_position,
            camera_center,
            camera_up);
        auto projection = glm::perspective<float>(glm::radians(fovy), float(display_w) / display_h, 0.1, 100);
        auto mvp = projection * view * model;
        auto vp = projection * view;
        auto mvp_no_translation = projection * glm::mat4(glm::mat3(view * model));

        auto car_mvp = projection * view * car_model;

        // get sun shadow
        sun_shadow.set_shadow(
            glm::ortho(-max_radius, max_radius, -max_radius, max_radius, -max_radius, max_radius) * glm::lookAt(sun_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)));

        id_shader.use();
        {
            glm::mat4 shadow_mvp;
            shadow_mvp = sun_shadow.view * model;
            id_shader.set_uniform("u_mvp", glm::value_ptr(shadow_mvp));
            for (Mesh &mesh : meshes) {
                mesh.draw();
            }
            shadow_mvp = sun_shadow.view * car_model;
            id_shader.set_uniform("u_mvp", glm::value_ptr(shadow_mvp));
            for (Mesh &mesh : car_meshes) {
                mesh.draw();
            }
        }

        sun_shadow.unset_shadow();

        glm::vec3 torch_dir = forward * 1.2f - 0.1f * model_up;
        glm::vec3 torch_pos = model_pos + model_up * 0.07f + forward * 0.2f;


        // get torch shadow
        torch_shadow.set_shadow(
            glm::perspective<float>(glm::radians(130.0), 1, 0.01, 10) *
            glm::lookAt(torch_pos, torch_pos + torch_dir, model_up));

        id_shader.use();
        {
            glm::mat4 shadow_mvp;
            shadow_mvp = torch_shadow.view * model;
            id_shader.set_uniform("u_mvp", glm::value_ptr(shadow_mvp));
            for (Mesh &mesh : meshes) {
                mesh.draw();
            }
            shadow_mvp = torch_shadow.view * car_model;
            id_shader.set_uniform("u_mvp", glm::value_ptr(shadow_mvp));
            for (Mesh &mesh : car_meshes) {
                mesh.draw();
            }
        }

        torch_shadow.unset_shadow();

        // start actual drawing

        // Set viewport to fill the whole window area
        glViewport(0, 0, display_w, display_h);

        // Fill background with solid color
        // glClearColor(0.30f, 0.55f, 0.60f, 1.00f);
        // glEnable(GL_CULL_FACE);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        // Gui start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // GUI
        ImGui::Begin("Triangle Position/Color");
        static float fovy = 90;
        ImGui::SliderFloat("fovy", &fovy, 10, 180);
        ImGui::SliderFloat("sun start a", &sun_start_a, 0, 2 * M_PI);
        ImGui::SliderFloat("sun speed log", &sun_speed_log, -20, 0);
        ImGui::SliderFloat("camera radius", &camera_radius_mult, 0.5, 5);

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


        glColorMask(0, 0, 0, 0);
        id_shader.use();
        id_shader.set_uniform("u_mvp", glm::value_ptr(mvp));
        for (Mesh &mesh : meshes) {
            mesh.draw();
        }
        id_shader.set_uniform("u_mvp", glm::value_ptr(car_mvp));
        for (Mesh &mesh : car_meshes) {
            mesh.draw();
        }

        glColorMask(1, 1, 1, 1);
        moon_shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        sun_shadow.bind_shadow_texture(10);
        torch_shadow.bind_shadow_texture(11);

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

            shader.set_uniform("pd_num", 1);
            shader.set_uniform("pd_dir", torch_dir);
            shader.set_uniform("pd_pos", torch_pos);
            shader.set_uniform("pd_light", glm::vec3(1.0f, 1.0f, 0.5f) * 0.5f);
            shader.set_uniform("pd_angle", 0.6f);
            shader.set_uniform("pd_depth", 11);
            shader.set_uniform("pd_vp", glm::value_ptr(torch_shadow.view));

            shader.set_uniform("background_light", glm::vec3(1, 1, 1) * 0.9f);
        };

        moon_shader.set_uniform<float>("u_tile", tile_x, tile_y);
        moon_shader.set_uniform("u_m", glm::value_ptr(model));
        moon_shader.set_uniform("u_mvp", glm::value_ptr(mvp));
        moon_shader.set_uniform("u_badrock_height", badrock_height);
        pass_everything_lambda(moon_shader);

        for (Mesh &mesh : meshes) {
            mesh.draw(moon_shader);
        }

        mvp_no_translation = projection * glm::mat4(glm::mat3(view * car_model));

        object_shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        object_shader.set_uniform("u_m", glm::value_ptr(car_model));
        object_shader.set_uniform("u_mvp", glm::value_ptr(car_mvp));
        object_shader.set_uniform<float>("u_tile", 1, 1);
        pass_everything_lambda(object_shader);
        // object_shader.set_uniform("background_light", 0.7f, 0.7f, 0.7f);

        for (Mesh &mesh : car_meshes) {
            mesh.draw(object_shader);
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

    model_p = nullptr;

    return 0;
}
