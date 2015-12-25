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

    SDL_Event sdl_event;
    while (!state.stopping)
    {
        state.new_input->delta_t = 1.0f / 60.0f;

        game_sound_output sound_output;
        sound_output.samples_per_second = 48000;
        sound_output.channels = 2;
        sound_output.number_of_samples = 48000 / 60;

        state.new_input->window.width = state.window_width;
        state.new_input->window.height = state.window_height;

        while(SDL_PollEvent(&sdl_event))
        {
            switch (sdl_event.type)
            {
                case SDL_QUIT:
                {
                    state.stopping = true;
                } break;
            }
        }

        SDL_GL_SwapWindow(state.window);

        code.game_update_and_render((hajonta_thread_context *)&state, &memory, state.new_input, &sound_output);
    }

    hassert(!state.stop_reason);

    sdl_cleanup(&state);
    return 0;
}
