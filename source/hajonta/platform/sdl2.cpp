#include <stddef.h>
#include <stdio.h>

#include <SDL.h>
#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>
#include "hajonta/platform/common.h"

struct sdl2_state
{
    bool stopping;
};

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        hassert(!"SDL_Init failed");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("My First Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    if (!window)
    {
        const char *error = SDL_GetError();
        hassert(!"SDL_CreateWindow failed");
        hassert(!error);
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        const char *error = SDL_GetError();
        SDL_DestroyWindow(window);
        hassert(!"SDL_CreateRenderer failed");
        hassert(!error);
        SDL_Quit();
        return 1;
    }

    SDL_Surface *surface = SDL_CreateRGBSurface(0, 100, 100, 32, 0, 0, 0, 0);
    if (!surface)
    {
        const char *error = SDL_GetError();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        hassert(!"SDL_CreateRGBSurface failed");
        hassert(!error);
        SDL_Quit();
        return 1;
    }

    SDL_FillRect(surface, 0, SDL_MapRGB(surface->format, 255, 0, 0));

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture)
    {
        const char *error = SDL_GetError();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        hassert(!"SDL_CreateTextureFromSurface failed");
        hassert(!error);
        SDL_Quit();
        return 1;
    }
    sdl2_state state = {};
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
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, 0, 0);
        SDL_RenderPresent(renderer);
        SDL_Delay(1000);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
