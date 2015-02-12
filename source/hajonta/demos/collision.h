#pragma once

struct demo_collision_state
{
    uint32_t vbo;
    v2 line_starting;
    v2 line_velocity;
};

GAME_UPDATE_AND_RENDER(demo_collision);
