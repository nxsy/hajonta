#include <stddef.h>
#include <stdio.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include "hajonta/platform/common.h"

struct sdl2_state
{
    bool stopping;
    char *stop_reason;

    bool sdl_inited;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_GLContext context;

    char *base_path;
    char asset_path[MAX_PATH];

    game_input *new_input;
    game_input *old_input;

    int32_t window_width;
    int32_t window_height;
};

struct game_code
{
    game_update_and_render_func *game_update_and_render;

    void *handle;
};

static bool
_fail(sdl2_state *state, const char *failure_reason)
{
    state->stopping = true;
    state->stop_reason = strdup(failure_reason);
    return false;
}

static void
sdl_cleanup(sdl2_state *state)
{
    if (state->texture)
    {
        SDL_DestroyTexture(state->texture);
        state->texture = 0;
    }
    if (state->renderer)
    {
        SDL_DestroyRenderer(state->renderer);
        state->renderer = 0;
    }
    if (state->window)
    {
        SDL_DestroyWindow(state->window);
        state->window = 0;
    }
    if (state->sdl_inited)
    {
        SDL_Quit();
        state->sdl_inited = false;
    }
}

static bool
find_asset_path(sdl2_state *state)
{
    _snprintf(state->asset_path, sizeof(state->asset_path), "%s%s", state->base_path, "../data");
    return true;
}

static bool
sdl_init(sdl2_state *state)
{
    state->base_path = SDL_GetBasePath();
    find_asset_path(state);
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        const char *error = SDL_GetError();
        hassert(error);
        sdl_cleanup(state);
        return _fail(state, "SDL_Init failed");
    }
    state->sdl_inited = true;

    state->window = SDL_CreateWindow("My First Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (!state->window)
    {
        const char *error = SDL_GetError();
        hassert(error);
        sdl_cleanup(state);
        return _fail(state, "SDL_CreateWindow failed");
    }

    state->renderer = SDL_CreateRenderer(state->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!state->renderer)
    {
        const char *error = SDL_GetError();
        hassert(error);
        sdl_cleanup(state);
        return _fail(state, "SDL_CreateRenderer failed");
    }

    SDL_Surface *surface = SDL_CreateRGBSurface(0, 100, 100, 32, 0, 0, 0, 0);
    if (!surface)
    {
        const char *error = SDL_GetError();
        hassert(error);
        sdl_cleanup(state);
        return _fail(state, "SDL_CreateRGBSurface failed");
    }

    SDL_FillRect(surface, 0, SDL_MapRGB(surface->format, 255, 0, 0));

    state->texture = SDL_CreateTextureFromSurface(state->renderer, surface);
    SDL_FreeSurface(surface);
    if (!state->texture)
    {
        const char *error = SDL_GetError();
        hassert(error);
        sdl_cleanup(state);
        return _fail(state, "SDL_CreateTextureFromSurface failed");
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    state->context = SDL_GL_CreateContext(state->window);

    return true;
}

PLATFORM_FAIL(platform_fail)
{
    _fail((sdl2_state *)ctx, failure_reason);
}

PLATFORM_DEBUG_MESSAGE(platform_debug_message)
{
    SDL_Log("%s\n", message);
}

PLATFORM_GLGETPROCADDRESS(platform_glgetprocaddress)
{
    void *result = SDL_GL_GetProcAddress(function_name);
    return result;
}

PLATFORM_LOAD_ASSET(platform_load_asset)
{
    sdl2_state *state = (sdl2_state *)ctx;
    char full_pathname[MAX_PATH];
    _snprintf(full_pathname, sizeof(full_pathname), "%s/%s", state->asset_path, asset_path);
    SDL_RWops* sdlrw = SDL_RWFromFile(full_pathname, "rb");
    if (!sdlrw)
    {
        const char *error = SDL_GetError();
        hassert(!error);
        return false;
    }
    size_t bytes_read = SDL_RWread(sdlrw, dest, 1, size);
    SDL_RWclose(sdlrw);
    if (bytes_read != size)
    {
        const char *error = SDL_GetError();
        hassert(!error);
        return false;
    }
    return true;
}

bool sdl_load_game_code(game_code *code, char *filename)
{
    if (code->handle)
    {
        SDL_UnloadObject(code->handle);
    }
    code->handle = SDL_LoadObject(filename);
    code->game_update_and_render = (game_update_and_render_func *)SDL_LoadFunction(code->handle, "game_update_and_render");
    return true;
}

static void
sdl2_process_keypress(game_button_state *new_button_state, bool was_down, bool is_down)
{
    if (was_down == is_down)
    {
        return;
    }

    if(new_button_state->ended_down != is_down)
    {
        new_button_state->ended_down = is_down;
        new_button_state->repeat = false;
    }
}

static void
handle_sdl2_events(sdl2_state *state)
{
    SDL_Event sdl_event;
    game_controller_state *new_keyboard_controller = get_controller(state->new_input, 0);

    while(SDL_PollEvent(&sdl_event))
    {
        switch (sdl_event.type)
        {
            case SDL_QUIT:
            {
                state->stopping = true;
            } break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                bool was_down = (sdl_event.type == SDL_KEYUP);
                bool is_down = (sdl_event.type != SDL_KEYUP);

                game_button_state *button_state = 0;
                switch(sdl_event.key.keysym.sym)
                {
                    case SDLK_w:
                    {
                        button_state = &new_keyboard_controller->buttons.move_up;
                    } break;
                    case SDLK_s:
                    {
                        button_state = &new_keyboard_controller->buttons.move_down;
                    } break;
                    case SDLK_a:
                    {
                        button_state = &new_keyboard_controller->buttons.move_left;
                    } break;
                    case SDLK_d:
                    {
                        button_state = &new_keyboard_controller->buttons.move_right;
                    } break;
                    case SDLK_ESCAPE:
                    {
                        button_state = &new_keyboard_controller->buttons.back;
                    } break;
                    case SDLK_RETURN:
                    case SDLK_RETURN2:
                    {
                        button_state = &new_keyboard_controller->buttons.start;
                    } break;
                }
                if (button_state)
                {
                    sdl2_process_keypress(button_state, was_down, is_down);
                }
            } break;
        }
    }
}

int
#ifdef _WIN32
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
main(int argc, char *argv[])
#endif
{
    sdl2_state state = {};
    sdl_init(&state);

    platform_memory memory = {};
    memory.size = 256 * 1024 * 1024;
    memory.memory = malloc(memory.size);
    memory.cursor_settings.supported_modes[(uint32_t)platform_cursor_mode::normal] = true;
    memory.cursor_settings.supported_modes[(uint32_t)platform_cursor_mode::unlimited] = false;
    memory.cursor_settings.mode = platform_cursor_mode::normal;

    memory.platform_fail = platform_fail;
    memory.platform_glgetprocaddress = platform_glgetprocaddress;
    memory.platform_load_asset = platform_load_asset;
    memory.platform_debug_message = platform_debug_message;

    game_input input[2] = {};
    state.new_input = &input[0];
    state.old_input = &input[1];
    state.window_width = 640;
    state.window_height = 480;

    game_code code = {};
    sdl_load_game_code(&code, "game.dll");

    while (!state.stopping)
    {
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
            new_keyboard_controller->_buttons[button_index].repeat = true;
        }

        state.new_input->mouse.is_active = true;
        for (
                uint32_t button_index = 0;
                button_index < harray_count(state.new_input->mouse._buttons);
                ++button_index)
        {
            state.new_input->mouse._buttons[button_index].ended_down = state.old_input->mouse._buttons[button_index].ended_down;
            state.new_input->mouse._buttons[button_index].repeat = true;
        }

        state.new_input->mouse.x = state.old_input->mouse.x;
        state.new_input->mouse.y = state.old_input->mouse.y;
        state.new_input->mouse.vertical_wheel_delta = 0;

        state.new_input->window.width = state.window_width;
        state.new_input->window.height = state.window_height;

        game_sound_output sound_output;
        sound_output.samples_per_second = 48000;
        sound_output.channels = 2;
        sound_output.number_of_samples = 48000 / 60;

        handle_sdl2_events(&state);

        SDL_GL_SwapWindow(state.window);

        code.game_update_and_render((hajonta_thread_context *)&state, &memory, state.new_input, &sound_output);

        if (memory.quit)
        {
            state.stopping = 1;
        }
    }

    hassert(!state.stop_reason);

    sdl_cleanup(&state);
    return 0;
}
