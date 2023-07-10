#include "droplet.h"

std::mt19937 Droplets::rng(533);

Droplets::Droplets(size_t n, float tile_size, float height, float speed, float drop_width, float drop_height)
    : n(n)
    , positions(3 * n)
    , starting_heights(n)
    , tile_size(tile_size)
    , height(height)
    , speed(speed)
    , random_flat_position(-tile_size / 2, tile_size / 2)
    , drop_width(drop_width)
    , drop_height(drop_height) {

    // init positions
    std::uniform_real_distribution<> random_height(0, height);

    for (size_t i = 0; i < n; i++) {
        generate_flatcoord(i);
        starting_heights[i] = positions[3 * i + 1] = random_height(rng);
    }

    // init vertex array
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * n, &positions[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)(0));
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Droplets::draw(shader_t &shader, float time) {
    if (last_time != 0) {
        for (size_t i = 0; i < n; i++) {
            float new_height = starting_heights[i] - time * speed;
            float last_height = starting_heights[i] - last_time * speed;
            if (ceil(new_height / height) != ceil(last_height / height)) {
                generate_flatcoord(i);
            }
            positions[3 * i + 1] = new_height + height * ceil(-new_height / height);
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * n, &positions[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    last_time = time;

    shader.set_uniform("width", drop_width);
    shader.set_uniform("height", drop_height);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, n);
    glBindVertexArray(0);
}

void Droplets::generate_flatcoord(size_t id) {
    float x, z;
    // do {
    //     x = random_flat_position(rng);
    //     z = random_flat_position(rng);
    // } while (x * x + z*z > tile_size); // wrong coz camera moves

    x = random_flat_position(rng);
    z = random_flat_position(rng);
    positions[3 * id + 0] = x;
    positions[3 * id + 2] = z;
}