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
#include "hajonta/math.cpp"

struct game_state
{
    bool initialized;

    uint8_t render_buffer[4 * 1024 * 1024];
    render_entry_list render_list;
    m4 matrices[2];
    asset_descriptor assets[1];
    ui2d_vertex_format vertices[6000];
    uint32_t elements[6000 / 4 * 6];
    uint32_t textures[10];
    ui2d_push_context pushctx;

    float clear_color[3];
    float quad_color[3];

    uint32_t mouse_texture;
};

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;

    if (!memory->initialized)
    {
        memory->initialized = 1;
        state->assets[0].asset_name = "mouse_cursor";
        RenderListBuffer(state->render_list, state->render_buffer);
    }
    if (memory->imgui_state)
    {
        ImGui::SetInternalState(memory->imgui_state);
    }
    float ratio = (float)input->window.width / (float)input->window.height;
    state->matrices[0] = m4orthographicprojection(1.0f, -1.0f, {-ratio * 10, -10.0f}, {ratio * 10, 10.0f});
    state->matrices[1] = m4orthographicprojection(1.0f, -1.0f, {0.0f, 0.0f}, {(float)input->window.width, (float)input->window.height});

    RenderListReset(state->render_list);

    ui2d_push_context *pushctx = &state->pushctx;
    state->pushctx = {};
    pushctx->vertices = state->vertices;
    pushctx->max_vertices = harray_count(state->vertices);
    pushctx->elements = state->elements;
    pushctx->max_elements = harray_count(state->elements);
    pushctx->textures = state->textures;
    pushctx->max_textures = harray_count(state->textures);

    ImGui::Text("Hello, world!");
    ImGui::ColorEdit3("Clear colour", state->clear_color);
    ImGui::ColorEdit3("Quad colour", state->quad_color);

    v4 colorv4 = {
        state->clear_color[0],
        state->clear_color[1],
        state->clear_color[2],
        1.0f,
    };

    PushClear(&state->render_list, colorv4);

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

    v4 quad_color = {
        state->quad_color[0],
        state->quad_color[1],
        state->quad_color[2],
        1.0f,
    };
    v4 quad_color_2 = v4sub({1,1,1,2}, quad_color);
    PushMatrices(&state->render_list, harray_count(state->matrices), state->matrices);
    PushAssetDescriptors(&state->render_list, harray_count(state->assets), state->assets);

    PushQuad(&state->render_list, {-0.5, -0.5, -0.5 }, {1.0f, 1.0f, 1.0f}, quad_color, -1, -1);
    PushQuad(&state->render_list, {-0.5, -0.5, -0.5 }, {1.0f, 1.0f, 1.0f}, quad_color_2, 0, -1);

    v3 mouse_bl = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y), 0.0f};
    v3 mouse_size = {16.0f, -16.0f, 0.0f};
    PushQuad(&state->render_list, mouse_bl, mouse_size, quad_color_2, 1, 0);

    AddRenderList(memory, &state->render_list);
}
