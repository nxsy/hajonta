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
#include "hajonta/demos/model.h"
#include "hajonta/demos/firework.h"

struct demo_data {
    const char *name;
    demo_func *func;
};

struct demos_state
{
    uint32_t active_demo;
    demo_data *registry;
    uint32_t number_of_demos;

    demo_menu_state menu;
    demo_rainbow_state rainbow;
    demo_normals_state normals;
    demo_collision_state collision;
    demo_bounce_state bounce;
    demo_rotate_state rotate;
    demo_model_state model;
    demo_firework_state firework;
};
