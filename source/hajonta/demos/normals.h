#pragma once

struct demo_normals_state
{
    uint32_t vbo;
    v2 line_ending;
    v2 line_velocity;
};

GAME_UPDATE_AND_RENDER(demo_normals);
