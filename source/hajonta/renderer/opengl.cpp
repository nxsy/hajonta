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

struct renderer_state
{
    bool initialized;

    imgui_program_struct imgui_program;
    uint32_t vbo;
    uint32_t ibo;
    uint32_t vao;
    int32_t tex_id;
    uint32_t font_texture;
};

static renderer_state GlobalRendererState;

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
        GlobalRendererState.initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();

    int w, h;
    w = 960;
    h = 540;
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    io.DeltaTime = (float)(1.0f / 60.0f);

    io.MousePos = ImVec2(-1, -1);
    io.MouseDown[0] = 0;
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
                    glScissor(0, 0, 960, 540);
                    glClearColor(color->r, color->g, color->b, color->a);
                    glClear(GL_COLOR_BUFFER_BIT);
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
