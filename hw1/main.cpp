#include <iostream>
#include <vector>
#include <chrono>

#include <fmt/format.h>

#include <GL/glew.h>

// STB, load images
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Imgui + bindings
#include "imgui.h"
#include "bindings/imgui_impl_glfw.h"
#include "bindings/imgui_impl_opengl3.h"

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// Math constant and routines for OpenGL interop
#include <glm/gtc/constants.hpp>

#include "opengl_shader.h"

template <typename T>
void crop_interval(T &x, T minv, T maxv) {
    x = std::max(minv, std::min(maxv, x));
}

template <typename T>
void crop_abs(T &x, T max_abs) {
    crop_interval(x, -max_abs, max_abs);
}

const float MAXC_C_ABS = 1;
const float MAX_CANVAS_SIZE = (1 + sqrt(1 + 4 * sqrt(2) * MAXC_C_ABS)) * 0.95;

static void
glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

void create_triangle(GLuint &vbo, GLuint &vao, GLuint &ebo)
{
   // create the triangle
   float mult = 1;
   float triangle_vertices[] = {
       -mult, -mult, 0.0f, // position vertex 1
       2 * mult, 0.0f, 0.0f, // coord vertex 1

       mult * 3, -mult, 0.0f, // position vertex 2
       0.0f, 2 * mult, 0.0f, // coord vertex 2

       -mult, mult * 3, 0.0f, // position vertex 3
       0.0f, 0.0f, 2 * mult, // coord vertex 3
   };
   unsigned int triangle_indices[] = {
       0, 1, 2 };
   glGenVertexArrays(1, &vao);
   glGenBuffers(1, &vbo);
   glGenBuffers(1, &ebo);
   glBindVertexArray(vao);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle_indices), triangle_indices, GL_STATIC_DRAW);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
   glEnableVertexAttribArray(1);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);
}

int display_w, display_h;

const float ZOOM_MIN = 1 / MAX_CANVAS_SIZE;
const float ZOOM_MAX = 80;
const float ZOOM_STEP = 0.1;
float zoom = 1.0;

float cursor_position[] = { 0.0, 0.0 };
static float translation[] = { 0.0, 0.0 };

void crop_translation() {
    const float translation_max = MAX_CANVAS_SIZE / 1 - 1 / zoom;
    crop_abs(translation[0], translation_max);
    crop_abs(translation[1], translation_max);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    float old_zoom = zoom;
    zoom += ZOOM_STEP * yoffset;
    crop_interval(zoom, ZOOM_MIN, ZOOM_MAX);
    float zoom_diff = zoom - old_zoom;

    float xdiff = (1 / old_zoom) * (cursor_position[0] / display_w - 0.5);
    float ydiff = (1 / old_zoom) * (cursor_position[1] / display_h - 0.5);

    translation[0] += xdiff * (1 - old_zoom / zoom);
    translation[1] -= ydiff * (1 - old_zoom / zoom);

    crop_translation();
}

void mouse_moved(GLFWwindow *window, double xoffset, double yoffset) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
        translation[0] -= xoffset / zoom / display_w;
        translation[1] += yoffset / zoom / display_h;
        crop_translation();
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    mouse_moved(window, xpos - cursor_position[0], ypos - cursor_position[1]);
    cursor_position[0] = xpos;
    cursor_position[1] = ypos;
}

GLuint getTexure() {

    int width = 0, height = 0, channels;
    unsigned char *image = stbi_load("assets/gradient.bmp",
                                     &width,
                                     &height,
                                     &channels,
                                     STBI_rgb);
    
    if (height != 1) {
        std::cout << "Texture is not 1D: " << height << ' ' << width << std::endl;
        exit(123);
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_1D, texture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB8, width, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_1D);

    stbi_image_free(image);

    return texture;
}

int main(int, char **) {
   // Use GLFW to create a simple window
   glfwSetErrorCallback(glfw_error_callback);
   if (!glfwInit())
      return 1;


   // GL 3.3 + GLSL 330
   const char *glsl_version = "#version 330";
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

   // Create window with graphics context
   GLFWwindow *window = glfwCreateWindow(1280, 720, "Dear ImGui - Conan", NULL, NULL);
   if (window == NULL)
      return 1;
   glfwMakeContextCurrent(window);
   glfwSwapInterval(1); // Enable vsync

   // Initialize GLEW, i.e. fill all possible function pointers for current OpenGL context
   if (glewInit() != GLEW_OK)
   {
      std::cerr << "Failed to initialize OpenGL loader!\n";
      return 1;
   }

   // create our geometries
   GLuint vbo, vao, ebo;
   create_triangle(vbo, vao, ebo);

   // init shader
   shader_t triangle_shader("assets/simple-shader.vs", "assets/simple-shader.fs");

   // Setup GUI context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO &io = ImGui::GetIO();
   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init(glsl_version);
   ImGui::StyleColorsDark();

   auto const start_time = std::chrono::steady_clock::now();


   glfwSetScrollCallback(window, &scroll_callback);
   glfwSetCursorPosCallback(window, &mouse_callback);

   GLuint texture = getTexure();

   while (!glfwWindowShouldClose(window)) {

       // Get windows size
       glfwGetFramebufferSize(window, &display_w, &display_h);

       glfwPollEvents();

       // Set viewport to fill the whole window area
       glViewport(0, 0, display_w, display_h);

       // Fill background with solid color
       glClearColor(0.30f, 0.55f, 0.60f, 1.00f);
       glClear(GL_COLOR_BUFFER_BIT);

       // Gui start new frame
       ImGui_ImplOpenGL3_NewFrame();
       ImGui_ImplGlfw_NewFrame();
       ImGui::NewFrame();


       // GUI
       ImGui::Begin("Triangle Position/Color");
       ImGui::SliderFloat("zoom", &zoom, ZOOM_MIN, ZOOM_MAX);
       ImGui::SliderFloat2("position", translation, -1.0, 1.0);
       static float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
       ImGui::ColorEdit3("color", color);
       static float c[] = { -0.8f, 0.156f };
       ImGui::SliderFloat2("c", c, -MAXC_C_ABS, MAXC_C_ABS);
       static int iterations = 15;
       ImGui::SliderInt("iterations", &iterations, 1, 150);
       ImGui::End();

       // Pass the parameters to the shader as uniforms
       triangle_shader.set_uniform("u_zoom", zoom);
       triangle_shader.set_uniform("u_canvassize", MAX_CANVAS_SIZE);
       triangle_shader.set_uniform("u_iterations", iterations);
       triangle_shader.set_uniform("u_translation", translation[0], translation[1]);
       triangle_shader.set_uniform("u_c", c[0], c[1]);
       float const time_from_start = (float)(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count() / 1000.0);
       triangle_shader.set_uniform("u_time", time_from_start);
       triangle_shader.set_uniform("u_color", color[0], color[1], color[2]);
       triangle_shader.set_uniform("u_tex", int(0));

       glActiveTexture(GL_TEXTURE0);
       glBindTexture(GL_TEXTURE_2D, texture);
       glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
       glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

       // Bind triangle shader
       triangle_shader.use();
       // Bind vertex array = buffers + indices
       glBindVertexArray(vao);
       // Execute draw call
       glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
       glBindVertexArray(0);

       // Generate gui render commands
       ImGui::Render();

       // Execute gui render commands using OpenGL backend
       ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

       // Swap the backbuffer with the frontbuffer that is used for screen display
       glfwSwapBuffers(window);
   }

   // Cleanup
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();

   glfwDestroyWindow(window);
   glfwTerminate();

   return 0;
}
