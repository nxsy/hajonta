#pragma once

struct demo_firework_state {
    float delta_t;

    uint32_t firework_vbo;
    uint32_t firework_ibo;
    uint32_t firework_texture;

    uint32_t num_faces;
};

DEMO(demo_firework);
