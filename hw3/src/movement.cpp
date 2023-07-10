#include "movement.h"

namespace {
    float eps = 1e-5;
}

TorMovementModel::TorMovementModel(
    std::vector<float> geometry,
    float r, float R,
    size_t vertex_size,
    size_t pos_off,
    size_t flat_pos_off,
    size_t height_off,
    size_t norm_off, 
    int x_frag, int y_frag)
    : R(R)
    , r(r)
    , geometry(geometry)
    , vertex_size(vertex_size)
    , pos_off(pos_off)
    , flat_pos_off(flat_pos_off)
    , height_off(height_off)
    , norm_off(norm_off)
    , x_frag(x_frag)
    , y_frag(y_frag) {
    set_face();
}

glm::vec3 TorMovementModel::extract3(size_t start) {
    return glm::vec3(geometry[start], geometry[start + 1], geometry[start + 2]);
}
glm::vec2 TorMovementModel::extract2(size_t start) {
    return glm::vec2(geometry[start], geometry[start + 1]);
}

size_t TorMovementModel::faces_count() const {
    return geometry.size() / (vertex_size * 3);
}

glm::vec3 TorMovementModel::get_vertex_pos(size_t face, size_t vertex) {
    return extract3((face * 3 + vertex) * vertex_size + pos_off);
}
glm::vec2 TorMovementModel::get_vertex_flat_pos(size_t face, size_t vertex) {
    return extract2((face * 3 + vertex) * vertex_size + flat_pos_off);
}
glm::vec3 TorMovementModel::get_vertex_height(size_t face, size_t vertex) {
    return extract3((face * 3 + vertex) * vertex_size + height_off);
}
glm::vec3 TorMovementModel::get_vertex_norm(size_t face, size_t vertex) {
    return extract3((face * 3 + vertex) * vertex_size + norm_off);
}

glm::vec3 TorMovementModel::face_coord(size_t face, glm::vec2 flat_c) {
    float coord[3];
    for (size_t n0 = 0; n0 < 3; n0++) {
        size_t n1 = (n0 + 1) % 3;
        size_t n2 = (n0 + 2) % 3;

        glm::vec2 DBG = get_vertex_flat_pos(face, n2);
        glm::vec2 norm = get_vertex_flat_pos(face, n2) - get_vertex_flat_pos(face, n1);

        norm.x *= -1;
        std::swap(norm.x, norm.y); // rotate right
        norm = glm::normalize(norm);

        float from_coord = glm::dot(norm, get_vertex_flat_pos(face, n1));
        float this_coord = glm::dot(norm, flat_c);
        float to_coord = glm::dot(norm, get_vertex_flat_pos(face, n0));
        coord[n0] = (this_coord - from_coord) / (to_coord - from_coord);
    }
    return glm::vec3(coord[0], coord[1], coord[2]);
}

size_t TorMovementModel::find_face(glm::vec2 flat_coord) {
    // TODO replace using grid knowlege

    int x_i = int(flat_coord.x * x_frag);
    int y_i = int(flat_coord.y * y_frag);
    int num = x_i * (y_frag * 2) + y_i * 2;

    size_t fc = faces_count();
    for (size_t face = num; face < num + 2; face++) {
        glm::vec3 vc = face_coord(face, flat_pos);
        if (vc.x >= -eps && vc.y >= -eps && vc.z >= -eps) {
            return face;
        }
    }

    return num;
}

void TorMovementModel::set_face() {
    glm::vec3 vc = face_coord(current_face, flat_pos);
    if (vc.x >= -eps && vc.y >= -eps && vc.z >= -eps) {
        return;
    }
    current_face = find_face(flat_pos);
    // add shenanigans when bouncy camera is implemented
}

glm::mat3 TorMovementModel::get_face_mat(size_t face, size_t off) {
    return glm::mat3(
        extract3((face * 3 + 0) * vertex_size + off),
        extract3((face * 3 + 1) * vertex_size + off),
        extract3((face * 3 + 2) * vertex_size + off));
}

glm::vec3 TorMovementModel::get_pos() {
    glm::vec3 face_coord_v = face_coord(current_face, flat_pos);
    glm::mat3 face_poses = get_face_mat(current_face, pos_off);
    return face_poses * face_coord_v;
}
glm::vec3 TorMovementModel::get_up() {
    glm::vec3 face_coord_v = face_coord(current_face, flat_pos);
    glm::mat3 face_normals = get_face_mat(current_face, norm_off);
    return glm::normalize(face_normals * face_coord_v);
}
glm::vec3 TorMovementModel::get_forward() {
    glm::vec3 face_coord_curr = face_coord(current_face, flat_pos);
    glm::vec3 face_coord_dir = face_coord(current_face, flat_pos + flat_dir);

    glm::mat3 face_poses = get_face_mat(current_face, pos_off);
    return face_poses * (face_coord_dir - face_coord_curr);
    // return face_poses * face_coord(current_face, flat_dir); // TODO CHECK IF THIS WORKS
}
glm::vec3 TorMovementModel::get_tor_normal() {
    glm::vec2 xz = -glm::vec2(glm::cos(flat_pos.x * 2 * M_PI), glm::sin(flat_pos.x * 2 * M_PI)) * glm::cos(flat_pos.y * 2 * (float)M_PI);
    return glm::vec3(
        xz.x,
        glm::sin(flat_pos.y * 2 * (float)M_PI),
        xz.y);
}

namespace {
    void cycle_to_one(float& v) {
        v -= int(v);
        if (v < 0)
            v += 1;
    }
}

void TorMovementModel::move_forvard(int value) {
    float dir_mod = 1;
    if (value < 0) {
        value = -value;
        dir_mod = -1;
    }

    for (int i = 0; i < value; i++) {
        glm::vec3 norm = glm::normalize(get_up());
        glm::vec2 xz = glm::vec2(glm::cos(flat_pos.x * 2 * M_PI), glm::sin(flat_pos.x * 2 * M_PI)) * glm::cos(flat_pos.y * 2 * (float)M_PI);
        glm::vec3 true_norm = get_tor_normal();
    
        float norm_factor = glm::length(glm::cross(glm::normalize(get_forward()), true_norm));
        flat_pos += dir_mod * flat_dir * speed * std::max(0.1f, norm_factor);

        cycle_to_one(flat_pos.x);
        cycle_to_one(flat_pos.y);
        set_face();
    }
}
void TorMovementModel::rotate(float alpha) {
    flat_dir.x *= R;
    flat_dir.y *= r;
    flat_dir = glm::vec2(glm::rotate(alpha, glm::vec3(0, 0, 1)) * glm::vec4(flat_dir, 0, 1));
    flat_dir.x /= R;
    flat_dir.y /= r;

    flat_dir = glm::normalize(flat_dir);
}