#pragma once

#define matrix_buffer_width 300
#define matrix_buffer_height 14
struct demo_rotate_state {
    uint32_t vbo;
    uint32_t ibo;
    uint32_t ibo_length;
    uint32_t line_ibo;
    uint32_t line_ibo_length;
    float delta_t;
    bool frustum_mode;
    float near_;
    float far_;
    float back_size;
    float spacing;

    uint8_t matrix_buffer[4 * matrix_buffer_width * matrix_buffer_height];
    draw_buffer matrix_draw_buffer;
    uint32_t texture_id;
};

DEMO(demo_rotate);
