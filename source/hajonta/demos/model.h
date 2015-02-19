#pragma once

struct face_index
{
    uint32_t vertex;
    uint32_t texture_coord;
};

struct face
{
    face_index indices[3];
};

struct model
{
    v3 *vertices;
    uint32_t num_vertices;

    v3 *texture_coords;
    uint32_t num_texture_coords;

    face *faces;
    uint32_t num_faces;

};

struct demo_model_state {
    uint32_t vbo;
    uint32_t ibo;
    uint32_t ibo_length;
    uint32_t texture_id;

    float near_;
    float far_;

    model minimalist_dudes;
};

DEMO(demo_model);
