#pragma once

#include <sys/syslimits.h>

void process_keyboard_message(game_button_state *new_state, bool is_down);

struct osx_game_code
{
    game_update_and_render_func *game_update_and_render;

    void *library_handle;
    time_t last_updated;
};

struct osx_state
{
    bool stopping;
    bool keyboard_mode;
    char *stop_reason;

    char binary_name[PATH_MAX];
    char *last_slash;
    char library_path[PATH_MAX];
    char asset_path[PATH_MAX];

    game_input inputs[2];
    game_input *new_input;
    game_input *old_input;

    osx_game_code game_code;
    platform_memory memory;
};


void get_binary_name(osx_state *state);
void build_full_filename(osx_state *state, char *filename, int dest_count, char *dest);
void load_game(osx_game_code *game_code, char *path);
void osx_init(osx_state *state);
void loop_cycle(osx_state *state);
