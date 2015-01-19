#include <stdint.h>
#include <stdio.h>

#include <windows.h>

struct platform_state
{
    int32_t stopping;
};

LRESULT CALLBACK
main_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    platform_state *state;
    if (uMsg == WM_NCCREATE)
    {
        state = (platform_state *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
        OutputDebugStringA("WM_NCCREATE called\n");
    }
    else
    {
        state = (platform_state *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (!state)
        {
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
        case WM_CLOSE:
        {
            state->stopping = 1;
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
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                state->stopping = true;
            } break;
            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }
}

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    platform_state state = {};

    WNDCLASSEXA window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEXA);
    window_class.lpfnWndProc = main_window_callback;
    window_class.hInstance = hInstance;
    window_class.lpszClassName = "NajontaMainWindow";

    if (!RegisterClassExA(&window_class))
    {
        MessageBoxA(0, "Unable to register window class", 0, MB_OK | MB_ICONSTOP);
        return 1;
    }
    HWND window = CreateWindowExA(
        0,
        "NajontaMainWindow",
        "Najonta",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        960,
        540,
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

    while(!state.stopping)
    {
        handle_win32_messages(&state);
    }

    return 0;
}
