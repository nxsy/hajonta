#pragma once

#define menu_buffer_width 300
#define menu_buffer_height 14
struct demo_menu_state
{
    uint8_t menu_buffer[4 * menu_buffer_width * menu_buffer_height];
    draw_buffer menu_draw_buffer;
    uint32_t texture_id;
    uint32_t vbo;

    uint32_t selected_index;
};

GAME_UPDATE_AND_RENDER(demo_menu);
