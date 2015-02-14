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
#include "hajonta/programs/b.h"

#include "hajonta/math.cpp"
#include "hajonta/bmp.cpp"
#include "hajonta/font.cpp"

#include "hajonta/demos/demos.h"

static float pi = 3.14159265358979f;

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

struct demo_data {
    char *name;
    void *func;
};

struct demo_rotate_state {
    triangle2 t;
    v2 rotation_point;
    uint32_t vbo;
    float delta_t;
    bool paused;
};

struct game_state
{
    uint32_t active_demo;
    demo_data *demos;
    uint32_t number_of_demos;

    a_program_struct program_a;
    b_program_struct program_b;
    debug_font_program_struct program_debug_font;

    uint32_t vao;

    demo_menu_state menu;
    demo_rainbow_state rainbow;
    demo_normals_state normals;
    demo_collision_state collision;
    demo_bounce_state bounce;
    demo_rotate_state rotate;

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

struct vertex_with_style
{
    vertex v;
    float style[4];
};

bool
gl_setup(hajonta_thread_context *ctx, platform_memory *memory)
{
    glErrorAssert();
    game_state *state = (game_state *)memory->memory;

#if !defined(NEEDS_EGL)
    if (glGenVertexArrays != 0)
    {
        glGenVertexArrays(1, &state->vao);
        glBindVertexArray(state->vao);
    }
#endif

    bool loaded;

    loaded = a_program(&state->program_a, ctx, memory);
    if (!loaded)
    {
        return loaded;
    }

    loaded = b_program(&state->program_b, ctx, memory);
    if (!loaded)
    {
        return loaded;
    }

    loaded = debug_font_program(&state->program_debug_font, ctx, memory);
    if (!loaded)
    {
        return loaded;
    }
    glErrorAssert();

    glUseProgram(state->program_debug_font.program);

    {
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
    }

    if (!memory->platform_load_asset(ctx, "fonts/kenpixel_future/kenpixel_future_regular_14.zfi", sizeof(state->debug_font.zfi), state->debug_font.zfi))
    {
        memory->platform_fail(ctx, "Failed to open zfi file");
        return false;
    }
    if (!memory->platform_load_asset(ctx, "fonts/kenpixel_future/kenpixel_future_regular_14.bmp", sizeof(state->debug_font.bmp), state->debug_font.bmp))
    {
        memory->platform_fail(ctx, "Failed to open bmp file");
        return false;
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

    return true;
}

void font_output(game_state *state, draw_buffer b, uint32_t vbo, uint32_t texture_id)
{
    glUseProgram(state->program_debug_font.program);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray((GLuint)state->program_debug_font.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_debug_font.a_tex_coord_id);
    glVertexAttribPointer((GLuint)state->program_debug_font.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer((GLuint)state->program_debug_font.a_tex_coord_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexSubImage2D(GL_TEXTURE_2D,
        0,
        0,
        0,
        (GLsizei)b.width,
        (GLsizei)b.height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        b.memory);
    glUniform1i(state->fps_sampler_id, 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void debug_output(game_state *state)
{
    font_output(state, state->fps_draw_buffer, state->fps_vbo, state->fps_texture_id);
}

GAME_UPDATE_AND_RENDER(demo_rotate)
{
    game_state *state = (game_state *)memory->memory;
    demo_rotate_state *demo_state = &state->rotate;

    glClearColor(0.0f, 0.0f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!demo_state->vbo)
    {
        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
        glErrorAssert();
        demo_state->rotation_point = {0.0f, 0.0f};
        demo_state->t = {
            { 0.0f, 6.0f},
            {-5.2f,-3.0f},
            { 5.2f,-3.0f},
        };
    }


    for (uint32_t i = 0;
            i < harray_count(input->controllers);
            ++i)
    {
        if (!input->controllers[i].is_active)
        {
            continue;
        }

        game_controller_state *controller = &input->controllers[i];
        if (controller->buttons.back.ended_down)
        {
            state->active_demo = 0;
        }
        if (controller->buttons.start.ended_down && !controller->buttons.start.repeat)
        {
            demo_state->paused ^= true;
        }
    }

    if (!demo_state->paused)
    {
        demo_state->delta_t += input->delta_t;
    }

    glUseProgram(state->program_b.program);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);

    glEnableVertexAttribArray((GLuint)state->program_b.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_color_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_style_id);
    glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), 0);
    glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex, color));
    glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex_with_style, style));

    triangle2 *t = &demo_state->t;

    vertex_with_style vertices[] = {
        { { {-3.0f, 3.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
        { { { 3.0f, 3.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
        { { { 3.0f,-3.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
        { { {-3.0f,-3.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
    };

    float ratio = 960.0f / 540.0f;
    for (uint32_t vi = 0;
            vi < harray_count(vertices);
            ++vi)
    {
        vertex_with_style *v = vertices + vi;
        v->v.position[0] = v->v.position[0] / 10.0f;
        v->v.position[1] = v->v.position[1] / 10.0f;
    }

    glErrorAssert();
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glErrorAssert();
    v2 position = {0,0};
    glUniform2fv(state->program_b.u_offset_id, 1, (float *)&position);
    glErrorAssert();
    v4 u_mvp_enabled = {1.0f, 0.0f, 0.0f, 0.0f};
    glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
    glErrorAssert();
    v3 axis = {0.0f, 0.0f, -1.0f};
    m4 u_model = m4rotation(axis, demo_state->delta_t);
    glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
    m4 u_view = m4identity();
    glUniformMatrix4fv(state->program_b.u_view_id, 1, false, (float *)&u_view);
    m4 u_perspective = m4identity();
    u_perspective.cols[0].E[0] = 1 / ratio;
    glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);
    glErrorAssert();
    glDrawArrays(GL_TRIANGLE_FAN, 0, harray_count(vertices));
    glErrorAssert();

    // circle
    vertex_with_style circle_vertices[64+1];
    for (uint32_t idx = 0;
            idx < harray_count(circle_vertices);
            ++idx)
    {
        vertex_with_style *v = circle_vertices + idx;
        float a = idx * (2.0f * pi) / (harray_count(circle_vertices) - 1);
        *v = {
            {
                {sinf(a) * 0.3f, cosf(a) * 0.3f, 0.0f, 1.0f},
                {1.0f, 1.0f, 1.0f, 0.5f},
            },
            {0.0f, 0.0f, 0.0f, 0.0f},
        };
    }
    u_model = m4identity();
    glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
    glErrorAssert();
    glBufferData(GL_ARRAY_BUFFER, sizeof(circle_vertices), circle_vertices, GL_STATIC_DRAW);
    glErrorAssert();
    glDrawArrays(GL_LINE_STRIP, 0, harray_count(circle_vertices));
}

#include "hajonta/demos/demos.cpp"

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
        state->active_demo = 0;
        state->demos = (demo_data *)menu_items;
        state->number_of_demos = harray_count(menu_items);
        if(!gl_setup(ctx, memory))
        {
            return;
        }
        memory->initialized = 1;
    }

    glErrorAssert();
    game_update_and_render_func *demo = (game_update_and_render_func *)state->demos[(uint32_t)state->active_demo].func;
    demo(ctx, memory, input, sound_output);
    glErrorAssert();
    debug_output(state);
    glErrorAssert();
}

