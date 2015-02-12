#pragma once

struct demo_bounce_state
{
    uint32_t vbo;
    v2 velocity[10];
    v2 position[10];
    float t;
    uint32_t num_collisions;
};

GAME_UPDATE_AND_RENDER(demo_bounce);
