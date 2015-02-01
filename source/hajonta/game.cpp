#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(NEEDS_EGL)
#include <EGL/egl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif


#include "hajonta/platform/common.h"
#include "hajonta/programs/a.h"
#include "hajonta/programs/debug_font.h"

#include "hajonta/math.cpp"
#include "hajonta/bmp.cpp"
#include "hajonta/font.cpp"

struct kenpixel_future_14
{
    uint8_t zfi[5159];
    uint8_t bmp[131210];
    font_data font;
};

#define fps_buffer_width 960
#define fps_buffer_height 14

struct game_state
{
    a_program_struct program_a;
    debug_font_program_struct program_debug_font;

    uint32_t vbo;

    v2 velocity;
    v2 position;

    int audio_offset;
    void *audio_buffer_data;

    kenpixel_future_14 debug_font;
    uint8_t fps_buffer[4 * fps_buffer_width * fps_buffer_height];
    draw_buffer fps_draw_buffer;
    uint32_t fps_texture_id;
    int32_t fps_sampler_id;
    uint32_t fps_vbo;
};

struct vertex
{
    float position[4];
    float color[4];
};

void gl_setup(hajonta_thread_context *ctx, platform_memory *memory)
{
    game_state *state = (game_state *)memory->memory;

    bool loaded = a_program(&state->program_a, ctx, memory);
    if (!loaded)
    {
        return;
    }

    loaded = debug_font_program(&state->program_debug_font, ctx, memory);
    if (!loaded)
    {
        return;
    }

    glUseProgram(state->program_a.program);
    glGenBuffers(1, &state->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    vertex vertices[4] = {
        {{-0.5, 0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5, 0.5, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 0.5,-0.5, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{-0.5,-0.5, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}},
    };
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(state->program_a.a_pos_id);
    glEnableVertexAttribArray(state->program_a.a_color_id);
    glVertexAttribPointer(state->program_a.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer(state->program_a.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(state->program_debug_font.program);
    glGenBuffers(1, &state->fps_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, state->fps_vbo);

    float height = 14.0f / (540.0f / 2.0f);
    float top = -(1-height);
    vertex font_vertices[4] = {
        {{-1.0, top, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 1.0, top, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}},
        {{ 1.0,-1.0, 0.0, 1.0}, {1.0, 0.0, 1.0, 1.0}},
        {{-1.0,-1.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
    };
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), font_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(state->program_debug_font.a_pos_id);
    glEnableVertexAttribArray(state->program_debug_font.a_tex_coord_id);
    glVertexAttribPointer(state->program_debug_font.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer(state->program_debug_font.a_tex_coord_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

    if (!memory->platform_load_asset(ctx, "fonts/kenpixel_future/kenpixel_future_regular_14.zfi", sizeof(state->debug_font.zfi), state->debug_font.zfi))
    {
        memory->platform_fail(ctx, "Failed to open zfi file");
        return;
    }
    if (!memory->platform_load_asset(ctx, "fonts/kenpixel_future/kenpixel_future_regular_14.bmp", sizeof(state->debug_font.bmp), state->debug_font.bmp))
    {
        memory->platform_fail(ctx, "Failed to open bmp file");
        return;
    }
    load_font(state->debug_font.zfi, state->debug_font.bmp, &state->debug_font.font, ctx, memory);
    state->fps_draw_buffer.memory = state->fps_buffer;
    state->fps_draw_buffer.width = fps_buffer_width;
    state->fps_draw_buffer.height = fps_buffer_height;
    state->fps_draw_buffer.pitch = 4 * fps_buffer_width;

    glGenTextures(1, &state->fps_texture_id);
    glBindTexture(GL_TEXTURE_2D, state->fps_texture_id);
    state->fps_sampler_id = glGetUniformLocation(state->program_debug_font.program, "tex");

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        fps_buffer_width, fps_buffer_height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, state->fps_buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

    float delta_t = input->delta_t;

#if !defined(NEEDS_EGL) && !defined(__APPLE__)
    if (!glCreateProgram)
    {
        load_glfuncs(ctx, memory->platform_glgetprocaddress);
    }
#endif
    if (!memory->initialized)
    {
        state->velocity = {0, 0};
        state->position = {0, 0};

        gl_setup(ctx, memory);

        memory->initialized = 1;

        int volume = 3000;
        float pi = 3.14159265358979f;
        state->audio_buffer_data = malloc(48000 * 2 * (16 / 8));
        for (int i = 0; i < 48000 * 2;)
        {
            volume = i < 48000 ? i / 16 : abs(96000 - i) / 16;
            ((uint16_t *)state->audio_buffer_data)[i] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565 / 48000.0));
            ((uint16_t *)state->audio_buffer_data)[i+1] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565 / 48000.0));
            i += 2;
        }
    }

    v2 acceleration = {};

    for (uint32_t i = 0;
            i < harray_count(input->controllers);
            ++i)
    {
        if (!input->controllers[i].is_active)
        {
            continue;
        }

        game_controller_state *controller = &input->controllers[i];
        if (controller->buttons.move_up.ended_down)
        {
            acceleration.y += 1.0f;
        }
        if (controller->buttons.move_down.ended_down)
        {
            acceleration.y -= 1.0f;
        }
        if (controller->buttons.move_left.ended_down)
        {
            acceleration.x -= 1.0f;
        }
        if (controller->buttons.move_right.ended_down)
        {
            acceleration.x += 1.0f;
        }
    }

    acceleration = v2normalize(acceleration);
    acceleration = v2sub(acceleration, v2mul(state->velocity, 5.0f));
    v2 movement = v2add(
            v2mul(acceleration, 0.5f * (delta_t * delta_t)),
            v2mul(state->velocity, delta_t)
    );
    state->position = v2add(state->position, movement);
    state->velocity = v2add(
            v2mul(acceleration, delta_t),
            state->velocity
    );
    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    char msg[1024];
    sprintf(msg, "P: %+.2f, %+.2f", state->position.x, state->position.y);
    sprintf(msg + strlen(msg), "V: %+.2f, %+.2f", state->velocity.x, state->velocity.y);
    sprintf(msg + strlen(msg), "A: %+.2f, %+.2f", acceleration.x, acceleration.y);
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg, ctx, memory);

    glClearColor(0.1f, 0.1f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(state->program_a.program);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    glEnableVertexAttribArray(state->program_a.a_pos_id);
    glEnableVertexAttribArray(state->program_a.a_color_id);
    glVertexAttribPointer(state->program_a.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer(state->program_a.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));
    glUniform2fv(state->program_a.u_offset_id, 1, (float *)&state->position);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glUseProgram(state->program_debug_font.program);
    glBindBuffer(GL_ARRAY_BUFFER, state->fps_vbo);
    glEnableVertexAttribArray(state->program_debug_font.a_pos_id);
    glEnableVertexAttribArray(state->program_debug_font.a_tex_coord_id);
    glVertexAttribPointer(state->program_debug_font.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer(state->program_debug_font.a_tex_coord_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

    glBindTexture(GL_TEXTURE_2D, state->fps_texture_id);
    glTexSubImage2D(GL_TEXTURE_2D,
        0,
        0,
        0,
        fps_buffer_width,
        fps_buffer_height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        state->fps_buffer);

    glUniform1i(state->fps_sampler_id, 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    sound_output->samples = &(((uint8_t *)state->audio_buffer_data)[state->audio_offset * 2 * sound_output->channels * sound_output->number_of_samples]);
    state->audio_offset = (state->audio_offset + 1) % 60;
}

