struct demo_context
{
    bool switched;
};

#define DEMO(func_name) void func_name(hajonta_thread_context *ctx, platform_memory *memory, game_input *input, game_sound_output *sound_output, demo_context *context)
typedef DEMO(demo_func);

#include "hajonta/demos/rainbow.h"
#include "hajonta/demos/bounce.h"
#include "hajonta/demos/normals.h"
#include "hajonta/demos/collision.h"
#include "hajonta/demos/menu.h"
#include "hajonta/demos/rotate.h"

struct demo_data {
    char *name;
    demo_func *func;
};
