#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl3.h>
#endif

#include <stdio.h>

#include <chrono>

#include "hajonta/platform/common.h"
#include "hajonta/renderer/common.h"

#include "hajonta/thirdparty/glext.h"
#define HGLD(b,a) extern PFNGL##a##PROC gl##b;
extern "C" {
#include "hajonta/platform/glextlist.txt"
}
#undef HGLD

#define HGLD(b,a)  PFNGL##a##PROC gl##b;
#include "hajonta/platform/glextlist.txt"
#undef HGLD

#include "hajonta/programs/imgui.h"
#include "hajonta/programs/ui2d.h"
#include "hajonta/programs/phong_no_normal_map.h"

#include "stb_truetype.h"
#include "hajonta/ui/ui2d.cpp"
#include "hajonta/image.cpp"
#include "hajonta/math.cpp"

inline void
load_glfuncs(hajonta_thread_context *ctx, platform_glgetprocaddress_func *get_proc_address)
{
#define HGLD(b,a) gl##b = (PFNGL##a##PROC)get_proc_address(ctx, (char *)"gl"#b);
#include "hajonta/platform/glextlist.txt"
#undef HGLD
}

inline void
glErrorAssert(bool skip = false)
{
    GLenum error = glGetError();
    if (skip)
    {
         return;
    }
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

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4456 4457 4774 4577)
#endif
#include "imgui.cpp"
#include "imgui_draw.cpp"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

struct ui2d_state
{
    ui2d_program_struct ui2d_program;
    uint32_t vbo;
    uint32_t ibo;
    uint32_t vao;
    uint32_t mouse_texture;
};

struct asset_file_to_texture
{
    int32_t asset_file_id;
    int32_t texture_offset;
};

struct asset_file_to_mesh
{
    int32_t asset_file_id;
    int32_t mesh_offset;
};

struct
asset_file
{
    char asset_file_path[1024];
};

enum struct
AssetType
{
    texture,
    mesh,
};

struct asset
{
    AssetType type;
    uint32_t asset_id;
    char asset_name[100];
    int32_t asset_file_id;
    v2 st0;
    v2 st1;
};

struct mesh_asset
{
    uint32_t asset_id;
    char asset_name[100];
    int32_t asset_file_id;
};

struct renderer_state
{
    bool initialized;

    imgui_program_struct imgui_program;
    phong_no_normal_map_program_struct phong_no_normal_map_program;
    uint32_t vbo;
    uint32_t ibo;
    uint32_t QUADS_ibo;
    uint32_t mesh_vertex_vbo;
    uint32_t mesh_uvs_vbo;
    uint32_t mesh_normals_vbo;
    uint32_t vao;
    int32_t tex_id;
    int32_t phong_no_normal_map_tex_id;
    int32_t phong_no_normal_map_shadowmap_tex_id;
    uint32_t font_texture;
    uint32_t white_texture;

    ui2d_state ui_state;

    char bitmap_scratch[4096 * 4096 * 4];
    uint8_t asset_scratch[4096 * 4096 * 4];
    game_input *input;

    uint32_t textures[16];
    uint32_t texture_count;
    asset_file_to_texture texture_lookup[16];
    uint32_t texture_lookup_count;

    Mesh meshes[16];
    uint32_t mesh_count;
    asset_file_to_mesh mesh_lookup[16];
    uint32_t mesh_lookup_count;

    asset_file asset_files[32];
    uint32_t asset_file_count;
    asset assets[64];
    uint32_t asset_count;

    uint32_t generation_id;

    mouse_input original_mouse_input;

    uint32_t vertices_count;
    struct
    {
        v2 pos;
        v2 uv;
        uint32_t col;
    } vertices_scratch[4 * 2000];
    uint32_t indices_scratch[6 * 2000];

    m4 m4identity;

    struct
    {
        v3 plane_vertices[4];
        v2 plane_uvs[4];
        uint16_t plane_indices[6];
    } debug_mesh_data;
};

int32_t
lookup_asset_file_to_texture(renderer_state *state, int32_t asset_file_id)
{
    int32_t result = -1;
    for (uint32_t i = 0; i < state->texture_lookup_count; ++i)
    {
        asset_file_to_texture *lookup = state->texture_lookup + i;
        if (lookup->asset_file_id == asset_file_id)
        {
            result = lookup->texture_offset;
            break;
        }
    }
    return result;
}

int32_t
lookup_asset_file_to_mesh(renderer_state *state, int32_t asset_file_id)
{
    int32_t result = -1;
    for (uint32_t i = 0; i < state->mesh_lookup_count; ++i)
    {
        asset_file_to_mesh *lookup = state->mesh_lookup + i;
        if (lookup->asset_file_id == asset_file_id)
        {
            result = lookup->mesh_offset;
            break;
        }
    }
    return result;
}

void
add_asset_file_texture_lookup(renderer_state *state, int32_t asset_file_id, int32_t texture_offset)
{
    hassert(state->texture_lookup_count < harray_count(state->texture_lookup));
    asset_file_to_texture *lookup = state->texture_lookup + state->texture_lookup_count;
    ++state->texture_lookup_count;
    lookup->asset_file_id = asset_file_id;
    lookup->texture_offset = texture_offset;
}

void
add_asset_file_mesh_lookup(renderer_state *state, int32_t asset_file_id, int32_t mesh_offset)
{
    hassert(state->mesh_lookup_count < harray_count(state->mesh_lookup));
    asset_file_to_mesh *lookup = state->mesh_lookup + state->mesh_lookup_count;
    ++state->mesh_lookup_count;
    lookup->asset_file_id = asset_file_id;
    lookup->mesh_offset = mesh_offset;
}

static renderer_state _GlobalRendererState;

void
draw_ui2d(hajonta_thread_context *ctx, platform_memory *memory, renderer_state *state, ui2d_push_context *pushctx)
{
    ui2d_state *ui_state = &state->ui_state;
    ui2d_program_struct *ui2d_program = &ui_state->ui2d_program;

    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glUseProgram(ui2d_program->program);
    glBindVertexArray(ui_state->vao);

    float screen_pixel_size[] =
    {
        (float)state->input->window.width,
        (float)state->input->window.height,
    };
    glUniform2fv(ui2d_program->screen_pixel_size_id, 1, (float *)&screen_pixel_size);

    glBindBuffer(GL_ARRAY_BUFFER, ui_state->vbo);
    glBufferData(GL_ARRAY_BUFFER,
            (GLsizei)(pushctx->num_vertices * sizeof(pushctx->vertices[0])),
            pushctx->vertices,
            GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_state->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            (GLsizei)(pushctx->num_elements * sizeof(pushctx->elements[0])),
            pushctx->elements,
            GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray((GLuint)ui2d_program->a_pos_id);
    glEnableVertexAttribArray((GLuint)ui2d_program->a_tex_coord_id);
    glEnableVertexAttribArray((GLuint)ui2d_program->a_texid_id);
    glEnableVertexAttribArray((GLuint)ui2d_program->a_options_id);
    glEnableVertexAttribArray((GLuint)ui2d_program->a_channel_color_id);
    glVertexAttribPointer((GLuint)ui2d_program->a_pos_id, 2, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), 0);
    glVertexAttribPointer((GLuint)ui2d_program->a_tex_coord_id, 2, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, tex_coord));
    glVertexAttribPointer((GLuint)ui2d_program->a_texid_id, 1, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, texid));
    glVertexAttribPointer((GLuint)ui2d_program->a_options_id, 1, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, options));
    glVertexAttribPointer((GLuint)ui2d_program->a_channel_color_id, 3, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, channel_color));

    GLint uniform_locations[10] = {};
    char msg[] = "tex[xx]";
    for (int idx = 0; idx < harray_count(uniform_locations); ++idx)
    {
        sprintf(msg, "tex[%d]", idx);
        uniform_locations[idx] = glGetUniformLocation(ui2d_program->program, msg);

    }

    for (uint32_t i = 0; i < pushctx->num_textures; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, pushctx->textures[i]);
        glUniform1i(
            uniform_locations[i],
            (GLint)i);
    }

    // TODO(nbm): Get textures working properly.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ui_state->mouse_texture);
    glUniform1i(uniform_locations[0], 0);

    glDrawElements(GL_TRIANGLES, (GLsizei)pushctx->num_elements, GL_UNSIGNED_INT, 0);
    glErrorAssert();
}

bool
load_texture_asset(
    hajonta_thread_context *ctx,
    platform_memory *memory,
    renderer_state *state,
    const char *filename,
    uint8_t *image_buffer,
    uint32_t image_size,
    int32_t *x,
    int32_t *y,
    GLuint *tex,
    GLenum target
)
{
    if (!memory->platform_load_asset(ctx, filename, image_size, image_buffer)) {
        return false;
    }

    uint32_t actual_size;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return true;
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

bool
load_mesh_asset(
    hajonta_thread_context *ctx,
    platform_memory *memory,
    renderer_state *state,
    const char *filename,
    uint8_t *mesh_buffer,
    uint32_t mesh_buffer_size,
    Mesh *mesh
)
{
    if (!memory->platform_load_asset(ctx, filename, mesh_buffer_size, mesh_buffer)) {
        return false;
    }

    struct binary_format_v1
    {
        uint32_t format;
        uint32_t version;

        uint32_t vertices_offset;
        uint32_t vertices_size;
        uint32_t normals_offset;
        uint32_t normals_size;
        uint32_t texcoords_offset;
        uint32_t texcoords_size;
        uint32_t indices_offset;
        uint32_t indices_size;
        uint32_t num_triangles;

    } *format = (binary_format_v1 *)mesh_buffer;
    uint32_t memory_size = format->vertices_size + format->normals_size + format->texcoords_size + format->indices_size;
    uint8_t *m = (uint8_t *)malloc(memory_size);

    uint32_t offset = 0;
    memcpy((void *)(m + offset), mesh_buffer + sizeof(binary_format_v1) + format->vertices_offset, format->vertices_size);
    mesh->vertices = {
        (void *)m,
        format->vertices_size,
    };
    offset += format->vertices_size;

    memcpy((void *)(m + offset), mesh_buffer + sizeof(binary_format_v1) + format->indices_offset, format->indices_size);
    mesh->indices = {
        (void *)(m + offset),
        format->indices_size,
    };
    offset += format->indices_size;

    memcpy((void *)(m + offset), mesh_buffer + sizeof(binary_format_v1) + format->texcoords_offset, format->texcoords_size);
    mesh->uvs = {
        (void *)(m + offset),
        format->texcoords_size,
    };
    offset += format->texcoords_size;

    memcpy((void *)(m + offset), mesh_buffer + sizeof(binary_format_v1) + format->normals_offset, format->normals_size);
    mesh->normals = {
        (void *)(m + offset),
        format->normals_size,
    };
    offset += format->normals_size;

    mesh->num_triangles = format->num_triangles;
    return true;
}

void
load_mesh_asset_failed(
    hajonta_thread_context *ctx,
    platform_memory *memory,
    char *filename
)
{
    char msg[1024];
    sprintf(msg, "Could not load mesh %s\n", filename);
    memory->platform_fail(ctx, msg);
    memory->quit = true;
}

bool
ui2d_program_init(hajonta_thread_context *ctx, platform_memory *memory, renderer_state *state)
{
    ui2d_state *ui_state = &state->ui_state;
    ui2d_program_struct *program = &ui_state->ui2d_program;
    ui2d_program(program, ctx, memory);

    glGenBuffers(1, &ui_state->vbo);
    glGenBuffers(1, &ui_state->ibo);

    glGenVertexArrays(1, &ui_state->vao);
    glBindVertexArray(ui_state->vao);

    int32_t x, y;
    char filename[] = "ui/slick_arrows/slick_arrow-delta.png";
    bool loaded = load_texture_asset(ctx, memory, state, filename, state->asset_scratch, sizeof(state->asset_scratch), &x, &y, &state->ui_state.mouse_texture, GL_TEXTURE_2D);
    if (!loaded)
    {
        load_texture_asset_failed(ctx, memory, filename);
        return false;
    }
    return true;
}

bool
program_init(hajonta_thread_context *ctx, platform_memory *memory, renderer_state *state)
{
    {
        imgui_program_struct *program = &state->imgui_program;
        imgui_program(program, ctx, memory);

        state->tex_id = glGetUniformLocation(program->program, "tex");

        glGenBuffers(1, &state->vbo);
        glGenBuffers(1, &state->ibo);
        glGenBuffers(1, &state->QUADS_ibo);
        glGenBuffers(1, &state->mesh_vertex_vbo);
        glGenBuffers(1, &state->mesh_uvs_vbo);

        glGenVertexArrays(1, &state->vao);
        glBindVertexArray(state->vao);
        glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
        glEnableVertexAttribArray((GLuint)program->a_position_id);
        glEnableVertexAttribArray((GLuint)program->a_uv_id);
        glEnableVertexAttribArray((GLuint)program->a_color_id);

        glVertexAttribPointer((GLuint)program->a_position_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, pos));
        glVertexAttribPointer((GLuint)program->a_uv_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, uv));
        glVertexAttribPointer((GLuint)program->a_color_id, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, col));

        ImGuiIO& io = ImGui::GetIO();
        uint8_t *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        glGenTextures(1, &state->font_texture);
        glBindTexture(GL_TEXTURE_2D, state->font_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        io.Fonts->TexID = (void *)(intptr_t)state->font_texture;

        glGenTextures(1, &state->white_texture);
        glBindTexture(GL_TEXTURE_2D, state->white_texture);
        uint32_t color32 = 0xffffffff;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color32);
    }

    {
        phong_no_normal_map_program_struct *program = &state->phong_no_normal_map_program;
        phong_no_normal_map_program(program, ctx, memory);
        state->phong_no_normal_map_tex_id = glGetUniformLocation(program->program, "tex");
        state->phong_no_normal_map_shadowmap_tex_id = glGetUniformLocation(program->program, "shadowmap_tex");
        glGenBuffers(1, &state->mesh_normals_vbo);
    }

    return true;
}

int32_t
lookup_asset_file(renderer_state *state, const char *asset_file_path)
{
    int32_t result = -1;
    for (uint32_t i = 0; i < state->asset_file_count; ++i)
    {
        if (strcmp(asset_file_path, state->asset_files[i].asset_file_path) == 0)
        {
            result = (int32_t)i;
            break;
        }
    }
    return result;
}

int32_t
add_asset_file(renderer_state *state, const char *asset_file_path)
{
    int32_t result = lookup_asset_file(state, asset_file_path);
    hassert(state->asset_file_count < harray_count(state->asset_files));
    if (result < 0 && state->asset_file_count < harray_count(state->asset_files))
    {
        asset_file *f = state->asset_files + state->asset_file_count;
        result = (int32_t)state->asset_file_count;
        ++state->asset_file_count;
        strcpy(f->asset_file_path, asset_file_path);
    }
    return result;
}

int32_t
add_asset_file_texture(hajonta_thread_context *ctx, platform_memory *memory, renderer_state *state, int32_t asset_file_id)
{
    int32_t result = lookup_asset_file_to_texture(state, asset_file_id);
    hassert(state->texture_count < harray_count(state->textures));
    if (result < 0)
    {
        int32_t x, y;
        asset_file *asset_file0 = state->asset_files + asset_file_id;
        bool loaded = load_texture_asset(ctx, memory, state, asset_file0->asset_file_path, state->asset_scratch, sizeof(state->asset_scratch), &x, &y, state->textures + state->texture_count, GL_TEXTURE_2D);
        if (!loaded)
        {
            load_texture_asset_failed(ctx, memory, asset_file0->asset_file_path);
            return false;
        }
        result = (int32_t)state->texture_count;
        ++state->texture_count;
        add_asset_file_texture_lookup(state, asset_file_id, (int32_t)state->textures[result]);
    }
    return result;
}

int32_t
add_asset_file_mesh(hajonta_thread_context *ctx, platform_memory *memory, renderer_state *state, int32_t asset_file_id)
{
    int32_t result = lookup_asset_file_to_mesh(state, asset_file_id);
    hassert(state->mesh_count < harray_count(state->meshes));
    if (result < 0)
    {
        asset_file *asset_file0 = state->asset_files + asset_file_id;
        bool loaded = load_mesh_asset(
            ctx,
            memory,
            state,
            asset_file0->asset_file_path,
            state->asset_scratch,
            sizeof(state->asset_scratch),
            state->meshes + state->mesh_count);
        if (!loaded)
        {
            load_mesh_asset_failed(ctx, memory, asset_file0->asset_file_path);
            return false;
        }
        result = (int32_t)state->mesh_count;
        ++state->mesh_count;
        add_asset_file_mesh_lookup(state, asset_file_id, result);
    }
    return result;
}

int32_t
_add_asset(renderer_state *state, AssetType type, const char *asset_name, int32_t asset_file_id)
{
    int32_t result = -1;
    hassert(state->asset_count < harray_count(state->assets));
    asset *asset0 = state->assets + state->asset_count;
    asset0->asset_id = 0;
    strcpy(asset0->asset_name, asset_name);
    asset0->asset_file_id = asset_file_id;
    result = (int32_t)state->asset_count;
    ++state->asset_count;
    return result;
}

int32_t
add_asset(renderer_state *state, const char *asset_name, int32_t asset_file_id, v2 st0, v2 st1)
{
    int32_t result = _add_asset(state, AssetType::texture, asset_name, asset_file_id);
    asset *asset0 = state->assets + result;
    asset0->st0 = st0;
    asset0->st1 = st1;
    return result;
}

int32_t
add_asset(renderer_state *state, const char *asset_name, const char *asset_file_name, v2 st0, v2 st1)
{
    int32_t result = -1;

    int32_t asset_file_id = add_asset_file(state, asset_file_name);
    if (asset_file_id >= 0)
    {
        result = add_asset(state, asset_name, asset_file_id, st0, st1);
    }
    return result;
}

int32_t
add_mesh_asset(renderer_state *state, const char *asset_name, int32_t asset_file_id)
{
    int32_t result = _add_asset(state, AssetType::mesh, asset_name, asset_file_id);
    return result;
}

int32_t
add_mesh_asset(renderer_state *state, const char *asset_name, const char *asset_file_name)
{
    int32_t result = -1;

    int32_t asset_file_id = add_asset_file(state, asset_file_name);
    if (asset_file_id >= 0)
    {
        result = add_mesh_asset(state, asset_name, asset_file_id);
    }
    return result;
}

int32_t
add_tilemap_asset(renderer_state *state, const char *asset_name, const char *asset_file_name, uint32_t width, uint32_t height, uint32_t tile_width, uint32_t tile_height, uint32_t spacing, uint32_t tile_id)
{
    uint32_t tiles_wide = (width + spacing) / (tile_width + spacing);
    uint32_t tile_x_position = tile_id % tiles_wide;
    uint32_t tile_y_position = tile_id / tiles_wide;
    float tile_x_base = tile_x_position * (tile_width + spacing) / (float)width;
    float tile_y_base = tile_y_position * (tile_height + spacing) / (float)height;
    float tile_x_offset = (float)(tile_width - 1) / width;
    float tile_y_offset = (float)(tile_height - 1) / height;
    v2 st0 = { tile_x_base + 0.001f, tile_y_base + tile_y_offset };
    v2 st1 = { tile_x_base + tile_x_offset, tile_y_base + 0.001f};

    return add_asset(state, asset_name, asset_file_name, st0, st1);
}

extern "C" RENDERER_SETUP(renderer_setup)
{
    static std::chrono::steady_clock::time_point last_frame_start_time = std::chrono::steady_clock::now();

#if !defined(NEEDS_EGL) && !defined(__APPLE__)
    if (!glCreateProgram)
    {
        load_glfuncs(ctx, memory->platform_glgetprocaddress);
    }
#endif
    memory->render_lists_count = 0;
    renderer_state *state = &_GlobalRendererState;

    memory->imgui_state = ImGui::GetInternalState();
    if (!_GlobalRendererState.initialized)
    {
        program_init(ctx, memory, &_GlobalRendererState);
        hassert(ui2d_program_init(ctx, memory, &_GlobalRendererState));
        _GlobalRendererState.initialized = true;
        state->generation_id = 1;
        add_asset(state, "mouse_cursor_old", "ui/slick_arrows/slick_arrow-delta.png", {0.0f, 0.0f}, {1.0f, 1.0f});
        add_asset(state, "mouse_cursor", "testing/kenney/cursorSword_silver.png", {0.0f, 0.0f}, {1.0f, 1.0f});
        add_tilemap_asset(state, "sea_0", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 31);
        add_tilemap_asset(state, "ground_0", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 21);
        add_tilemap_asset(state, "sea_ground_br", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 13);
        add_tilemap_asset(state, "sea_ground_bl", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 14);
        add_tilemap_asset(state, "sea_ground_tr", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 33);
        add_tilemap_asset(state, "sea_ground_tl", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 34);
        add_tilemap_asset(state, "sea_ground_r", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 32);
        add_tilemap_asset(state, "sea_ground_l", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 30);
        add_tilemap_asset(state, "sea_ground_t", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 11);
        add_tilemap_asset(state, "sea_ground_b", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 51);
        add_tilemap_asset(state, "sea_ground_t_l", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 10);
        add_tilemap_asset(state, "sea_ground_t_r", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 12);
        add_tilemap_asset(state, "sea_ground_b_l", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 50);
        add_tilemap_asset(state, "sea_ground_b_r", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 52);
        add_tilemap_asset(state, "bottom_wall", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 69);
        add_asset(state, "player", "testing/kenney/alienPink_stand.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_asset(state, "familiar_ship", "testing/kenney/shipBlue.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_asset(state, "familiar", "testing/kenney/alienBlue_stand.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "plane_mesh", "testing/plane.hjm");
        add_mesh_asset(state, "tree_mesh", "testing/low_poly_tree/tree.hjm");
        add_asset(state, "tree_texture", "testing/low_poly_tree/bake.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "horse_mesh", "testing/rigged_horse/horse.hjm");
        add_asset(state, "horse_texture", "testing/rigged_horse/bake.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "chest_mesh", "testing/chest/chest.hjm");
        add_asset(state, "chest_texture", "testing/chest/diffuse.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "konserian_mesh", "testing/konserian_swamptree/konserian.hjm");
        add_asset(state, "konserian_texture", "testing/konserian_swamptree/BAKE.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "cactus_mesh", "testing/cactus/cactus.hjm");
        add_asset(state, "cactus_texture", "testing/cactus/diffuse.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "kitchen_mesh", "testing/kitchen/kitchen.hjm");
        add_asset(state, "kitchen_texture", "testing/kitchen/BAKE.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "nature_pack_tree_mesh", "testing/kenney/Nature_Pack_3D/tree.hjm");
        add_asset(state, "nature_pack_tree_texture", "testing/kenney/Nature_Pack_3D/tree.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_asset(state, "another_ground_0", "testing/kenney/rpgTile039.png", {0.0f, 1.0f}, {1.0f, 0.0f});

        uint32_t scratch_pos = 0;
        for (uint32_t i = 0; i < harray_count(state->indices_scratch) / 6; ++i)
        {
            uint32_t start = i * 4;
            state->indices_scratch[scratch_pos++] = start + 0;
            state->indices_scratch[scratch_pos++] = start + 1;
            state->indices_scratch[scratch_pos++] = start + 2;
            state->indices_scratch[scratch_pos++] = start + 2;
            state->indices_scratch[scratch_pos++] = start + 3;
            state->indices_scratch[scratch_pos++] = start + 0;
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->QUADS_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            sizeof(state->indices_scratch),
            state->indices_scratch,
            GL_STATIC_DRAW);
        state->m4identity = m4identity();
    }

    _GlobalRendererState.input = input;

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)input->window.width, (float)input->window.height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    std::chrono::steady_clock::time_point this_frame_start_time = std::chrono::steady_clock::now();
    auto diff = this_frame_start_time - last_frame_start_time;
    using microseconds = std::chrono::microseconds;
    int64_t duration_us = std::chrono::duration_cast<microseconds>(diff).count();
    io.DeltaTime = (float)duration_us / 1000 / 1000;
    last_frame_start_time = this_frame_start_time;

    io.MousePos = ImVec2((float)input->mouse.x, (float)input->mouse.y);
    io.MouseDown[0] = input->mouse.buttons.left.ended_down;
    io.MouseDown[1] = input->mouse.buttons.middle.ended_down;
    io.MouseDown[2] = input->mouse.buttons.right.ended_down;
    io.MouseWheel = 0.0f;

    io.KeysDown[8] = false;
    io.KeyMap[ImGuiKey_Backspace] = 8;

    if (memory->debug_keyboard)
    {
        keyboard_input *k = input->keyboard_inputs;
        for (uint32_t idx = 0;
                idx < harray_count(input->keyboard_inputs);
                ++idx)
        {
            keyboard_input *ki = k + idx;
            if (ki->type == keyboard_input_type::ASCII)
            {
                if (ki->ascii == 8)
                {
                     io.KeysDown[(uint32_t)ki->ascii] = true;
                }
                else
                {
                    io.AddInputCharacter((ImWchar)ki->ascii);
                }
            }
        }
    }

    state->original_mouse_input = input->mouse;
    bool wanted_capture_mouse = io.WantCaptureMouse;

    ImGui::NewFrame();

    if (io.WantCaptureKeyboard)
    {
        memory->debug_keyboard = true;
    } else
    {
        memory->debug_keyboard = false;
    }

    if (!io.WantCaptureMouse && wanted_capture_mouse)
    {
        input->mouse.buttons.left.repeat = true;
        state->original_mouse_input.buttons.left.repeat = true;
    }

    if (io.WantCaptureMouse)
    {
        input->mouse.buttons.left.repeat = !BUTTON_WENT_UP(input->mouse.buttons.left) || wanted_capture_mouse;
        input->mouse.buttons.left.ended_down = false;
        input->mouse.buttons.right.repeat = !BUTTON_WENT_UP(input->mouse.buttons.right) || wanted_capture_mouse;
        input->mouse.buttons.right.ended_down = false;
        input->mouse.buttons.middle.repeat = !BUTTON_WENT_UP(input->mouse.buttons.middle) || wanted_capture_mouse;
        input->mouse.buttons.middle.ended_down = false;
    }

    io.MouseDrawCursor = io.WantCaptureMouse;

    return true;
}

void render_draw_lists(ImDrawData* draw_data, renderer_state *state)
{
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(state->imgui_program.program);

    // Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    const float ortho_projection[4][4] =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    glUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, &ortho_projection[0][0]);
    glUniformMatrix4fv(state->imgui_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    glUniformMatrix4fv(state->imgui_program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    glUniform1f(state->imgui_program.u_use_color_id, 1.0f);
    glBindVertexArray(state->vao);

    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    glVertexAttribPointer((GLuint)state->imgui_program.a_position_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, pos));
    glVertexAttribPointer((GLuint)state->imgui_program.a_uv_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, uv));
    glVertexAttribPointer((GLuint)state->imgui_program.a_color_id, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, col));

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(cmd_list->VtxBuffer.size() * sizeof(ImDrawVert)), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx)), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }
    glBindVertexArray(0);
}

static uint32_t generations_updated_this_frame = 0;

void
update_asset_descriptor_asset_id(renderer_state *state, asset_descriptor *descriptor)
{
    if (descriptor->generation_id != state->generation_id)
    {
        ++generations_updated_this_frame;
        int32_t result = -1;
        for (uint32_t i = 0; i < state->asset_count; ++i)
        {
            if (strcmp(descriptor->asset_name, state->assets[i].asset_name) == 0)
            {
                result = (int32_t)i;
                break;
            }
        }
        descriptor->asset_id = result;
        descriptor->generation_id = state->generation_id;
    }
}

void
draw_quad(hajonta_thread_context *ctx, platform_memory *memory, renderer_state *state, m4 *matrices, asset_descriptor *descriptors, render_entry_type_quad *quad)
{
    m4 projection = matrices[quad->matrix_id];

    uint32_t texture = state->white_texture;
    v2 st0 = {0, 0};
    v2 st1 = {1, 1};
    if (quad->asset_descriptor_id != -1)
    {
        asset_descriptor *descriptor = descriptors + quad->asset_descriptor_id;
        update_asset_descriptor_asset_id(state, descriptor);
        if (descriptor->asset_id >= 0)
        {
            asset *descriptor_asset = state->assets + descriptor->asset_id;
            st0 = descriptor_asset->st0;
            st1 = descriptor_asset->st1;
            int32_t asset_file_id = descriptor_asset->asset_file_id;
            int32_t texture_lookup = lookup_asset_file_to_texture(state, asset_file_id);
            if (texture_lookup >= 0)
            {
                texture = (uint32_t)texture_lookup;
            }
            else
            {
                int32_t texture_id = add_asset_file_texture(ctx, memory, state, asset_file_id);
                if (texture_id >= 0)
                {
                    texture = (uint32_t)texture_id;
                }
            }
        }
    }

    glUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, (float *)&projection);
    glUniform1f(state->imgui_program.u_use_color_id, 1.0f);
    glBindVertexArray(state->vao);

    uint32_t col = 0x00000000;
    col |= (uint32_t)(quad->color.w * 255.0f) << 24;
    col |= (uint32_t)(quad->color.z * 255.0f) << 16;
    col |= (uint32_t)(quad->color.y * 255.0f) << 8;
    col |= (uint32_t)(quad->color.x * 255.0f) << 0;
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    struct
    {
        v2 pos;
        v2 uv;
        uint32_t col;
    } vertices[] = {
        {
            { quad->position.x, quad->position.y, },
            { st0.x, st0.y },
            col,
        },
        {
            { quad->position.x + quad->size.x, quad->position.y, },
            { st1.x, st0.y },
            col,
        },
        {
            { quad->position.x + quad->size.x, quad->position.y + quad->size.y, },
            { st1.x, st1.y },
            col,
        },
        {
            { quad->position.x, quad->position.y + quad->size.y, },
            { st0.x, st1.y },
            col,
        },
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    uint32_t indices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), (GLvoid*)indices, GL_STREAM_DRAW);

    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, (GLsizei)harray_count(indices), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void
get_texture_id_from_asset_descriptor(
    hajonta_thread_context *ctx,
    platform_memory *memory,
    renderer_state *state,
    asset_descriptor *descriptors,
    int32_t asset_descriptor_id,
    // out
    uint32_t *texture,
    v2 *st0,
    v2 *st1)
{
    *texture = state->white_texture;
    *st0 = {0, 0};
    *st1 = {1, 1};

    if (asset_descriptor_id != -1)
    {
        asset_descriptor *descriptor = descriptors + asset_descriptor_id;
        switch (descriptor->type)
        {
            case asset_descriptor_type::name:
            {
                update_asset_descriptor_asset_id(state, descriptor);
                if (descriptor->asset_id >= 0)
                {
                    asset *descriptor_asset = state->assets + descriptor->asset_id;
                    *st0 = descriptor_asset->st0;
                    *st1 = descriptor_asset->st1;
                    int32_t asset_file_id = descriptor_asset->asset_file_id;
                    int32_t texture_lookup = lookup_asset_file_to_texture(state, asset_file_id);
                    if (texture_lookup >= 0)
                    {
                        *texture = (uint32_t)texture_lookup;
                    }
                    else
                    {
                        int32_t texture_id = add_asset_file_texture(ctx, memory, state, asset_file_id);
                        if (texture_id >= 0)
                        {
                            *texture = (uint32_t)texture_id;
                        }
                    }
                }
            } break;
            case asset_descriptor_type::framebuffer:
            {
                if (FramebufferInitialized(descriptor->framebuffer))
                {
                    *texture = descriptor->framebuffer->_texture;
                }
            } break;
            case asset_descriptor_type::framebuffer_depth:
            {
                if (FramebufferInitialized(descriptor->framebuffer))
                {
                    *texture = descriptor->framebuffer->_renderbuffer;
                }
            } break;
        }

    }
}

void
get_mesh_from_asset_descriptor(
    hajonta_thread_context *ctx,
    platform_memory *memory,
    renderer_state *state,
    asset_descriptor *descriptors,
    int32_t asset_descriptor_id,
    // out
    Mesh *mesh)
{
    if (asset_descriptor_id != -1)
    {
        asset_descriptor *descriptor = descriptors + asset_descriptor_id;
        update_asset_descriptor_asset_id(state, descriptor);
        if (descriptor->asset_id >= 0)
        {
            asset *descriptor_asset = state->assets + descriptor->asset_id;
            int32_t asset_file_id = descriptor_asset->asset_file_id;

            int32_t mesh_id = lookup_asset_file_to_mesh(state, asset_file_id);
            if (mesh_id < 0)
            {
                mesh_id = add_asset_file_mesh(ctx, memory, state, asset_file_id);
            }
            hassert(mesh_id >= 0);
            if (mesh_id >= 0)
            {
                *mesh = state->meshes[mesh_id];

            }
        }
    }
}

void
draw_quads(hajonta_thread_context *ctx, platform_memory *memory, renderer_state *state, m4 *matrices, asset_descriptor *descriptors, render_entry_type_QUADS *quads)
{
    m4 projection = matrices[quads->matrix_id];

    v2 st0 = {};
    v2 st1 = {};
    uint32_t texture = 0;
    get_texture_id_from_asset_descriptor(
        ctx, memory, state, descriptors, quads->asset_descriptor_id,
        &texture, &st0, &st1);

    glUseProgram(state->imgui_program.program);
    glUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, (float *)&projection);
    glUniformMatrix4fv(state->imgui_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    glUniformMatrix4fv(state->imgui_program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    glUniform1f(state->imgui_program.u_use_color_id, 1.0f);
    glBindVertexArray(state->vao);

    hassert(harray_count(state->vertices_scratch) >= quads->entry_count * 4);

    uint32_t scratch_pos = 0;
    for (uint32_t i = 0; i < quads->entry_count; ++i)
    {
        auto &entry = quads->entries[i];
        uint32_t col = 0x00000000;
        col |= (uint32_t)(entry.color.w * 255.0f) << 24;
        col |= (uint32_t)(entry.color.z * 255.0f) << 16;
        col |= (uint32_t)(entry.color.y * 255.0f) << 8;
        col |= (uint32_t)(entry.color.x * 255.0f) << 0;
        state->vertices_scratch[scratch_pos++] = {
            { entry.position.x, entry.position.y, },
            { st0.x, st0.y },
            col,
        };
        state->vertices_scratch[scratch_pos++] = {
            { entry.position.x + entry.size.x, entry.position.y, },
            { st1.x, st0.y },
            col,
        };

        state->vertices_scratch[scratch_pos++] = {
            { entry.position.x + entry.size.x, entry.position.y + entry.size.y, },
            { st1.x, st1.y },
            col,
        };

        state->vertices_scratch[scratch_pos++] = {
            { entry.position.x, entry.position.y + entry.size.y, },
            { st0.x, st1.y },
            col,
        };
    }

    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    glVertexAttribPointer((GLuint)state->imgui_program.a_position_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, pos));
    glVertexAttribPointer((GLuint)state->imgui_program.a_uv_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, uv));
    glVertexAttribPointer((GLuint)state->imgui_program.a_color_id, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, col));
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(state->vertices_scratch[0])) * 4 * quads->entry_count, state->vertices_scratch, GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->QUADS_ibo);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, (GLsizei)quads->entry_count * 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glErrorAssert();
}

void
draw_mesh(hajonta_thread_context *ctx, platform_memory *memory, renderer_state *state, m4 *matrices, asset_descriptor *descriptors, render_entry_type_mesh *mesh)
{
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    m4 projection = matrices[mesh->projection_matrix_id];
    m4 model = matrices[mesh->model_matrix_id];

    v2 st0 = {};
    v2 st1 = {};
    uint32_t texture = 0;
    get_texture_id_from_asset_descriptor(
        ctx, memory, state, descriptors, mesh->texture_asset_descriptor_id,
        &texture, &st0, &st1);

    glUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, (float *)&projection);
    glUniform1f(state->imgui_program.u_use_color_id, 0.0f);
    glUniformMatrix4fv(state->imgui_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    glUniformMatrix4fv(state->imgui_program.u_model_matrix_id, 1, GL_FALSE, (float *)&model);
    glBindVertexArray(state->vao);

    glBindBuffer(GL_ARRAY_BUFFER, state->mesh_vertex_vbo);
    glBufferData(GL_ARRAY_BUFFER,
            mesh->mesh.vertices.size,
            mesh->mesh.vertices.data,
            GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)state->imgui_program.a_position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, state->mesh_uvs_vbo);
    glBufferData(GL_ARRAY_BUFFER,
            mesh->mesh.uvs.size,
            mesh->mesh.uvs.data,
            GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)state->imgui_program.a_uv_id, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            mesh->mesh.indices.size,
            mesh->mesh.indices.data,
            GL_STATIC_DRAW);

    glDisableVertexAttribArray((GLuint)state->imgui_program.a_color_id);
    glDrawElements(GL_TRIANGLES, (GLsizei)(mesh->mesh.num_triangles * 3), GL_UNSIGNED_SHORT, 0);
    glEnableVertexAttribArray((GLuint)state->imgui_program.a_color_id);

    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_BLEND);
}

void
draw_mesh_from_asset(hajonta_thread_context *ctx, platform_memory *memory, renderer_state *state, m4 *matrices, asset_descriptor *descriptors, LightDescriptor *light_descriptors, render_entry_type_mesh_from_asset *mesh_from_asset)
{
    glErrorAssert();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    m4 projection = matrices[mesh_from_asset->projection_matrix_id];
    m4 model = matrices[mesh_from_asset->model_matrix_id];

    v2 st0 = {};
    v2 st1 = {};
    uint32_t texture = 0;
    get_texture_id_from_asset_descriptor(
        ctx, memory, state, descriptors, mesh_from_asset->texture_asset_descriptor_id,
        &texture, &st0, &st1);

    Mesh mesh = {};
    get_mesh_from_asset_descriptor(ctx, memory, state, descriptors, mesh_from_asset->mesh_asset_descriptor_id, &mesh);

    auto &mesh_descriptor = descriptors[mesh_from_asset->mesh_asset_descriptor_id];

    auto &program = state->phong_no_normal_map_program;
    glUseProgram(program.program);
    glUniform1i(state->phong_no_normal_map_tex_id, 0);
    glUniform1i(state->phong_no_normal_map_shadowmap_tex_id, 1);
    glUniformMatrix4fv(state->phong_no_normal_map_program.u_projection_id, 1, GL_FALSE, (float *)&projection);
    glUniformMatrix4fv(state->phong_no_normal_map_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    glUniformMatrix4fv(state->phong_no_normal_map_program.u_model_matrix_id, 1, GL_FALSE, (float *)&model);

    glEnableVertexAttribArray((GLuint)program.a_position_id);
    glEnableVertexAttribArray((GLuint)program.a_texcoord_id);
    glEnableVertexAttribArray((GLuint)program.a_normal_id);
    glErrorAssert();

    glUniform1i(
        state->phong_no_normal_map_program.u_lightspace_available_id, 0);

    if (mesh_from_asset->lights_mask == 1)
    {
        auto &light = light_descriptors[0];

        v4 position_or_direction;

        switch (light.type)
        {
            case LightType::directional:
            {
                v3 direction = light.direction;
                direction = v3normalize(direction);

                position_or_direction = {
                    direction.x,
                    direction.y,
                    direction.z,
                    0.0f,
                };
            } break;
        }

        if (mesh_from_asset->attach_shadowmap)
        {
            glActiveTexture(GL_TEXTURE0 + 1);
            uint32_t shadowmap_texture;
            get_texture_id_from_asset_descriptor(
                ctx, memory, state, descriptors,
                light.shadowmap_asset_descriptor_id,
                &shadowmap_texture, &st0, &st1);
            glBindTexture(GL_TEXTURE_2D, shadowmap_texture);
            glActiveTexture(GL_TEXTURE0);

            m4 shadowmap_matrix = matrices[light.shadowmap_matrix_id];
            glUniformMatrix4fv(state->phong_no_normal_map_program.u_lightspace_matrix_id, 1, GL_FALSE, (float *)&shadowmap_matrix);

            glUniform1i(
                state->phong_no_normal_map_program.u_lightspace_available_id, 1);
            glUniformMatrix4fv(state->phong_no_normal_map_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
        }
        glUniform4fv(
            glGetUniformLocation(program.program, "light.position_or_direction"), 1,
            (float *)&position_or_direction.x);
        glUniform3fv(
            glGetUniformLocation(program.program, "light.color"), 1,
            (float *)&light.color);
        glUniform1f(
            glGetUniformLocation(program.program, "light.ambient_intensity"),
            light.ambient_intensity);
        glUniform1f(
            glGetUniformLocation(program.program, "light.diffuse_intensity"),
            light.diffuse_intensity);
    }

    glBindVertexArray(state->vao);

    glBindBuffer(GL_ARRAY_BUFFER, state->mesh_vertex_vbo);
    glBufferData(GL_ARRAY_BUFFER,
            mesh.vertices.size,
            mesh.vertices.data,
            GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)state->phong_no_normal_map_program.a_position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glErrorAssert();

    glBindBuffer(GL_ARRAY_BUFFER, state->mesh_uvs_vbo);
    glBufferData(GL_ARRAY_BUFFER,
            mesh.uvs.size,
            mesh.uvs.data,
            GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)state->phong_no_normal_map_program.a_texcoord_id, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glErrorAssert();

    glBindBuffer(GL_ARRAY_BUFFER, state->mesh_normals_vbo);
    glBufferData(GL_ARRAY_BUFFER,
            mesh.normals.size,
            mesh.normals.data,
            GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)state->phong_no_normal_map_program.a_normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glErrorAssert();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            mesh.indices.size,
            mesh.indices.data,
            GL_STATIC_DRAW);


    glBindTexture(GL_TEXTURE_2D, texture);
    glErrorAssert();

    int32_t max_faces = (int32_t)mesh.num_triangles;
    int32_t start_face = 0;
    int32_t end_face = max_faces;
    if (mesh_descriptor.debug)
    {
        if (mesh_descriptor.mesh_debug.start_face > max_faces)
        {
            mesh_descriptor.mesh_debug.start_face = 0;
        }
        if (mesh_descriptor.mesh_debug.end_face > max_faces)
        {
            mesh_descriptor.mesh_debug.end_face = max_faces;
        }

        ImGui::DragIntRange2("mesh faces", &mesh_descriptor.mesh_debug.start_face, &mesh_descriptor.mesh_debug.end_face, 5.0f, 0, max_faces);
        if (ImGui::Button("Mesh debug reset"))
        {
            mesh_descriptor.mesh_debug.start_face = 0;
            mesh_descriptor.mesh_debug.end_face = max_faces;
        }
        start_face = mesh_descriptor.mesh_debug.start_face;
        end_face = mesh_descriptor.mesh_debug.end_face;
    }
    int32_t num_faces = end_face - start_face;
    glDrawElements(GL_TRIANGLES, (GLsizei)(num_faces * 3), GL_UNSIGNED_INT, (GLvoid *)(start_face * 3 * sizeof(GLuint)));
    glErrorAssert();

    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_BLEND);
    glErrorAssert();
}

extern "C" RENDERER_RENDER(renderer_render)
{
    ImGuiIO& io = ImGui::GetIO();
    generations_updated_this_frame = 0;
    glErrorAssert(true);

    renderer_state *state = &_GlobalRendererState;
    hassert(memory->render_lists_count > 0);

    window_data *window = &state->input->window;
    glErrorAssert();

    uint32_t quads_drawn = 0;
    for (uint32_t i = 0; i < memory->render_lists_count; ++i)
    {
        glViewport(0, 0, (GLsizei)window->width, (GLsizei)window->height);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glScissor(0, 0, window->width, window->height);
        glUseProgram(state->imgui_program.program);
        glUniform1i(state->tex_id, 0);

        glUniformMatrix4fv(state->imgui_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
        glUniformMatrix4fv(state->imgui_program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);

        render_entry_list *render_list = memory->render_lists[i];
        uint32_t offset = 0;
        hassert(render_list->element_count > 0);

        m4 *matrices = {};
        asset_descriptor *asset_descriptors = {};
        LightDescriptors lights = {};

        uint32_t matrix_count = 0;
        uint32_t asset_descriptor_count = 0;

        FramebufferDescriptor *framebuffer = render_list->framebuffer;
        if (framebuffer)
        {
            if (!FramebufferInitialized(framebuffer))
            {
                glGenFramebuffers(1, &framebuffer->_fbo);
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->_fbo);

                if (!framebuffer->_flags.no_color_buffer)
                {
                    glGenTextures(1, &framebuffer->_texture);
                    glBindTexture(GL_TEXTURE_2D, framebuffer->_texture);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, framebuffer->size.x, framebuffer->size.y, 0, GL_RGBA, GL_BYTE, NULL);

                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }

                if (!framebuffer->_flags.use_depth_texture)
                {
                    glGenRenderbuffers(1, &framebuffer->_renderbuffer);
                    glBindRenderbuffer(GL_RENDERBUFFER, framebuffer->_renderbuffer);
                    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, framebuffer->size.x, framebuffer->size.y);
                }
                else
                {
                    glGenTextures(1, &framebuffer->_renderbuffer);
                    glBindTexture(GL_TEXTURE_2D, framebuffer->_renderbuffer);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, framebuffer->size.x, framebuffer->size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }

                FramebufferMakeInitialized(framebuffer);
            }
            glErrorAssert();

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->_fbo);
            if (!framebuffer->_flags.no_color_buffer)
            {
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->_texture, 0);
                GLenum draw_buffers[] =
                {
                    GL_COLOR_ATTACHMENT0,
                };
                glDrawBuffers(harray_count(draw_buffers), draw_buffers);
            }
            else
            {
                glDrawBuffer(GL_NONE);
            }

            if (!framebuffer->_flags.use_depth_texture)
            {
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer->_renderbuffer);
            }
            else
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebuffer->_renderbuffer, 0);

            }

            glViewport(0, 0, framebuffer->size.x, framebuffer->size.y);
            GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            hassert(framebuffer_status == GL_FRAMEBUFFER_COMPLETE);
        }

        for (uint32_t elements = 0; elements < render_list->element_count; ++elements)
        {
            render_entry_header *header = (render_entry_header *)(render_list->base + offset);
            uint32_t element_size = 0;

            switch (header->type)
            {
                case render_entry_type::clear:
                {
                    ExtractRenderElementWithSize(clear, item, header, element_size);
                    v4 *color = &item->color;
                    glScissor(0, 0, state->input->window.width, state->input->window.height);
                    glClearColor(color->r, color->g, color->b, color->a);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                } break;
                case render_entry_type::ui2d:
                {
                    ExtractRenderElementWithSize(ui2d, item, header, element_size);
                    draw_ui2d(ctx, memory, state, item->pushctx);
                } break;
                case render_entry_type::quad:
                {
                    ExtractRenderElementWithSize(quad, item, header, element_size);
                    hassert(matrix_count > 0 || item->matrix_id == -1);
                    hassert(asset_descriptor_count > 0 || item->asset_descriptor_id == -1);
                    draw_quad(ctx, memory, state, matrices, asset_descriptors, item);
                    ++quads_drawn;
                } break;
                case render_entry_type::matrices:
                {
                    ExtractRenderElementWithSize(matrices, item, header, element_size);
                    matrices = item->matrices;
                    matrix_count = item->count;
                } break;
                case render_entry_type::asset_descriptors:
                {
                    ExtractRenderElementWithSize(asset_descriptors, item, header, element_size);
                    asset_descriptors = item->descriptors;
                    asset_descriptor_count = item->count;
                } break;
                case render_entry_type::descriptors:
                {
                    ExtractRenderElementWithSize(descriptors, item, header, element_size);
                    lights = item->lights;
                } break;
                case render_entry_type::QUADS:
                {
                    ExtractRenderElementWithSize(QUADS, item, header, element_size);
                    draw_quads(ctx, memory, state, matrices, asset_descriptors, item);
                } break;
                case render_entry_type::QUADS_lookup:
                {
                    ExtractRenderElementSizeOnly(QUADS_lookup, element_size);
                } break;
                case render_entry_type::mesh:
                {
                    ExtractRenderElementWithSize(mesh, item, header, element_size);
                    draw_mesh(ctx, memory, state, matrices, asset_descriptors, item);
                } break;
                case render_entry_type::mesh_from_asset:
                {
                    ExtractRenderElementWithSize(mesh_from_asset, item, header, element_size);
                    draw_mesh_from_asset(ctx, memory, state, matrices, asset_descriptors, lights.descriptors, item);
                } break;
                default:
                {
                    hassert(!"Unhandled render entry type")
                };
            }
            glErrorAssert();
            hassert(element_size > 0);
            offset += element_size;
        }

        if (framebuffer)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }
    ImGui::Text("Quads drawn: %d", quads_drawn);
    ImGui::Text("Generations updated: %d", generations_updated_this_frame);
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("This frame: %.3f ms", 1000.0f * io.DeltaTime);

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    render_draw_lists(draw_data, state);

    glErrorAssert();

    input->mouse = state->original_mouse_input;

    return true;
};
