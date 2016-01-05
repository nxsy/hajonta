#include "hajonta/platform/common.h"

struct game_state
{
    bool initialized;
};

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

    if (!memory->initialized)
    {
        memory->initialized = 1;
    }
}
