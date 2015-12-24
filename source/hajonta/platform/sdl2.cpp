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

    char *base_path;
};

bool
_fail(sdl2_state *state, char *failure_reason)
{
    state->stopping = true;
    state->stop_reason = strdup(failure_reason);
    return false;
}

void
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

bool
sdl_init(sdl2_state *state)
{
    state->base_path = SDL_GetBasePath();
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        const char *error = SDL_GetError();
        hassert(error);
        sdl_cleanup(state);
        return _fail(state, "SDL_Init failed");
    }
    state->sdl_inited = true;

    state->window = SDL_CreateWindow("My First Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
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

    SDL_Event sdl_event;

    while (!state.stopping)
    {
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
        SDL_RenderClear(state.renderer);
        SDL_RenderCopy(state.renderer, state.texture, 0, 0);
        SDL_RenderPresent(state.renderer);
    }

    sdl_cleanup(&state);
    return 0;
}
