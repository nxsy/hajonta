/*
 * Copyright (c) 2015, Neil Blakey-Milner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

struct xcb_state
{
    int32_t stopping;
    char *stop_reason;

    const xcb_setup_t *setup;
    xcb_format_t *fmt;
    xcb_connection_t *connection;
    xcb_key_symbols_t *key_symbols;
    xcb_window_t window;

    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_window;

    game_input *new_input;
    game_input *old_input;

    char binary_name[PATH_MAX];
    char *last_slash;
};

struct xcb_game_code
{
    game_update_and_render_func *game_update_and_render;

    void *library_handle;
    time_t last_updated;
};

struct xcb_sound_output
{
    int32_t samples_per_second;
    int32_t bytes_per_sample;;
    int32_t secondary_buffer_size;
};
