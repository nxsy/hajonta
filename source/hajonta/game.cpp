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

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!demo_state->vbo)
    {
        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
        glGenBuffers(1, &demo_state->ibo);
        GLushort cube_elements[] = {
            // 0. front top right
            // 1. front top left
            // 2. front bottom right
            // 3. front bottom left
            // 4. back top right
            // 5. back top left
            // 6. back bottom right
            // 7. back bottom left
            //
            // front
            0, 1, 3,
            3, 2, 0,
            // bottom
            3, 2, 6,
            6, 7, 3,
            // left
            5, 1, 3,
            3, 7, 5,
            // top
            5, 4, 0,
            0, 1, 5,
            // right
            0, 4, 6,
            6, 2, 0,
            // back
            5, 4, 6,
            6, 7, 5,
        };
        demo_state->ibo_length = harray_count(cube_elements);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);

        glGenBuffers(1, &demo_state->line_ibo);
        GLushort line_elements[] = {
            0, 1, 1, 3,
            3, 2, 2, 0,
            // bottom
            3, 2, 2, 6,
            6, 7, 7, 3,
            // left
            5, 1, 1, 3,
            3, 7, 7, 5,
            // top
            5, 4, 4, 0,
            0, 1, 1, 5,
            // right
            0, 4, 4, 6,
            6, 2, 2, 0,
            // back
            5, 4, 4, 6,
            6, 7, 7, 5,
        };
        demo_state->line_ibo_length = harray_count(line_elements);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->line_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(line_elements), line_elements, GL_STATIC_DRAW);
        glErrorAssert();
        demo_state->near_ = {5.0f};
        demo_state->far_ = {50.0f};
        demo_state->spacing = {0.0f};
        demo_state->back_size = {1.0f};

        demo_state->matrix_draw_buffer.memory = demo_state->matrix_buffer;
        demo_state->matrix_draw_buffer.width = matrix_buffer_width;
        demo_state->matrix_draw_buffer.height = matrix_buffer_height;
        demo_state->matrix_draw_buffer.pitch = 4 * demo_state->matrix_draw_buffer.width;

        glGenTextures(1, &demo_state->texture_id);
        glBindTexture(GL_TEXTURE_2D, demo_state->texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            (GLsizei)demo_state->matrix_draw_buffer.width, (GLsizei)demo_state->matrix_draw_buffer.height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, demo_state->matrix_buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
            demo_state->frustum_mode ^= true;
        }
        if (demo_state->frustum_mode)
        {
            if (controller->buttons.move_up.ended_down)
            {
                demo_state->far_ += 1.0f;
            }
            if (controller->buttons.move_down.ended_down)
            {
                demo_state->far_ -= 1.0f;
            }
            if (controller->buttons.move_left.ended_down)
            {
                demo_state->back_size -= 0.1f;
            }
            if (controller->buttons.move_right.ended_down)
            {
                demo_state->back_size += 0.1f;
            }
        }
        else
        {
            if (controller->buttons.move_up.ended_down)
            {
                demo_state->spacing += 0.02f;
            }
            if (controller->buttons.move_down.ended_down)
            {
                demo_state->spacing -= 0.02f;
            }
            if (controller->buttons.move_left.ended_down)
            {
                demo_state->near_ -= 0.01f;
            }
            if (controller->buttons.move_right.ended_down)
            {
                demo_state->near_ += 0.01f;
            }
        }
    }
    if (demo_state->spacing < 0.00001f)
    {
        demo_state->spacing = 0.00001f;
    }
    if (demo_state->spacing > 8.0f)
    {
        demo_state->spacing = 8.0f;
    }

    if (demo_state->near_ > 4.99999f)
    {
        demo_state->near_ = 4.99999f;
    }

    demo_state->delta_t += input->delta_t;

    glUseProgram(state->program_b.program);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);

    glEnableVertexAttribArray((GLuint)state->program_b.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_color_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_style_id);
    glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), 0);
    glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex, color));
    glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex_with_style, style));

    struct mdtm{
        float s; // size_multiplier;
        float t; // time_multiplier;
    };
    mdtm multipliers[] = {
        {-0.16f, 0.5f},
        {-0.32f,-0.5f},
        {-0.48f, 1.0f},
    };
    float ratio = 960.0f / 540.0f;
    for (uint32_t circle_idx = 0;
            circle_idx < harray_count(multipliers);
            ++circle_idx)
    {
        mdtm *m = multipliers + circle_idx;
        vertex_with_style vertices[] = {
            { { {-m->s, m->s, m->s * demo_state->spacing - 5.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            { { { m->s, m->s, m->s * demo_state->spacing - 5.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            { { { m->s,-m->s, m->s * demo_state->spacing - 5.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            { { {-m->s,-m->s, m->s * demo_state->spacing - 5.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
        };

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
        m4 u_model = m4rotation(axis, demo_state->delta_t * m->t);
        glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
        m4 u_view = m4identity();
        glUniformMatrix4fv(state->program_b.u_view_id, 1, false, (float *)&u_view);
        m4 u_perspective = m4frustumprojection(demo_state->near_, demo_state->far_, {-ratio * demo_state->back_size, -1.0f * demo_state->back_size}, {ratio * demo_state->back_size, 1.0f * demo_state->back_size});
        glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);
        glErrorAssert();
        glDrawArrays(GL_TRIANGLE_FAN, 0, harray_count(vertices));
        glErrorAssert();
    }

    for (uint32_t circle_idx = 0;
            circle_idx < harray_count(multipliers);
            ++circle_idx)
    {
        mdtm *m = multipliers + circle_idx;
        vertex_with_style vertices[] = {
            // front top right
            { { { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // front top left
            { { {-1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // front bottom right
            { { { 1.0f,-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // front bottom left
            { { {-1.0f,-1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },

            // back top right
            { { { 1.0f, 1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // back top left
            { { {-1.0f, 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // back bottom right
            { { { 1.0f,-1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // back bottom left
            { { {-1.0f,-1.0f,-1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
        };
        v3 axis = {0.1f, 0.5f, -1.0f};
        axis = v3normalize(axis);
        m4 rotate = m4rotation(axis, demo_state->delta_t);
        m4 scale = m4identity();
        m4 translate = m4identity();
        translate.cols[3] = v4{7.0f * m->t, 0.0f, -10.0f * m->t - 20.0f, 1.0f};

        m4 a = scale;
        m4 b = rotate;
        m4 c = translate;

        m4 u_model = m4mul(c, m4mul(a, b));

        v4 u_mvp_enabled = {1.0f, 0.0f, 0.0f, 0.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
        glErrorAssert();
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glErrorAssert();
        m4 u_perspective = m4frustumprojection(demo_state->near_, demo_state->far_, {-ratio * demo_state->back_size, -1.0f * demo_state->back_size}, {ratio * demo_state->back_size, 1.0f * demo_state->back_size});
        glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ibo);
        glDrawElements(GL_TRIANGLES, (GLsizei)demo_state->ibo_length, GL_UNSIGNED_SHORT, 0);

        u_mvp_enabled = {1.0f, 0.0f, 0.0f, 1.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->line_ibo);
        glDrawElements(GL_LINES, (GLsizei)demo_state->line_ibo_length, GL_UNSIGNED_SHORT, 0);
    }

    for (uint32_t circle_idx = 0;
            circle_idx < harray_count(multipliers);
            ++circle_idx)
    {
        mdtm *m = multipliers + circle_idx;
        // circle
        vertex_with_style circle_vertices[128+1];
        for (uint32_t idx = 0;
                idx < harray_count(circle_vertices);
                ++idx)
        {
            vertex_with_style *v = circle_vertices + idx;
            float a = idx * (2.0f * pi) / (harray_count(circle_vertices) - 1);
            *v = {
                {
                    {sinf(a) * m->s, cosf(a) * m->s, m->s, 1.0f},
                    {1.0f, 1.0f, 1.0f, 0.5f},
                },
                {0.0f, 0.0f, 0.0f, 0.0f},
            };
        }
        v4 u_mvp_enabled = {1.0f, 0.0f, 0.0f, 0.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        m4 u_model = m4identity();
        glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
        glErrorAssert();
        glBufferData(GL_ARRAY_BUFFER, sizeof(circle_vertices), circle_vertices, GL_STATIC_DRAW);
        glErrorAssert();
        m4 u_perspective = m4identity();
        u_perspective.cols[0].E[0] = 1 / ratio;
        glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_LINE_STRIP, 0, harray_count(circle_vertices));
    }

    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    char msg[1024];
    sprintf(msg, "NEAR: %+.2f SPACING: %+.2f", demo_state->near_, demo_state->spacing);
    sprintf(msg + strlen(msg), " FAR: %+.2f BACK_SIZE: %+.2f", demo_state->far_, demo_state->back_size);
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg);
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

