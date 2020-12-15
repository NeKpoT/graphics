#include "droplet.h"

std::mt19937 Droplets::rng(533);

Droplets::Droplets(size_t n, float tile_size, float height, float speed)
    : n(n)
    , positions(3 * n)
    , starting_heights(n)
    , tile_size(tile_size)
    , height(height)
    , speed(speed)
    , random_xy_position(-tile_size / 2, tile_size / 2) {

    // init positions
    std::uniform_real_distribution<> random_height(0, height);

    for (size_t i = 0; i < n; i++) {
        positions[3 * i + 0] = random_xy_position(rng);
        positions[3 * i + 1] = random_xy_position(rng);
        starting_heights[i] = positions[3 * i + 2] = random_height(rng);
    }

    // init vertex array
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * n, &positions[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3  , GL_FLOAT, GL_FALSE, 0, (void *)(0));
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Droplets::draw(shader_t &shader, float time) {
    // if (last_time != 0) {
    //     for (size_t i = 0; i < n; i += 3) {
    //         float new_height = starting_heights[i] - time * speed;
    //         float last_height = starting_heights[i] - last_time * speed;
    //         if (ceil(new_height / height) != ceil(last_height / height)) {
    //             positions[2 * i] = random_xy_position(rng);
    //             positions[2 * i + 1] = random_xy_position(rng);
    //         }
    //         positions[2 * i + 2] = new_height + height * ceil(-new_height / height);
    //     }
    // }
    // last_time = time;

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, n);
    glBindVertexArray(0);
}