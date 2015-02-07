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
    NORMALS,
};

struct demo_data {
    char *name;
    void *func;
};

struct demo_rainbow_state
{
    v2 velocity;
    v2 position;

    uint32_t vbo;

    int audio_offset;
    void *audio_buffer_data;
};

struct demo_normals_state
{
    uint32_t vbo;
    v2 line_ending;
    v2 line_velocity;
};

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

struct game_state
{
    top_level_demo active_demo;
    demo_data *demos;
    uint32_t number_of_demos;

    a_program_struct program_a;
    debug_font_program_struct program_debug_font;

    uint32_t vao;

    demo_menu_state menu;
    demo_rainbow_state rainbow;
    demo_normals_state normals;

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

    bool loaded;

    loaded = a_program(&state->program_a, ctx, memory);
    if (!loaded)
    {
        return;
    }

    loaded = debug_font_program(&state->program_debug_font, ctx, memory);
    if (!loaded)
    {
        return;
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

    {
        glGenBuffers(1, &state->menu.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, state->menu.vbo);
        float height = 14.0f / (540.0f / 2.0f);
        float width = 300.0f / (960.0f / 2.0f);
        vertex font_vertices[4] = {
            {{-width/2.0f, 0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
            {{ width/2.0f, 0, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}},
            {{ width/2.0f,-height, 0.0, 1.0}, {1.0, 0.0, 1.0, 1.0}},
            {{-width/2.0f,-height, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        };
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), font_vertices, GL_STATIC_DRAW);
    }

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

    state->menu.menu_draw_buffer.memory = state->menu.menu_buffer;
    state->menu.menu_draw_buffer.width = menu_buffer_width;
    state->menu.menu_draw_buffer.height = menu_buffer_height;
    state->menu.menu_draw_buffer.pitch = 4 * state->menu.menu_draw_buffer.width;

    glGenTextures(1, &state->menu.texture_id);
    glBindTexture(GL_TEXTURE_2D, state->menu.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        (GLsizei)state->menu.menu_draw_buffer.width, (GLsizei)state->menu.menu_draw_buffer.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, state->menu.menu_buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glErrorAssert();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
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
        if (controller->buttons.back.ended_down && !controller->buttons.back.repeat)
        {
            memory->quit = true;
        }
        if (controller->buttons.move_down.ended_down && !controller->buttons.move_down.repeat)
        {
            state->menu.selected_index++;
            state->menu.selected_index %= state->number_of_demos;
        }
        if (controller->buttons.move_up.ended_down && !controller->buttons.move_up.repeat)
        {
            if (state->menu.selected_index == 0)
            {
                state->menu.selected_index = state->number_of_demos - 1;
            }
            else
            {
                state->menu.selected_index--;
            }
        }
        if (controller->buttons.move_right.ended_down)
        {
            state->active_demo = (top_level_demo)state->menu.selected_index;
        }
        if (controller->buttons.start.ended_down)
        {
            state->active_demo = (top_level_demo)state->menu.selected_index;
        }
    }

    glClearColor(0.1f, 0.1f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, state->menu.vbo);
    float height = 14.0f / (540.0f / 2.0f);
    float width = 300.0f / (960.0f / 2.0f);
    for (uint32_t menu_index = 0;
            menu_index < state->number_of_demos;
            ++menu_index)
    {
        float offset = -height * 2 * menu_index;
        vertex font_vertices[4] = {
            {{-width/2.0f, offset, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
            {{ width/2.0f, offset, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}},
            {{ width/2.0f,offset-height, 0.0, 1.0}, {1.0, 0.0, 1.0, 1.0}},
            {{-width/2.0f,offset-height, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        };
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), font_vertices, GL_STATIC_DRAW);
        memset(state->menu.menu_buffer, 0, sizeof(state->menu.menu_buffer));
        write_to_buffer(&state->menu.menu_draw_buffer, &state->debug_font.font, state->demos[menu_index].name);
        if (menu_index == state->menu.selected_index)
        {
            for (uint32_t *b = (uint32_t *)state->menu.menu_buffer;
                    b < (uint32_t*)(state->menu.menu_buffer + sizeof(state->menu.menu_buffer));
                    ++b)
            {
                uint8_t *subpixel = (uint8_t *)b;
                uint8_t *alpha = subpixel + 3;
                if (*alpha)
                {
                    *b = 0xffff00ff;
                }
            }
        }
        font_output(state, state->menu.menu_draw_buffer, state->menu.vbo, state->menu.texture_id);
    }
    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, "MENU");

    sound_output->samples = 0;
}

GAME_UPDATE_AND_RENDER(demo_normals)
{
    game_state *state = (game_state *)memory->memory;
    demo_normals_state *demo_state = &state->normals;

    glClearColor(0.1f, 0.0f, 0.0f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!demo_state->vbo)
    {
        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
        glErrorAssert();

        demo_state->line_ending = {1.0f, 1.0f};
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
        if (controller->buttons.back.ended_down)
        {
            state->active_demo = top_level_demo::MENU;
        }
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

    float delta_t = input->delta_t;
    acceleration = v2normalize(acceleration);
    acceleration = v2sub(acceleration, v2mul(demo_state->line_velocity, 5.0f));
    v2 movement = v2add(
            v2mul(acceleration, 0.5f * (delta_t * delta_t)),
            v2mul(demo_state->line_velocity, delta_t)
    );
    demo_state->line_ending = v2add(demo_state->line_ending, movement);
    demo_state->line_velocity = v2add(
            v2mul(acceleration, delta_t),
            demo_state->line_velocity
    );

    glUseProgram(state->program_a.program);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);

    glEnableVertexAttribArray((GLuint)state->program_a.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_a.a_color_id);
    glVertexAttribPointer((GLuint)state->program_a.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer((GLuint)state->program_a.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

    v2 *lv = &demo_state->line_ending;

    float ratio = 960.0f / 540.0f;

    v2 rhn = v2normalize({-lv->y,  lv->x});
    v2 lhn = v2normalize({ lv->y, -lv->x});

    v2 nlv = v2normalize(*lv);
    v2 nlvrh = v2add(nlv, v2mul(rhn, 0.1f));
    v2 nlvlh = v2add(nlv, v2mul(lhn, 0.1f));

    v2 xproj = v2projection(v2{1,0}, *lv);
    v2 yproj = v2projection(v2{0,1}, *lv);

    vertex vertices[] = {
        {{ 0.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ rhn.x, rhn.y, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 0.0, 0.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{ lhn.x, lhn.y, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{ nlvrh.x, nlvrh.y, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ nlvlh.x, nlvlh.y, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 1.0, 1.0}},
        {{ xproj.x, xproj.y, 0.0, 1.0}, {0.0, 1.0, 1.0, 1.0}},
        {{ 0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}},
        {{ yproj.x, yproj.y, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}},
        {{ 0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}},
        {{ lv->x, lv->y, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}},
    };

    char msg[1024];
    sprintf(msg, "L: %+.2f, %+.2f", demo_state->line_ending.x, demo_state->line_ending.y);
    sprintf(msg + strlen(msg), "NL: %+.2f, %+.2f", nlv.x, nlv.y);
    sprintf(msg + strlen(msg), "RHN: %+.2f, %+.2f", rhn.x, rhn.y);
    sprintf(msg + strlen(msg), "LHN: %+.2f, %+.2f", lhn.x, lhn.y);
    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg);
    for (uint32_t vi = 0;
            vi < harray_count(vertices);
            ++vi)
    {
        vertex *v = vertices + vi;
        v->position[0] = v->position[0] / ratio * 0.3f;
        v->position[1] = v->position[1] * 0.3f;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    float position[] = {0,0};
    glUniform2fv(state->program_a.u_offset_id, 1, (float *)position);
    glDrawArrays(GL_LINES, 0, harray_count(vertices));
}

GAME_UPDATE_AND_RENDER(demo_rainbow)
{
    game_state *state = (game_state *)memory->memory;
    if (!state->rainbow.vbo)
    {
        state->rainbow.velocity = {0, 0};
        state->rainbow.position = {0, 0};

        glErrorAssert();

        glUseProgram(state->program_a.program);
        glGenBuffers(1, &state->rainbow.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, state->rainbow.vbo);
        vertex vertices[4] = {
            {{-0.5, 0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
            {{ 0.5, 0.5, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
            {{ 0.5,-0.5, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
            {{-0.5,-0.5, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}},
        };
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray((GLuint)state->program_a.a_pos_id);
        glEnableVertexAttribArray((GLuint)state->program_a.a_color_id);
        glVertexAttribPointer((GLuint)state->program_a.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
        glVertexAttribPointer((GLuint)state->program_a.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

        glErrorAssert();


        int volume = 3000;
        float pi = 3.14159265358979f;
        state->rainbow.audio_buffer_data = malloc(48000 * 2 * (16 / 8));
        for (int i = 0; i < 48000 * 2;)
        {
            volume = i < 48000 ? i / 16 : abs(96000 - i) / 16;
            ((int16_t *)state->rainbow.audio_buffer_data)[i] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565f / 48000.0f));
            ((int16_t *)state->rainbow.audio_buffer_data)[i+1] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565f / 48000.0f));
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
        if (controller->buttons.back.ended_down)
        {
            state->active_demo = top_level_demo::MENU;
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

    glUseProgram(state->program_a.program);
    glBindBuffer(GL_ARRAY_BUFFER, state->rainbow.vbo);
    glEnableVertexAttribArray((GLuint)state->program_a.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_a.a_color_id);
    glVertexAttribPointer((GLuint)state->program_a.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer((GLuint)state->program_a.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));
    glUniform2fv(state->program_a.u_offset_id, 1, (float *)&state->rainbow.position);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


    sound_output->samples = &(((uint8_t *)state->rainbow.audio_buffer_data)[state->rainbow.audio_offset * 2 * sound_output->channels * sound_output->number_of_samples]);
    state->rainbow.audio_offset = (state->rainbow.audio_offset + 1) % 60;
}

static const demo_data menu_items[] = {
    {"Menu", demo_menu},
    {"Rainbow", demo_rainbow},
    {"Normals", demo_normals},
};

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
        state->active_demo = top_level_demo::MENU;
        state->demos = (demo_data *)menu_items;
        state->number_of_demos = harray_count(menu_items);
        gl_setup(ctx, memory);
        memory->initialized = 1;
    }

    glErrorAssert();
    game_update_and_render_func *demo = (game_update_and_render_func *)state->demos[(uint32_t)state->active_demo].func;
    demo(ctx, memory, input, sound_output);
    glErrorAssert();
    debug_output(state);
    glErrorAssert();
}

