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
#include "hajonta/math.cpp"

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

struct face_index
{
    uint32_t vertex;
    uint32_t texture_coord;
};

struct face
{
    face_index indices[3];
};


struct game_state
{
    uint32_t vao;

    char *objfile;
    uint32_t objfile_size;

    v3 vertices[10000];
    uint32_t num_vertices;
    v3 texture_coords[10000];
    uint32_t num_texture_coords;
    face faces[10000];
    uint32_t num_faces;
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
        glErrorAssert();
    }
#endif

    glErrorAssert();

    return true;
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

    static uint32_t last_active_demo = UINT32_MAX;

#if !defined(NEEDS_EGL) && !defined(__APPLE__)
    if (!glCreateProgram)
    {
        load_glfuncs(ctx, memory->platform_glgetprocaddress);
    }
#endif

    if (!memory->initialized)
    {
        if(!gl_setup(ctx, memory))
        {
            return;
        }
        memory->initialized = 1;

        while (!memory->platform_editor_load_file(ctx, &state->objfile, &state->objfile_size))
        {
        }
        char *position = (char *)state->objfile;
        char *eol;
        uint32_t max_lines = 100;
        uint32_t counter = 0;
        while ((eol = strchr(position, '\n')) || (eol = strchr(position, '\r')) || (eol = strchr(position, '\0')))
        {
            if (eol > (state->objfile + state->objfile_size))
            {
                eol = state->objfile + state->objfile_size;
                if (position == eol)
                {
                    break;
                }
            }
            char msg[1024];
            char line[1024];
            strncpy(line, position, (size_t)(eol - position));
            line[eol - position] = '\0';
            sprintf(msg, "position: %d; eol: %d; line: %s\n", position - state->objfile, eol - position, line);
            memory->platform_debug_message(ctx, msg);

            if (position[0] == 'v')
            {
                if (position[1] == 't')
                {
                    float a, b, c;
                    if (sscanf(position + 2, "%f %f %f", &a, &b, &c) == 3)
                    {
                        v3 texture_coord = {a,b,c};
                        state->texture_coords[state->num_texture_coords++] = texture_coord;
                        memory->platform_debug_message(ctx, "Saved texture coords\n");
                    }
                    else
                    {
                        memory->platform_debug_message(ctx, "Failed to decode texture coords\n");
                        break;
                    }
                }
                else if (position[1] == ' ')
                {
                    float a, b, c;
                    if (sscanf(position + 2, "%f %f %f", &a, &b, &c) == 3)
                    {
                        v3 vertex = {a,b,c};
                        state->vertices[state->num_vertices++] = vertex;
                    }
                }
                else
                {
                    memory->platform_debug_message(ctx, "Unknown command\n");
                    break;

                }
            }
            else if (position[0] == 'f')
            {
                char a[100], b[100], c[100], d[100];
                int num_found = sscanf(position + 2, "%s %s %s %s", &a, &b, &c, &d);
                if (num_found == 4)
                {
                    uint32_t t1, t2;
                    int num_found2 = sscanf(a, "%d/%d", &t1, &t2);
                    if (num_found2 == 2)
                    {
                        uint32_t a_vertex_id, a_texture_coord_id;
                        uint32_t b_vertex_id, b_texture_coord_id;
                        uint32_t c_vertex_id, c_texture_coord_id;
                        uint32_t d_vertex_id, d_texture_coord_id;
                        int num_found2;
                        num_found2 = sscanf(a, "%d/%d", &a_vertex_id, &a_texture_coord_id);
                        hassert(num_found2 == 2);
                        num_found2 = sscanf(b, "%d/%d", &b_vertex_id, &b_texture_coord_id);
                        hassert(num_found2 == 2);
                        num_found2 = sscanf(c, "%d/%d", &c_vertex_id, &c_texture_coord_id);
                        hassert(num_found2 == 2);
                        num_found2 = sscanf(d, "%d/%d", &d_vertex_id, &d_texture_coord_id);
                        hassert(num_found2 == 2);
                        face face1 = {
                            {
                                {a_vertex_id, a_texture_coord_id},
                                {b_vertex_id, b_texture_coord_id},
                                {c_vertex_id, c_texture_coord_id},
                            },
                        };
                        face face2 = {
                            {
                                {c_vertex_id, c_texture_coord_id},
                                {d_vertex_id, d_texture_coord_id},
                                {a_vertex_id, a_texture_coord_id},
                            }
                        };
                        state->faces[state->num_faces++] = face1;
                        state->faces[state->num_faces++] = face2;
                    }
                }
                char msg[100];
                sprintf(msg, "Found %d face positions\n", num_found);
                memory->platform_debug_message(ctx, msg);
            }

            if (*eol == '\0')
            {
                break;
            }
            position = eol + 1;
            if (counter++ >= max_lines)
            {
                break;
            }
        }
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
        if (controller->buttons.back.ended_down && !controller->buttons.back.repeat)
        {
            memory->quit = true;
        }
    }

    glBindVertexArray(state->vao);

    glErrorAssert();

    // Revert to something resembling defaults
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);

    glErrorAssert();

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glErrorAssert();
}

