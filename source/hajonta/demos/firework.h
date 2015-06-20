#pragma once

struct firework_particle
{
    v3 position;
    v3 velocity;

    v3 constant_acceleration;

    v3 force_accumulator;
};

struct demo_firework_state {
    float delta_t;

    uint32_t firework_vbo;
    uint32_t firework_ibo;
    uint32_t firework_texture;
    uint32_t firework_num_faces;

    uint32_t ground_vbo;
    uint32_t ground_ibo;
    uint32_t ground_texture;
    uint32_t ground_num_faces;

    uint32_t num_particles;
    firework_particle particles[1024];
};

DEMO(demo_firework);
