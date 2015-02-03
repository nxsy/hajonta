#include <stddef.h>
#include <stdio.h>

#include <windows.h>
#include <gl/gl.h>
#include <xaudio2.h>

#include "hajonta/platform/common.h"
#include "hajonta/platform/win32.h"

struct platform_state
{
    int32_t stopping;
    char *stop_reason;

    HDC device_context;

    char binary_path[MAX_PATH];
    char asset_path[MAX_PATH];

    game_input *new_input;
    game_input *old_input;
};

static bool
find_asset_path(platform_state *state)
{
    char dir[sizeof(state->binary_path)];
    strcpy(dir, state->binary_path);

    while(char *location_of_last_slash = strrchr(dir, '\\')) {
        *location_of_last_slash = 0;

        strcpy(state->asset_path, dir);
        strcat(state->asset_path, "\\data");
        WIN32_FILE_ATTRIBUTE_DATA data;
        if (!GetFileAttributesEx(state->asset_path, GetFileExInfoStandard, &data))
        {
            continue;
        }
        return true;
    }
    return false;
}

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
                height = (int32_t)(width / ratio);
            }
            else if (width > height * ratio)
            {
                width = (int32_t)(height * ratio);
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
win32_process_keypress(game_button_state *new_button_state, bool was_down, bool is_down)
{
    if (was_down == is_down)
    {
        return;
    }

    if(new_button_state->ended_down != is_down)
    {
        new_button_state->ended_down = is_down;
    }
}

static void
handle_win32_messages(platform_state *state)
{
    game_controller_state *new_keyboard_controller = get_controller(state->new_input, 0);

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
                WPARAM vkcode = message.wParam;
                /*
                    30 The previous key state. The value is 1 if the key is
                       down before the message is sent, or it is zero if the
                       key is up.
                    31 The transition state. The value is always 0 for a WM_KEYDOWN message.
                */
                bool was_down = ((message.lParam & (1 << 30)) != 0);
                bool is_down = ((message.lParam & (1 << 31)) == 0);
                switch(vkcode)
                {
                    case VK_ESCAPE:
                    {
                        state->stopping = true;
                    } break;
                    case 'W':
                    {
                        win32_process_keypress(&new_keyboard_controller->buttons.move_up, was_down, is_down);
                    } break;
                    case 'A':
                    {
                        win32_process_keypress(&new_keyboard_controller->buttons.move_left, was_down, is_down);
                    } break;
                    case 'S':
                    {
                        win32_process_keypress(&new_keyboard_controller->buttons.move_down, was_down, is_down);
                    } break;
                    case 'D':
                    {
                        win32_process_keypress(&new_keyboard_controller->buttons.move_right, was_down, is_down);
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

PLATFORM_DEBUG_MESSAGE(win32_debug_message)
{
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}

PLATFORM_GLGETPROCADDRESS(platform_glgetprocaddress)
{
    return wglGetProcAddress(function_name);
}

PLATFORM_LOAD_ASSET(win32_load_asset)
{
    platform_state *state = (platform_state *)ctx;
    char full_pathname[MAX_PATH];
    _snprintf(full_pathname, sizeof(full_pathname), "%s\\%s", state->asset_path, asset_path);

    HANDLE handle = CreateFile(full_pathname, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE)
    {
        MessageBoxA(0, "Failed", 0, MB_OK | MB_ICONSTOP);
        return false;
    }
    LARGE_INTEGER _size;
    GetFileSizeEx(handle, &_size);
    if (_size.QuadPart != size)
    {
        char msg[1024];
        sprintf(msg, "File size mismatch: Got %d, expected %d", _size.QuadPart, size);
        MessageBoxA(0, msg, 0, MB_OK | MB_ICONSTOP);
        return false;
    }
    DWORD bytes_read;
    if (!ReadFile(handle, dest, size, &bytes_read, 0))
    {
        return false;
    }
    if (bytes_read != size)
    {
        char msg[1024];
        sprintf(msg, "File read mismatch: Got %d, expected %d", bytes_read, size);
        MessageBoxA(0, msg, 0, MB_OK | MB_ICONSTOP);
        return false;
    }
    return true;
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

    DWORD style;
    style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    int width = 960;
    int height = 540;
    RECT window_rect = {};
    window_rect.left = 0;
    window_rect.top = 0;
    window_rect.right = window_rect.left + width;
    window_rect.bottom = window_rect.top + height;
    BOOL result = AdjustWindowRect(&window_rect, style, 0);
    if (!result)
    {
        MessageBoxA(0, "Unable to determine window size", 0, MB_OK | MB_ICONSTOP);
        return 1;
    }

    HWND window = CreateWindowExA(
        0,
        "HajontaMainWindow",
        "Hajonta",
        style,
        window_rect.left,
        window_rect.right,
        window_rect.right - window_rect.left,
        window_rect.bottom - window_rect.top,
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
    memory.platform_load_asset = win32_load_asset;
    memory.platform_debug_message = win32_debug_message;

    GetModuleFileNameA(0, state.binary_path, sizeof(state.binary_path));
    char game_library_path[sizeof(state.binary_path)];
    char *location_of_last_slash = strrchr(state.binary_path, '\\');
    strncpy(game_library_path, state.binary_path, location_of_last_slash - state.binary_path);
    strcat(game_library_path, "\\game.dll");
    if (!find_asset_path(&state))
    {
        MessageBoxA(0, "Failed to locate asset path", 0, MB_OK | MB_ICONSTOP);
        return 1;
    }

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    IXAudio2 *xaudio = {};
    if(FAILED(XAudio2Create(&xaudio, 0))) {
        MessageBoxA(0, "Unable to create XAudio2 instance", 0, MB_OK | MB_ICONSTOP);
        return 1;
    }

    IXAudio2MasteringVoice *master_voice = {};
    if(FAILED(xaudio->CreateMasteringVoice(&master_voice, 2, 48000))) {
        MessageBoxA(0, "Unable to create XAudio2 master voice", 0, MB_OK | MB_ICONSTOP);
        return 1;
    }
    WAVEFORMATEX wave_format = {};
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = 2;
    wave_format.nSamplesPerSec = 48000;
    wave_format.nAvgBytesPerSec = 48000 * 2 * (16 / 8);
    wave_format.nBlockAlign = 2 * (16 / 8);
    wave_format.wBitsPerSample = 16;

    IXAudio2SourceVoice *source_voice = {};
    xaudio->CreateSourceVoice(&source_voice, &wave_format);

    int audio_offset = 0;

    source_voice->Start(0);

    XAUDIO2_BUFFER audio_buffer = {};
    audio_buffer.Flags |= XAUDIO2_END_OF_STREAM;
    uint8_t intro[48000 * 2 * (16 / 8) / 250] = {};
    audio_buffer.AudioBytes = sizeof(intro);
    audio_buffer.pAudioData = (uint8_t *)intro;
    source_voice->SubmitSourceBuffer(&audio_buffer);

    XAUDIO2_VOICE_STATE voice_state;

    struct foo {
        uint32_t BuffersQueued;
        uint64_t SamplesPlayed;
    };

    foo audio_history[60] = {};

    game_input input[2] = {};
    state.new_input = &input[0];
    state.old_input = &input[1];

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

        state.new_input->delta_t = 1.0f / 60.0f;

        game_controller_state *old_keyboard_controller = get_controller(state.old_input, 0);
        game_controller_state *new_keyboard_controller = get_controller(state.new_input, 0);
        *new_keyboard_controller = {};
        new_keyboard_controller->is_active = true;
        for (
                uint32_t button_index = 0;
                button_index < harray_count(new_keyboard_controller->_buttons);
                ++button_index)
        {
            new_keyboard_controller->_buttons[button_index].ended_down =
                old_keyboard_controller->_buttons[button_index].ended_down;
        }

        handle_win32_messages(&state);

        game_sound_output sound_output;
        sound_output.samples_per_second = 48000;
        sound_output.channels = 2;
        sound_output.number_of_samples = 48000 / 60;
        code.game_update_and_render((hajonta_thread_context *)&state, &memory, state.new_input, &sound_output);

        source_voice->GetState(&voice_state);
        audio_buffer.AudioBytes = 48000 * 2 * (16 / 8) / 60;
        audio_buffer.pAudioData = (uint8_t *)sound_output.samples;

        audio_history[audio_offset].BuffersQueued = voice_state.BuffersQueued;
        audio_history[audio_offset].SamplesPlayed = voice_state.SamplesPlayed;

        audio_offset = (audio_offset + 1) % 60;
        if (audio_offset == 0 && 0)
        {
            char longmsg[16384];
#define BQ(x, y, z) audio_history[(x*y)+z].BuffersQueued
            sprintf(longmsg, "BuffersQueued: "
"%4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d "
"%4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d "
"%4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d\n",
            BQ(0, 6, 0), BQ(0, 6, 1), BQ(0, 6, 2), BQ(0, 6, 3), BQ(0, 6, 4), BQ(0, 6, 5),
            BQ(1, 6, 0), BQ(1, 6, 1), BQ(1, 6, 2), BQ(1, 6, 3), BQ(1, 6, 4), BQ(1, 6, 5),
            BQ(2, 6, 0), BQ(2, 6, 1), BQ(2, 6, 2), BQ(2, 6, 3), BQ(2, 6, 4), BQ(2, 6, 5),
            BQ(3, 6, 0), BQ(3, 6, 1), BQ(3, 6, 2), BQ(3, 6, 3), BQ(3, 6, 4), BQ(3, 6, 5),
            BQ(4, 6, 0), BQ(4, 6, 1), BQ(4, 6, 2), BQ(4, 6, 3), BQ(4, 6, 4), BQ(4, 6, 5),
            BQ(5, 6, 0), BQ(5, 6, 1), BQ(5, 6, 2), BQ(5, 6, 3), BQ(5, 6, 4), BQ(5, 6, 5),
            BQ(6, 6, 0), BQ(6, 6, 1), BQ(6, 6, 2), BQ(6, 6, 3), BQ(6, 6, 4), BQ(6, 6, 5),
            BQ(7, 6, 0), BQ(7, 6, 1), BQ(7, 6, 2), BQ(7, 6, 3), BQ(7, 6, 4), BQ(7, 6, 5),
            BQ(8, 6, 0), BQ(8, 6, 1), BQ(8, 6, 2), BQ(8, 6, 3), BQ(8, 6, 4), BQ(8, 6, 5),
            BQ(9, 6, 0), BQ(9, 6, 1), BQ(9, 6, 2), BQ(9, 6, 3), BQ(9, 6, 4), BQ(9, 6, 5),
            BQ(10, 6, 0), BQ(10, 6, 1), BQ(10, 6, 2), BQ(10, 6, 3), BQ(10, 6, 4), BQ(10, 6, 5),
            BQ(11, 6, 0), BQ(11, 6, 1), BQ(11, 6, 2), BQ(11, 6, 3), BQ(11, 6, 4), BQ(11, 6, 5),
            BQ(12, 6, 0), BQ(12, 6, 1), BQ(12, 6, 2), BQ(12, 6, 3), BQ(12, 6, 4), BQ(12, 6, 5),
            BQ(13, 6, 0), BQ(13, 6, 1), BQ(13, 6, 2), BQ(13, 6, 3), BQ(13, 6, 4), BQ(13, 6, 5),
            BQ(14, 6, 0), BQ(14, 6, 1), BQ(14, 6, 2), BQ(14, 6, 3), BQ(14, 6, 4), BQ(14, 6, 5));
            OutputDebugStringA(longmsg);
            audio_offset = 0;
#define SP(x, y, z) audio_history[(x*y)+z].SamplesPlayed
            sprintf(longmsg, "SamplesPlayed: "
"%4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d "
"%4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d "
"%4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d\n",
            SP(0, 6, 0), SP(0, 6, 1), SP(0, 6, 2), SP(0, 6, 3), SP(0, 6, 4), SP(0, 6, 5),
            SP(1, 6, 0), SP(1, 6, 1), SP(1, 6, 2), SP(1, 6, 3), SP(1, 6, 4), SP(1, 6, 5),
            SP(2, 6, 0), SP(2, 6, 1), SP(2, 6, 2), SP(2, 6, 3), SP(2, 6, 4), SP(2, 6, 5),
            SP(3, 6, 0), SP(3, 6, 1), SP(3, 6, 2), SP(3, 6, 3), SP(3, 6, 4), SP(3, 6, 5),
            SP(4, 6, 0), SP(4, 6, 1), SP(4, 6, 2), SP(4, 6, 3), SP(4, 6, 4), SP(4, 6, 5),
            SP(5, 6, 0), SP(5, 6, 1), SP(5, 6, 2), SP(5, 6, 3), SP(5, 6, 4), SP(5, 6, 5),
            SP(6, 6, 0), SP(6, 6, 1), SP(6, 6, 2), SP(6, 6, 3), SP(6, 6, 4), SP(6, 6, 5),
            SP(7, 6, 0), SP(7, 6, 1), SP(7, 6, 2), SP(7, 6, 3), SP(7, 6, 4), SP(7, 6, 5),
            SP(8, 6, 0), SP(8, 6, 1), SP(8, 6, 2), SP(8, 6, 3), SP(8, 6, 4), SP(8, 6, 5),
            SP(9, 6, 0), SP(9, 6, 1), SP(9, 6, 2), SP(9, 6, 3), SP(9, 6, 4), SP(9, 6, 5),
            SP(10, 6, 0), SP(10, 6, 1), SP(10, 6, 2), SP(10, 6, 3), SP(10, 6, 4), SP(10, 6, 5),
            SP(11, 6, 0), SP(11, 6, 1), SP(11, 6, 2), SP(11, 6, 3), SP(11, 6, 4), SP(11, 6, 5),
            SP(12, 6, 0), SP(12, 6, 1), SP(12, 6, 2), SP(12, 6, 3), SP(12, 6, 4), SP(12, 6, 5),
            SP(13, 6, 0), SP(13, 6, 1), SP(13, 6, 2), SP(13, 6, 3), SP(13, 6, 4), SP(13, 6, 5),
            SP(14, 6, 0), SP(14, 6, 1), SP(14, 6, 2), SP(14, 6, 3), SP(14, 6, 4), SP(14, 6, 5));
            OutputDebugStringA(longmsg);
            audio_offset = 0;
        }

        source_voice->SubmitSourceBuffer(&audio_buffer);


        SwapBuffers(state.device_context);

        game_input *temp_input = state.new_input;
        state.new_input = state.old_input;
        state.old_input = temp_input;
    }

    if (state.stop_reason)
    {
        MessageBoxA(0, state.stop_reason, 0, MB_OK | MB_ICONSTOP);
        return 1;
    }

    return 0;
}
