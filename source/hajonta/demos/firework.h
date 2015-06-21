#pragma once

enum struct firework_type
{
    unused = -1,
    basic_initial,
    basic_second_part,
    NUMBER_FIREWORK_TYPES,
};

struct firework_particle
{
    v3 position;
    v3 velocity;

    v3 constant_acceleration;

    v3 force_accumulator;

    float ttl;
    firework_type type;
};

struct firework_payload
{
    uint8_t min_num_fireworks;
    uint8_t max_num_fireworks;
    firework_type type;
};

struct firework_behaviour
{
    v2 ttl_range;
    v3 min_velocity;
    v3 max_velocity;

    firework_payload payload[3];
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

    firework_behaviour behaviours[firework_type::NUMBER_FIREWORK_TYPES];
};

DEMO(demo_firework);
