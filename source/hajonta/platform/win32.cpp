#include <stddef.h>
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

    char binary_path[MAX_PATH];
};

struct game_code
{
    game_update_and_render_func *game_update_and_render;

    HMODULE game_code_module;
    FILETIME last_updated;
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

PLATFORM_FAIL(platform_fail)
{
    platform_state *state = (platform_state *)ctx;
    state->stopping = 1;
    state->stop_reason = strdup(failure_reason);
}

PLATFORM_GLGETPROCADDRESS(platform_glgetprocaddress)
{
    return wglGetProcAddress(function_name);
}

bool win32_load_game_code(game_code *code, char *filename)
{
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
    {
        return false;
    }
    FILETIME last_updated = data.ftLastWriteTime;
    if (CompareFileTime(&last_updated, &code->last_updated) != 1)
    {
        return true;
    }

    char locksuffix[] = ".lck";
    char lockfilename[MAX_PATH];

    strcpy(lockfilename, filename);
    strcat(lockfilename, locksuffix);

    if (GetFileAttributesEx(lockfilename, GetFileExInfoStandard, &data))
    {
        return false;
    }

    char tempsuffix[] = ".tmp";
    char tempfilename[MAX_PATH];
    strcpy(tempfilename, filename);
    strcat(tempfilename, tempsuffix);

    if (code->game_code_module)
    {
        FreeLibrary(code->game_code_module);
    }
    CopyFile(filename, tempfilename, 0);

    HMODULE new_module = LoadLibraryA(tempfilename);
    if (!new_module)
    {
        return false;
    }

    game_update_and_render_func *game_update_and_render = (game_update_and_render_func *)GetProcAddress(new_module, "game_update_and_render");
    if (!game_update_and_render)
    {
        return false;
    }

    code->game_code_module = new_module;
    code->last_updated = last_updated;
    code->game_update_and_render = game_update_and_render;
    return true;
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
    game_code code = {};

    wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    wglSwapIntervalEXT(1);

    platform_memory memory = {};
    memory.size = 64 * 1024 * 1024;
    memory.memory = malloc(memory.size);
    memory.platform_fail = platform_fail;
    memory.platform_glgetprocaddress = platform_glgetprocaddress;

    game_input input = {};
    input.delta_t = 1 / 60;

    GetModuleFileNameA(0, state.binary_path, sizeof(state.binary_path));
    char game_library_path[sizeof(state.binary_path)];
    char *location_of_last_slash = strrchr(state.binary_path, '\\');
    strncpy(game_library_path, state.binary_path, location_of_last_slash - state.binary_path);
    strcat(game_library_path, "\\game.dll");

    while(!state.stopping)
    {
        bool updated = win32_load_game_code(&code, game_library_path);
        if (!code.game_code_module)
        {
            state.stopping = 1;
            state.stop_reason = "Unable to load game code";
        }
        if (!updated)
        {
            state.stopping = 1;
            state.stop_reason = "Unable to reload game code";
        }
        handle_win32_messages(&state);

        code.game_update_and_render((hajonta_thread_context *)&state, &memory, &input);

        SwapBuffers(state.device_context);
    }

    if (state.stop_reason)
    {
        MessageBoxA(0, state.stop_reason, 0, MB_OK | MB_ICONSTOP);
        return 1;
    }

    return 0;
}
