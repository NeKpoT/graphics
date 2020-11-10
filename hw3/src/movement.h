#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <string>
#include <vector>

class TorMovementModel {
  public:
    TorMovementModel(std::vector<float> geometry, float r, float R, size_t vertex_size, size_t pos_off, size_t flat_pos_off, size_t height_off, size_t norm_off);

    glm::vec3 get_pos();
    glm::vec3 get_up();
    glm::vec3 get_forward();
    glm::vec3 get_tor_normal();

    void move_forvard(int value);
    void rotate(float alpha);

  private:

    float model_height = 0.2;
    float speed = 0.01;

    glm::vec2 flat_pos = glm::vec2(0, 0);
    glm::vec2 flat_dir = glm::vec2(0, 1);
    size_t current_face = 0;

    const float r;
    const float R;
    const std::vector<float> geometry;
    const size_t vertex_size;
    const size_t pos_off;
    const size_t flat_pos_off;
    const size_t height_off;
    const size_t norm_off;

    size_t find_face(glm::vec2 flat_coord);
    void set_face();

    glm::vec3 face_coord(size_t face, glm::vec2 flat_c);

    size_t faces_count() const;
    glm::vec3 get_vertex_pos(size_t face, size_t vertex);
    glm::vec2 get_vertex_flat_pos(size_t face, size_t vertex);
    glm::vec3 get_vertex_height(size_t face, size_t vertex);
    glm::vec3 get_vertex_norm(size_t face, size_t vertex);
    glm::mat3 get_face_mat(size_t face, size_t off);
    glm::vec3 extract3(size_t start);
    glm::vec2 extract2(size_t start);
};