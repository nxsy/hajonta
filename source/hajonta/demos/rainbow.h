#pragma once

struct demo_rainbow_state
{
    v2 velocity;
    v2 position;

    uint32_t vbo;

    int audio_offset;
    void *audio_buffer_data;
};

GAME_UPDATE_AND_RENDER(demo_rainbow);
