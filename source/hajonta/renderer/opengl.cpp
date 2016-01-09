#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>

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

RENDERER_SETUP(renderer_setup)
{
#if !defined(NEEDS_EGL) && !defined(__APPLE__)
    if (!glCreateProgram)
    {
        load_glfuncs(ctx, memory->platform_glgetprocaddress);
    }
#endif
    memory->render_lists_count = 0;
    return true;
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
    glErrorAssert();
    return true;
};
