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
#include "hajonta/image.cpp"
#include "hajonta/bmp.cpp"
#include "hajonta/font.cpp"

#include "hajonta/programs/b.h"

static float pi = 3.14159265358979f;

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

struct vertex
{
    float position[4];
    float color[4];
};

struct editor_vertex_format
{
    vertex v;
    float style[4];
    v4 normal;
    v4 tangent;
};

struct face_index
{
    uint32_t vertex;
    uint32_t texture_coord;
};

struct face
{
    face_index indices[3];
    int texture_offset;
    int bump_texture_offset;
};

struct material
{
    char name[100];
    int32_t texture_offset;
    int32_t bump_texture_offset;
};

struct kenpixel_future_14
{
    uint8_t zfi[5159];
    uint8_t bmp[131210];
    font_data font;
};

struct game_state
{
    uint32_t vao;
    uint32_t vbo;
    uint32_t ibo;
    uint32_t line_ibo;
    int32_t sampler_ids[6];
    uint32_t texture_ids[10];
    uint32_t num_texture_ids;
    uint32_t aabb_cube_vbo;
    uint32_t aabb_cube_ibo;
    uint32_t bounding_sphere_vbo;
    uint32_t bounding_sphere_ibo;
    uint32_t mouse_vbo;
    uint32_t mouse_texture;

    uint32_t num_bounding_sphere_elements;

    float near_;
    float far_;
    float delta_t;

    loaded_file model_file;
    loaded_file mtl_file;

    char *objfile;
    uint32_t objfile_size;

    v3 vertices[100000];
    uint32_t num_vertices;
    v3 texture_coords[100000];
    uint32_t num_texture_coords;
    face faces[100000];
    uint32_t num_faces;

    material materials[10];
    uint32_t num_materials;

    GLushort faces_array[300000];
    uint32_t num_faces_array;
    GLushort line_elements[600000];
    uint32_t num_line_elements;
    editor_vertex_format vbo_vertices[300000];
    uint32_t num_vbo_vertices;

    b_program_struct program_b;

    v3 model_max;
    v3 model_min;

    char bitmap_scratch[2048 * 2048 * 4];

    bool hide_lines;
    int model_mode;
    int shading_mode;

    int x_rotation;
    int y_rotation;

    uint8_t mouse_bitmap[4096];

    uint32_t debug_texture_id;
    uint32_t debug_vbo;
    kenpixel_future_14 debug_font;
    draw_buffer debug_draw_buffer;
#define debug_buffer_width 960
#define debug_buffer_height 14
    uint8_t debug_buffer[4 * debug_buffer_width * debug_buffer_height];
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

    bool loaded;

    loaded = b_program(&state->program_b, ctx, memory);
    if (!loaded)
    {
        return loaded;
    }

    return true;
}

char *
next_newline(char *str, uint32_t str_length)
{
    for (uint32_t idx = 0;
            idx < str_length;
            ++idx)
    {
        switch (str[idx]) {
            case '\n':
            case '\r':
            case '\0':
                return str + idx;
        }
    }
    return str + str_length;
}

#define starts_with(line, s) (strncmp(line, s, sizeof(s) - 1) == 0)

bool
load_mtl(hajonta_thread_context *ctx, platform_memory *memory)
{
    game_state *state = (game_state *)memory->memory;


    char *start_position = (char *)state->mtl_file.contents;
    char *eof = start_position + state->mtl_file.size;
    char *position = start_position;
    uint32_t max_lines = 100000;
    uint32_t counter = 0;
    material *current_material = 0;
    while (position < eof)
    {
        uint32_t remainder = state->mtl_file.size - (uint32_t)(position - start_position);
        char *eol = next_newline(position, remainder);
        char _line[1024];
        strncpy(_line, position, (size_t)(eol - position));
        _line[eol - position] = '\0';
        char *line = _line;
        /*
        char msg[1024];
        sprintf(msg, "position: %d; eol: %d; line: %s\n", position - state->mtl_file.contents, eol - position, line);
        memory->platform_debug_message(ctx, msg);
        */

        for(;;)
        {
            if (line[0] == '\t')
            {
                line++;
                continue;
            }
            if (line[0] == ' ')
            {
                line++;
                continue;
            }
            break;
        }

        if (line[0] == '\0')
        {

        }
        else if (line[0] == '#')
        {

        }
        else if (starts_with(line, "newmtl "))
        {
            current_material = state->materials + state->num_materials++;
            strncpy(current_material->name, line + 7, (size_t)(eol - position - 7));
            current_material->texture_offset = -1;
            current_material->bump_texture_offset = -1;
        }
        else if (starts_with(line, "Ns "))
        {
        }
        else if (starts_with(line, "Ka "))
        {
        }
        else if (starts_with(line, "Kd "))
        {
        }
        else if (starts_with(line, "Ke "))
        {
        }
        else if (starts_with(line, "Ks "))
        {
        }
        else if (starts_with(line, "Ni "))
        {
        }
        else if (starts_with(line, "Ns "))
        {
        }
        else if (starts_with(line, "Tr "))
        {
        }
        else if (starts_with(line, "Tf "))
        {
        }
        else if (starts_with(line, "d "))
        {
        }
        else if (starts_with(line, "illum "))
        {
        }
        else if (starts_with(line, "map_Bump ") || starts_with(line, "map_bump "))
        {
            char *filename = line + sizeof("map_Bump ") - 1;
            hassert(strlen(filename) > 0);
            if (filename[0] == '.')
            {

            }
            else
            {
                loaded_file texture;
                bool loaded = memory->platform_editor_load_nearby_file(ctx, &texture, state->mtl_file, filename);
                hassert(loaded);
                int32_t x, y, size;
                load_image((uint8_t *)texture.contents, texture.size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch), &x, &y, &size, false);

                current_material->bump_texture_offset = (int32_t)(state->num_texture_ids++);
                glErrorAssert();
                glBindTexture(GL_TEXTURE_2D, state->texture_ids[current_material->bump_texture_offset]);
                glErrorAssert();
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    x, y, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, state->bitmap_scratch);
                glErrorAssert();
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glErrorAssert();
            }
        }
        else if (starts_with(line, "map_d "))
        {
        }
        else if (starts_with(line, "map_Kd "))
        {
            char *filename = line + sizeof("map_Kd ") - 1;
            hassert(strlen(filename) > 0);
            if (filename[0] == '.')
            {

            }
            else
            {
                loaded_file texture;
                bool loaded = memory->platform_editor_load_nearby_file(ctx, &texture, state->mtl_file, filename);
                hassert(loaded);
                int32_t x, y, size;
                load_image((uint8_t *)texture.contents, texture.size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch), &x, &y, &size, false);
                /*
                char msg[1024];
                sprintf(msg, "name = %s; x = %i, y = %i, size = %i", filename, x, y, size);
                memory->platform_debug_message(ctx, msg);
                */

                current_material->texture_offset = (int32_t)(state->num_texture_ids++);
                glErrorAssert();
                glBindTexture(GL_TEXTURE_2D, state->texture_ids[current_material->texture_offset]);
                glErrorAssert();
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    x, y, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, state->bitmap_scratch);
                glErrorAssert();
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glErrorAssert();
            }

        }
        else if (starts_with(line, "map_Ke "))
        {
        }
        else if (starts_with(line, "map_Ks "))
        {
        }
        else if (starts_with(line, "refl "))
        {
        }
        else if (starts_with(line, "bump "))
        {
        }
        else
        {
            hassert(!"Invalid code path");
        }
        if (counter++ > max_lines)
        {
            hassert(!"Too many lines in mtl file");
        }

        if (*eol == '\0')
        {
            break;
        }
        position = eol + 1;
        while((*position == '\r') && (*position == '\n'))
        {
            position++;
        }
        glErrorAssert();
    }
    return true;
}

void
load_aabb_buffer_objects(game_state *state, v3 model_min, v3 model_max)
{
    editor_vertex_format vertices[] = {
        {
            {model_min.x, model_min.y, model_min.z, 1.0},
        },
        {
            {model_max.x, model_min.y, model_min.z, 1.0},
        },
        {
            {model_max.x, model_max.y, model_min.z, 1.0},
        },
        {
            {model_min.x, model_max.y, model_min.z, 1.0},
        },
        {
            {model_min.x, model_min.y, model_max.z, 1.0},
        },
        {
            {model_max.x, model_min.y, model_max.z, 1.0},
        },
        {
            {model_max.x, model_max.y, model_max.z, 1.0},
        },
        {
            {model_min.x, model_max.y, model_max.z, 1.0},
        },
    };
    glBindBuffer(GL_ARRAY_BUFFER, state->aabb_cube_vbo);

    glBufferData(GL_ARRAY_BUFFER,
            (GLsizeiptr)sizeof(vertices),
            vertices,
            GL_STATIC_DRAW);
    glErrorAssert();

    GLushort elements[] = {
        0, 1,
        1, 2,
        2, 3,
        3, 0,

        4, 5,
        5, 6,
        6, 7,
        7, 4,

        0, 4,
        1, 5,
        2, 6,
        3, 7,
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->aabb_cube_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            (GLsizeiptr)sizeof(elements),
            elements,
            GL_STATIC_DRAW);
    glErrorAssert();
}

void
load_bounding_sphere(game_state *state, v3 model_min, v3 model_max)
{
    v3 center = v3div(v3add(state->model_max, state->model_min), 2.0f);
    v3 diff = v3sub(center, state->model_min);
    float size = v3length(diff);

    editor_vertex_format vertices[64 * 3] = {};
    for (uint32_t idx = 0;
            idx < harray_count(vertices);
            ++idx)
    {
        editor_vertex_format *v = vertices + idx;
        if (idx < 64)
        {
            float a = idx * (2.0f * pi) / (harray_count(vertices) / 3 - 1);
            *v = {
                {
                    {center.x + sinf(a) * size, center.y + cosf(a) * size, center.z, 1.0f},
                    {1.0f, 1.0f, 1.0f, 0.5f},
                },
                {0.0f, 0.0f, 0.0f, 0.0f},
            };
        }
        else if (idx < 128)
        {
            float a = (idx - 64) * (2.0f * pi) / (harray_count(vertices) / 3 - 1);
            *v = {
                {
                    {center.x, center.y + sinf(a) * size, center.z + cosf(a) * size, 1.0f},
                    {1.0f, 1.0f, 1.0f, 0.5f},
                },
                {0.0f, 0.0f, 0.0f, 0.0f},
            };
        }
        else
        {
            float a = (idx - 128) * (2.0f * pi) / (harray_count(vertices) / 3 - 1);
            *v = {
                {
                    {center.x + cosf(a) * size, center.y, center.z + sinf(a) * size, 1.0f},
                    {1.0f, 1.0f, 1.0f, 0.5f},
                },
                {0.0f, 0.0f, 0.0f, 0.0f},
            };
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, state->bounding_sphere_vbo);

    glBufferData(GL_ARRAY_BUFFER,
            (GLsizeiptr)sizeof(vertices),
            vertices,
            GL_STATIC_DRAW);
    glErrorAssert();

    GLushort elements[harray_count(vertices)];
    for (GLushort idx = 0;
            idx < harray_count(vertices);
            ++idx)
    {
        elements[idx] = idx;
        if ((idx % 64) == 63)
        {
            elements[idx] = 65535;
        }
    }

    state->num_bounding_sphere_elements = harray_count(elements);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->bounding_sphere_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            (GLsizeiptr)sizeof(elements),
            elements,
            GL_STATIC_DRAW);
    glErrorAssert();
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

        state->near_ = {5.0f};
        state->far_ = {50.0f};


        glErrorAssert();
        glGenBuffers(1, &state->vbo);
        glGenBuffers(1, &state->aabb_cube_vbo);
        glGenBuffers(1, &state->bounding_sphere_vbo);
        glGenBuffers(1, &state->ibo);
        glGenBuffers(1, &state->line_ibo);
        glGenBuffers(1, &state->aabb_cube_ibo);
        glGenBuffers(1, &state->bounding_sphere_ibo);
        glErrorAssert();
        glGenTextures(harray_count(state->texture_ids), state->texture_ids);
        glGenBuffers(1, &state->mouse_vbo);
        glGenTextures(1, &state->mouse_texture);

        {
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

            state->debug_draw_buffer.memory = state->debug_buffer;
            state->debug_draw_buffer.width = debug_buffer_width;
            state->debug_draw_buffer.height = debug_buffer_height;
            state->debug_draw_buffer.pitch = 4 * debug_buffer_width;

            glGenBuffers(1, &state->debug_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, state->debug_vbo);
            glGenTextures(1, &state->debug_texture_id);
            glBindTexture(GL_TEXTURE_2D, state->debug_texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                debug_buffer_width, debug_buffer_height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, state->debug_buffer);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            float height = 14.0f / ((float)input->window.height / 2.0f);
            float top = -(1-height);
            editor_vertex_format font_vertices[4] = {
                {
                    {
                        {-1.0, top, 0.0, 1.0},
                        { 0.0, 1.0, 1.0, 1.0},
                    },
                    {2.0, 0.0, 0.0, 0.0},
                },
                {
                    {
                        { 1.0, top, 0.0, 1.0},
                        { 1.0, 1.0, 1.0, 1.0},
                    },
                    {2.0, 0.0, 0.0, 0.0},
                },
                {
                    {
                        { 1.0,-1.0, 0.0, 1.0},
                        {1.0, 0.0, 1.0, 1.0},
                    },
                    {2.0, 0.0, 0.0, 0.0},
                },
                {
                    {
                        {-1.0,-1.0, 0.0, 1.0},
                        {0.0, 0.0, 1.0, 1.0},
                    },
                    {2.0, 0.0, 0.0, 0.0},
                },
            };
            glBufferData(GL_ARRAY_BUFFER, sizeof(font_vertices), font_vertices, GL_STATIC_DRAW);
        }

        {
            char *filename = "ui/slick_arrows/slick_arrow-delta.png";
            uint32_t a_filesize = 256;
            uint8_t image[1000];
            if (!memory->platform_load_asset(ctx, filename, a_filesize, image)) {
                memory->platform_fail(ctx, "Could not load ui/slick_arrows/slick_arrow-delta.png");
                memory->quit = true;
                return;
            }

            int32_t x, y, actual_size;
            load_image(image, a_filesize, state->mouse_bitmap, sizeof(state->mouse_bitmap),
                    &x, &y, &actual_size);

            char msg[1024];
            sprintf(msg, "x, y, size: %d, %d, %d\n", x, y, actual_size);
            memory->platform_debug_message(ctx, msg);

            glBindTexture(GL_TEXTURE_2D, state->mouse_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                x, y, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, state->mouse_bitmap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

        glErrorAssert();
        state->sampler_ids[0] = glGetUniformLocation(state->program_b.program, "tex");
        state->sampler_ids[1] = glGetUniformLocation(state->program_b.program, "tex1");
        state->sampler_ids[2] = glGetUniformLocation(state->program_b.program, "tex2");
        state->sampler_ids[3] = glGetUniformLocation(state->program_b.program, "tex3");
        state->sampler_ids[4] = glGetUniformLocation(state->program_b.program, "tex4");
        state->sampler_ids[5] = glGetUniformLocation(state->program_b.program, "tex5");
        glErrorAssert();

        while (!memory->platform_editor_load_file(ctx, &state->model_file))
        {
        }
        glErrorAssert();

        char *start_position = (char *)state->model_file.contents;
        char *eof = start_position + state->model_file.size;
        char *position = start_position;
        uint32_t max_lines = 200000;
        uint32_t counter = 0;
        material null_material = {};
        null_material.texture_offset = -1;
        null_material.bump_texture_offset = -1;
        material *current_material = &null_material;
        while (position < eof)
        {
            uint32_t remainder = state->model_file.size - (uint32_t)(position - start_position);
            char *eol = next_newline(position, remainder);
            char line[1024];
            strncpy(line, position, (size_t)(eol - position));
            line[eol - position] = '\0';
            /*
            char msg[1024];
            sprintf(msg, "position: %d; eol: %d; line: %s\n", position - state->model_file.contents, eol - position, line);
            memory->platform_debug_message(ctx, msg);
            */

            if (line[0] == '\0')
            {
            }
            else if (line[0] == 'v')
            {
                if (line[1] == 't')
                {
                    float a, b, c;
                    int num_found = sscanf(position + 2, "%f %f %f", &a, &b, &c);
                    if (num_found == 3)
                    {
                        v3 texture_coord = {a,b,c};
                        state->texture_coords[state->num_texture_coords++] = texture_coord;
                    }
                    else if (num_found == 2)
                    {
                        v3 texture_coord = {a, b, 0.0f};
                        state->texture_coords[state->num_texture_coords++] = texture_coord;
                    }
                    else
                    {
                        hassert(!"Invalid code path");
                    }
                }
                else if (line[1] == 'n')
                {

                }
                else if (line[1] == ' ')
                {
                    float a, b, c;
                    if (sscanf(line + 2, "%f %f %f", &a, &b, &c) == 3)
                    {
                        if (state->num_vertices == 0)
                        {
                            state->model_max = {a, b, c};
                            state->model_min = {a, b, c};
                        }
                        else
                        {
                            if (a > state->model_max.x)
                            {
                                state->model_max.x = a;
                            }
                            if (b > state->model_max.y)
                            {
                                state->model_max.y = b;
                            }
                            if (c > state->model_max.z)
                            {
                                state->model_max.z = c;
                            }
                            if (a < state->model_min.x)
                            {
                                state->model_min.x = a;
                            }
                            if (b < state->model_min.y)
                            {
                                state->model_min.y = b;
                            }
                            if (c < state->model_min.z)
                            {
                                state->model_min.z = c;
                            }
                        }
                        v3 vertex = {a,b,c};
                        state->vertices[state->num_vertices++] = vertex;
                    }
                }
                else
                {
                    hassert(!"Invalid code path");
                }
            }
            else if (line[0] == 'f')
            {
                char a[100], b[100], c[100], d[100];
                int num_found = sscanf(line + 2, "%s %s %s %s", (char *)&a, (char *)&b, (char *)&c, (char *)&d);
                hassert((num_found == 4) || (num_found == 3));
                if (num_found == 4)
                {
                    uint32_t t1, t2;
                    int num_found2 = sscanf(a, "%d/%d", &t1, &t2);
                    if (num_found2 == 1)
                    {

                    }
                    else if (num_found2 == 2)
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
                            current_material->texture_offset,
                            current_material->bump_texture_offset,
                        };
                        face face2 = {
                            {
                                {c_vertex_id, c_texture_coord_id},
                                {d_vertex_id, d_texture_coord_id},
                                {a_vertex_id, a_texture_coord_id},
                            },
                            current_material->texture_offset,
                            current_material->bump_texture_offset,
                        };
                        state->faces[state->num_faces++] = face1;
                        state->faces[state->num_faces++] = face2;
                    }
                    else
                    {
                        hassert(!"Invalid number of face attributes");
                    }
                }
                else if (num_found == 3)
                {
                    uint32_t t1, t2;
                    int num_found2 = sscanf(a, "%d/%d", &t1, &t2);
                    if (num_found2 == 1)
                    {

                    }
                    else if (num_found2 == 2)
                    {
                        uint32_t a_vertex_id, a_texture_coord_id;
                        uint32_t b_vertex_id, b_texture_coord_id;
                        uint32_t c_vertex_id, c_texture_coord_id;
                        int num_found2;
                        num_found2 = sscanf(a, "%d/%d", &a_vertex_id, &a_texture_coord_id);
                        hassert(num_found2 == 2);
                        num_found2 = sscanf(b, "%d/%d", &b_vertex_id, &b_texture_coord_id);
                        hassert(num_found2 == 2);
                        num_found2 = sscanf(c, "%d/%d", &c_vertex_id, &c_texture_coord_id);
                        hassert(num_found2 == 2);
                        face face1 = {
                            {
                                {a_vertex_id, a_texture_coord_id},
                                {b_vertex_id, b_texture_coord_id},
                                {c_vertex_id, c_texture_coord_id},
                            },
                            current_material->texture_offset,
                            current_material->bump_texture_offset,
                        };
                        state->faces[state->num_faces++] = face1;
                    }
                    else
                    {
                        hassert(!"Invalid number of face attributes");
                    }
                }
                /*
                char msg[100];
                sprintf(msg, "Found %d face positions\n", num_found);
                memory->platform_debug_message(ctx, msg);
                */
            }
            else if (line[0] == '#')
            {
            }
            else if (line[0] == 'g')
            {
            }
            else if (line[0] == 'o')
            {
            }
            else if (line[0] == 's')
            {
            }
            else if (starts_with(line, "mtllib"))
            {
                char *filename = line + 7;
                bool loaded = memory->platform_editor_load_nearby_file(ctx, &state->mtl_file, state->model_file, filename);
                hassert(loaded);
                load_mtl(ctx, memory);
            }
            else if (starts_with(line, "usemtl"))
            {
                material *tm;
                char material_name[100];
                auto material_name_length = eol - position - 7;
                strncpy(material_name, line + 7, (size_t)material_name_length + 1);

                current_material = 0;
                for (tm = state->materials;
                        tm < state->materials + state->num_materials;
                        tm++)
                {
                    if (strcmp(tm->name, material_name) == 0)
                    {
                        current_material = tm;
                        break;
                    }
                }
                hassert(current_material);
            }
            else
            {
                hassert(!"Invalid code path");
            }

            if (*eol == '\0')
            {
                break;
            }
            position = eol + 1;
            while((*position == '\r') && (*position == '\n'))
            {
                position++;
            }
            if (counter++ >= max_lines)
            {
                hassert(!"Counter too high!");
                break;
            }
        }

        glErrorAssert();

        for (uint32_t face_idx = 0;
                face_idx < state->num_faces * 3;
                ++face_idx)
        {
            state->num_faces_array++;
            state->faces_array[face_idx] = (GLushort)face_idx;
        }


        for (GLushort face_array_idx = 0;
                face_array_idx < state->num_faces_array;
                face_array_idx += 3)
        {
            state->line_elements[state->num_line_elements++] = face_array_idx;
            state->line_elements[state->num_line_elements++] = (GLushort)(face_array_idx + 1);
            state->line_elements[state->num_line_elements++] = (GLushort)(face_array_idx + 1);
            state->line_elements[state->num_line_elements++] = (GLushort)(face_array_idx + 2);
            state->line_elements[state->num_line_elements++] = (GLushort)(face_array_idx + 2);
            state->line_elements[state->num_line_elements++] = face_array_idx;
        }

        glErrorAssert();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
        glErrorAssert();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                (GLsizeiptr)(state->num_faces_array * sizeof(state->faces_array[0])),
                state->faces_array,
                GL_STATIC_DRAW);
        glErrorAssert();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->line_ibo);
        glErrorAssert();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                (GLsizeiptr)(state->num_line_elements * sizeof(state->line_elements[0])),
                state->line_elements,
                GL_STATIC_DRAW);
        glErrorAssert();

        for (uint32_t face_idx = 0;
                face_idx < state->num_faces;
                ++face_idx)
        {
            state->num_vbo_vertices += 3;
            editor_vertex_format *vbo_v1 = state->vbo_vertices + (3 * face_idx);
            editor_vertex_format *vbo_v2 = vbo_v1 + 1;
            editor_vertex_format *vbo_v3 = vbo_v2 + 1;
            face *f = state->faces + face_idx;
            v3 *face_v1 = state->vertices + (f->indices[0].vertex - 1);
            v3 *face_v2 = state->vertices + (f->indices[1].vertex - 1);
            v3 *face_v3 = state->vertices + (f->indices[2].vertex - 1);

            triangle3 t = {
                {face_v1->x, face_v1->y, face_v1->z},
                {face_v2->x, face_v2->y, face_v2->z},
                {face_v3->x, face_v3->y, face_v3->z},
            };
            v3 normal3 = winded_triangle_normal(t);

            v4 normal = {normal3.x, normal3.y, normal3.z, 1.0f};

            v3 *face_vt1 = state->texture_coords + (f->indices[0].texture_coord - 1);
            v3 *face_vt2 = state->texture_coords + (f->indices[1].texture_coord - 1);
            v3 *face_vt3 = state->texture_coords + (f->indices[2].texture_coord - 1);

            v4 tangent = {};
            {
                v3 q1 = v3sub(t.p1, t.p0);
                v3 q2 = v3sub(t.p2, t.p0);
                float x1 = q1.x;
                float x2 = q2.x;
                float y1 = q1.y;
                float y2 = q2.y;
                float z1 = q1.z;
                float z2 = q2.z;

                float s1 = face_vt2->x - face_vt1->x;
                float t1 = face_vt2->y - face_vt1->y;
                float s2 = face_vt3->x - face_vt1->x;
                float t2 = face_vt3->y - face_vt1->y;

                float r = 1 / (s1 * t2 - s2 * t1);

                v3 sdir = {
                    r * (t2 * x1 - t1 * x2),
                    r * (t2 * y1 - t1 * y2),
                    r * (t2 * z1 - t1 * z2),
                };
                v3 tdir = {
                    r * (s1 * x2 - s1 * x1),
                    r * (s1 * y2 - s1 * y1),
                    r * (s1 * z2 - s1 * z1),
                };

                v3 tangent3 = v3normalize(v3sub(sdir, v3mul(normal3, (v3dot(normal3, sdir)))));
                float w = v3dot(v3cross(normal3, sdir), tdir) < 0.0f ? -1.0f : 1.0f;
                tangent = {
                    tangent3.x,
                    tangent3.y,
                    tangent3.z,
                    w,
                };
            }

            vbo_v1->v.position[0] = face_v1->x;
            vbo_v1->v.position[1] = face_v1->y;
            vbo_v1->v.position[2] = face_v1->z;
            vbo_v1->v.position[3] = 1.0f;
            vbo_v1->normal = normal;
            vbo_v1->tangent = tangent;

            vbo_v2->v.position[0] = face_v2->x;
            vbo_v2->v.position[1] = face_v2->y;
            vbo_v2->v.position[2] = face_v2->z;
            vbo_v2->v.position[3] = 1.0f;
            vbo_v2->normal = normal;
            vbo_v2->tangent = tangent;

            vbo_v3->v.position[0] = face_v3->x;
            vbo_v3->v.position[1] = face_v3->y;
            vbo_v3->v.position[2] = face_v3->z;
            vbo_v3->v.position[3] = 1.0f;
            vbo_v3->normal = normal;
            vbo_v3->tangent = tangent;

            vbo_v1->v.color[0] = face_vt1->x;
            vbo_v1->v.color[1] = 1 - face_vt1->y;
            vbo_v1->v.color[2] = 0.0f;
            vbo_v1->v.color[3] = 1.0f;

            vbo_v2->v.color[0] = face_vt2->x;
            vbo_v2->v.color[1] = 1 - face_vt2->y;
            vbo_v2->v.color[2] = 0.0f;
            vbo_v2->v.color[3] = 1.0f;

            vbo_v3->v.color[0] = face_vt3->x;
            vbo_v3->v.color[1] = 1 - face_vt3->y;
            vbo_v3->v.color[2] = 0.0f;
            vbo_v3->v.color[3] = 1.0f;

            vbo_v1->style[0] = 3.0f;
            vbo_v1->style[1] = (float)f->texture_offset;
            vbo_v1->style[2] = (float)f->bump_texture_offset;
            vbo_v2->style[0] = 3.0f;
            vbo_v2->style[1] = (float)f->texture_offset;
            vbo_v2->style[2] = (float)f->bump_texture_offset;
            vbo_v3->style[0] = 3.0f;
            vbo_v3->style[1] = (float)f->texture_offset;
            vbo_v3->style[2] = (float)f->bump_texture_offset;
        }

        glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
        glErrorAssert();

        glBufferData(GL_ARRAY_BUFFER,
                (GLsizeiptr)(sizeof(state->vbo_vertices[0]) * state->num_faces * 3),
                state->vbo_vertices,
                GL_STATIC_DRAW);
        glErrorAssert();

        load_aabb_buffer_objects(state, state->model_min, state->model_max);
        load_bounding_sphere(state, state->model_min, state->model_max);
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
        if (controller->buttons.start.ended_down && !controller->buttons.start.repeat)
        {
            state->hide_lines ^= true;
        }
        if (controller->buttons.move_up.ended_down && !controller->buttons.move_up.repeat)
        {
            state->model_mode = (state->model_mode + 1) % 5;
        }
        if (controller->buttons.move_right.ended_down && !controller->buttons.move_right.repeat)
        {
            state->shading_mode = (state->shading_mode + 1) % 3;
        }
        if (controller->buttons.move_down.ended_down && !controller->buttons.move_down.repeat)
        {
            state->x_rotation = (state->x_rotation + 1) % 4;
        }
        if (controller->buttons.move_left.ended_down && !controller->buttons.move_left.repeat)
        {
            state->y_rotation = (state->y_rotation + 1) % 4;
        }
    }

    v2 mouse_loc = {
        (float)input->mouse.x / ((float)input->window.width / 2.0f) - 1.0f,
        ((float)input->mouse.y / ((float)input->window.height / 2.0f) - 1.0f) * -1.0f,
    };

    glBindVertexArray(state->vao);

    glErrorAssert();

    // Revert to something resembling defaults
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_PRIMITIVE_RESTART);

    glErrorAssert();

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glErrorAssert();

    state->delta_t += input->delta_t;

    glUseProgram(state->program_b.program);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);

    glErrorAssert();
    glEnableVertexAttribArray((GLuint)state->program_b.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_color_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_style_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_normal_id);
    glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), 0);
    glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(vertex, color));
    glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, style));
    glVertexAttribPointer((GLuint)state->program_b.a_normal_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, normal));
    glVertexAttribPointer((GLuint)state->program_b.a_tangent_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, tangent));

    glErrorAssert();
    for (uint32_t idx = 0;
            idx < state->num_texture_ids;
            ++idx)
    {
        glUniform1i(state->sampler_ids[idx], (GLint)idx);
        glActiveTexture(GL_TEXTURE0 + idx);
        glBindTexture(GL_TEXTURE_2D, state->texture_ids[idx]);
        glErrorAssert();
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
    glErrorAssert();

    v3 center = v3div(v3add(state->model_max, state->model_min), 2.0f);
    m4 center_translate = m4identity();
    center_translate.cols[3] = {-center.x, -center.y, -center.z, 1.0f};

    v3 dimension = v3sub(state->model_max, state->model_min);
    float max_dimension = dimension.x;
    if (dimension.y > max_dimension)
    {
        max_dimension = dimension.y;
    }
    if (dimension.z > max_dimension)
    {
        max_dimension = dimension.z;
    }

    v3 axis = {0.0f, 1.0f, 0.0f};
    m4 rotate = m4rotation(axis, state->delta_t);
    v3 x_axis = {1.0f, 0.0f, 0.0f};
    v3 y_axis = {0.0f, 0.0f, 1.0f};
    m4 x_rotate = m4rotation(x_axis, (pi / 2) * state->x_rotation);
    m4 y_rotate = m4rotation(y_axis, (pi / 2) * state->y_rotation);
    rotate = m4mul(rotate, x_rotate);
    rotate = m4mul(rotate, y_rotate);

    m4 scale = m4identity();
    scale.cols[0].E[0] = 2.0f / max_dimension;
    scale.cols[1].E[1] = 2.0f / max_dimension;
    scale.cols[2].E[2] = 2.0f / max_dimension;
    m4 translate = m4identity();
    translate.cols[3] = v4{0.0f, 0.0f, -10.0f, 1.0f};

    m4 a = center_translate;
    m4 b = scale;
    m4 c = rotate;
    m4 d = translate;

    m4 u_model = m4mul(d, m4mul(c, m4mul(b, a)));

    v4 light_position = {10.0f, 10.0f, -10.0f, 1.0f};
    glUniform4fv(state->program_b.u_w_lightPosition_id, 1, (float *)&light_position);
    v4 u_mvp_enabled = {1.0f, 0.0f, 0.0f, 0.0f};
    glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
    glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
    m4 u_view = m4identity();
    glUniformMatrix4fv(state->program_b.u_view_id, 1, false, (float *)&u_view);
    float ratio = (float)input->window.width / (float)input->window.height;
    m4 u_perspective = m4frustumprojection(state->near_, state->far_, {-ratio, -1.0f}, {ratio, 1.0f});
    //m4 u_perspective = m4identity();
    glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);
    glUniform1i(state->program_b.u_model_mode_id, state->model_mode);
    glUniform1i(state->program_b.u_shading_mode_id, state->shading_mode);

    glDrawElements(GL_TRIANGLES, (GLsizei)state->num_faces_array, GL_UNSIGNED_SHORT, 0);
    glErrorAssert();

    if (!state->hide_lines)
    {
        glUniform1i(state->program_b.u_model_mode_id, 0);
        glUniform1i(state->program_b.u_shading_mode_id, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->line_ibo);
        glDepthFunc(GL_LEQUAL);
        u_mvp_enabled = {1.0f, 0.0f, 0.0f, 1.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        glDrawElements(GL_LINES, (GLsizei)state->num_line_elements, GL_UNSIGNED_SHORT, 0);
    }

    {
        glUniform1i(state->program_b.u_model_mode_id, 0);
        glUniform1i(state->program_b.u_shading_mode_id, 0);
        glBindBuffer(GL_ARRAY_BUFFER, state->aabb_cube_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->aabb_cube_ibo);
        glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), 0);
        glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(vertex, color));
        glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, style));
        glVertexAttribPointer((GLuint)state->program_b.a_normal_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, normal));
        glVertexAttribPointer((GLuint)state->program_b.a_tangent_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, tangent));
        glDepthFunc(GL_LEQUAL);
        u_mvp_enabled = {1.0f, 0.0f, 0.0f, 1.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_SHORT, 0);
        glErrorAssert();
    }

    {
        glUniform1i(state->program_b.u_model_mode_id, 0);
        glUniform1i(state->program_b.u_shading_mode_id, 0);
        glBindBuffer(GL_ARRAY_BUFFER, state->bounding_sphere_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->bounding_sphere_ibo);
        glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), 0);
        glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(vertex, color));
        glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, style));
        glVertexAttribPointer((GLuint)state->program_b.a_normal_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, normal));
        glVertexAttribPointer((GLuint)state->program_b.a_tangent_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, tangent));
        glDepthFunc(GL_LEQUAL);
        u_mvp_enabled = {1.0f, 0.0f, 0.0f, 1.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        glPrimitiveRestartIndex(65535);
        glDrawElements(GL_LINE_LOOP, (GLsizei)state->num_bounding_sphere_elements, GL_UNSIGNED_SHORT, 0);
        glErrorAssert();
    }

    {

        memset(state->debug_buffer, 0, sizeof(state->debug_buffer));
        char msg[1024] = {};
        sprintf(msg + strlen(msg), "MODEL MODE:...");
        switch (state->model_mode)
        {
            case 0:
            {
                sprintf(msg + strlen(msg), "STD");
            } break;
            case 1:
            {
                sprintf(msg + strlen(msg), "DISCARD");
            } break;
            case 2:
            {
                sprintf(msg + strlen(msg), "NORMAL");
            } break;
            case 3:
            {
                sprintf(msg + strlen(msg), "LIGHT.REFLECT");
            } break;
            case 4:
            {
                sprintf(msg + strlen(msg), "BUMP.MAP");
            } break;
            default:
            {
                sprintf(msg + strlen(msg), "UNKNOWN");
            } break;
        }
        sprintf(msg + strlen(msg), "...");

        sprintf(msg + strlen(msg), "SHADING MODE...");
        switch (state->shading_mode)
        {
            case 0:
            {
                sprintf(msg + strlen(msg), "DIFFUSE");
            } break;
            case 1:
            {
                sprintf(msg + strlen(msg), "LIGHTING.NOBUMP");
            } break;
            case 2:
            {
                sprintf(msg + strlen(msg), "LIGHTING.WITHBUMP");
            } break;
            default:
            {
                sprintf(msg + strlen(msg), "UNKNOWN");
            } break;
        }
        sprintf(msg + strlen(msg), "...");
        sprintf(msg + strlen(msg), "%d x %d", input->window.width, input->window.height);
        write_to_buffer(&state->debug_draw_buffer, &state->debug_font.font, msg);

        glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glUniform1i(state->program_b.u_model_mode_id, 0);
        glUniform1i(state->program_b.u_shading_mode_id, 0);

        glBindBuffer(GL_ARRAY_BUFFER, state->debug_vbo);
        glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), 0);
        glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(vertex, color));
        glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, style));
        glVertexAttribPointer((GLuint)state->program_b.a_normal_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, normal));
        glVertexAttribPointer((GLuint)state->program_b.a_tangent_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, tangent));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, state->debug_texture_id);
        glUniform1i(
            glGetUniformLocation(state->program_b.program, "tex"),
            0);
        glTexSubImage2D(GL_TEXTURE_2D,
            0,
            0,
            0,
            (GLsizei)debug_buffer_width,
            (GLsizei)debug_buffer_height,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            state->debug_buffer);
        u_mvp_enabled = {0.0f, 0.0f, 0.0f, 0.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        v2 position = {0, 0};
        glUniform2fv(state->program_b.u_offset_id, 1, (float *)&position);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    {
        glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glUniform1i(state->program_b.u_model_mode_id, 0);
        glUniform1i(state->program_b.u_shading_mode_id, 0);
        float mouse_width = 16.0f / ((float)input->window.width / 2.0f);
        float mouse_height = 16.0f / ((float)input->window.height / 2.0f);
        editor_vertex_format vertices[] =
        {
            {
                {
                    {mouse_loc.x, mouse_loc.y, 0.0, 1.0},
                    {0.0, 0.0, 1.0, 1.0},
                },
                {2.0, 0.0, 0.0, 0.0},
            },
            {
                {
                    {mouse_loc.x + mouse_width, mouse_loc.y, 0.0, 1.0},
                    {1.0, 0.0, 1.0, 1.0},
                },
                {2.0, 0.0, 0.0, 0.0},
            },
            {
                {
                    {mouse_loc.x + mouse_width, mouse_loc.y - mouse_height, 0.0, 1.0},
                    {1.0, 1.0, 1.0, 1.0},
                },
                {2.0, 0.0, 0.0, 0.0},
            },
            {
                {
                    {mouse_loc.x, mouse_loc.y - mouse_height, 0.0, 1.0},
                    {0.0, 1.0, 1.0, 1.0},
                },
                {2.0, 0.0, 0.0, 0.0},
            },
        };
        glBindBuffer(GL_ARRAY_BUFFER, state->mouse_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), 0);
        glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(vertex, color));
        glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, style));
        glVertexAttribPointer((GLuint)state->program_b.a_normal_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, normal));
        glVertexAttribPointer((GLuint)state->program_b.a_tangent_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format), (void *)offsetof(editor_vertex_format, tangent));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, state->mouse_texture);
        glUniform1i(
            glGetUniformLocation(state->program_b.program, "tex"),
            0);
        u_mvp_enabled = {0.0f, 0.0f, 0.0f, 0.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        v2 position = {0, 0};
        glUniform2fv(state->program_b.u_offset_id, 1, (float *)&position);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glErrorAssert();
    }
}

