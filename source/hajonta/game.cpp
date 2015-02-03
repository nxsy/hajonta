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
#include <OpenGL/gl3.h>
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

inline void
glErrorAssert()
{
    GLenum error = glGetError();
    switch(error)
    {
        case GL_NO_ERROR:
        {
            return;
        } break;
        case GL_INVALID_ENUM:
        {
            hassert(!"Invalid enum");
        } break;
        case GL_INVALID_VALUE:
        {
            hassert(!"Invalid value");
        } break;
        case GL_INVALID_OPERATION:
        {
            hassert(!"Invalid operation");
        } break;
#if !defined(_WIN32)
        case GL_INVALID_FRAMEBUFFER_OPERATION:
        {
            hassert(!"Invalid framebuffer operation");
        } break;
#endif
        case GL_OUT_OF_MEMORY:
        {
            hassert(!"Out of memory");
        } break;
#if !defined(__APPLE__) && !defined(NEEDS_EGL)
        case GL_STACK_UNDERFLOW:
        {
            hassert(!"Stack underflow");
        } break;
        case GL_STACK_OVERFLOW:
        {
            hassert(!"Stack overflow");
        } break;
#endif
        default:
        {
            hassert(!"Unknown error");
        } break;
    }
}

#define fps_buffer_width 960
#define fps_buffer_height 14

enum struct top_level_demo {
    MENU,
    RAINBOW,
};

struct demo_rainbow_state
{
    a_program_struct program_a;
    v2 velocity;
    v2 position;

    uint32_t vbo;

    int audio_offset;
    void *audio_buffer_data;
};

struct game_state
{
    top_level_demo demo;
    debug_font_program_struct program_debug_font;

    demo_rainbow_state rainbow;
    uint32_t vao;

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
    glErrorAssert();
    game_state *state = (game_state *)memory->memory;

#if !defined(NEEDS_EGL)
    if (&glGenVertexArrays)
    {
        glGenVertexArrays(1, &state->vao);
        glBindVertexArray(state->vao);
    }
#endif

    bool loaded = debug_font_program(&state->program_debug_font, ctx, memory);
    if (!loaded)
    {
        return;
    }
    glErrorAssert();

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
    glErrorAssert();


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
}

void debug_output(game_state *state)
{
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
}

GAME_UPDATE_AND_RENDER(demo_menu)
{
    game_state *state = (game_state *)memory->memory;

    for (uint32_t i = 0;
            i < harray_count(input->controllers);
            ++i)
    {
        if (!input->controllers[i].is_active)
        {
            continue;
        }
        game_controller_state *controller = &input->controllers[i];
        if (controller->buttons.move_right.ended_down)
        {
            state->demo = top_level_demo::RAINBOW;
        }
    }

    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    char msg[1024];
    sprintf(msg, "MENU MENU MENU MENU");
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg);

    glClearColor(0.1f, 0.1f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    sound_output->samples = 0;
}

GAME_UPDATE_AND_RENDER(demo_rainbow)
{
    game_state *state = (game_state *)memory->memory;
    if (!state->rainbow.program_a.program)
    {
        state->rainbow.velocity = {0, 0};
        state->rainbow.position = {0, 0};

        bool loaded = a_program(&state->rainbow.program_a, ctx, memory);
        if (!loaded)
        {
            return;
        }
        glErrorAssert();

        glUseProgram(state->rainbow.program_a.program);
        glGenBuffers(1, &state->rainbow.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, state->rainbow.vbo);
        vertex vertices[4] = {
            {{-0.5, 0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
            {{ 0.5, 0.5, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
            {{ 0.5,-0.5, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
            {{-0.5,-0.5, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}},
        };
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(state->rainbow.program_a.a_pos_id);
        glEnableVertexAttribArray(state->rainbow.program_a.a_color_id);
        glVertexAttribPointer(state->rainbow.program_a.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
        glVertexAttribPointer(state->rainbow.program_a.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

        glErrorAssert();


        int volume = 3000;
        float pi = 3.14159265358979f;
        state->rainbow.audio_buffer_data = malloc(48000 * 2 * (16 / 8));
        for (int i = 0; i < 48000 * 2;)
        {
            volume = i < 48000 ? i / 16 : abs(96000 - i) / 16;
            ((uint16_t *)state->rainbow.audio_buffer_data)[i] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565f / 48000.0f));
            ((uint16_t *)state->rainbow.audio_buffer_data)[i+1] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565f / 48000.0f));
            i += 2;
        }
    }

    float delta_t = input->delta_t;
    glClearColor(0.1f, 0.1f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

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
        if (
            controller->buttons.move_up.ended_down &&
            controller->buttons.move_down.ended_down &&
            controller->buttons.move_left.ended_down
           )
        {
            state->demo = top_level_demo::MENU;
        }
    }

    acceleration = v2normalize(acceleration);
    acceleration = v2sub(acceleration, v2mul(state->rainbow.velocity, 5.0f));
    v2 movement = v2add(
            v2mul(acceleration, 0.5f * (delta_t * delta_t)),
            v2mul(state->rainbow.velocity, delta_t)
    );
    state->rainbow.position = v2add(state->rainbow.position, movement);
    state->rainbow.velocity = v2add(
            v2mul(acceleration, delta_t),
            state->rainbow.velocity
    );
    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    char msg[1024];
    sprintf(msg, "P: %+.2f, %+.2f", state->rainbow.position.x, state->rainbow.position.y);
    sprintf(msg + strlen(msg), " V: %+.2f, %+.2f", state->rainbow.velocity.x, state->rainbow.velocity.y);
    sprintf(msg + strlen(msg), " A: %+.2f, %+.2f", acceleration.x, acceleration.y);
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg);

    glUseProgram(state->rainbow.program_a.program);
    glBindBuffer(GL_ARRAY_BUFFER, state->rainbow.vbo);
    glEnableVertexAttribArray(state->rainbow.program_a.a_pos_id);
    glEnableVertexAttribArray(state->rainbow.program_a.a_color_id);
    glVertexAttribPointer(state->rainbow.program_a.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer(state->rainbow.program_a.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));
    glUniform2fv(state->rainbow.program_a.u_offset_id, 1, (float *)&state->rainbow.position);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


    sound_output->samples = &(((uint8_t *)state->rainbow.audio_buffer_data)[state->rainbow.audio_offset * 2 * sound_output->channels * sound_output->number_of_samples]);
    state->rainbow.audio_offset = (state->rainbow.audio_offset + 1) % 60;
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

#if !defined(NEEDS_EGL) && !defined(__APPLE__)
    if (!glCreateProgram)
    {
        load_glfuncs(ctx, memory->platform_glgetprocaddress);
    }
#endif
    if (!memory->initialized)
    {
        state->demo = top_level_demo::MENU;
        gl_setup(ctx, memory);
        memory->initialized = 1;
    }

    glErrorAssert();
    game_update_and_render_func *demo;
    switch (state->demo)
    {
        case top_level_demo::MENU:
        {
            demo = demo_menu;
        } break;
        case top_level_demo::RAINBOW:
        {
            demo = demo_rainbow;
        } break;
        default:
        {
            memory->platform_fail(ctx, "Woah!?");
            return;
        } break;
    }
    demo(ctx, memory, input, sound_output);
    glErrorAssert();
    debug_output(state);
    glErrorAssert();
}

