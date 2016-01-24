#include "hajonta/platform/common.h"
#include "hajonta/renderer/common.h"

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4456 4457 4774 4577)
#endif
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_demo.cpp"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "hajonta/math.cpp"

struct demo_context
{
    bool switched;
};

#define DEMO(func_name) void func_name(hajonta_thread_context *ctx, platform_memory *memory, game_input *input, game_sound_output *sound_output, demo_context *context)
typedef DEMO(demo_func);

struct demo_data {
    const char *name;
    demo_func *func;
};

struct demo_b_state
{
    bool show_window;
};

struct
_asset_ids
{
    int32_t mouse_cursor;
    int32_t sea_0;
    int32_t ground_0;
    int32_t sea_ground_br;
    int32_t sea_ground_bl;
    int32_t sea_ground_tr;
    int32_t sea_ground_tl;
    int32_t sea_ground_r;
    int32_t sea_ground_l;
    int32_t sea_ground_t;
    int32_t sea_ground_b;
};

struct game_state
{
    bool initialized;

    uint8_t render_buffer[4 * 1024 * 1024];
    render_entry_list render_list;
    m4 matrices[2];

    _asset_ids asset_ids;
    uint32_t asset_count;
    asset_descriptor assets[16];

    uint32_t elements[6000 / 4 * 6];

    float clear_color[3];
    float quad_color[3];

    int32_t active_demo;

    demo_b_state b;

    uint32_t mouse_texture;

#define MAP_HEIGHT 32
#define MAP_WIDTH 32
    int32_t tiles[MAP_HEIGHT * MAP_WIDTH];
};

DEMO(demo_b)
{
    game_state *state = (game_state *)memory->memory;
    ImGui::ShowTestWindow(&state->b.show_window);

}

void
set_tile(game_state *state, uint32_t x, uint32_t y, int32_t value)
{
    state->tiles[y * MAP_WIDTH + x] = value;
}

void
build_lake(game_state *state, v2 bl, v2 tr)
{
    for (uint32_t y = (uint32_t)bl.y; y <= (uint32_t)tr.y; ++y)
    {
        for (uint32_t x = (uint32_t)bl.x; x <= (uint32_t)tr.x; ++x)
        {
            set_tile(state, x, y, state->asset_ids.sea_0);
        }
    }
}

void
build_map(game_state *state)
{
    for (uint32_t y = 0; y < MAP_HEIGHT; ++y)
    {
        for (uint32_t x = 0; x < MAP_WIDTH; ++x)
        {
            int32_t result = state->asset_ids.ground_0;
            switch(x)
            {
                case 0:
                {
                    if (y == 0)
                    {
                        result = state->asset_ids.sea_ground_tr;
                    }
                    else if (y == (MAP_HEIGHT - 1))
                    {
                        result = state->asset_ids.sea_ground_br;
                    }
                    else
                    {
                        result = state->asset_ids.sea_ground_r;
                    }
                } break;
                case (MAP_WIDTH - 1):
                {
                    if (y == 0)
                    {
                        result = state->asset_ids.sea_ground_tl;
                    }
                    else if (y == (MAP_HEIGHT - 1))
                    {
                        result = state->asset_ids.sea_ground_bl;
                    }
                    else
                    {
                        result = state->asset_ids.sea_ground_l;
                    }
                } break;
                default:
                {
                    if (y == 0)
                    {
                        result = state->asset_ids.sea_ground_t;
                    }
                    else if (y == (MAP_HEIGHT - 1))
                    {
                        result = state->asset_ids.sea_ground_b;
                    }
                    else
                    {
                        result = state->asset_ids.ground_0;
                    }
                } break;
            }
            state->tiles[y * MAP_WIDTH + x] = result;
        }
    }
    build_lake(state, {16, 16}, {20, 20});
}

int32_t
add_asset(game_state *state, char *name)
{
    int32_t result = -1;
    if (state->asset_count < harray_count(state->assets))
    {
        state->assets[state->asset_count].asset_name = name;
        result = (int32_t)state->asset_count;
        ++state->asset_count;
    }
    return result;
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

    if (!memory->initialized)
    {
        memory->initialized = 1;
        //state->assets[0].asset_name = "mouse_cursor";
        state->asset_ids.mouse_cursor = add_asset(state, "mouse_cursor");
        state->asset_ids.sea_0 = add_asset(state, "sea_0");
        state->asset_ids.ground_0 = add_asset(state, "ground_0");
        state->asset_ids.sea_ground_br = add_asset(state, "sea_ground_br");
        state->asset_ids.sea_ground_bl = add_asset(state, "sea_ground_bl");
        state->asset_ids.sea_ground_tr = add_asset(state, "sea_ground_tr");
        state->asset_ids.sea_ground_tl = add_asset(state, "sea_ground_tl");
        state->asset_ids.sea_ground_r = add_asset(state, "sea_ground_r");
        state->asset_ids.sea_ground_l = add_asset(state, "sea_ground_l");
        state->asset_ids.sea_ground_t = add_asset(state, "sea_ground_t");
        state->asset_ids.sea_ground_b = add_asset(state, "sea_ground_b");
        hassert(state->asset_ids.sea_ground_b > 0);
        build_map(state);
        RenderListBuffer(state->render_list, state->render_buffer);
    }
    if (memory->imgui_state)
    {
        ImGui::SetInternalState(memory->imgui_state);
    }
    float max_x = (float)input->window.width / 16.0f / 2.0f;
    float max_y = (float)input->window.height / 16.0f / 2.0f;
    state->matrices[0] = m4orthographicprojection(1.0f, -1.0f, {0.0f, 0.0f}, {(float)input->window.width, (float)input->window.height});
    state->matrices[1] = m4orthographicprojection(1.0f, -1.0f, {-max_x, -max_y}, {max_x, max_y});

    RenderListReset(state->render_list);

    ImGui::ColorEdit3("Clear colour", state->clear_color);
    demo_data demoes[] = {
        {
            "none",
            0,
        },
        {
            "b",
            &demo_b,
        },
    };
    int32_t previous_demo = state->active_demo;
    for (int32_t i = 0; i < harray_count(demoes); ++i)
    {
        ImGui::RadioButton(demoes[i].name, &state->active_demo, i);
    }

    if (state->active_demo)
    {
        demo_context demo_ctx = {};
        demo_ctx.switched = previous_demo != state->active_demo;
         demoes[state->active_demo].func(ctx, memory, input, sound_output, &demo_ctx);
    }

    v4 colorv4 = {
        state->clear_color[0],
        state->clear_color[1],
        state->clear_color[2],
        1.0f,
    };

    PushClear(&state->render_list, colorv4);

    PushMatrices(&state->render_list, harray_count(state->matrices), state->matrices);
    PushAssetDescriptors(&state->render_list, harray_count(state->assets), state->assets);

    for (uint32_t y = 0; y < 100; ++y)
    {
        for (uint32_t x = 0; x < 100; ++x)
        {
            int32_t texture_id = 1;
            uint32_t x_start = 50 - (MAP_WIDTH / 2);
            uint32_t x_end = 50 + (MAP_WIDTH / 2);
            uint32_t y_start = 50 - (MAP_HEIGHT / 2);
            uint32_t y_end = 50 + (MAP_HEIGHT / 2);
            if ((x >= x_start && x < x_end) && (y >= y_start && y < y_end))
            {
                texture_id = state->tiles[(y - y_start) * MAP_WIDTH + x - x_start];
            }
            v3 q = {(float)x - 50, (float)y - 50, 0};
            v3 q_size = {1, 1, 0};
            PushQuad(&state->render_list, q, q_size, {1,1,1,1}, 1, texture_id);
        }
    }

    v3 mouse_bl = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y), 0.0f};
    v3 mouse_size = {16.0f, -16.0f, 0.0f};
    PushQuad(&state->render_list, mouse_bl, mouse_size, {1,1,1,1}, 0, 0);

    AddRenderList(memory, &state->render_list);
}
