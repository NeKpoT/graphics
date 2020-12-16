#include <glm/glm.hpp>
#include <random>
#include <vector>
#include "opengl_shader.h"

class Droplets {
  public:
    Droplets(size_t n, float tile_size, float height, float speed = 7.0, float drop_width = 0.005, float drop_height = 0.3);

    void draw(shader_t &shader, float time);

    const size_t n;
    const float tile_size;
    float height;
    float speed;
    float drop_width;
    float drop_height;

  private:
    void generate_flatcoord(size_t id);

    std::vector<float> positions;
    std::vector<float> starting_heights;
    float last_time = 0;

    GLuint vao, vbo;

    static std::mt19937 rng;
    std::uniform_real_distribution<float> random_flat_position;
};