#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <windows.h>
#include <gl/gl.h>

#include "hajonta/platform/common.h"
#include "hajonta/platform/win32.h"

struct platform_state
{
    int32_t stopping;
    char *stop_reason;

    HDC device_context;
};

struct vertex
{
    float position[4];
    float color[4];
};

LRESULT CALLBACK
main_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    platform_state *state;
    if (uMsg == WM_NCCREATE)
    {
        state = (platform_state *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
    }
    else
    {
        state = (platform_state *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (!state)
        {
            // This message is allowed to arrive before WM_NCCREATE (!)
            if (uMsg != WM_GETMINMAXINFO)
            {
                char debug_string[1024] = {};
                _snprintf(debug_string, 1023, "Non-WM_NCCREATE without any state set: %d\n", uMsg);
                OutputDebugStringA(debug_string);
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    LRESULT lResult = 0;

    switch (uMsg)
    {
        case WM_CREATE:
        {
            PIXELFORMATDESCRIPTOR pixel_format_descriptor = {
                sizeof(PIXELFORMATDESCRIPTOR),
                1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                PFD_TYPE_RGBA,
                32,
                0, 0, 0, 0, 0, 0,
                0,
                0,
                0,
                0, 0, 0, 0,
                24,
                8,
                0,
                PFD_MAIN_PLANE,
                0,
                0, 0, 0
            };

            state->device_context = GetDC(hwnd);
            if (!state->device_context)
            {
                MessageBoxA(0, "Unable to acquire device context", 0, MB_OK | MB_ICONSTOP);
                state->stopping = 1;
                return 1;
            }

            int pixel_format = ChoosePixelFormat(state->device_context, &pixel_format_descriptor);
            if (!pixel_format)
            {
                MessageBoxA(0, "Unable to choose requested pixel format", 0, MB_OK | MB_ICONSTOP);
                state->stopping = 1;
                return 1;
            }

            if(!SetPixelFormat(state->device_context, pixel_format, &pixel_format_descriptor))
            {
                MessageBoxA(0, "Unable to set pixel format", 0, MB_OK | MB_ICONSTOP);
                state->stopping = 1;
                return 1;
            }

            HGLRC wgl_context = wglCreateContext(state->device_context);
            if(!wgl_context)
            {
                MessageBoxA(0, "Unable to create wgl context", 0, MB_OK | MB_ICONSTOP);
                state->stopping = 1;
                return 1;
            }

            if(!wglMakeCurrent(state->device_context, wgl_context))
            {
                MessageBoxA(0, "Unable to make wgl context current", 0, MB_OK | MB_ICONSTOP);
                state->stopping = 1;
                return 1;
            }

            wglCreateContextAttribsARB =
                (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

            if(!wglCreateContextAttribsARB)
            {
                MessageBoxA(0, "Unable to get function pointer to wglCreateContextAttribsARB", 0, MB_OK | MB_ICONSTOP);
                state->stopping = 1;
                return 1;
            }

            int context_attribs[] =
            {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, 2,
                WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };

            HGLRC final_context = wglCreateContextAttribsARB(state->device_context, 0, context_attribs);

            if(!final_context)
            {
                MessageBoxA(0, "Unable to upgrade to OpenGL 3.2", 0, MB_OK | MB_ICONSTOP);
                state->stopping = 1;
                return 1;
            }

            wglMakeCurrent(0, 0);
            wglDeleteContext(wgl_context);
            wglMakeCurrent(state->device_context, final_context);

            load_glfuncs();

            glViewport(0, 0, 960, 540);
        } break;
        case WM_SIZE:
        {
            int32_t window_width = LOWORD(lParam);
            int32_t window_height = HIWORD(lParam);
            int32_t height = window_height;
            int32_t width = window_width;
            float ratio = 960.0f / 540.0f;
            if (height > width / ratio)
            {
                height = width / ratio;
            }
            else if (width > height * ratio)
            {
                width = height * ratio;
            }
            glViewport((window_width - width) / 2, (window_height - height) / 2, width, height);
        } break;
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;
        default:
        {
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
        } break;
    }
    return lResult;
}

static void
handle_win32_messages(platform_state *state)
{
    MSG message;
    while(!state->stopping && PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        switch(message.message)
        {
            case WM_QUIT:
            {
                state->stopping = true;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32_t vkcode = message.wParam;
                switch(vkcode)
                {
                    case VK_ESCAPE:
                    {
                        state->stopping = true;
                    } break;
                }
            } break;
            default:
            {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
        }
    }
}

struct game_state {
    int32_t program;

    int32_t u_offset_id;
    int32_t a_pos_id;
    int32_t a_color_id;

    uint32_t vao;
    uint32_t vbo;

    float x;
    float y;
    float x_increment;
    float y_increment;
};

void platform_fail(hajonta_thread_context *ctx, char *failure_reason)
{
    platform_state *state = (platform_state *)ctx;
    state->stopping = 1;
    state->stop_reason = strdup(failure_reason);
}

GAME_UPDATE_AND_RENDER(guar)
{
    game_state *state = (game_state *)memory->memory;

    if (!memory->initialized)
    {
        state->program = glCreateProgram();
        char *vertex_shader_source =
            "#version 150 \n"
            "uniform vec2 u_offset; \n"
            "in vec4 a_pos; \n"
            "in vec4 a_color; \n"
            "out vec4 v_color; \n"
            "void main (void) \n"
            "{ \n"
            "    v_color = a_color; \n"
            "    gl_Position = a_pos + vec4(u_offset, 0.0, 0.0);\n"
            "} \n"
            ;


        char *fragment_shader_source =
            "#version 150 \n"
            "in vec4 v_color; \n"
            "out vec4 o_color; \n"
            "void main(void) \n"
            "{ \n"
            "    o_color = v_color; \n"
            "} \n"
            ;

        uint32_t vertex_shader_id;
        uint32_t fragment_shader_id;

        {
            uint32_t shader = vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
            int compiled;
            glShaderSource(shader, 1, &vertex_shader_source, 0);
            glCompileShader(shader);
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled)
            {
                char info_log[1024];
                glGetShaderInfoLog(shader, sizeof(info_log), 0, info_log);
                return platform_fail(ctx, info_log);
            }
        }
        {
            uint32_t shader = fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
            int compiled;
            glShaderSource(shader, 1, &fragment_shader_source, 0);
            glCompileShader(shader);
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled)
            {
                char info_log[1024];
                glGetShaderInfoLog(shader, sizeof(info_log), 0, info_log);
                return platform_fail(ctx, info_log);
            }
        }
        glAttachShader(state->program, vertex_shader_id);
        glAttachShader(state->program, fragment_shader_id);
        glLinkProgram(state->program);

        int linked;
        glGetProgramiv(state->program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char info_log[1024];
            glGetProgramInfoLog(state->program, sizeof(info_log), 0, info_log);
            return platform_fail(ctx, info_log);
        }

        glUseProgram(state->program);

        state->u_offset_id = glGetUniformLocation(state->program, "u_offset");
        if (state->u_offset_id < 0) {
            char info_log[] = "Could not locate u_offset uniform";
            return platform_fail(ctx, info_log);
        }
        state->a_color_id = glGetAttribLocation(state->program, "a_color");
        if (state->a_color_id < 0) {
            char info_log[] = "Could not locate a_color attribute";
            return platform_fail(ctx, info_log);
        }
        state->a_pos_id = glGetAttribLocation(state->program, "a_pos");
        if (state->a_pos_id < 0) {
            char info_log[] = "Could not locate a_pos attribute";
            return platform_fail(ctx, info_log);
        }

        glGenVertexArrays(1, &state->vao);
        glBindVertexArray(state->vao);

        glGenBuffers(1, &state->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
        vertex vertices[4] = {
            {{-0.5, 0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
            {{ 0.5, 0.5, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
            {{ 0.5,-0.5, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
            {{-0.5,-0.5, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}},
        };
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(state->a_pos_id);
        glEnableVertexAttribArray(state->a_color_id);
        glVertexAttribPointer(state->a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
        glVertexAttribPointer(state->a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

        glDepthFunc(GL_ALWAYS);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);

        state->x = 0;
        state->y = 0;
        state->x_increment = 0.02f;
        state->y_increment = 0.002f;

        memory->initialized = 1;
    }

    state->x += state->x_increment;
    state->y += state->y_increment;
    if ((state->x < -0.5) || (state->x > 0.5))
    {
        state->x_increment *= -1;
    }
    if ((state->y < -0.5) || (state->y > 0.5))
    {
        state->y_increment *= -1;
    }

    glUseProgram(state->program);
    glClearColor(0.1f, 0.1f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    float position[] = {state->x, state->y};
    glUniform2fv(state->u_offset_id, 1, (float *)&position);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    platform_state state = {};

    WNDCLASSEXA window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEXA);
    window_class.lpfnWndProc = main_window_callback;
    window_class.hInstance = hInstance;
    window_class.lpszClassName = "HajontaMainWindow";
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

    if (!RegisterClassExA(&window_class))
    {
        MessageBoxA(0, "Unable to register window class", 0, MB_OK | MB_ICONSTOP);
        return 1;
    }

    int width = 960;
    int height = 540;

    HWND window = CreateWindowExA(
        0,
        "HajontaMainWindow",
        "Hajonta",
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0,
        0,
        width,
        height,
        0,
        0,
        hInstance,
        (LPVOID)&state
    );

    if (!window)
    {
        MessageBoxA(0, "Unable to create window", 0, MB_OK | MB_ICONSTOP);
        return 1;
    }

    ShowWindow(window, nCmdShow);
    game_update_and_render *guar_func = guar;

    wglSwapIntervalEXT(1);

    platform_memory memory = {};
    memory.size = 64 * 1024 * 1024;
    memory.memory = malloc(memory.size);

    game_input input = {};
    input.delta_t = 1 / 60;

    while(!state.stopping)
    {
        handle_win32_messages(&state);

        guar((hajonta_thread_context *)&state, &memory, &input);

        SwapBuffers(state.device_context);
    }

    if (state.stop_reason)
    {
        MessageBoxA(0, state.stop_reason, 0, MB_OK | MB_ICONSTOP);
        return 1;
    }

    return 0;
}
