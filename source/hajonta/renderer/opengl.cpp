#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>

#include <stdio.h>

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

struct
asset_file
{
    char asset_file_path[1024];
};

struct asset
{
    uint32_t asset_id;
    char asset_name[100];
    int32_t asset_file_id;
    v2 st0;
    v2 st1;
};

struct renderer_state
{
    bool initialized;

    imgui_program_struct imgui_program;
    uint32_t vbo;
    uint32_t ibo;
    uint32_t vao;
    int32_t tex_id;
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

    asset_file asset_files[16];
    uint32_t asset_file_count;
    asset assets[16];
    uint32_t asset_count;

    uint32_t generation_id;
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

void
add_asset_file_texture_lookup(renderer_state *state, int32_t asset_file_id, int32_t texture_offset)
{
    hassert(state->texture_lookup_count < harray_count(state->texture_lookup));
    asset_file_to_texture *lookup = state->texture_lookup + state->texture_lookup_count;
    ++state->texture_lookup_count;
    lookup->asset_file_id = asset_file_id;
    lookup->texture_offset = texture_offset;
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
    imgui_program_struct *program = &state->imgui_program;
    imgui_program(program, ctx, memory);

    state->tex_id = glGetUniformLocation(program->program, "tex");

    glGenBuffers(1, &state->vbo);
    glGenBuffers(1, &state->ibo);

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

    return true;
}

int32_t
lookup_asset_file(renderer_state *state, char *asset_file_path)
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
add_asset_file(renderer_state *state, char *asset_file_path)
{
    int32_t result = lookup_asset_file(state, asset_file_path);
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
add_asset(renderer_state *state, char *asset_name, int32_t asset_file_id, v2 st0, v2 st1)
{
    int32_t result = -1;
    asset *asset0 = state->assets + state->asset_count;
    asset0->asset_id = 0;
    strcpy(asset0->asset_name, asset_name);
    asset0->asset_file_id = asset_file_id;
    asset0->st0 = st0;
    asset0->st1 = st1;
    result = (int32_t)state->asset_count;
    ++state->asset_count;
    return result;
}

int32_t
add_asset(renderer_state *state, char *asset_name, char *asset_file_name, v2 st0, v2 st1)
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
add_tilemap_asset(renderer_state *state, char *asset_name, char *asset_file_name, uint32_t width, uint32_t height, uint32_t tile_width, uint32_t tile_height, uint32_t spacing, uint32_t tile_id)
{
    uint32_t tiles_wide = (width + spacing) / (tile_width + spacing);
    uint32_t tile_x_position = tile_id % tiles_wide;
    uint32_t tile_y_position = tile_id / tiles_wide;
    float tile_x_base = tile_x_position * (tile_width + spacing) / (float)width;
    float tile_y_base = tile_y_position * (tile_height + spacing) / (float)height;
    float tile_x_offset = (float)(tile_width - 1) / width;
    float tile_y_offset = (float)(tile_height - 1) / height;
    v2 st0 = { tile_x_base, tile_y_base + tile_y_offset };
    v2 st1 = { tile_x_base + tile_x_offset, tile_y_base };

    //add_asset(state, "sea_0", "testing/kenney/roguelikeSheet_transparent.png", {0.0f / 967.0f, 15.0f / 525.0f}, {15.0f / 967.0f, 0.0f / 525.0f});

    return add_asset(state, asset_name, asset_file_name, st0, st1);
}

RENDERER_SETUP(renderer_setup)
{
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
        //add_asset(state, "sea_0", "testing/kenney/roguelikeSheet_transparent.png", {0.0f / 967.0f, 15.0f / 525.0f}, {15.0f / 967.0f, 0.0f / 525.0f});
        add_tilemap_asset(state, "sea_0", "testing/kenney/roguelikeSheet_transparent.png", 968, 526, 16, 16, 1, 578);
    }

    _GlobalRendererState.input = input;

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)input->window.width, (float)input->window.height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    io.DeltaTime = input->delta_t;

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
                     io.KeysDown[ki->ascii] = true;
                }
                else
                {
                    io.AddInputCharacter((ImWchar)ki->ascii);
                }
            }
        }
    }

    ImGui::NewFrame();

    if (io.WantCaptureKeyboard)
    {
        memory->debug_keyboard = true;
    } else
    {
        memory->debug_keyboard = false;
    }

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
    glUseProgram(state->imgui_program.program);
    glUniform1i(state->tex_id, 0);
    glUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(state->vao);

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

void
update_asset_descriptor_asset_id(renderer_state *state, asset_descriptor *descriptor)
{
    if (descriptor->generation_id != state->generation_id)
    {
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

    window_data *window = &state->input->window;
    glViewport(0, 0, (GLsizei)window->width, (GLsizei)window->height);
    float ratio = (float)window->width / (float)window->height;
    m4 projection;
    if (quad->matrix_id == -1)
    {
        projection = m4orthographicprojection(1.0f, -1.0f, {-ratio, -1.0f}, {ratio, 1.0f});
    }
    else
    {
        projection = matrices[quad->matrix_id];
    }

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

    glUseProgram(state->imgui_program.program);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(state->tex_id, 0);
    glUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, (float *)&projection);
    glBindVertexArray(state->vao);

    uint32_t col = 0xff000000;
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
    glScissor(0, 0, window->width, window->height);
    glDrawElements(GL_TRIANGLES, (GLsizei)harray_count(indices), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

RENDERER_RENDER(renderer_render)
{
    renderer_state *state = &_GlobalRendererState;
    hassert(memory->render_lists_count > 0);
    for (uint32_t i = 0; i < memory->render_lists_count; ++i)
    {
        render_entry_list *render_list = memory->render_lists[i];
        uint32_t offset = 0;
        hassert(render_list->element_count > 0);

        m4 *matrices = {};
        asset_descriptor *asset_descriptors = {};
        uint32_t matrix_count = 0;
        uint32_t asset_descriptor_count = 0;

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
                    glClear(GL_COLOR_BUFFER_BIT);
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
                } break;
                default:
                {
                    hassert(!"Unhandled render entry type")
                };
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
            }
            hassert(element_size > 0);
            offset += element_size;
        }
    }

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    render_draw_lists(draw_data, state);

    glErrorAssert();

    return true;
};
