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

#include "hajonta/ui/ui2d.cpp"

struct game_state
{
    bool initialized;

    uint8_t render_buffer[4 * 1024 * 1024];
    render_entry_list render_list;
    ui2d_vertex_format vertices[6000];
    uint32_t elements[6000 / 4 * 6];
    uint32_t textures[10];
    ui2d_push_context pushctx;

    uint32_t mouse_texture;
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


    ui2d_push_context *pushctx = &state->pushctx;
    state->pushctx = {};
    pushctx->vertices = state->vertices;
    pushctx->max_vertices = harray_count(state->vertices);
    pushctx->elements = state->elements;
    pushctx->max_elements = harray_count(state->elements);
    pushctx->textures = state->textures;
    pushctx->max_textures = harray_count(state->textures);

    v4 color = {0, 1.0f, 0, 0};
    PushClear(&state->render_list, color);

    ImGui::Text("Hello, world!");

    stbtt_aligned_quad q = {};
    q.x0 = (float)input->mouse.x;
    q.x1 = (float)input->mouse.x + 16;

    q.y0 = (float)(input->window.height - input->mouse.y);
    q.y1 = (float)(input->window.height - input->mouse.y) - 16;
    q.s0 = 0;
    q.s1 = 1;
    q.t0 = 0;
    q.t1 = 1;
    push_quad(pushctx, q, state->mouse_texture, 0);
    PushUi2d(&state->render_list, &state->pushctx);

    AddRenderList(memory, &state->render_list);
}
