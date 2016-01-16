#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>

#include <stdio.h>

#include "hajonta/platform/common.h"
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

struct renderer_state
{
    bool initialized;

    imgui_program_struct imgui_program;
    uint32_t vbo;
    uint32_t ibo;
    uint32_t vao;
    int32_t tex_id;
    uint32_t font_texture;

    ui2d_state ui_state;

    char bitmap_scratch[4096 * 4096 * 4];
    game_input *input;
};

static renderer_state GlobalRendererState;

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
        (float)GlobalRendererState.input->window.width,
        (float)GlobalRendererState.input->window.height,
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

    uint8_t image[256];
    int32_t x, y;
    char filename[] = "ui/slick_arrows/slick_arrow-delta.png";
    bool loaded = load_texture_asset(ctx, memory, state, filename, image, sizeof(image), &x, &y, &state->ui_state.mouse_texture, GL_TEXTURE_2D);
    if (!loaded)
    {
        load_texture_asset_failed(ctx, memory, filename);
        return false;
    }
    return true;
}

bool program_init(hajonta_thread_context *ctx, platform_memory *memory)
{
    imgui_program_struct *program = &GlobalRendererState.imgui_program;
    imgui_program(program, ctx, memory);

    GlobalRendererState.tex_id = glGetUniformLocation(program->program, "tex");

    glGenBuffers(1, &GlobalRendererState.vbo);
    glGenBuffers(1, &GlobalRendererState.ibo);

    glGenVertexArrays(1, &GlobalRendererState.vao);
    glBindVertexArray(GlobalRendererState.vao);
    glBindBuffer(GL_ARRAY_BUFFER, GlobalRendererState.vbo);
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

    glGenTextures(1, &GlobalRendererState.font_texture);
    glBindTexture(GL_TEXTURE_2D, GlobalRendererState.font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    io.Fonts->TexID = (void *)(intptr_t)GlobalRendererState.font_texture;

    return true;
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

    memory->imgui_state = ImGui::GetInternalState();
    if (!GlobalRendererState.initialized)
    {
        program_init(ctx, memory);
        hassert(ui2d_program_init(ctx, memory, &GlobalRendererState));
        GlobalRendererState.initialized = true;
    }

    GlobalRendererState.input = input;

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)input->window.width, (float)input->window.height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    io.DeltaTime = input->delta_t;

    io.MousePos = ImVec2((float)input->mouse.x, (float)input->mouse.y);
    io.MouseDown[0] = input->mouse.buttons.left.ended_down;
    io.MouseDown[1] = 0;
    io.MouseDown[2] = 0;
    io.MouseWheel = 0.0f;

    ImGui::NewFrame();

    return true;
}

void render_draw_lists(ImDrawData* draw_data)
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
    glUseProgram(GlobalRendererState.imgui_program.program);
    glUniform1i(GlobalRendererState.tex_id, 0);
    glUniformMatrix4fv(GlobalRendererState.imgui_program.u_projection_id, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(GlobalRendererState.vao);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, GlobalRendererState.vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(cmd_list->VtxBuffer.size() * sizeof(ImDrawVert)), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GlobalRendererState.ibo);
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

RENDERER_RENDER(renderer_render)
{
    hassert(memory->render_lists_count > 0);
    for (uint32_t i = 0; i < memory->render_lists_count; ++i)
    {
        render_entry_list *render_list = memory->render_lists[i];
        uint32_t offset = 0;
        hassert(render_list->element_count > 0);
        for (uint32_t elements = 0; elements < render_list->element_count; ++elements)
        {
            render_entry_header *header = (render_entry_header *)(render_list->base + offset);
            uint32_t element_size = 0;
            switch (header->type)
            {
                case render_entry_type::clear:
                {
                    render_entry_type_clear *clear = (render_entry_type_clear *)header;
                    element_size = sizeof(*clear);
                    v4 *color = &clear->color;
                    glScissor(0, 0, GlobalRendererState.input->window.width, GlobalRendererState.input->window.height);
                    glClearColor(color->r, color->g, color->b, color->a);
                    glClear(GL_COLOR_BUFFER_BIT);
                } break;
                case render_entry_type::ui2d:
                {
                    render_entry_type_ui2d *ui2d = (render_entry_type_ui2d *)header;
                    draw_ui2d(ctx, memory, &GlobalRendererState, ui2d->pushctx);
                } break;
                default:
                {
                    hassert(!"Unhandled render entry type")
                };
            }
            offset += element_size;
        }
    }

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    render_draw_lists(draw_data);

    glErrorAssert();

    return true;
};
