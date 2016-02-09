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
    int32_t sea_ground_t_l;
    int32_t sea_ground_t_r;
    int32_t sea_ground_b_l;
    int32_t sea_ground_b_r;
    int32_t player;
};

enum struct
terrain
{
    water,
    land,
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

    int32_t pixel_size;
#define MAP_HEIGHT 32
#define MAP_WIDTH 32
    terrain terrain_tiles[MAP_HEIGHT * MAP_WIDTH];
    int32_t texture_tiles[MAP_HEIGHT * MAP_WIDTH];
    bool passable_x[(MAP_HEIGHT + 1) * (MAP_WIDTH + 1)];
    bool passable_y[(MAP_HEIGHT + 1) * (MAP_WIDTH + 1)];

    v2 camera_offset;

    v2 player_position;
    v2 player_velocity;
    float acceleration_multiplier;
};

v2
integrate_acceleration(v2 *velocity, v2 acceleration, float delta_t)
{
    acceleration = v2sub(acceleration, v2mul(*velocity, 5.0f));
    v2 movement = v2add(
        v2mul(acceleration, 0.5f * (delta_t * delta_t)),
        v2mul(*velocity, delta_t)
    );
    *velocity = v2add(
        v2mul(acceleration, delta_t),
        *velocity
    );
    return movement;
}

DEMO(demo_b)
{
    game_state *state = (game_state *)memory->memory;
    ImGui::ShowTestWindow(&state->b.show_window);

}

void
set_tile(game_state *state, uint32_t x, uint32_t y, terrain value)
{
    state->terrain_tiles[y * MAP_WIDTH + x] = value;
}

terrain
get_tile(game_state *state, uint32_t x, uint32_t y)
{
    return state->terrain_tiles[y * MAP_WIDTH + x];
}

void
set_passable_x(game_state *state, uint32_t x, uint32_t y, bool value)
{
    state->passable_x[y * (MAP_WIDTH + 1) + x] = value;
}

void
set_passable_y(game_state *state, uint32_t x, uint32_t y, bool value)
{
    state->passable_y[y * (MAP_WIDTH + 1) + x] = value;
}

bool
get_passable_x(game_state *state, uint32_t x, uint32_t y)
{
    return state->passable_x[y * (MAP_WIDTH + 1) + x];
}

bool
get_passable_y(game_state *state, uint32_t x, uint32_t y)
{
    return state->passable_y[y * (MAP_WIDTH + 1) + x];
}

void
set_terrain_texture(game_state *state, uint32_t x, uint32_t y, int32_t value)
{
    state->texture_tiles[y * MAP_WIDTH + x] = value;
}

int32_t
get_terrain_texture(game_state *state, uint32_t x, uint32_t y)
{
    return state->texture_tiles[y * MAP_WIDTH + x];
}

void
build_lake(game_state *state, v2 bl, v2 tr)
{
    for (uint32_t y = (uint32_t)bl.y; y <= (uint32_t)tr.y; ++y)
    {
        for (uint32_t x = (uint32_t)bl.x; x <= (uint32_t)tr.x; ++x)
        {
            set_tile(state, x, y, terrain::water);
        }
    }
}

void
build_terrain_tiles(game_state *state)
{
    for (uint32_t y = 0; y < MAP_HEIGHT; ++y)
    {
        for (uint32_t x = 0; x < MAP_WIDTH; ++x)
        {
            terrain result = terrain::land;
            if ((x == 0) || (x == MAP_WIDTH - 1) || (y == 0) || (y == MAP_HEIGHT - 1))
            {
                result = terrain::water;
            }
            state->terrain_tiles[y * MAP_WIDTH + x] = result;
        }
    }
    build_lake(state, {16, 16}, {20, 22});
    build_lake(state, {12, 13}, {17, 20});
    build_lake(state, {11, 13}, {12, 17});
    build_lake(state, {14, 11}, {16, 12});
    build_lake(state, {13, 12}, {13, 12});
}

terrain
get_terrain_for_tile(game_state *state, int32_t x, int32_t y)
{
    terrain result = terrain::water;
    if ((x >= 0) && (x < MAP_WIDTH) && (y >= 0) && (y < MAP_HEIGHT))
    {
        result = state->terrain_tiles[y * MAP_WIDTH + x];
    }
    return result;
}

int32_t
resolve_terrain_tile_to_texture(game_state *state, int32_t x, int32_t y)
{
    terrain tl = get_terrain_for_tile(state, x - 1, y + 1);
    terrain t = get_terrain_for_tile(state, x, y + 1);
    terrain tr = get_terrain_for_tile(state, x + 1, y + 1);

    terrain l = get_terrain_for_tile(state, x - 1, y);
    terrain me = get_terrain_for_tile(state, x, y);
    terrain r = get_terrain_for_tile(state, x + 1, y);

    terrain bl = get_terrain_for_tile(state, x - 1, y - 1);
    terrain b = get_terrain_for_tile(state, x, y - 1);
    terrain br = get_terrain_for_tile(state, x + 1, y - 1);

    int32_t result = state->asset_ids.mouse_cursor;

    if (me == terrain::land)
    {
        result = state->asset_ids.ground_0;
    }
    else if ((me == t) && (t == b) && (b == l) && (l == r))
    {
        result = state->asset_ids.sea_0;
        if (br == terrain::land)
        {
            result = state->asset_ids.sea_ground_br;
        }
        else if (bl == terrain::land)
        {
            result = state->asset_ids.sea_ground_bl;
        }
        else if (tr == terrain::land)
        {
            result = state->asset_ids.sea_ground_tr;
        }
        else if (tl == terrain::land)
        {
            result = state->asset_ids.sea_ground_tl;
        }
    }
    else if ((me == t) && (me == l) && (me == r))
    {
        result = state->asset_ids.sea_ground_b;
    }
    else if ((me == b) && (me == l) && (me == r))
    {
        result = state->asset_ids.sea_ground_t;
    }
    else if ((me == l) && (me == b) && (me == t))
    {
        result = state->asset_ids.sea_ground_r;
    }
    else if ((me == r) && (me == b) && (me == t))
    {
        result = state->asset_ids.sea_ground_l;
    }
    else if ((l == terrain::land) && (t == l))
    {
        result = state->asset_ids.sea_ground_t_l;
    }
    else if ((r == terrain::land) && (t == r))
    {
        result = state->asset_ids.sea_ground_t_r;
    }
    else if ((l == terrain::land) && (b == l))
    {
        result = state->asset_ids.sea_ground_b_l;
    }
    else if ((r == terrain::land) && (b == r))
    {
        result = state->asset_ids.sea_ground_b_r;
    }

    return result;
}

void
terrain_tiles_to_texture(game_state *state)
{
    for (uint32_t y = 0; y < MAP_HEIGHT; ++y)
    {
        for (uint32_t x = 0; x < MAP_WIDTH; ++x)
        {
            int32_t texture = resolve_terrain_tile_to_texture(state, (int32_t)x, (int32_t)y);
            set_terrain_texture(state, x, y, texture);
        }
    }
}

void
build_passable(game_state *state)
{
    for (uint32_t y = 0; y < MAP_HEIGHT; ++y)
    {
        for (uint32_t x = 0; x < MAP_WIDTH; ++x)
        {
            if (x == 0)
            {
                set_passable_x(state, x, y, false);
            }

            if (x == MAP_WIDTH - 1)
            {
                set_passable_x(state, x + 1, y, false);
            }
            else
            {
                set_passable_x(state, x + 1, y, get_tile(state, x, y) == get_tile(state, x + 1, y));
            }

            if (y == 0)
            {
                set_passable_y(state, x, y, false);
            }

            if (y == MAP_HEIGHT - 1)
            {
                set_passable_y(state, x, y + 1, false);
            }
            else
            {
                set_passable_y(state, x, y + 1, get_tile(state, x, y) == get_tile(state, x, y + 1));
            }
        }
    }
}

void
build_map(game_state *state)
{
    build_terrain_tiles(state);
    terrain_tiles_to_texture(state);
    build_passable(state);
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
        state->asset_ids.sea_ground_t_l = add_asset(state, "sea_ground_t_l");
        state->asset_ids.sea_ground_t_r = add_asset(state, "sea_ground_t_r");
        state->asset_ids.sea_ground_b_l = add_asset(state, "sea_ground_b_l");
        state->asset_ids.sea_ground_b_r = add_asset(state, "sea_ground_b_r");
        state->asset_ids.player = add_asset(state, "player");

        hassert(state->asset_ids.player > 0);
        build_map(state);
        RenderListBuffer(state->render_list, state->render_buffer);
        state->pixel_size = 64;

        state->acceleration_multiplier = 5.0f;
    }
    if (memory->imgui_state)
    {
        ImGui::SetInternalState(memory->imgui_state);
    }
    ImGui::DragInt("Tile pixel size", &state->pixel_size, 1.0f, 4, 128);
    ImGui::SliderFloat2("Camera Offset", (float *)&state->camera_offset, -0.5f, 0.5f);
    float max_x = (float)input->window.width / state->pixel_size / 2.0f;
    float max_y = (float)input->window.height / state->pixel_size / 2.0f;
    state->matrices[0] = m4orthographicprojection(1.0f, -1.0f, {0.0f, 0.0f}, {(float)input->window.width, (float)input->window.height});
    state->matrices[1] = m4orthographicprojection(1.0f, -1.0f, {-max_x + state->camera_offset.x, -max_y + state->camera_offset.y}, {max_x + state->camera_offset.x, max_y + state->camera_offset.y});

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

    v2 acceleration = {};

    for (uint32_t i = 0;
            i < harray_count(input->controllers);
            ++i)
    {
        if (!input->controllers[i].is_active)
        {
            continue;
        }

        game_controller_state *controller = &input->controllers[i];
        if (controller->buttons.move_up.ended_down)
        {
            acceleration.y += 1.0f;
        }
        if (controller->buttons.move_down.ended_down)
        {
            acceleration.y -= 1.0f;
        }
        if (controller->buttons.move_left.ended_down)
        {
            acceleration.x -= 1.0f;
        }
        if (controller->buttons.move_right.ended_down)
        {
            acceleration.x += 1.0f;
        }
    }
    ImGui::DragFloat("Acceleration multiplier", (float *)&state->acceleration_multiplier, 0.1f, 50.0f);
    acceleration = v2mul(acceleration, state->acceleration_multiplier);
    v2 movement = integrate_acceleration(&state->player_velocity, acceleration, input->delta_t);

    {
        line2 potential_lines[2];
        uint32_t num_potential_lines = 0;

        uint32_t x_start = 50 - (MAP_WIDTH / 2);
        uint32_t y_start = 50 - (MAP_HEIGHT / 2);

        uint32_t player_tile_x = 50 + (int32_t)floorf(state->player_position.x) - x_start;
        uint32_t player_tile_y = 50 + (int32_t)floorf(state->player_position.y) - y_start;

        ImGui::Text("Player is at %f,%f moving %f,%f", state->player_position.x, state->player_position.y, movement.x, movement.y);
        ImGui::Text("Player is at tile %dx%d", player_tile_x, player_tile_y);

        if (movement.x > 0)
        {
            if (!get_passable_x(state, player_tile_x + 1, player_tile_y))
            {
                v2 q = {(float)player_tile_x - 50 + x_start + 1, (float)player_tile_y - 50 + y_start};
                v2 direction = {0, 1};
                potential_lines[num_potential_lines++] = {q, direction};
                ImGui::Text("Path to right tile is blocked by %f,%f direction %f,%f",
                        q.x, q.y, direction.x, direction.y);
            };
        }
        else if (movement.x < 0)
        {
            if (!get_passable_x(state, player_tile_x, player_tile_y))
            {
                v2 q = {(float)player_tile_x - 50 + x_start, (float)player_tile_y - 50 + y_start};
                v2 direction = {0, 1};
                potential_lines[num_potential_lines++] = {q, direction};
                ImGui::Text("Path to left tile is blocked by %f,%f direction %f,%f",
                        q.x, q.y, direction.x, direction.y);
            }
        }
        if (movement.y > 0)
        {
            if (!get_passable_y(state, player_tile_x, player_tile_y + 1))
            {
                v2 q = {(float)player_tile_x - 50 + x_start, (float)player_tile_y - 50 + 1 + y_start};
                v2 direction = {1, 0};
                potential_lines[num_potential_lines++] = {q, direction};
                ImGui::Text("Path to up tile is blocked by %f,%f direction %f,%f",
                        q.x, q.y, direction.x, direction.y);
            };
        }
        else if (movement.y < 0)
        {
            if (!get_passable_y(state, player_tile_x, player_tile_y))
            {
                v2 q = {(float)player_tile_x - 50 + x_start, (float)player_tile_y - 50 + y_start};
                v2 direction = {1, 0};
                potential_lines[num_potential_lines++] = {q, direction};
                ImGui::Text("Path to down tile is blocked by %f,%f direction %f,%f",
                        q.x, q.y, direction.x, direction.y);
            };
        }

        ImGui::Text("%d potential colliding lines", num_potential_lines);

        int num_intersects = 0;
        while (v2length(movement) > 0)
        {
            if (num_intersects++ > 8)
            {
                break;
            }

            line2 *intersecting_line = {};
            line2 movement_line = {state->player_position, movement};
            v2 closest_intersect_point = {};
            float closest_length = -1;
            for (uint32_t i = 0; i < num_potential_lines; ++i)
            {
                v2 intersect_point;
                if (line_intersect(movement_line, potential_lines[i], &intersect_point)) {
                    ImGui::Text("Player at %f,%f movement %f,%f intersects with line %f,%f direction %f,%f at %f,%f",
                            state->player_position.x,
                            state->player_position.y,
                            movement.x,
                            movement.y,
                            potential_lines[i].position.x,
                            potential_lines[i].position.y,
                            potential_lines[i].direction.x,
                            potential_lines[i].direction.y,
                            intersect_point.x,
                            intersect_point.y
                            );
                    float distance_to = v2length(v2sub(intersect_point, state->player_position));
                    if ((closest_length < 0) || (closest_length > distance_to))
                    {
                        closest_intersect_point = intersect_point;
                        closest_length = distance_to;
                        intersecting_line = potential_lines + i;
                    }
                }
            }
            if (!intersecting_line)
            {
                state->player_position = v2add(state->player_position, movement);
                movement = {0,0};
                break;
            }
        }
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
            uint32_t x_start = 50 - (MAP_WIDTH / 2);
            uint32_t x_end = 50 + (MAP_WIDTH / 2);
            uint32_t y_start = 50 - (MAP_HEIGHT / 2);
            uint32_t y_end = 50 + (MAP_HEIGHT / 2);

            v3 q = {(float)x - 50, (float)y - 50, 0};
            v3 q_size = {1, 1, 0};

            if ((x >= x_start && x < x_end) && (y >= y_start && y < y_end))
            {
                uint32_t x_ = x - x_start;
                uint32_t y_ = y - y_start;
                int32_t texture_id = get_terrain_texture(state, x_, y_);
                PushQuad(&state->render_list, q, q_size, {1,1,1,1}, 1, texture_id);

                if (x_ == 0)
                {
                    bool passable = get_passable_x(state, x_, y_);
                    if (!passable)
                    {
                        v3 pq = v3sub(q, {0.05f, 0, 0});
                        v3 pq_size = {0.1f, 1.0f, 0};
                        PushQuad(&state->render_list, pq, pq_size, {1,1,1,1}, 1, -1);
                    }
                }
                {
                    bool passable = get_passable_x(state, x_ + 1, y_);
                    if (!passable)
                    {
                        v3 pq = v3add(q, {0.95f, 0, 0});
                        v3 pq_size = {0.1f, 1.0f, 0};
                        PushQuad(&state->render_list, pq, pq_size, {1,1,1,1}, 1, -1);
                    }
                }
                if (y_ == 0)
                {
                    bool passable = get_passable_y(state, x_, y_);
                    if (!passable)
                    {
                        v3 pq = v3sub(q, {0, 0.05f, 0});
                        v3 pq_size = {1.0f, 0.1f, 0};
                        PushQuad(&state->render_list, pq, pq_size, {1,1,1,1}, 1, -1);
                    }
                }
                {
                    bool passable = get_passable_y(state, x_, y_ + 1);
                    if (!passable)
                    {
                        v3 pq = v3add(q, {0, 0.95f, 0});
                        v3 pq_size = {1.0f, 0.1f, 0};
                        PushQuad(&state->render_list, pq, pq_size, {1,1,1,1}, 1, -1);
                    }
                }
            }
            else
            {
                int32_t texture_id = 1;
                PushQuad(&state->render_list, q, q_size, {1,1,1,1}, 1, texture_id);
            }
        }
    }

    v3 player_size = { 1.0f, 2.0f, 0 };
    v3 player_position = {state->player_position.x - 0.5f, state->player_position.y, 0};
    PushQuad(&state->render_list, player_position, player_size, {1,1,1,1}, 1, state->asset_ids.player);

    v3 player_position2 = {state->player_position.x - 0.2f, state->player_position.y, 0};
    v3 player_size2 = {0.4f, 0.1f, 0};
    PushQuad(&state->render_list, player_position2, player_size2, {1,1,1,1}, 1, -1);

    v3 mouse_bl = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y), 0.0f};
    v3 mouse_size = {16.0f, -16.0f, 0.0f};
    PushQuad(&state->render_list, mouse_bl, mouse_size, {1,1,1,1}, 0, 0);

    AddRenderList(memory, &state->render_list);
}
