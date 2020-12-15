#include <glm/glm.hpp>
#include <random>
#include <vector>
#include "opengl_shader.h"

class Droplets {
  public:
    Droplets(size_t n, float tile_size, float height, float speed = 10.0);

    void draw(shader_t &shader, float time);

    const size_t n;
    const float tile_size;
    const float height;
    const float speed;

  private:
    std::vector<float> positions;
    std::vector<float> starting_heights;
    float last_time = 0;

    GLuint vao, vbo;

    std::uniform_real_distribution<float> random_xy_position;
    static std::mt19937 rng;
};