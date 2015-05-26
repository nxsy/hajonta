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

#include "hajonta/programs/ui2d.h"
#include "hajonta/ui/ui2d.cpp"

#define DDS_FOURCC      0x00000004
#define MAKE_FOURCC(ch0, ch1, ch2, ch3) \
    (uint32_t)( \
        (uint32_t)((ch3) << 24) | \
        (uint32_t)((ch2) << 16) | \
        (uint32_t)((ch1) << 8) | \
        (uint32_t)(ch0) \
    )
#define DXT1 MAKE_FOURCC('D', 'X', 'T', '1')
#define DXT3 MAKE_FOURCC('D', 'X', 'T', '3')
#define DXT5 MAKE_FOURCC('D', 'X', 'T', '5')

struct DDSCAPS2
{
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    union
    {
        uint32_t caps4;
        uint32_t volume_depth;
    };
};

struct DDCOLORKEY
{
    uint32_t low;
    uint32_t high;
};

struct DDPIXELFORMAT
{
    uint32_t size;
    uint32_t flags;
    uint32_t four_cc;
    union
    {
        uint32_t rgb_bit_count;
        uint32_t yuv_bit_count;
        uint32_t zbuffer_bit_depth;
        uint32_t alpha_bit_depth;
        uint32_t luminance_bit_count;
        uint32_t bump_bit_count;
        uint32_t private_format_bit_count;
    };
    union
    {
        uint32_t r_bit_mask;
        uint32_t y_bit_mask;
        uint32_t stencil_bit_depth;
        uint32_t luminance_bit_mask;
        uint32_t bump_du_bit_mask;
        uint32_t operations;
    };
    union
    {
        uint32_t g_bit_mask;
        uint32_t u_bit_mask;
        uint32_t z_bit_mask;
        uint32_t bump_dv_bit_mask;
        struct {
            uint16_t flip_ms_types;
            uint16_t blt_ms_types;
        } MultiSampleCaps;
    };
    union
    {
        uint32_t b_bit_mask;
        uint32_t v_bit_mask;
        uint32_t stencil_bit_mask;
        uint32_t bump_luminance_bit_mask;
    };
    union
    {
        uint32_t rgb_alpha_bit_mask;
        uint32_t yuv_alpha_bit_mask;
        uint32_t luminance_alpha_bit_mask;
        uint32_t rgb_z_bit_mask;
        uint32_t yuv_z_bit_mask;
    };
};

struct DDSURFACEDESC
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    union
    {
        int32_t pitch;
        uint32_t linear_size;
    };
    union
    {
        uint32_t back_buffer_count;
        uint32_t depth;
    };
    union
    {
        uint32_t mip_map_count;
        uint32_t refresh_rate;
    };
    uint32_t alpha_bit_depth;
    uint32_t _reserved0;
    uint32_t surface;

    DDCOLORKEY dest_overlay;
    DDCOLORKEY dest_blt;
    DDCOLORKEY src_overlay;
    DDCOLORKEY src_blt;
    DDPIXELFORMAT format;
    DDSCAPS2     caps;
    uint32_t    texture_stage;
};

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
    if (glGenVertexArrays != 0)
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
            printf("Woo, a DDS file!\n");
            DDSURFACEDESC *surface = (DDSURFACEDESC *)(start_position + 4);
            printf("sizeof(DDSURFACEDESC) = %ld\n", sizeof(DDSURFACEDESC));
            printf("surface->size = %d\n", surface->size);
            hassert(surface->size == sizeof(DDSURFACEDESC));
            printf("surface->width = %d\n", surface->width);
            printf("surface->height = %d\n", surface->height);
            printf("surface->mip_map_count = %d\n", surface->mip_map_count);
            printf("surface->format.flags = %d\n", surface->format.flags);
            printf("surface->format.flags & DDS_FOURCC (0x4) = %d\n", surface->format.flags & DDS_FOURCC);
            printf("surface->format.four_cc = %d\n", surface->format.four_cc);
            printf("DXT1 == %d\n", DXT1);

            switch (surface->format.four_cc)
            {
                case DXT1:
                {
                    glBindTexture(GL_TEXTURE_2D, state->image_tex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    uint32_t block_size = 8;
                    uint32_t size = ((surface->width + 3) / 4) * ((surface->height + 3) / 4) * block_size;
                    printf("TexImage2D size = %d\n", size);
                    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                        surface->width, surface->height, 0, size,
                        (uint8_t *)(start_position + 4 + surface->size));
                    glErrorAssert();
                    glBindTexture (GL_TEXTURE_2D, 0);
                    state->image_width = surface->width;
                    state->image_height = surface->height;
                    loaded = true;
                } break;
            }
        }

        {
            uint8_t image[636];
            int32_t x, y;
            char filename[] = "ui/kenney/glassPanel.png";
            bool loaded = load_texture_asset(ctx, memory, filename, image, sizeof(image), &x, &y, &state->other_tex, GL_TEXTURE_2D);
            hassert(loaded);
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
        game_controller_state *controller = &input->controllers[i];
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

