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

#define HJ_DIRECTGL 1
#include "hajonta/platform/common.h"
#include "hajonta/math.cpp"
#include "hajonta/image.cpp"
#include "hajonta/image/dds.h"
#include "hajonta/bmp.cpp"
#include "hajonta/font.cpp"

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4505)
#endif
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "hajonta/programs/ui2d.h"
#include "hajonta/ui/ui2d.cpp"

static float pi = 3.14159265358979f;

struct game_state
{
    uint32_t vao;

    uint32_t ui2d_vbo;
    uint32_t ui2d_ibo;

    loaded_file image_file;
    uint32_t image_tex;
    uint32_t image_width;
    uint32_t image_height;

    uint32_t other_tex;

    ui2d_program_struct program_ui2d;
    char bitmap_scratch[4096 * 4096 * 4];
};

bool
gl_setup(hajonta_thread_context *ctx, platform_memory *memory)
{
    glErrorAssert();
    game_state *state = (game_state *)memory->memory;

#if !defined(NEEDS_EGL)
    if (&glGenVertexArrays != 0)
    {
        glGenVertexArrays(1, &state->vao);
        glBindVertexArray(state->vao);
        glErrorAssert();
    }
#endif

    glErrorAssert();

    bool loaded;

    loaded = ui2d_program(&state->program_ui2d, ctx, memory);
    if (!loaded)
    {
        return loaded;
    }
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
    GLuint *tex,
    GLenum target
)
{
    game_state *state = (game_state *)memory->memory;
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
    return true;
}

void
draw_ui(hajonta_thread_context *ctx, platform_memory *memory, game_input *input)
{
    game_state *state = (game_state *)memory->memory;
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
    ui2d_vertex_format vertices[4000];
    uint32_t elements[harray_count(vertices) / 4 * 6];
    uint32_t textures[10];
    pushctx.vertices = vertices;
    pushctx.max_vertices = harray_count(vertices);
    pushctx.elements = elements;
    pushctx.max_elements = harray_count(elements);
    pushctx.textures = textures;
    pushctx.max_textures = harray_count(textures);


    {
        stbtt_aligned_quad q;
        q.x0 = 1;
        q.x1 = q.x0 + 256;
        q.y0 = 1;
        q.y1 = q.y0 + 256;
        q.s0 = 0;
        q.s1 = 1;
        q.t0 = 0;
        q.t1 = 1;
        push_quad(&pushctx, q, state->image_tex, 0);
    }

    {
        stbtt_aligned_quad q;
        q.x0 = 1;
        q.x1 = q.x0 + 256;
        q.y0 = 1;
        q.y1 = q.y0 + 256;
        q.s0 = 0;
        q.s1 = 1;
        q.t0 = 0;
        q.t1 = 1;
        push_quad(&pushctx, q, state->image_tex, 0);
    }

    ui2d_render_elements(&pushctx, &state->program_ui2d, state->ui2d_vbo, state->ui2d_ibo);
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

        memory->initialized = 1;

        glErrorAssert();
        glGenBuffers(1, &state->ui2d_vbo);
        glGenBuffers(1, &state->ui2d_ibo);
        glErrorAssert();
        glGenTextures(1, &state->image_tex);
        glGenTextures(1, &state->other_tex);

        bool loaded = false;
        char *start_position, *eof;
        while (!loaded)
        {
            if (!memory->platform_editor_load_file(ctx, &state->image_file))
            {
                continue;
            }
            start_position = (char *)state->image_file.contents;
            eof = start_position + state->image_file.size;
            if (
                (start_position[0] != 'D') ||
                (start_position[1] != 'D') ||
                (start_position[2] != 'S') ||
                (start_position[3] != ' '))
            {
                continue;
            }
            DDSURFACEDESC *surface = (DDSURFACEDESC *)(start_position + 4);
            hassert(surface->size == sizeof(DDSURFACEDESC));

            uint32_t block_size;
            uint32_t format;

            switch (surface->format.four_cc)
            {
                case DXT1:
                {
                    block_size = 8;
                    format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                } break;
                case DXT3:
                {
                    block_size = 16;
                    format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                } break;
                case DXT5:
                {
                    block_size = 16;
                    format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                } break;
                default:
                {
                    continue;
                }
            }

            glBindTexture(GL_TEXTURE_2D, state->image_tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            uint32_t size = ((surface->width + 3) / 4) * ((surface->height + 3) / 4) * block_size;
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, format,
                (GLsizei)surface->width, (GLsizei)surface->height, 0, (GLsizei)size,
                (uint8_t *)(start_position + 4 + surface->size));
            glErrorAssert();
            glBindTexture (GL_TEXTURE_2D, 0);
            state->image_width = surface->width;
            state->image_height = surface->height;
            loaded = true;
        }

        {
            uint8_t image[636];
            int32_t x, y;
            char filename[] = "ui/kenney/glassPanel.png";
            bool gp_loaded = load_texture_asset(ctx, memory, filename, image, sizeof(image), &x, &y, &state->other_tex, GL_TEXTURE_2D);
            hassert(gp_loaded);
        }

        glErrorAssert();
    }

    for (uint32_t i = 0;
            i < harray_count(input->controllers);
            ++i)
    {
        if (!input->controllers[i].is_active)
        {
            continue;
        }
        // game_controller_state *controller = &input->controllers[i];
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
    glEnable(GL_PRIMITIVE_RESTART);

    glErrorAssert();

    glViewport(0, 0, input->window.width, input->window.height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_ui(ctx, memory, input);

    glErrorAssert();
}

