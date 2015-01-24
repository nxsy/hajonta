#include <stddef.h>

#include <windows.h>
#include <gl/gl.h>

#include "hajonta\platform\common.h"

struct game_state {
    int32_t program;

    int32_t u_offset_id;
    int32_t a_pos_id;
    int32_t a_color_id;

    uint32_t vao;
    uint32_t vbo;

    float x;
    float y;
    float x_increment;
    float y_increment;
};

struct vertex
{
    float position[4];
    float color[4];
};

GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

    if (!glCreateProgram)
    {
        load_glfuncs(ctx, memory->platform_glgetprocaddress);
    }
    if (!memory->initialized)
    {
        state->program = glCreateProgram();
        char *vertex_shader_source =
            "#version 150 \n"
            "uniform vec2 u_offset; \n"
            "in vec4 a_pos; \n"
            "in vec4 a_color; \n"
            "out vec4 v_color; \n"
            "void main (void) \n"
            "{ \n"
            "    v_color = a_color; \n"
            "    gl_Position = a_pos + vec4(u_offset, 0.0, 0.0);\n"
            "} \n"
            ;


        char *fragment_shader_source =
            "#version 150 \n"
            "in vec4 v_color; \n"
            "out vec4 o_color; \n"
            "void main(void) \n"
            "{ \n"
            "    o_color = v_color; \n"
            "} \n"
            ;

        uint32_t vertex_shader_id;
        uint32_t fragment_shader_id;

        {
            uint32_t shader = vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
            int compiled;
            glShaderSource(shader, 1, &vertex_shader_source, 0);
            glCompileShader(shader);
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled)
            {
                char info_log[1024];
                glGetShaderInfoLog(shader, sizeof(info_log), 0, info_log);
                return memory->platform_fail(ctx, info_log);
            }
        }
        {
            uint32_t shader = fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
            int compiled;
            glShaderSource(shader, 1, &fragment_shader_source, 0);
            glCompileShader(shader);
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled)
            {
                char info_log[1024];
                glGetShaderInfoLog(shader, sizeof(info_log), 0, info_log);
                return memory->platform_fail(ctx, info_log);
            }
        }
        glAttachShader(state->program, vertex_shader_id);
        glAttachShader(state->program, fragment_shader_id);
        glLinkProgram(state->program);

        int linked;
        glGetProgramiv(state->program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char info_log[1024];
            glGetProgramInfoLog(state->program, sizeof(info_log), 0, info_log);
            return memory->platform_fail(ctx, info_log);
        }

        glUseProgram(state->program);

        state->u_offset_id = glGetUniformLocation(state->program, "u_offset");
        if (state->u_offset_id < 0) {
            char info_log[] = "Could not locate u_offset uniform";
            return memory->platform_fail(ctx, info_log);
        }
        state->a_color_id = glGetAttribLocation(state->program, "a_color");
        if (state->a_color_id < 0) {
            char info_log[] = "Could not locate a_color attribute";
            return memory->platform_fail(ctx, info_log);
        }
        state->a_pos_id = glGetAttribLocation(state->program, "a_pos");
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

        state->x = 0;
        state->y = 0;
        state->x_increment = 0.02f;
        state->y_increment = 0.002f;

        memory->initialized = 1;
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

    glUseProgram(state->program);
    glClearColor(0.1f, 0.1f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    float position[] = {state->x, state->y};
    glUniform2fv(state->u_offset_id, 1, (float *)&position);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

