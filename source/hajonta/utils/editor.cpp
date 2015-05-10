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

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4505)
#endif
#define STB_RECT_PACK_IMPLEMENTATION
#include "hajonta/thirdparty/stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "hajonta/thirdparty/stb_truetype.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "hajonta/programs/b.h"
#include "hajonta/programs/ui2d.h"
#include "hajonta/programs/skybox.h"
#include "hajonta/programs/c.h"

struct directional_light
{
    v3 direction;
    v3 color;
    float ambient_intensity;
    float diffuse_intensity;
};

struct attenuation_config
{
    float constant;
    float linear;
    float exponential;
};

struct point_light
{
    v3 position;
    v3 color;
    float ambient_intensity;
    float diffuse_intensity;
    attenuation_config attenuation;
};

static float pi = 3.14159265358979f;

struct vertex
{
    float position[4];
    float color[4];
};

struct editor_vertex_format_b
{
    vertex v;
    float style[4];
    v4 normal;
    v4 tangent;
};

struct editor_vertex_format_c
{
    v4 position;
    v4 normal;
    v4 tangent;
    v2 tex_coord;
};

struct face_index
{
    uint32_t vertex;
    uint32_t texture_coord;
    uint32_t normal;
};

struct ui2d_vertex_format
{
    float position[2];
    float tex_coord[2];
    uint32_t texid;
    uint32_t options;
    v3 channel_color;
};

struct ui2d_push_context
{
    ui2d_vertex_format *vertices;
    uint32_t num_vertices;
    uint32_t max_vertices;
    GLuint *elements;
    uint32_t num_elements;
    uint32_t max_elements;
    uint32_t *textures;
    uint32_t num_textures;
    uint32_t max_textures;

    uint32_t seen_textures[1024];
};

struct material
{
    char name[100];
    int32_t texture_offset;
    int32_t bump_texture_offset;
    int32_t emit_texture_offset;
    int32_t ao_texture_offset;
    int32_t specular_exponent_texture_offset;
};

struct face
{
    face_index indices[3];
    material *current_material;
};

struct editor_vertex_indices
{
    material *current_material;
    uint32_t final_vertex_id;
};

struct stb_font_data
{
    stbtt_packedchar chardata[128];
    GLuint font_tex;
    uint32_t vbo;
    uint32_t ibo;
};

struct sprite
{
    uint32_t width;
    uint32_t height;
    float s0;
    float s1;
    float t0;
    float t1;
};

struct spritesheet
{
    uint32_t tex;
    uint32_t width;
    uint32_t height;
    sprite *sprites;
};


struct kenney_ui_data
{
    GLuint panel_tex;
    uint32_t vbo;
    uint32_t ibo;

    spritesheet ui_pack_sheet;
    sprite ui_pack_sprites[20];
};

struct skybox_vertex_format
{
    float position[3];
};

struct skybox_faces
{
    uint32_t vertex_ids[3];
};

struct skybox_data
{
    skybox_vertex_format vertices[12];
    skybox_faces faces[20];

    uint32_t tex;
    uint32_t vbo;
    uint32_t ibo;
};

enum struct shader_config_flags {
    ignore_normal_texture = (1<<0),
    ignore_ao_texture = (1<<1),
    ignore_emit_texture = (1<<2),
    ignore_specular_exponent_texture = (1<<3),
};

enum struct shader_mode {
    standard,
    diffuse_texture,
    normal_texture,
    ao_texture,
    emit_texture,
    blinn_phong,
    specular_exponent_texture,
};

struct game_state
{
    uint32_t vao;
    uint32_t vbo;
    uint32_t ibo;
    uint32_t line_ibo;
    uint32_t texture_ids[20];
    uint32_t num_texture_ids;
    uint32_t aabb_cube_vbo;
    uint32_t aabb_cube_ibo;
    uint32_t bounding_sphere_vbo;
    uint32_t bounding_sphere_ibo;
    uint32_t mouse_texture;

    uint32_t num_bounding_sphere_elements;

    float near_;
    float far_;
    float delta_t;

    loaded_file model_file;
    loaded_file mtl_file;

    uint32_t num_vertices;
    v3 vertices[100000];
    uint32_t num_texture_coords;
    v3 texture_coords[100000];
    uint32_t num_normals;
    v3 normals[100000];
    uint32_t num_faces;
    face faces[100000];

    uint32_t num_materials;
    material materials[15];

    uint32_t num_faces_array;
    uint32_t faces_array[300000];
    uint32_t num_line_elements;
    uint32_t line_elements[600000];
    uint32_t num_vbo_vertices;
    editor_vertex_format_c vbo_vertices[300000];

    editor_vertex_indices material_indices[100];
    uint32_t num_material_indices;

    b_program_struct program_b;
    ui2d_program_struct program_ui2d;
    skybox_program_struct program_skybox;
    c_program_struct program_c;

    v3 model_max;
    v3 model_min;

    char bitmap_scratch[4096 * 4096 * 4];

    bool hide_lines;
    int shader_mode;
    int shader_config_flags;

    int x_rotation_correction;
    int z_rotation_correction;

    float x_rotation;
    float y_rotation;
    float z_rotation;

    stb_font_data stb_font;
    kenney_ui_data kenney_ui;
    skybox_data skybox;
};


void load_fonts(hajonta_thread_context *ctx, platform_memory *memory)
{
    game_state *state = (game_state *)memory->memory;

    uint8_t ttf_buffer[158080];
    unsigned char temp_bitmap[512][512];
    char *filename = "fonts/AnonymousPro-1.002.001/Anonymous Pro.ttf";

    if (!memory->platform_load_asset(ctx, filename, sizeof(ttf_buffer), ttf_buffer)) {
        char msg[1024];
        sprintf(msg, "Could not load %s\n", filename);
        memory->platform_fail(ctx, msg);
        memory->quit = true;
        return;
    }

    stbtt_pack_context pc;

    stbtt_PackBegin(&pc, (unsigned char *)temp_bitmap[0], 512, 512, 0, 1, NULL);
    stbtt_PackSetOversampling(&pc, 1, 1);
    stbtt_PackFontRange(&pc, ttf_buffer, 0, 10.0f, 32, 95, state->stb_font.chardata+32);
    stbtt_PackEnd(&pc);

    glGenTextures(1, &state->stb_font.font_tex);
    glErrorAssert();
    glBindTexture(GL_TEXTURE_2D, state->stb_font.font_tex);
    glErrorAssert();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap);
    glErrorAssert();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glErrorAssert();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glErrorAssert();
    glGenBuffers(1, &state->stb_font.vbo);
    glGenBuffers(1, &state->stb_font.ibo);
}

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

    loaded = ui2d_program(&state->program_ui2d, ctx, memory);
    if (!loaded)
    {
        return loaded;
    }

    loaded = skybox_program(&state->program_skybox, ctx, memory);
    if (!loaded)
    {
        return loaded;
    }

    loaded = c_program(&state->program_c, ctx, memory);
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
            hassert(state->num_materials < harray_count(state->materials));
            current_material = state->materials + state->num_materials++;
            strncpy(current_material->name, line + 7, (size_t)(eol - position - 7));
            current_material->texture_offset = -1;
            current_material->bump_texture_offset = -1;
            current_material->emit_texture_offset = -1;
            current_material->ao_texture_offset = -1;
            current_material->specular_exponent_texture_offset = -1;
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
                loaded = load_image((uint8_t *)texture.contents, texture.size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch), &x, &y, &size, false);
                hassert(loaded);

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
                loaded = load_image((uint8_t *)texture.contents, texture.size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch), &x, &y, &size, false);
                hassert(loaded);

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
            char *filename = line + sizeof("map_Ke ") - 1;
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
                loaded = load_image((uint8_t *)texture.contents, texture.size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch), &x, &y, &size, false);
                hassert(loaded);

                current_material->emit_texture_offset = (int32_t)(state->num_texture_ids++);
                glErrorAssert();
                glBindTexture(GL_TEXTURE_2D, state->texture_ids[current_material->emit_texture_offset]);
                glErrorAssert();
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    x, y, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, state->bitmap_scratch);
                glErrorAssert();
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glErrorAssert();
            }
        }
        else if (starts_with(line, "map_ao "))
        {
            char *filename = line + sizeof("map_ao ") - 1;
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
                loaded = load_image((uint8_t *)texture.contents, texture.size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch), &x, &y, &size, false);
                hassert(loaded);

                current_material->ao_texture_offset = (int32_t)(state->num_texture_ids++);
                glErrorAssert();
                glBindTexture(GL_TEXTURE_2D, state->texture_ids[current_material->ao_texture_offset]);
                glErrorAssert();
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    x, y, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, state->bitmap_scratch);
                glErrorAssert();
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glErrorAssert();
            }
        }
        else if (starts_with(line, "map_Ks "))
        {
        }
        else if (starts_with(line, "map_Ka "))
        {
        }
        else if (starts_with(line, "refl "))
        {
        }
        else if (starts_with(line, "bump "))
        {
        }
        else if (starts_with(line, "map_Ns "))
        {
            char *filename = line + sizeof("map_Ns ") - 1;
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
                loaded = load_image((uint8_t *)texture.contents, texture.size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch), &x, &y, &size, false);
                hassert(loaded);

                current_material->specular_exponent_texture_offset = (int32_t)(state->num_texture_ids++);
                glErrorAssert();
                glBindTexture(GL_TEXTURE_2D, state->texture_ids[current_material->specular_exponent_texture_offset]);
                glErrorAssert();
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    x, y, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, state->bitmap_scratch);
                glErrorAssert();
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glErrorAssert();
            }
        }
        else if (starts_with(line, "Km "))
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
    editor_vertex_format_b vertices[] = {
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

    editor_vertex_format_b vertices[64 * 3] = {};
    for (uint32_t idx = 0;
            idx < harray_count(vertices);
            ++idx)
    {
        editor_vertex_format_b *v = vertices + idx;
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

void
setup_vertex_attrib_array_b(game_state *state)
{
    glEnableVertexAttribArray((GLuint)state->program_b.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_color_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_style_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_normal_id);
    glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format_b), 0);
    glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format_b), (void *)offsetof(vertex, color));
    glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format_b), (void *)offsetof(editor_vertex_format_b, style));
    glVertexAttribPointer((GLuint)state->program_b.a_normal_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format_b), (void *)offsetof(editor_vertex_format_b, normal));
    glVertexAttribPointer((GLuint)state->program_b.a_tangent_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format_b), (void *)offsetof(editor_vertex_format_b, tangent));

    glErrorAssert();
}

void
setup_vertex_attrib_array_c(game_state *state)
{
    glEnableVertexAttribArray((GLuint)state->program_c.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_c.a_normal_id);
    glEnableVertexAttribArray((GLuint)state->program_c.a_tangent_id);
    glEnableVertexAttribArray((GLuint)state->program_c.a_tex_coord_id);

    glVertexAttribPointer((GLuint)state->program_c.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format_c), 0);
    glVertexAttribPointer((GLuint)state->program_c.a_normal_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format_c), (void *)offsetof(editor_vertex_format_c, normal));
    glVertexAttribPointer((GLuint)state->program_c.a_tangent_id, 4, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format_c), (void *)offsetof(editor_vertex_format_c, tangent));
    glVertexAttribPointer((GLuint)state->program_c.a_tex_coord_id, 2, GL_FLOAT, GL_FALSE, sizeof(editor_vertex_format_c), (void *)offsetof(editor_vertex_format_c, tex_coord));

    glErrorAssert();
}

uint32_t
push_texture(ui2d_push_context *pushctx, uint32_t tex)
{
    hassert(tex < sizeof(pushctx->seen_textures));

    if (pushctx->seen_textures[tex] != 0)
    {
        return pushctx->seen_textures[tex];
    }

    hassert(pushctx->num_textures + 4 <= pushctx->max_textures);
    pushctx->textures[pushctx->num_textures++] = tex;

    // Yes, this is post-increment.  It is decremented in the shader.
    pushctx->seen_textures[tex] = pushctx->num_textures;
    return pushctx->num_textures;
}

void
push_quad(ui2d_push_context *pushctx, stbtt_aligned_quad q, uint32_t tex, uint32_t options)
{
        ui2d_vertex_format *vertices = pushctx->vertices;
        uint32_t *num_vertices = &pushctx->num_vertices;
        uint32_t *elements = pushctx->elements;
        uint32_t *num_elements = &pushctx->num_elements;

        uint32_t tex_handle = push_texture(pushctx, tex);

        hassert(pushctx->num_vertices + 4 <= pushctx->max_vertices);

        uint32_t bl_vertex = (*num_vertices)++;
        vertices[bl_vertex] =
        {
            { q.x0, q.y0 },
            { q.s0, q.t0 },
            tex_handle,
            options,
            { 1, 1, 1 },
        };
        uint32_t br_vertex = (*num_vertices)++;
        vertices[br_vertex] =
        {
            { q.x1, q.y0 },
            { q.s1, q.t0 },
            tex_handle,
            options,
            { 1, 1, 0 },
        };
        uint32_t tr_vertex = (*num_vertices)++;
        vertices[tr_vertex] =
        {
            { q.x1, q.y1 },
            { q.s1, q.t1 },
            tex_handle,
            options,
            { 1, 1, 0 },
        };
        uint32_t tl_vertex = (*num_vertices)++;
        vertices[tl_vertex] =
        {
            { q.x0, q.y1 },
            { q.s0, q.t1 },
            tex_handle,
            options,
            { 1, 1, 0 },
        };
        hassert(pushctx->num_elements + 6 <= pushctx->max_elements);
        elements[(*num_elements)++] = bl_vertex;
        elements[(*num_elements)++] = br_vertex;
        elements[(*num_elements)++] = tr_vertex;

        elements[(*num_elements)++] = tr_vertex;
        elements[(*num_elements)++] = tl_vertex;
        elements[(*num_elements)++] = bl_vertex;
}

void
push_sprite(ui2d_push_context *pushctx, sprite *s, uint32_t tex, v2 pos_bl)
{
    stbtt_aligned_quad q;

    // BL
    q.x0 = pos_bl.x;
    q.x1 = q.x0 + s->width;
    q.y0 = pos_bl.y;
    q.y1 = q.y0 + s->height;
    q.s0 = s->s0;
    q.s1 = s->s1;
    q.t0 = s->t0;
    q.t1 = s->t1;
    push_quad(pushctx, q, tex, 0);
}

void
push_panel(game_state *state, ui2d_push_context *pushctx, rectangle2 rect)
{
    uint32_t tex = state->kenney_ui.panel_tex;
    stbtt_aligned_quad q;

    // BL
    q.x0 = rect.position.x;
    q.x1 = rect.position.x + 8.0f;
    q.y0 = rect.position.y;
    q.y1 = rect.position.y + 8.0f;
    q.s0 = 0.00f;
    q.s1 = 0.08f;
    q.t0 = 0.00f;
    q.t1 = 0.08f;
    push_quad(pushctx, q, tex, 0);

    // TL
    q.y0 = rect.position.y + rect.dimension.y;
    q.y1 = q.y0 - 8.0f;
    q.t0 = 1.00f;
    q.t1 = 0.92f;
    push_quad(pushctx, q, tex, 0);

    // TR
    q.x0 = rect.position.x + rect.dimension.x;
    q.x1 = q.x0 - 8.0f;
    q.s0 = 1.00f;
    q.s1 = 0.92f;
    push_quad(pushctx, q, tex, 0);

    // BR
    q.y0 = rect.position.y;
    q.y1 = rect.position.y + 8.0f;
    q.t0 = 0.00f;
    q.t1 = 0.08f;
    push_quad(pushctx, q, tex, 0);

    // TR-BR
    q.y0 = rect.position.y + 8.0f;
    q.y1 = rect.position.y + rect.dimension.y - 8.0f;
    q.t0 = 0.08f;
    q.t1 = 0.92f;
    push_quad(pushctx, q, tex, 0);

    // TL-BL
    q.x0 = rect.position.x;
    q.x1 = rect.position.x + 8.0f;
    q.s0 = 0.00f;
    q.s1 = 0.08f;
    push_quad(pushctx, q, tex, 0);

    // TL-TR
    q.x0 = rect.position.x + 8.0f;
    q.x1 = rect.position.x + rect.dimension.x - 8.0f;
    q.y0 = rect.position.y + rect.dimension.y;
    q.y1 = rect.position.y + rect.dimension.y - 8.0f;
    q.s0 = 0.08f;
    q.s1 = 0.92f;
    q.t0 = 1.00f;
    q.t1 = 0.92f;
    push_quad(pushctx, q, tex, 0);

    // BL-BR
    q.y0 = rect.position.y;
    q.y1 = rect.position.y + 8.0f;
    q.t0 = 0.00f;
    q.t1 = 0.08f;
    push_quad(pushctx, q, tex, 0);

    // CENTER
    q.x0 = rect.position.x + 8.0f;
    q.x1 = rect.position.x + rect.dimension.x - 8.0f;
    q.y0 = rect.position.y + 8.0f;
    q.y1 = rect.position.y + rect.dimension.y - 8.0f;
    q.s0 = 0.08f;
    q.s1 = 0.92f;
    q.t0 = 0.08f;
    q.t1 = 0.92f;
    push_quad(pushctx, q, tex, 0);
}

void
push_window(game_state *state, ui2d_push_context *pushctx, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    for (uint32_t hidx = 0; hidx < height; ++hidx)
    {
        for (uint32_t idx = 0; idx < width; ++idx)
        {
            uint32_t sprite = 6;
            if (hidx == 0)
            {
                sprite += 3;
            }
            else if (hidx == height - 1)
            {
                sprite -= 3;
            }
            if (idx == 0)
            {
                --sprite;
            }
            else if (idx == width - 1)
            {
                ++sprite;
            }
            push_sprite(pushctx, state->kenney_ui.ui_pack_sprites + sprite, state->kenney_ui.ui_pack_sheet.tex, v2{x + (float)(idx * 16), y + (float)(hidx * 16)});
        }
    }
}

void
ui2d_render_elements(game_state *state, ui2d_push_context *pushctx)
{
    glBindBuffer(GL_ARRAY_BUFFER, state->stb_font.vbo);
    glBufferData(GL_ARRAY_BUFFER,
            (GLsizei)(pushctx->num_vertices * sizeof(pushctx->vertices[0])),
            pushctx->vertices,
            GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->stb_font.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            (GLsizei)(pushctx->num_elements * sizeof(pushctx->elements[0])),
            pushctx->elements,
            GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray((GLuint)state->program_ui2d.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_ui2d.a_tex_coord_id);
    glEnableVertexAttribArray((GLuint)state->program_ui2d.a_texid_id);
    glEnableVertexAttribArray((GLuint)state->program_ui2d.a_options_id);
    glEnableVertexAttribArray((GLuint)state->program_ui2d.a_channel_color_id);
    glVertexAttribPointer((GLuint)state->program_ui2d.a_pos_id, 2, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), 0);
    glVertexAttribPointer((GLuint)state->program_ui2d.a_tex_coord_id, 2, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, tex_coord));
    glVertexAttribPointer((GLuint)state->program_ui2d.a_texid_id, 1, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, texid));
    glVertexAttribPointer((GLuint)state->program_ui2d.a_options_id, 1, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, options));
    glVertexAttribPointer((GLuint)state->program_ui2d.a_channel_color_id, 3, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, channel_color));

    GLint uniform_locations[10] = {};
    char msg[] = "tex[xx]";
    for (int idx = 0; idx < harray_count(uniform_locations); ++idx)
    {
        sprintf(msg, "tex[%d]", idx);
        uniform_locations[idx] = glGetUniformLocation(state->program_ui2d.program, msg);

    }
    for (uint32_t i = 0; i < pushctx->num_textures; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, pushctx->textures[i]);
        glUniform1i(
            uniform_locations[i],
            (GLint)i);
    }

    glDrawElements(GL_TRIANGLES, (GLsizei)pushctx->num_elements, GL_UNSIGNED_INT, 0);
    glErrorAssert();
}
bool
load_texture_asset(
    hajonta_thread_context *ctx,
    platform_memory *memory,
    char *filename,
    uint8_t *image_buffer,
    uint32_t image_size,
    int32_t *x,
    int32_t *y,
    GLuint *tex,
    GLenum target
)
{
    game_state *state = (game_state *)memory->memory;
    if (!memory->platform_load_asset(ctx, filename, image_size, image_buffer)) {
        return false;
    }

    int32_t actual_size;
    load_image(image_buffer, image_size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch),
            x, y, &actual_size);

    if (tex)
    {
        if (!*tex)
        {
            glGenTextures(1, tex);
        }
        glBindTexture(GL_TEXTURE_2D, *tex);
    }

    glTexImage2D(target, 0, GL_RGBA,
        *x, *y, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, state->bitmap_scratch);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return true;
}

bool
load_texture_asset(
    hajonta_thread_context *ctx,
    platform_memory *memory,
    char *filename,
    uint8_t *image_buffer,
    uint32_t image_size,
    int32_t *x,
    int32_t *y,
    GLuint *tex
)
{
    return load_texture_asset(ctx, memory, filename, image_buffer, image_size, x, y, tex, GL_TEXTURE_2D);
}

void
load_texture_asset_failed(
    hajonta_thread_context *ctx,
    platform_memory *memory,
    char *filename
)
{
    char msg[1024];
    sprintf(msg, "Could not load %s\n", filename);
    memory->platform_fail(ctx, msg);
    memory->quit = true;
}

struct sprite_config
{
    // top left = 0,0
    uint32_t x;
    uint32_t y;

    uint32_t width;
    uint32_t height;
};

void
populate_spritesheet(spritesheet *sp, sprite *s0, uint32_t num_sprites, sprite_config *sc0, uint32_t num_sprite_configs)
{
    sp->sprites = s0;
    hassert(num_sprites >= num_sprite_configs);
    for (uint32_t idx = 0; idx < num_sprite_configs; ++idx)
    {
        sprite *s = s0 + idx;
        sprite_config *sc = sc0 + idx;

        *s = {
            sc->width,
            sc->height,
            (float)sc->x / sp->width,
            (float)(sc->x + sc->width) / sp->width,
            (float)(sc->y + sc->height) / sp->height,
            (float)sc->y / sp->height,
        };
    }
}

bool
populate_skybox(hajonta_thread_context *ctx, platform_memory *memory, skybox_data *skybox)
{
    glErrorAssert();
    glGenBuffers(1, &skybox->vbo);
    glGenBuffers(1, &skybox->ibo);

    glErrorAssert();

    skybox_vertex_format vertices[] = {
        { 0.000f,  0.000f,  1.000f },
        { 0.894f,  0.000f,  0.447f },
        { 0.276f,  0.851f,  0.447f },
        {-0.724f,  0.526f,  0.447f },
        {-0.724f, -0.526f,  0.447f },
        { 0.276f, -0.851f,  0.447f },
        { 0.724f,  0.526f, -0.447f },
        {-0.276f,  0.851f, -0.447f },
        {-0.894f,  0.000f, -0.447f },
        {-0.276f, -0.851f, -0.447f },
        { 0.724f, -0.526f, -0.447f },
        { 0.000f,  0.000f, -1.000f },
    };
    memcpy(skybox->vertices, vertices, sizeof(vertices));
    glBindBuffer(GL_ARRAY_BUFFER, skybox->vbo);
    glBufferData(GL_ARRAY_BUFFER,
            sizeof(skybox->vertices),
            skybox->vertices,
            GL_STATIC_DRAW);

    glErrorAssert();

    skybox_faces faces[] = {
        { 0, 1, 2},
        { 0, 2, 3},
        { 0, 3, 4},
        { 0, 4, 5},
        { 0, 5, 1},
        {11, 6, 7},
        {11, 7, 8},
        {11, 8, 9},
        {11, 9,10},
        {11,10, 6},
        { 1, 2, 6},
        { 2, 3, 7},
        { 3, 4, 8},
        { 4, 5, 9},
        { 5, 1,10},
        { 6, 7, 2},
        { 7, 8, 3},
        { 8, 9, 4},
        { 9,10, 5},
        {10, 6, 1},
    };
    memcpy(skybox->faces, faces, sizeof(faces));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox->ibo);
    glErrorAssert();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            sizeof(skybox->faces),
            skybox->faces,
            GL_STATIC_DRAW);

    glErrorAssert();

    glGenTextures(1, &skybox->tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->tex);

    uint8_t image[162019]; // maximum size
    struct _assets {
        char *filename;
        GLenum target;
        uint32_t filesize;
    } assets[] = {
        { "skybox/miramar/miramar_rt.jpg", GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 141442 },
        { "skybox/miramar/miramar_lf.jpg", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 130511 },
        { "skybox/miramar/miramar_up.jpg", GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 81931 },
        { "skybox/miramar/miramar_dn.jpg", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 23437 },
        { "skybox/miramar/miramar_bk.jpg", GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 162019 },
        { "skybox/miramar/miramar_ft.jpg", GL_TEXTURE_CUBE_MAP_POSITIVE_X, 151010 },
    };

    for (uint32_t i = 0; i < harray_count(assets); ++i)
    {
        _assets *asset = assets + i;
        int32_t x;
        int32_t y;
        bool loaded = load_texture_asset(ctx, memory, asset->filename, image, asset->filesize, &x, &y, 0, asset->target);
        hassert(loaded);
        if (!loaded)
        {
            return false;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return true;
}

void
draw_shader_mode(hajonta_thread_context *ctx, platform_memory *memory, game_input *input, ui2d_push_context *pushctx)
{
    game_state *state = (game_state *)memory->memory;

    float x = 38;
    float y_base = 12;
    push_window(state, pushctx, 30, (uint32_t)(y_base) - 8, 51, 2);
    char *shader_mode_names[] =
    {
        "standard",
        "diffuse tex",
        "normal tex",
        "ao tex",
        "emit tex",
        "blinn phong",
        "spec. exp.",
    };
    bool mouse_pressed = input->mouse.buttons.left.ended_down == false && input->mouse.buttons.left.repeat == false;
    for (int32_t idx = 0; idx < harray_count(shader_mode_names); ++idx)
    {
        char *text = shader_mode_names[idx];
        float y = input->window.height - (y_base + 3.0f);

        if (state->shader_mode == idx)
        {
            push_sprite(pushctx, state->kenney_ui.ui_pack_sprites + 12, state->kenney_ui.ui_pack_sheet.tex, v2{x, y_base});
        }
        else
        {
            push_sprite(pushctx, state->kenney_ui.ui_pack_sprites + 11, state->kenney_ui.ui_pack_sheet.tex, v2{x, y_base});
        }

        if (mouse_pressed)
        {
            v2 m = {(float)input->mouse.x, (float)input->window.height - (float)input->mouse.y};
            rectangle2 r = {{x,y_base+3},{16,16}};
            if (point_in_rectangle(m, r))
            {
                state->shader_mode = idx;
            }
        }

        x += 22;

        while (*text) {
            stbtt_aligned_quad q;
            stbtt_GetPackedQuad(state->stb_font.chardata, 512, 512, *text++, &x, &y, &q, 0);
            q.y0 = input->window.height - q.y0;
            q.y1 = input->window.height - q.y1;
            push_quad(pushctx, q, state->stb_font.font_tex, 1);
        }

        x += 10;
    }
}

void
draw_shader_config(hajonta_thread_context *ctx, platform_memory *memory, game_input *input, ui2d_push_context *pushctx)
{
    game_state *state = (game_state *)memory->memory;

    float x = 38;
    float y_base = 50;
    push_window(state, pushctx, 30, (uint32_t)(y_base) - 8, 51, 2);
    char *shader_config_names[] =
    {
        "diffuse tex",
        "normal tex",
        "ao tex",
        "emit tex",
        "spec. exp.",
    };
    bool mouse_pressed = input->mouse.buttons.left.ended_down == false && input->mouse.buttons.left.repeat == false;

    for (int32_t idx = 0; idx < harray_count(shader_config_names); ++idx)
    {
        char *text = shader_config_names[idx];
        float y = input->window.height - (y_base + 3.0f);

        if (idx > 0)
        {
            if (state->shader_config_flags & (1<<(idx-1)))
            {
                push_sprite(pushctx, state->kenney_ui.ui_pack_sprites, state->kenney_ui.ui_pack_sheet.tex, v2{x, y_base});
            }
            else
            {
                push_sprite(pushctx, state->kenney_ui.ui_pack_sprites + 1, state->kenney_ui.ui_pack_sheet.tex, v2{x, y_base});
            }
        }

        if (mouse_pressed)
        {
            v2 m = {(float)input->mouse.x, (float)input->window.height - (float)input->mouse.y};
            rectangle2 r = {{x,y_base+3},{16,16}};
            if (point_in_rectangle(m, r))
            {
                state->shader_config_flags ^= (1<<(idx-1));
            }
        }

        x += 22;

        while (*text) {
            stbtt_aligned_quad q;
            stbtt_GetPackedQuad(state->stb_font.chardata, 512, 512, *text++, &x, &y, &q, 0);
            q.y0 = input->window.height - q.y0;
            q.y1 = input->window.height - q.y1;
            push_quad(pushctx, q, state->stb_font.font_tex, 1);
        }

        x += 10;
    }
}

void
draw_skybox(hajonta_thread_context *ctx, platform_memory *memory, skybox_data *skybox, m4 u_model, m4 u_view, m4 u_projection)
{
    glErrorAssert();

    game_state *state = (game_state *)memory->memory;

    glUseProgram(state->program_skybox.program);
    glActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, skybox->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox->ibo);

    glUniformMatrix4fv(state->program_skybox.u_model_id, 1, false, (float *)&u_model);
    glUniformMatrix4fv(state->program_skybox.u_view_id, 1, false, (float *)&u_view);
    glUniformMatrix4fv(state->program_skybox.u_projection_id, 1, false, (float *)&u_projection);

    glEnableVertexAttribArray((GLuint)state->program_skybox.a_pos_id);
    glVertexAttribPointer((GLuint)state->program_skybox.a_pos_id, 3, GL_FLOAT, GL_FALSE, sizeof(skybox_vertex_format), 0);

    glErrorAssert();
    int32_t tex = glGetUniformLocation(state->program_skybox.program, "u_cube_tex");
    glUniform1i(tex, 0);
    glErrorAssert();
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->tex);

    glErrorAssert();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDrawElements(GL_TRIANGLES, harray_count(skybox->faces) * 3, GL_UNSIGNED_INT, 0);

    glErrorAssert();
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

    glErrorAssert();

    if (!memory->initialized)
    {
        if(!gl_setup(ctx, memory))
        {
            return;
        }
        load_fonts(ctx, memory);
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
        glGenTextures(1, &state->mouse_texture);

        {
            uint8_t image[256];
            int32_t x, y;
            char filename[] = "ui/slick_arrows/slick_arrow-delta.png";
            bool loaded = load_texture_asset(ctx, memory, filename, image, sizeof(image), &x, &y, &state->mouse_texture);
            if (!loaded)
            {
                return load_texture_asset_failed(ctx, memory, filename);
            }
        }

        {
            uint8_t image[636];
            int32_t x, y;
            char filename[] = "ui/kenney/glassPanel.png";
            bool loaded = load_texture_asset(ctx, memory, filename, image, sizeof(image), &x, &y, &state->kenney_ui.panel_tex);
            if (!loaded)
            {
                return load_texture_asset_failed(ctx, memory, filename);
            }
        }

        {
            uint8_t image[25459];
            int32_t x, y;
            char filename[] = "ui/kenney/UIpackSheet_transparent.png";
            bool loaded = load_texture_asset(ctx, memory, filename, image, sizeof(image), &x, &y, &state->kenney_ui.ui_pack_sheet.tex);
            if (!loaded)
            {
                return load_texture_asset_failed(ctx, memory, filename);
            }
            state->kenney_ui.ui_pack_sheet.width = (uint32_t)x;
            state->kenney_ui.ui_pack_sheet.height = (uint32_t)abs(y);

            sprite_config configs[] =
            {
                {486, 90, 16, 16}, // checkbox empty
                {486, 90 + 18, 16, 16}, // checkbox checked
                {234, 360, 16, 16}, // top left blue dungeon tile
                {234 + 18, 360, 16, 16},
                {234 + 36, 360, 16, 16},
                {234, 360 + 18, 16, 16},
                {234 + 18, 360 + 18, 16, 16},
                {234 + 36, 360 + 18, 16, 16},
                {234, 360 + 36, 16, 16},
                {234 + 18, 360 + 36, 16, 16},
                {234 + 36, 360 + 36, 16, 16},
                {486 + 18, 90, 16, 16}, // radio empty
                {486 + 36, 90, 16, 16}, // radio selected
                {396, 414, 16, 16}, // top left green long arrow ("up")
                {396 + 18, 414, 16, 16}, // top right green long arrow ("down")
                {396, 414 + 18, 16, 16}, // bottom left green long arrow ("left")
                {396 + 18, 414 + 18, 16, 16}, // bottom right green long arrow ("right")
            };
            populate_spritesheet(&state->kenney_ui.ui_pack_sheet,
                state->kenney_ui.ui_pack_sprites,
                harray_count(state->kenney_ui.ui_pack_sprites),
                configs,
                harray_count(configs));
        }

        bool loaded = populate_skybox(ctx, memory, &state->skybox);
        if (!loaded)
        {
            memory->platform_fail(ctx, "Could not populate skybox");
            memory->quit = true;
            return;
        }

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
        null_material.emit_texture_offset = -1;
        null_material.ao_texture_offset = -1;
        null_material.specular_exponent_texture_offset = -1;
        material *current_material = &null_material;
        while (position < eof)
        {
            uint32_t remainder = state->model_file.size - (uint32_t)(position - start_position);
            char *eol = next_newline(position, remainder);
            char line[1024];
            strncpy(line, position, (size_t)(eol - position));
            line[eol - position] = '\0';

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
                    float a, b, c;
                    int num_found = sscanf(position + 2, "%f %f %f", &a, &b, &c);
                    if (num_found == 3)
                    {
                        v3 normal = {a,b,c};
                        state->normals[state->num_normals++] = normal;
                    }
                    else
                    {
                        hassert(!"Invalid code path");
                    }
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
                            current_material,
                        };
                        face face2 = {
                            {
                                {c_vertex_id, c_texture_coord_id},
                                {d_vertex_id, d_texture_coord_id},
                                {a_vertex_id, a_texture_coord_id},
                            },
                            current_material,
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
                    uint32_t t1, t2, t3;
                    int num_found2 = sscanf(a, "%d/%d/%d", &t1, &t2, &t3);
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
                            current_material,
                        };
                        state->faces[state->num_faces++] = face1;
                    }
                    else if (num_found2 == 3)
                    {
                        uint32_t a_vertex_id, a_texture_coord_id, a_normal_id;
                        uint32_t b_vertex_id, b_texture_coord_id, b_normal_id;
                        uint32_t c_vertex_id, c_texture_coord_id, c_normal_id;
                        int num_found2;
                        num_found2 = sscanf(a, "%d/%d/%d", &a_vertex_id, &a_texture_coord_id, &a_normal_id);
                        hassert(num_found2 == 3);
                        num_found2 = sscanf(b, "%d/%d/%d", &b_vertex_id, &b_texture_coord_id, &b_normal_id);
                        hassert(num_found2 == 3);
                        num_found2 = sscanf(c, "%d/%d/%d", &c_vertex_id, &c_texture_coord_id, &c_normal_id);
                        hassert(num_found2 == 3);
                        face face1 = {
                            {
                                {a_vertex_id, a_texture_coord_id, a_normal_id},
                                {b_vertex_id, b_texture_coord_id, b_normal_id},
                                {c_vertex_id, c_texture_coord_id, c_normal_id},
                            },
                            current_material,
                        };
                        state->faces[state->num_faces++] = face1;
                    }
                    else
                    {
                        hassert(!"Invalid number of face attributes");
                    }
                }
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
            state->faces_array[face_idx] = face_idx;
        }


        for (uint32_t face_array_idx = 0;
                face_array_idx < state->num_faces_array;
                face_array_idx += 3)
        {
            state->line_elements[state->num_line_elements++] = face_array_idx;
            state->line_elements[state->num_line_elements++] = face_array_idx + 1;
            state->line_elements[state->num_line_elements++] = face_array_idx + 1;
            state->line_elements[state->num_line_elements++] = face_array_idx + 2;
            state->line_elements[state->num_line_elements++] = face_array_idx + 2;
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

        material *last_material = 0;
        for (uint32_t face_idx = 0;
                face_idx < state->num_faces;
                ++face_idx)
        {
            face *f = state->faces + face_idx;
            if (f->current_material != last_material) {
                if (last_material)
                {
                    state->material_indices[state->num_material_indices++] =
                    {
                        last_material,
                        3 * face_idx,
                    };
                }
                last_material = f->current_material;
            }
            v3 *face_v1 = state->vertices + (f->indices[0].vertex - 1);
            v3 *face_v2 = state->vertices + (f->indices[1].vertex - 1);
            v3 *face_v3 = state->vertices + (f->indices[2].vertex - 1);

            state->num_vbo_vertices += 3;
            editor_vertex_format_c *vbo_v1 = state->vbo_vertices + (3 * face_idx);
            editor_vertex_format_c *vbo_v2 = vbo_v1 + 1;
            editor_vertex_format_c *vbo_v3 = vbo_v2 + 1;

            triangle3 t = {
                {face_v1->x, face_v1->y, face_v1->z},
                {face_v2->x, face_v2->y, face_v2->z},
                {face_v3->x, face_v3->y, face_v3->z},
            };

            v3 face_n1_v3;
            v4 face_n1_v4;
            v3 face_n2_v3;
            v4 face_n2_v4;
            v3 face_n3_v3;
            v4 face_n3_v4;

            if (f->indices[0].normal == 0)
            {
                v3 normal3 = winded_triangle_normal(t);
                v4 normal = {normal3.x, normal3.y, normal3.z, 1.0f};
                face_n1_v3 = normal3;
                face_n2_v3 = normal3;
                face_n3_v3 = normal3;
                face_n1_v4 = normal;
                face_n2_v4 = normal;
                face_n3_v4 = normal;
            }
            else
            {
                face_n1_v3 = state->normals[f->indices[0].normal - 1];
                face_n2_v3 = state->normals[f->indices[1].normal - 1];
                face_n3_v3 = state->normals[f->indices[2].normal - 1];
                face_n1_v4 = { face_n1_v3.x, face_n1_v3.y, face_n1_v3.z, 1.0f };
                face_n2_v4 = { face_n2_v3.x, face_n2_v3.y, face_n2_v3.z, 1.0f };
                face_n3_v4 = { face_n3_v3.x, face_n3_v3.y, face_n3_v3.z, 1.0f };
            }


            v3 *face_vt1 = state->texture_coords + (f->indices[0].texture_coord - 1);
            v3 *face_vt2 = state->texture_coords + (f->indices[1].texture_coord - 1);
            v3 *face_vt3 = state->texture_coords + (f->indices[2].texture_coord - 1);

            v4 face_t1;
            v4 face_t2;
            v4 face_t3;

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

                {
                    v3 normal3 = face_n1_v3;
                    v3 tangent3 = v3normalize(v3sub(sdir, v3mul(normal3, (v3dot(normal3, sdir)))));
                    float w = v3dot(v3cross(normal3, sdir), tdir) < 0.0f ? -1.0f : 1.0f;
                    face_t1 = {
                        tangent3.x,
                        tangent3.y,
                        tangent3.z,
                        w,
                    };
                }
                {
                    v3 normal3 = face_n2_v3;
                    v3 tangent3 = v3normalize(v3sub(sdir, v3mul(normal3, (v3dot(normal3, sdir)))));
                    float w = v3dot(v3cross(normal3, sdir), tdir) < 0.0f ? -1.0f : 1.0f;
                    face_t2 = {
                        tangent3.x,
                        tangent3.y,
                        tangent3.z,
                        w,
                    };
                }
                {
                    v3 normal3 = face_n3_v3;
                    v3 tangent3 = v3normalize(v3sub(sdir, v3mul(normal3, (v3dot(normal3, sdir)))));
                    float w = v3dot(v3cross(normal3, sdir), tdir) < 0.0f ? -1.0f : 1.0f;
                    face_t3 = {
                        tangent3.x,
                        tangent3.y,
                        tangent3.z,
                        w,
                    };
                }
            }

            vbo_v1->position.x = face_v1->x;
            vbo_v1->position.y = face_v1->y;
            vbo_v1->position.z = face_v1->z;
            vbo_v1->position.w = 1.0f;
            vbo_v1->normal = face_n1_v4;
            vbo_v1->tangent = face_t1;

            vbo_v2->position.x = face_v2->x;
            vbo_v2->position.y = face_v2->y;
            vbo_v2->position.z = face_v2->z;
            vbo_v2->position.w = 1.0f;
            vbo_v2->normal = face_n2_v4;
            vbo_v2->tangent = face_t2;

            vbo_v3->position.x = face_v3->x;
            vbo_v3->position.y = face_v3->y;
            vbo_v3->position.z = face_v3->z;
            vbo_v3->position.w = 1.0f;
            vbo_v3->normal = face_n3_v4;
            vbo_v3->tangent = face_t3;

            vbo_v1->tex_coord.x = face_vt1->x;
            vbo_v1->tex_coord.y = 1 - face_vt1->y;

            vbo_v2->tex_coord.x = face_vt2->x;
            vbo_v2->tex_coord.y = 1 - face_vt2->y;

            vbo_v3->tex_coord.x = face_vt3->x;
            vbo_v3->tex_coord.y = 1 - face_vt3->y;
        }

        state->material_indices[state->num_material_indices++] =
        {
            last_material,
            state->num_vbo_vertices,
        };

        glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
        glErrorAssert();

        glBufferData(GL_ARRAY_BUFFER,
                (GLsizeiptr)(sizeof(state->vbo_vertices[0]) * state->num_faces * 3),
                state->vbo_vertices,
                GL_STATIC_DRAW);
        glErrorAssert();

        load_aabb_buffer_objects(state, state->model_min, state->model_max);
        load_bounding_sphere(state, state->model_min, state->model_max);
        state->hide_lines = true;
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
        if (controller->buttons.move_down.ended_down && !controller->buttons.move_down.repeat)
        {
            state->x_rotation_correction = (state->x_rotation_correction + 1) % 4;
        }
        if (controller->buttons.move_left.ended_down && !controller->buttons.move_left.repeat)
        {
            state->z_rotation_correction = (state->z_rotation_correction + 1) % 4;
        }
    }

    bool mouse_pressed = input->mouse.buttons.left.ended_down == false && input->mouse.buttons.left.repeat == false;
    v2 mouse_loc = {
        (float)input->mouse.x,
        (float)input->window.height - (float)input->mouse.y,
    };

    if (mouse_pressed)
    {
        rectangle2 r = {{38,100},{16,16}};
        if (point_in_rectangle(mouse_loc, r))
        {
            state->hide_lines ^= true;
        }
    }

    glErrorAssert();

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

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glErrorAssert();

    state->delta_t += input->delta_t;

    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);

    glErrorAssert();

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
    m4 rotate = m4rotation(axis, state->y_rotation);
    v3 x_axis = {1.0f, 0.0f, 0.0f};
    v3 z_axis = {0.0f, 0.0f, 1.0f};
    m4 x_rotate = m4rotation(x_axis, state->x_rotation);
    m4 x_correction_rotate = m4rotation(x_axis, (pi / 2) * state->x_rotation_correction);
    m4 z_correction_rotate = m4rotation(z_axis, (pi / 2) * state->z_rotation_correction);
    rotate = m4mul(rotate, x_rotate);
    rotate = m4mul(rotate, x_correction_rotate);
    rotate = m4mul(rotate, z_correction_rotate);

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

    glUseProgram(state->program_c.program);
    glUniform1i(state->program_c.u_shader_mode_id, state->shader_mode);
    glUniform1i(state->program_c.u_shader_config_flags_id, state->shader_config_flags);
    setup_vertex_attrib_array_c(state);
    v4 light_position = {-5.0f, -3.0f, -12.0f, 1.0f};
    glUniform4fv(state->program_c.u_w_lightPosition_id, 1, (float *)&light_position);
    glUniformMatrix4fv(state->program_c.u_model_id, 1, false, (float *)&u_model);
    m4 u_view = m4identity();
    glUniformMatrix4fv(state->program_c.u_view_id, 1, false, (float *)&u_view);
    float ratio = (float)input->window.width / (float)input->window.height;
    m4 u_projection = m4frustumprojection(state->near_, state->far_, {-ratio, -1.0f}, {ratio, 1.0f});
    glUniformMatrix4fv(state->program_c.u_projection_id, 1, false, (float *)&u_projection);

    glUniform1i(
        glGetUniformLocation(state->program_c.program, "tex"),
        0);
    glUniform1i(
        glGetUniformLocation(state->program_c.program, "normal_texture"),
        1);
    glUniform1i(
        glGetUniformLocation(state->program_c.program, "ao_texture"),
        2);
    glUniform1i(
        glGetUniformLocation(state->program_c.program, "emit_texture"),
        3);
    glUniform1i(
        glGetUniformLocation(state->program_c.program, "specular_exponent_texture"),
        4);

    directional_light sun = {
        {0.0f, 1.0f, 0.0f},
        {0.9f, 0.8f, 0.7f},
        0.4f,
        1.0f,
    };

    glUniform3fv(
        glGetUniformLocation(state->program_c.program, "u_directional_light.direction"), 1,
        (float *)&sun.direction);
    glUniform3fv(
        glGetUniformLocation(state->program_c.program, "u_directional_light.color"), 1,
        (float *)&sun.color);
    glUniform1f(
        glGetUniformLocation(state->program_c.program, "u_directional_light.ambient_intensity"),
        sun.ambient_intensity);
    glUniform1f(
        glGetUniformLocation(state->program_c.program, "u_directional_light.diffuse_intensity"),
        sun.diffuse_intensity);

    point_light lights[] = {
        {
            {-5.0f, -3.0f, -12.0f},
            {1.0f, 1.0f, 0.9f},
            0.0f,
            1.0f,
            { 0.0f, 0.0f, 0.03f },
        },
        {
            {8.0f, -3.0f, -8.0f},
            {0.9f, 0.9f, 1.0f},
            0.0f,
            1.0f,
            { 0.0f, 0.0f, 0.05f },
        },
    };
    uint32_t num_point_lights = harray_count(lights);
    for (uint32_t idx = 0; idx < num_point_lights; ++idx)
    {
        point_light *l = lights + idx;
        uint32_t program = state->program_c.program;
        char un[100];
        sprintf(un, "u_point_lights[%d].position", idx);
        glUniform3fv(
            glGetUniformLocation(program, un), 1,
            (float *)&l->position);
        sprintf(un, "u_point_lights[%d].color", idx);
        glUniform3fv(
            glGetUniformLocation(program, un), 1,
            (float *)&l->color);
        sprintf(un, "u_point_lights[%d].ambient_intensity", idx);
        glUniform1f(
            glGetUniformLocation(program, un),
            l->ambient_intensity);
        sprintf(un, "u_point_lights[%d].diffuse_intensity", idx);
        glUniform1f(
            glGetUniformLocation(program, un),
            l->diffuse_intensity);
        sprintf(un, "u_point_lights[%d].attenuation.constant", idx);
        glUniform1f(
            glGetUniformLocation(program, un),
            l->attenuation.constant);
        sprintf(un, "u_point_lights[%d].attenuation.linear", idx);
        glUniform1f(
            glGetUniformLocation(program, un),
            l->attenuation.linear);
        sprintf(un, "u_point_lights[%d].attenuation.exponential", idx);
        glUniform1f(
            glGetUniformLocation(program, un),
            l->attenuation.exponential);
    }
    glUniform1i(
        glGetUniformLocation(state->program_c.program, "u_num_point_lights"),
        (GLint)num_point_lights);

    uint32_t last_vertex = 0;
    for (uint32_t idx = 0; idx < state->num_material_indices; ++idx)
    {
        editor_vertex_indices *i = state->material_indices + idx;

        glActiveTexture(GL_TEXTURE0);
        if (i->current_material->texture_offset >= 0)
        {
            glBindTexture(GL_TEXTURE_2D, state->texture_ids[i->current_material->texture_offset]);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glActiveTexture(GL_TEXTURE0 + 1);
        if (i->current_material->bump_texture_offset >= 0)
        {
            glBindTexture(GL_TEXTURE_2D, state->texture_ids[i->current_material->bump_texture_offset]);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glActiveTexture(GL_TEXTURE0 + 2);
        if (i->current_material->ao_texture_offset >= 0)
        {
            glBindTexture(GL_TEXTURE_2D, state->texture_ids[i->current_material->ao_texture_offset]);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glActiveTexture(GL_TEXTURE0 + 3);
        if (i->current_material->emit_texture_offset >= 0)
        {
            glBindTexture(GL_TEXTURE_2D, state->texture_ids[i->current_material->emit_texture_offset]);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glActiveTexture(GL_TEXTURE0 + 4);
        if (i->current_material->specular_exponent_texture_offset >= 0)
        {
            glBindTexture(GL_TEXTURE_2D, state->texture_ids[i->current_material->specular_exponent_texture_offset]);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glDrawElements(GL_TRIANGLES, (GLsizei)(i->final_vertex_id - last_vertex), GL_UNSIGNED_INT, (uint32_t *)0 + last_vertex);
        last_vertex = i->final_vertex_id;
    }

    glErrorAssert();

    glUseProgram(state->program_b.program);
    glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_projection);
    glUniform4fv(state->program_b.u_w_lightPosition_id, 1, (float *)&light_position);
    glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
    glUniformMatrix4fv(state->program_b.u_view_id, 1, false, (float *)&u_view);

    m4 u_mvp_enabled;
    if (!state->hide_lines)
    {
        setup_vertex_attrib_array_b(state);
        glUniform1i(state->program_b.u_model_mode_id, 0);
        glUniform1i(state->program_b.u_shading_mode_id, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->line_ibo);
        glDepthFunc(GL_LEQUAL);
        u_mvp_enabled = {1.0f, 0.0f, 0.0f, 1.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        glDrawElements(GL_LINES, (GLsizei)state->num_line_elements, GL_UNSIGNED_INT, 0);
    }

    {
        glUniform1i(state->program_b.u_model_mode_id, 0);
        glUniform1i(state->program_b.u_shading_mode_id, 0);
        glBindBuffer(GL_ARRAY_BUFFER, state->aabb_cube_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->aabb_cube_ibo);
        setup_vertex_attrib_array_b(state);
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
        setup_vertex_attrib_array_b(state);
        u_mvp_enabled = {1.0f, 0.0f, 0.0f, 1.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        glPrimitiveRestartIndex(65535);
        glDrawElements(GL_LINE_LOOP, (GLsizei)state->num_bounding_sphere_elements, GL_UNSIGNED_SHORT, 0);
        glErrorAssert();
    }

    {
        m4 u_view = m4identity();
        m4 u_model = m4identity();
        float scale = 100.0f;
        u_model.cols[0].E[0] *= scale;
        u_model.cols[1].E[1] *= scale;
        u_model.cols[2].E[2] *= scale;

        v3 y_axis = {0.0f, 1.0f, 0.0f};
        m4 y_rotate = m4rotation(y_axis, state->y_rotation);
        v3 x_axis = {1.0f, 0.0f, 0.0f};
        m4 x_rotate = m4rotation(x_axis, state->x_rotation);

        u_model = m4mul(x_rotate, u_model);
        u_model = m4mul(y_rotate, u_model);

        m4 u_projection = m4frustumprojection(1.0f, 100.0f, {-ratio, -1.0f}, {ratio, 1.0f});
        glErrorAssert();
        draw_skybox(ctx, memory, &state->skybox, u_model, u_view, u_projection);
        glErrorAssert();
    }

    {
        glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glUseProgram(state->program_ui2d.program);

        float screen_pixel_size[] =
        {
            (float)input->window.width,
            (float)input->window.height,
        };
        glUniform2fv(state->program_ui2d.screen_pixel_size_id, 1, (float *)&screen_pixel_size);

        ui2d_push_context pushctx = {};
        ui2d_vertex_format vertices[2000];
        uint32_t elements[harray_count(vertices) / 4 * 6];
        uint32_t textures[10];
        pushctx.vertices = vertices;
        pushctx.max_vertices = harray_count(vertices);
        pushctx.elements = elements;
        pushctx.max_elements = harray_count(elements);
        pushctx.textures = textures;
        pushctx.max_textures = harray_count(textures);

        push_window(state, &pushctx, 34, 92, 6, 2);

        if (state->hide_lines)
        {
            push_sprite(&pushctx, state->kenney_ui.ui_pack_sprites, state->kenney_ui.ui_pack_sheet.tex, v2{38, 100});
        }
        else
        {
            push_sprite(&pushctx, state->kenney_ui.ui_pack_sprites + 1, state->kenney_ui.ui_pack_sheet.tex, v2{38, 100});
        }

        {
            char full_text[] = "hide lines";
            char *text = (char *)full_text;
            float x = 58;
            float y = input->window.height - 103.0f;
            while (*text) {
                stbtt_aligned_quad q;
                stbtt_GetPackedQuad(state->stb_font.chardata, 512, 512, *text++, &x, &y, &q, 0);
                q.y0 = input->window.height - q.y0;
                q.y1 = input->window.height - q.y1;
                push_quad(&pushctx, q, state->stb_font.font_tex, 1);
            }
        }

        draw_shader_config(ctx, memory, input, &pushctx);
        draw_shader_mode(ctx, memory, input, &pushctx);

        push_window(state, &pushctx, 34, 140, 4, 3);
        {
            uint32_t xbase = 34 + 8;
            uint32_t ybase = 140 + 8;

            struct {
                uint32_t sprite;
                uint32_t x;
                uint32_t y;
                float *axis;
                float iter;
            } sprites[] =
            {
                { 13, 1, 1, &state->x_rotation, 0.005f },
                { 14, 1, 0, &state->x_rotation,-0.005f },
                { 15, 0, 0, &state->y_rotation, 0.005f },
                { 16, 2, 0, &state->y_rotation,-0.005f },
            };
            for (uint32_t idx = 0; idx < harray_count(sprites); ++idx)
            {
                auto sprite = sprites + idx;
                float x = (float)(xbase + (sprite->x * 16));
                float y = (float)(ybase + (sprite->y * 16));
                push_sprite(&pushctx, state->kenney_ui.ui_pack_sprites + sprite->sprite, state->kenney_ui.ui_pack_sheet.tex, v2{x, y});

                rectangle2 r = {{x,y},{16,16}};
                if (point_in_rectangle(mouse_loc, r))
                {
                    *(sprite->axis) += sprite->iter;
                }
            }
        }

        // Mouse should be last
        {
            stbtt_aligned_quad q = {};
            q.x0 = (float)input->mouse.x;
            q.x1 = (float)input->mouse.x + 16;

            q.y0 = (float)(input->window.height - input->mouse.y);
            q.y1 = (float)(input->window.height - input->mouse.y) - 16;
            q.s0 = 0;
            q.s1 = 1;
            q.t0 = 0;
            q.t1 = 1;
            push_quad(&pushctx, q, state->mouse_texture, 0);
        }
        ui2d_render_elements(state, &pushctx);
    }

    glErrorAssert();
}

