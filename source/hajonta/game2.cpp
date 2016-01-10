#include "hajonta/platform/common.h"

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4456 4457 4774 4577)
#endif
#include "imgui.cpp"
#include "imgui_draw.cpp"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

struct game_state
{
    bool initialized;

    uint8_t render_buffer[4 * 1024 * 1024];
    render_entry_list render_list;
};

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

    if (!memory->initialized)
    {
        memory->initialized = 1;
        state->render_list.max_size = sizeof(state->render_buffer);
        state->render_list.base = state->render_buffer;
    }
    if (memory->imgui_state)
    {
        ImGui::SetInternalState(memory->imgui_state);
    }

    state->render_list.current_size = 0;
    state->render_list.element_count = 0;

    v4 color = {0, 1.0f, 0, 0};
    PushClear(&state->render_list, color);

    ImGui::Text("Hello, world!");
    AddRenderList(memory, &state->render_list);
}
