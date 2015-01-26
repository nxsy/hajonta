#include <math.h>
#include <stddef.h>

#include <windows.h>
#include <gl/gl.h>

#include "hajonta\platform\common.h"
#include "hajonta\programs\a.h"

struct game_state {
    a_program_struct program_a;

    int32_t u_offset_id;
    int32_t a_pos_id;
    int32_t a_color_id;

    uint32_t vao;
    uint32_t vbo;

    float x;
    float y;
    float x_increment;
    float y_increment;

    int audio_offset;
    void *audio_buffer_data;
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

    state->u_offset_id = glGetUniformLocation(state->program_a.program, "u_offset");
    if (state->u_offset_id < 0) {
        char info_log[] = "Could not locate u_offset uniform";
        return memory->platform_fail(ctx, info_log);
    }
    state->a_color_id = glGetAttribLocation(state->program_a.program, "a_color");
    if (state->a_color_id < 0) {
        char info_log[] = "Could not locate a_color attribute";
        return memory->platform_fail(ctx, info_log);
    }
    state->a_pos_id = glGetAttribLocation(state->program_a.program, "a_pos");
    if (state->a_pos_id < 0) {
        char info_log[] = "Could not locate a_pos attribute";
        return memory->platform_fail(ctx, info_log);
    }

    glGenVertexArrays(1, &state->vao);
    glBindVertexArray(state->vao);

    glGenBuffers(1, &state->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    vertex vertices[4] = {
        {{-0.5, 0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5, 0.5, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 0.5,-0.5, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{-0.5,-0.5, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}},
    };
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(state->a_pos_id);
    glEnableVertexAttribArray(state->a_color_id);
    glVertexAttribPointer(state->a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer(state->a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
}

GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

    if (!glCreateProgram)
    {
        load_glfuncs(ctx, memory->platform_glgetprocaddress);
    }
    if (!memory->initialized)
    {
        state->x = 0;
        state->y = 0;
        state->x_increment = 0.02f;
        state->y_increment = 0.002f;

        gl_setup(ctx, memory);

        memory->initialized = 1;

        int volume = 3000;
        float pi = 3.14159265358979f;
        state->audio_buffer_data = malloc(48000 * 2 * (16 / 8));
        for (int i = 0; i < 48000 * 2;)
        {
            volume = i < 48000 ? i / 16 : abs(96000 - i) / 16;
            ((uint16_t *)state->audio_buffer_data)[i++] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565 / 48000.0));
            ((uint16_t *)state->audio_buffer_data)[i++] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565 / 48000.0));
        }
    }

    state->x += state->x_increment;
    state->y += state->y_increment;
    if ((state->x < -0.5) || (state->x > 0.5))
    {
        state->x_increment *= -1;
    }
    if ((state->y < -0.5) || (state->y > 0.5))
    {
        state->y_increment *= -1;
    }

    glUseProgram(state->program_a.program);
    glClearColor(0.1f, 0.1f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    float position[] = {state->x, state->y};
    glUniform2fv(state->u_offset_id, 1, (float *)&position);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    sound_output->samples = &(((uint8_t *)state->audio_buffer_data)[state->audio_offset * 2 * sound_output->channels * sound_output->number_of_samples]);
    state->audio_offset = (state->audio_offset + 1) % 60;
}

