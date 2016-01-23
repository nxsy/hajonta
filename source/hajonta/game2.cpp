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

struct game_state
{
    bool initialized;

    uint8_t render_buffer[4 * 1024 * 1024];
    render_entry_list render_list;
    m4 matrices[2];
    asset_descriptor assets[2];
    uint32_t elements[6000 / 4 * 6];
    uint32_t textures[10];

    float clear_color[3];
    float quad_color[3];

    int32_t active_demo;

    demo_b_state b;

    uint32_t mouse_texture;
};

DEMO(demo_b)
{
    game_state *state = (game_state *)memory->memory;
    ImGui::ShowTestWindow(&state->b.show_window);

}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

    if (!memory->initialized)
    {
        memory->initialized = 1;
        state->assets[0].asset_name = "mouse_cursor";
        state->assets[1].asset_name = "sea_0";
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
            v3 q = {(float)x - 50, (float)y - 50, 0};
            v3 q_size = {1, 1, 0};
            PushQuad(&state->render_list, q, q_size, {1,1,1,1}, 1, 1);
        }
    }

    v3 mouse_bl = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y), 0.0f};
    v3 mouse_size = {16.0f, -16.0f, 0.0f};
    PushQuad(&state->render_list, mouse_bl, mouse_size, {1,1,1,1}, 0, 0);

    AddRenderList(memory, &state->render_list);
}
