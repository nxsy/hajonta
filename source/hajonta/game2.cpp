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

#include "hajonta/game2/pq.h"
#include "hajonta/game2/pq.cpp"
#include "hajonta/game2/pq_debug.cpp"

struct demo_context
{
    bool switched;
};

#define DEMO(func_name) void func_name(hajonta_thread_context *ctx, platform_memory *memory, game_input *input, game_sound_output *sound_output, demo_context *context)
typedef DEMO(demo_func);

#define MAP_HEIGHT 32
#define MAP_WIDTH 32

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
    int32_t bottom_wall;
    int32_t player;
    int32_t familiar_ship;
    int32_t familiar;
    int32_t plane_mesh;
    int32_t tree_mesh;
    int32_t tree_texture;
    int32_t horse_mesh;
    int32_t horse_texture;
    int32_t chest_mesh;
    int32_t chest_texture;
    int32_t konserian_mesh;
    int32_t konserian_texture;
    int32_t cactus_mesh;
    int32_t cactus_texture;
    int32_t kitchen_mesh;
    int32_t kitchen_texture;
    int32_t framebuffer;
    int32_t shadowmap_framebuffer;
    int32_t nature_pack_tree_mesh;
    int32_t nature_pack_tree_texture;
};

struct
movement_data
{
    v2 position;
    v2 velocity;
    v2 acceleration;
    float delta_t;
};

struct
movement_slice
{
    uint32_t num_data;
    movement_data data[60];
};

struct
movement_history
{
    uint32_t start_slice;
    uint32_t current_slice;
    movement_slice slices[3];
};

struct
movement_history_playback
{
    uint32_t slice;
    uint32_t frame;
};

struct
entity_movement
{
    v2 position;
    v2 velocity;
};

template<uint32_t H, uint32_t W, typename T>
struct
array2
{
    T values[H * W];
    void setall(T value);
    void set(v2i location, T value);
    T get(v2i location);
    T get(int32_t x, int32_t y);
};

template<uint32_t H, uint32_t W, typename T>
void
array2<H,W,T>::setall(T value)
{
    for (uint32_t x = 0; x < H * W; ++x)
    {
        values[x] = value;
    }
}

template<uint32_t H, uint32_t W, typename T>
void
array2<H,W,T>::set(v2i location, T value)
{
    values[location.y * W + location.x] = value;
}

template<uint32_t H, uint32_t W, typename T>
T
array2<H,W,T>::get(v2i location)
{
    return values[location.y * W + location.x];
}

template<uint32_t H, uint32_t W, typename T>
T
array2<H,W,T>::get(int32_t x, int32_t y)
{
    return values[y * W + x];
}

struct
astar_data
{
    v2i start_tile;
    v2i end_tile;
    array2<MAP_HEIGHT, MAP_WIDTH, bool> open_set;
    array2<MAP_HEIGHT, MAP_WIDTH, float> g_score;
    array2<MAP_HEIGHT, MAP_WIDTH, bool> closed_set;
    array2<MAP_HEIGHT, MAP_WIDTH, v2i> came_from;
    tile_priority_queue_entry entries[MAP_HEIGHT * MAP_WIDTH];
    tile_priority_queue queue;
    bool completed;
    bool found_path;
    v2i path[MAP_HEIGHT * MAP_WIDTH];
    uint32_t path_length;

    char log[1024 * 1024];
    uint32_t log_position;
};

struct
debug_state
{
    bool player_movement;
    bool familiar_movement;
    bool rendering;
    priority_queue_debug priority_queue;
    v2i selected_tile;
    struct {
        bool show;
        astar_data data;
        bool single_step;
    } familiar_path;
    bool show_lights;
};

enum struct
game_mode
{
    normal,
    movement_history,
    pathfinding,
};

enum struct
terrain
{
    water,
    land,
};

enum struct
FurnitureType
{
    none,
    ship,
    wall,
    MAX = wall,
};

enum struct
furniture_status
{
    normal,
    constructing,
    deconstructing,
};

struct
Furniture
{
    FurnitureType type;
    furniture_status status;
};


enum struct
job_type
{
    build_furniture,
};

struct
furniture_job_data
{
    v2i tile;
    FurnitureType type;
    float remaining_build_time;
};

struct
job
{
    job_type type;
    union {
        furniture_job_data furniture_data;
    };
};


struct
map_data
{
    array2<MAP_WIDTH, MAP_HEIGHT, terrain> terrain_tiles;
    int32_t texture_tiles[MAP_HEIGHT * MAP_WIDTH];
    array2<MAP_WIDTH, MAP_HEIGHT, Furniture> furniture_tiles;
    bool passable_x[(MAP_HEIGHT + 1) * (MAP_WIDTH + 1)];
    bool passable_y[(MAP_HEIGHT + 1) * (MAP_WIDTH + 1)];
};

template<uint32_t TARGETS>
struct
_click_targets
{
    uint32_t target_count;
    rectangle2 targets[TARGETS];
};

template<uint32_t TARGETS>
void
clear_click_targets(_click_targets<TARGETS> *targets)
{
    targets->target_count = 0;
}

template<uint32_t TARGETS>
void
add_click_target(_click_targets<TARGETS> *targets, rectangle2 r)
{
    assert(targets->target_count < TARGETS);
    targets->targets[targets->target_count++] = r;
}

enum struct
ToolType
{
    none,
    furniture,
    MAX = furniture,
};

struct
SelectedTool
{
    ToolType type;
    union
    {
        FurnitureType furniture_type;
    };
};

enum struct matrix_ids
{
    pixel_projection_matrix,
    quad_projection_matrix,
    mesh_projection_matrix,
    horse_model_matrix,
    chest_model_matrix,
    plane_model_matrix,
    tree_model_matrix,
    light_projection_matrix,
    mesh_model_matrix,

    MAX = mesh_model_matrix,
};

template<uint32_t BUFFER_SIZE>
struct
_render_list
{
    render_entry_list list;
    uint8_t buffer[BUFFER_SIZE];
};

enum struct
LightIds
{
    sun,

    MAX = sun,
};

struct game_state
{
    bool initialized;

    SelectedTool selected_tool;

    _click_targets<10> click_targets;

    struct
    {
        float delta_t;
        v2 mouse_position;
    } frame_state;

    _render_list<4*1024*1024> main_renderer;
    _render_list<4*1024*1024> debug_renderer;
    _render_list<4*1024*1024> three_dee_renderer;
    _render_list<4*1024*1024> shadowmap_renderer;
    _render_list<4*1024*1024> framebuffer_renderer;

    FramebufferDescriptor framebuffer;
    FramebufferDescriptor shadowmap_framebuffer;

    m4 matrices[(uint32_t)matrix_ids::MAX + 1];

    LightDescriptor lights[(uint32_t)LightIds::MAX + 1];

    _asset_ids asset_ids;
    uint32_t asset_count;
    asset_descriptor assets[64];

    uint32_t elements[6000 / 4 * 6];

    float clear_color[3];

    int32_t active_demo;

    demo_b_state b;

    uint32_t mouse_texture;

    int32_t pixel_size;

    map_data map;

    v2 camera_offset;

    entity_movement player_movement;
    entity_movement familiar_movement;
    float acceleration_multiplier;

    game_mode mode;

    movement_history player_history;
    movement_history familiar_history;
    movement_history_playback history_playback;

    uint32_t repeat_count;

    debug_state debug;

    uint32_t job_count;
    job jobs[10];
    int32_t furniture_to_asset[(uint32_t)FurnitureType::MAX + 1];
};

game_state *_hidden_state;

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
set_passable_x(map_data *map, uint32_t x, uint32_t y, bool value)
{
    map->passable_x[y * (MAP_WIDTH + 1) + x] = value;
}

void
set_passable_y(map_data *map, uint32_t x, uint32_t y, bool value)
{
    map->passable_y[y * (MAP_WIDTH + 1) + x] = value;
}

bool
get_passable_x(map_data *map, uint32_t x, uint32_t y)
{
    return map->passable_x[y * (MAP_WIDTH + 1) + x];
}
bool
get_passable_x(map_data *map, v2i tile)
{
    hassert(tile.x >= 0);
    hassert(tile.y >= 0);
    return map->passable_x[tile.y * (MAP_WIDTH + 1) + tile.x];
}

bool
get_passable_y(map_data *map, uint32_t x, uint32_t y)
{
    return map->passable_y[y * (MAP_WIDTH + 1) + x];
}

bool
get_passable_y(map_data *map, v2i tile)
{
    hassert(tile.x >= 0);
    hassert(tile.y >= 0);
    return map->passable_y[tile.y * (MAP_WIDTH + 1) + tile.x];
}

void
set_terrain_texture(map_data *map, uint32_t x, uint32_t y, int32_t value)
{
    map->texture_tiles[y * MAP_WIDTH + x] = value;
}

int32_t
get_terrain_texture(map_data *map, uint32_t x, uint32_t y)
{
    return map->texture_tiles[y * MAP_WIDTH + x];
}

void
build_lake(map_data *map, v2 bl, v2 tr)
{
    for (int32_t y = (int32_t)bl.y; y <= (int32_t)tr.y; ++y)
    {
        for (int32_t x = (int32_t)bl.x; x <= (int32_t)tr.x; ++x)
        {
            map->terrain_tiles.set({x, y}, terrain::water);
        }
    }
}

void
build_terrain_tiles(map_data *map)
{
    for (int32_t y = 0; y < MAP_HEIGHT; ++y)
    {
        for (int32_t x = 0; x < MAP_WIDTH; ++x)
        {
            terrain result = terrain::land;
            if ((x == 0) || (x == MAP_WIDTH - 1) || (y == 0) || (y == MAP_HEIGHT - 1))
            {
                result = terrain::water;
            }
            map->terrain_tiles.set({x,y}, result);
        }
    }
    build_lake(map, {16+6, 16}, {20+6, 22});
    build_lake(map, {12+6, 13}, {17+6, 20});
    build_lake(map, {11+6, 13}, {12+6, 17});
    build_lake(map, {14+6, 11}, {16+6, 12});
    build_lake(map, {13+6, 12}, {13+6, 12});
}

terrain
get_terrain_for_tile(map_data *map, int32_t x, int32_t y)
{
    terrain result = terrain::water;
    if ((x >= 0) && (x < MAP_WIDTH) && (y >= 0) && (y < MAP_HEIGHT))
    {
        result = map->terrain_tiles.get(x, y);
    }
    return result;
}

int32_t
resolve_terrain_tile_to_texture(game_state *state, map_data *map, int32_t x, int32_t y)
{
    terrain tl = get_terrain_for_tile(map, x - 1, y + 1);
    terrain t = get_terrain_for_tile(map, x, y + 1);
    terrain tr = get_terrain_for_tile(map, x + 1, y + 1);

    terrain l = get_terrain_for_tile(map, x - 1, y);
    terrain me = get_terrain_for_tile(map, x, y);
    terrain r = get_terrain_for_tile(map, x + 1, y);

    terrain bl = get_terrain_for_tile(map, x - 1, y - 1);
    terrain b = get_terrain_for_tile(map, x, y - 1);
    terrain br = get_terrain_for_tile(map, x + 1, y - 1);

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
terrain_tiles_to_texture(game_state *state, map_data *map)
{
    for (uint32_t y = 0; y < MAP_HEIGHT; ++y)
    {
        for (uint32_t x = 0; x < MAP_WIDTH; ++x)
        {
            int32_t texture = resolve_terrain_tile_to_texture(state, map, (int32_t)x, (int32_t)y);
            set_terrain_texture(map, x, y, texture);
        }
    }
}

void
build_passable(map_data *map)
{
    for (uint32_t y = 0; y < MAP_HEIGHT; ++y)
    {
        for (uint32_t x = 0; x < MAP_WIDTH; ++x)
        {
            if (x == 0)
            {
                set_passable_x(map, x, y, false);
            }

            if (x == MAP_WIDTH - 1)
            {
                set_passable_x(map, x + 1, y, false);
            }
            else
            {
                set_passable_x(map, x + 1, y, map->terrain_tiles.get((int32_t)x, (int32_t)y) == map->terrain_tiles.get((int32_t)x + 1, (int32_t)y));
            }

            if (y == 0)
            {
                set_passable_y(map, x, y, false);
            }

            if (y == MAP_HEIGHT - 1)
            {
                set_passable_y(map, x, y + 1, false);
            }
            else
            {
                set_passable_y(map, x, y + 1, map->terrain_tiles.get((int32_t)x, (int32_t)y) == map->terrain_tiles.get((int32_t)x, (int32_t)y + 1));
            }
        }
    }
}

void
add_job(game_state *state, job Job)
{
    hassert(state->job_count < harray_count(state->jobs));
    state->jobs[state->job_count++] = Job;
}

void
add_furniture_job(game_state *state, map_data *map, v2i where, FurnitureType type)
{
    Furniture f = {type, furniture_status::constructing};
    map->furniture_tiles.set(where, f);
    job Job = {};
    Job.type = job_type::build_furniture;
    Job.furniture_data = { where, type, 2.0f };
    add_job(state, Job);
}

void
build_map(game_state *state, map_data *map)
{
    build_terrain_tiles(map);
    terrain_tiles_to_texture(state, map);
    build_passable(map);
}

int32_t
add_asset(game_state *state, const char *name, asset_descriptor_type type, bool debug)
{
    int32_t result = -1;
    if (state->asset_count < harray_count(state->assets))
    {
        state->assets[state->asset_count].type = asset_descriptor_type::name;
        state->assets[state->asset_count].asset_name = name;
        state->assets[state->asset_count].debug = debug;
        result = (int32_t)state->asset_count;
        ++state->asset_count;
    }
    return result;
}

int32_t
add_asset(game_state *state, const char *name, bool debug = false)
{
    return add_asset(state, name, asset_descriptor_type::name, debug);
}

int32_t
add_framebuffer_asset(game_state *state, FramebufferDescriptor *framebuffer, bool debug = false)
{
    int32_t result = -1;
    if (state->asset_count < harray_count(state->assets))
    {
        state->assets[state->asset_count].type = asset_descriptor_type::framebuffer;
        state->assets[state->asset_count].framebuffer = framebuffer;
        state->assets[state->asset_count].debug = debug;
        result = (int32_t)state->asset_count;
        ++state->asset_count;
    }
    return result;
}

int32_t
add_framebuffer_depth_asset(game_state *state, FramebufferDescriptor *framebuffer, bool debug = false)
{
    int32_t result = -1;
    if (state->asset_count < harray_count(state->assets))
    {
        state->assets[state->asset_count].type = asset_descriptor_type::framebuffer_depth;
        state->assets[state->asset_count].framebuffer = framebuffer;
        state->assets[state->asset_count].debug = debug;
        result = (int32_t)state->asset_count;
        ++state->asset_count;
    }
    return result;
}

void
save_movement_history(movement_history *history, movement_data data_in, bool debug)
{
    movement_slice *current_slice = history->slices + history->current_slice;
    if (current_slice->num_data >= harray_count(current_slice->data))
    {
        ++history->current_slice;
        history->current_slice %= harray_count(history->slices);
        current_slice = history->slices + history->current_slice;
        current_slice->num_data = 0;

        if (history->start_slice == history->current_slice)
        {
            ++history->start_slice;
            history->start_slice %= harray_count(history->slices);
        }
    }
    if (debug)
    {
        ImGui::Text("Movement history: Start slice %d, current slice %d, current slice count %d", history->start_slice, history->current_slice, current_slice->num_data);
    }

    current_slice->data[current_slice->num_data++] = data_in;
}

movement_data
load_movement_history(movement_history *history, movement_history_playback *playback)
{
    return history->slices[playback->slice].data[playback->frame];
}

movement_data
apply_movement(map_data *map, movement_data data_in, bool debug)
{
    movement_data data = data_in;
    v2 movement = integrate_acceleration(&data.velocity, data.acceleration, data.delta_t);

    {
        line2 potential_lines[12];
        uint32_t num_potential_lines = 0;

        uint32_t x_start = 50 - (MAP_WIDTH / 2);
        uint32_t y_start = 50 - (MAP_HEIGHT / 2);

        uint32_t player_tile_x = 50 + (int32_t)floorf(data.position.x) - x_start;
        uint32_t player_tile_y = 50 + (int32_t)floorf(data.position.y) - y_start;

        if (debug)
        {
            ImGui::Text("Player is at %f,%f moving %f,%f", data.position.x, data.position.y, movement.x, movement.y);
            ImGui::Text("Player is at tile %dx%d", player_tile_x, player_tile_y);
        }

        if (movement.x > 0)
        {
            for (int32_t i = -1; i <= 1; ++i)
            {
                if (!get_passable_x(map, player_tile_x + 1, player_tile_y + i))
                {
                    v2 q = {(float)player_tile_x - 50 + x_start + 1, (float)player_tile_y + i - 50 + y_start};
                    v2 direction = {0, 1};
                    potential_lines[num_potential_lines++] = {q, direction};
                    if (debug)
                    {
                        ImGui::Text("Path to right tile is blocked by %f,%f direction %f,%f",
                                q.x, q.y, direction.x, direction.y);
                    }
                };
            }
        }

        if (movement.x < 0)
        {
            for (int32_t i = -1; i <= 1; ++i)
            {
                if (!get_passable_x(map, player_tile_x, player_tile_y + i))
                {
                    v2 q = {(float)player_tile_x - 50 + x_start, (float)player_tile_y + i - 50 + y_start};
                    v2 direction = {0, 1};
                    potential_lines[num_potential_lines++] = {q, direction};
                    if (debug)
                    {
                        ImGui::Text("Path to left tile is blocked by %f,%f direction %f,%f",
                                q.x, q.y, direction.x, direction.y);
                    }
                }
            }
        }

        if (movement.y > 0)
        {
            for (int32_t i = -1; i <= 1; ++i)
            {
                if (!get_passable_y(map, player_tile_x + i, player_tile_y + 1))
                {
                    v2 q = {(float)player_tile_x + i - 50 + x_start, (float)player_tile_y - 50 + 1 + y_start};
                    v2 direction = {1, 0};
                    potential_lines[num_potential_lines++] = {q, direction};
                    if (debug)
                    {
                        ImGui::Text("Path to up tile is blocked by %f,%f direction %f,%f",
                                q.x, q.y, direction.x, direction.y);
                    }
                };
            }
        }

        if (movement.y < 0)
        {
            for (int32_t i = -1; i <= 1; ++i)
            {
                if (!get_passable_y(map, player_tile_x + i, player_tile_y))
                {
                    v2 q = {(float)player_tile_x + i - 50 + x_start, (float)player_tile_y - 50 + y_start};
                    v2 direction = {1, 0};
                    potential_lines[num_potential_lines++] = {q, direction};
                    if (debug)
                    {
                        ImGui::Text("Path to down tile is blocked by %f,%f direction %f,%f",
                                q.x, q.y, direction.x, direction.y);
                    }
                };
            }
        }

        for (uint32_t i = 0; i < num_potential_lines; ++i)
        {
            line2 *l = potential_lines + i;
            v3 q = {l->position.x, l->position.y};

            v3 pq = v3sub(q, {0.05f, 0.05f, 0});
            v3 pq_size = {l->direction.x, l->direction.y, 0};
            pq_size = v3add(pq_size, {0.05f, 0.05f, 0});

            PushQuad(&_hidden_state->debug_renderer.list, pq, pq_size, {1,0,1,0.5f}, 1, -1);

            v2 n = v2normalize({-l->direction.y,  l->direction.x});
            if (v2dot(movement, n) > 0)
            {
                n = v2normalize({ l->direction.y, -l->direction.x});
            }
            v3 nq = v3add(q, v3mul({l->direction.x, l->direction.y, 0}, 0.5f));

            v3 npq = v3sub(nq, {0.05f, 0.05f, 0});
            v3 npq_size = {n.x, n.y, 0};
            npq_size = v3add(npq_size, {0.05f, 0.05f, 0});

            PushQuad(&_hidden_state->debug_renderer.list, npq, npq_size, {1,1,0,0.5f}, 1, -1);

        }

        if (debug)
        {
            ImGui::Text("%d potential colliding lines", num_potential_lines);
        }

        int num_intersects = 0;
        while (v2length(movement) > 0)
        {
            if (debug)
            {
                ImGui::Text("Resolving remaining movement %f,%f from position %f, %f",
                        movement.x, movement.y, data.position.x, data.position.y);
            }
            if (num_intersects++ > 8)
            {
                break;
            }

            line2 *intersecting_line = {};
            v2 closest_intersect_point = {};
            float closest_length = -1;
            line2 movement_lines[] =
            {
                //{data.position, movement},
                {v2add(data.position, {-0.2f, 0} ), movement},
                {v2add(data.position, {0.2f, 0} ), movement},
                {v2add(data.position, {-0.2f, 0.1f} ), movement},
                {v2add(data.position, {0.2f, 0.1f} ), movement},
            };
            v2 used_movement = {};

            for (uint32_t m = 0; m < harray_count(movement_lines); ++m)
            {
                line2 movement_line = movement_lines[m];
                for (uint32_t i = 0; i < num_potential_lines; ++i)
                {
                    v2 intersect_point;
                    if (line_intersect(movement_line, potential_lines[i], &intersect_point)) {
                        if (debug)
                        {
                            ImGui::Text("Player at %f,%f movement %f,%f intersects with line %f,%f direction %f,%f at %f,%f",
                                    data.position.x,
                                    data.position.y,
                                    movement.x,
                                    movement.y,
                                    potential_lines[i].position.x,
                                    potential_lines[i].position.y,
                                    potential_lines[i].direction.x,
                                    potential_lines[i].direction.y,
                                    intersect_point.x,
                                    intersect_point.y
                                    );
                        }
                        float distance_to = v2length(v2sub(intersect_point, movement_line.position));
                        if ((closest_length < 0) || (closest_length > distance_to))
                        {
                            closest_intersect_point = intersect_point;
                            closest_length = distance_to;
                            intersecting_line = potential_lines + i;
                            used_movement = v2sub(closest_intersect_point, movement_line.position);
                        }
                    }
                }
            }

            if (!intersecting_line)
            {
                v2 new_position = v2add(data.position, movement);
                if (debug)
                {
                    ImGui::Text("No intersections, player moves from %f,%f to %f,%f", data.position.x, data.position.y,
                            new_position.x, new_position.y);
                }
                data.position = new_position;
                movement = {0,0};
            }
            else
            {
                used_movement = v2mul(used_movement, 0.999f);
                v2 new_position = v2add(data.position, used_movement);
                movement = v2sub(movement, used_movement);
                if (debug)
                {
                    ImGui::Text("Player moves from %f,%f to %f,%f using %f,%f of movement", data.position.x, data.position.y,
                            new_position.x, new_position.y, used_movement.x, used_movement.y);
                }
                data.position = new_position;

                v2 intersecting_direction = intersecting_line->direction;
                v2 new_movement = v2projection(intersecting_direction, movement);
                if (debug)
                {
                    ImGui::Text("Remaining movement of %f,%f projected along wall becoming %f,%f", movement.x, movement.y, new_movement.x, new_movement.y);
                }
                movement = new_movement;

                v2 rhn = v2normalize({-intersecting_direction.y,  intersecting_direction.x});
                v2 velocity_projection = v2projection(rhn, data.velocity);
                v2 new_velocity = v2sub(data.velocity, velocity_projection);
                if (debug)
                {
                    ImGui::Text("Velocity of %f,%f projected along wall becoming %f,%f", data.velocity.x, data.velocity.y, new_velocity.x, new_velocity.y);
                }
                data.velocity = new_velocity;

                v2 n = v2normalize({-intersecting_direction.y,  intersecting_direction.x});
                if (v2dot(used_movement, n) > 0)
                {
                    n = v2normalize({ intersecting_direction.y, -intersecting_direction.x});
                }
                n = v2mul(n, 0.002f);

                new_position = v2add(data.position, n);
                if (debug)
                {
                    ImGui::Text("Player moves %f,%f away from wall from %f,%f to %f,%f", n.x, n.y,
                            data.position.x, data.position.y,
                            new_position.x, new_position.y);
                }
                data.position = new_position;
            }

            for (uint32_t i = 0; i < num_potential_lines; ++i)
            {
                line2 *l = potential_lines + i;
                float distance = FLT_MAX;
                v2 closest_point_on_line = {};
                v2 closest_point_on_player = {};
                v2 line_to_player = {};

                for (uint32_t m = 0; m < harray_count(movement_lines); ++m)
                {
                    v2 position = movement_lines[m].position;
                    v2 player_relative_position = v2sub(position, l->position);
                    v2 m_closest_point_on_line = v2projection(l->direction, player_relative_position);
                    v2 m_line_to_player = v2sub(player_relative_position, m_closest_point_on_line);
                    float m_distance = v2length(m_line_to_player);
                    if (m_distance < distance)
                    {
                        distance = m_distance;
                        closest_point_on_line = m_closest_point_on_line;
                        closest_point_on_player = player_relative_position;
                        line_to_player = m_line_to_player;
                    }
                }

                if (debug)
                {
                    ImGui::Text("%0.4f distance from line to player", distance);
                }
                /*
                if (distance <= 0.25)
                {
                    v3 q = {l->position.x + closest_point_on_line.x - 0.05f, l->position.y + closest_point_on_line.y - 0.05f, 0};
                    v3 q_size = {0.1f, 0.1f, 0};
                    PushQuad(&_hidden_state->debug_renderer.list, q, q_size, {0,1,0,0.5f}, 1, -1);

                    if (distance < 0.002f)
                    {
                        //float corrective_distance = 0.002f - distance;
                        float corrective_distance = 0.002f;
                        v2 corrective_movement = v2mul(v2normalize(line_to_player), corrective_distance);
                        v2 new_position = v2add(data.position, corrective_movement);

                        if (debug)
                        {
                            ImGui::Text("Player moves %f,%f away from wall from %f,%f to %f,%f", corrective_movement.x, corrective_movement.y,
                                    data.position.x, data.position.y,
                                    new_position.x, new_position.y);
                        }
                        data.position = new_position;
                    }
                }
                */
            }
        }
    }
    return data;
}

void
show_debug_main_menu(game_state *state)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Debug"))
        {
            if (ImGui::BeginMenu("General"))
            {
                ImGui::MenuItem("Debug rendering", "", &state->debug.rendering);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Movement"))
            {
                ImGui::MenuItem("Player", "", &state->debug.player_movement);
                ImGui::MenuItem("Familiar", "", &state->debug.familiar_movement);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Pathfinding"))
            {
                ImGui::MenuItem("Priority Queue", "", &state->debug.priority_queue.show);
                ImGui::MenuItem("Familiar Path", "", &state->debug.familiar_path.show);
                ImGui::EndMenu();
            }
            ImGui::MenuItem("Show lights", "", &state->debug.show_lights);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

float
cost_estimate(v2i tile_start, v2i tile_end)
{
    v2 movement = {(float)(tile_start.x - tile_end.x), (float)(tile_start.y - tile_end.y)};
    return v2length(movement);
}

void
_astar_init(astar_data *data)
{
    data->queue.num_entries = 0;
    data->open_set.setall(false);
    data->g_score.setall(FLT_MAX);
    data->closed_set.setall(false);
    queue_clear(&data->queue);
    data->found_path = false;
    data->completed = false;
}
void
astar_start(astar_data *data, v2i start_tile, v2i end_tile)
{
    _astar_init(data);
    float cost = cost_estimate(start_tile, end_tile);

    auto *open_set_queue = &data->queue;
    queue_add(open_set_queue, { cost, start_tile });

    data->open_set.set(start_tile, true);
    data->g_score.set(start_tile, 0);
    data->end_tile = end_tile;
    data->start_tile = start_tile;
}

bool
astar_passable(astar_data *data, map_data *map, v2i current, v2i neighbour)
{
    auto &log = data->log;
    auto &log_position = data->log_position;

    int32_t x = (int32_t)neighbour.x - current.x;
    int32_t y = (int32_t)neighbour.y - current.y;

    if (x < 0)
    {
        if (!get_passable_x(map, current))
        {
            log_position += sprintf(log + log_position, "Ignoring neighbour %d, %d because impassable x.\n",
                neighbour.x, neighbour.y);
            return false;
        }
    }
    if (x > 0)
    {
        if (!get_passable_x(map, {current.x + 1, current.y}))
        {
            log_position += sprintf(log + log_position, "Ignoring neighbour %d, %d because impassable x.\n",
                neighbour.x, neighbour.y);
            return false;
        }
    }
    if (y < 0)
    {
        if (!get_passable_y(map, current))
        {
            log_position += sprintf(log + log_position, "Ignoring neighbour %d, %d because impassable y.\n",
                neighbour.x, neighbour.y);
            return false;
        }
    }
    if (y > 0)
    {
        if (!get_passable_y(map, {current.x, current.y + 1}))
        {
            log_position += sprintf(log + log_position, "Ignoring neighbour %d, %d because impassable y.\n",
                neighbour.x, neighbour.y);
            return false;
        }
    }

    if (abs(y) == 1 && abs(x) == 1)
    {
        // corners need to check _neighbour's_ passability towards each x/y
        // neighbour shared with current
        int32_t neighbour_rel_x = x < 1 ? 1 : 0;
        if (!get_passable_x(map, {neighbour.x + neighbour_rel_x, neighbour.y}))
        {
            return false;

        }
        int32_t neighbour_rel_y = y < 1 ? 1 : 0;
        if (!get_passable_y(map, {neighbour.x, neighbour.y + neighbour_rel_y}))
        {
            return false;

        }
    }
    return true;
}

void
astar(astar_data *data, map_data *map, bool one_step = false)
{
    auto &log = data->log;
    auto &log_position = data->log_position;

    auto &open_set_queue = data->queue;
    auto &open_set = data->open_set;
    auto &closed_set = data->open_set;
    auto &end_tile = data->end_tile;
    auto &g_score = data->g_score;
    auto &came_from = data->came_from;

    v2i result = {};
    log_position = 0;
    log_position += sprintf(log + log_position, "End tile is %d, %d!\n", end_tile.x, end_tile.y);
    while (!data->completed && open_set_queue.num_entries)
    {
        auto current = queue_pop(&open_set_queue);
        open_set.set(current.tile_position, false);
        closed_set.set(current.tile_position, true);

        log_position += sprintf(log + log_position, "Lowest score tile is %d, %d with score %f!\n", current.tile_position.x, current.tile_position.y, current.score);
        if (v2iequal(current.tile_position, end_tile))
        {
            log_position += sprintf(log + log_position, "Current tile is end tile!\n");
            data->completed = true;
            data->found_path = true;
            break;
        }

        int32_t y_min = -1;
        if (current.tile_position.y == 0)
        {
            y_min = 0;
        }
        int32_t y_max = 1;
        if (current.tile_position.y == MAP_HEIGHT - 1)
        {
            y_max = 0;
        }

        int32_t x_min = -1;
        if (current.tile_position.x == 0)
        {
            x_min = 0;
        }
        int32_t x_max = 1;
        if (current.tile_position.x == MAP_WIDTH - 1)
        {
            x_max = 0;
        }
        for (int32_t y = y_min; y <= y_max; ++y)
        {
            for (int32_t x = x_min; x <= x_max; ++x)
            {
                if (x == 0 && y == 0)
                {
                    continue;
                }
                v2i neighbour_position = {
                    current.tile_position.x + x,
                    current.tile_position.y + y,
                };

                log_position += sprintf(log + log_position, "Considering neighbour %d, %d!\n", neighbour_position.x, neighbour_position.y);
                if (!astar_passable(data, map, current.tile_position, neighbour_position))
                {
                    log_position += sprintf(log + log_position, "Path impassable, ignoring.\n");
                    continue;

                }
                if (closed_set.get(neighbour_position))
                {
                    log_position += sprintf(log + log_position, "Neighbour is in closed set, ignoring.\n");
                    continue;
                }
                float tentative_g_score = g_score.get(current.tile_position) + cost_estimate(current.tile_position, neighbour_position);
                if (open_set.get(neighbour_position))
                {
                    if (tentative_g_score >= g_score.get(neighbour_position))
                    {
                        log_position += sprintf(log + log_position, "Neighbour is in open set, but tentative g_score of %f is higher than existing g_score of %f.",
                                tentative_g_score, g_score.get(neighbour_position));
                        continue;
                    }
                }

                came_from.set(neighbour_position, current.tile_position);
                g_score.set(neighbour_position, tentative_g_score);
                float f_score = tentative_g_score + cost_estimate(neighbour_position, end_tile);
                if (open_set.get(neighbour_position))
                {
                    queue_update(&open_set_queue, {f_score, neighbour_position});
                }
                else
                {
                    queue_add(&open_set_queue, {f_score, neighbour_position});
                }
                open_set.set(neighbour_position, true);
            };
        }
        if (one_step)
        {
            return;
        }
    }
    data->completed = true;

    auto &path = data->path;
    auto &path_length = data->path_length;
    if (data->found_path)
    {
        v2i current_tile = end_tile;
        path_length = 0;
        while (!v2iequal(current_tile, data->start_tile))
        {
            path[path_length++] = current_tile;
            current_tile = came_from.get(current_tile);
        }
    }
}

v2
calculate_acceleration(entity_movement entity, v2i next_tile, float acceleration_modifier)
{
    v2 result = {
        (float)(next_tile.x - MAP_WIDTH / 2) + 0.5f - entity.position.x,
        (float)(next_tile.y - MAP_HEIGHT / 2) + 0.5f - entity.position.y,
    };

    if (v2length(result) > 1.0f)
    {
        result = v2normalize(result);
    }
    result = v2mul(result, acceleration_modifier);
    return result;
}

void
draw_map(map_data *map, render_entry_list *render_list, render_entry_list *debug_render_list, v2i selected_tile)
{
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
                v2i tile = {(int32_t)x_, (int32_t)y_};
                int32_t texture_id = get_terrain_texture(map, x_, y_);
                PushQuad(render_list, q, q_size, {1,1,1,1}, 1, texture_id);

                {
                    v4 color = {0.0f, 0.0f, 0.5f, 0.2f};
                    if (v2iequal(tile, selected_tile))
                    {
                         color = {0.75f, 0.25f, 0.5f, 0.5f};
                    }
                    v3 pq = v3add(q, {0.4f, 0.4f, 0.0f});
                    v3 pq_size = {0.2f, 0.2f, 0.0f};
                    PushQuad(debug_render_list, pq, pq_size, color, 1, -1);
                }

                if (x_ == 0)
                {
                    bool passable = get_passable_x(map, x_, y_);
                    if (!passable)
                    {
                        v3 pq = v3sub(q, {0.05f, 0, 0});
                        v3 pq_size = {0.1f, 1.0f, 0};
                        PushQuad(debug_render_list, pq, pq_size, {1,1,1,0.2f}, 1, -1);
                    }
                }
                {
                    bool passable = get_passable_x(map, x_ + 1, y_);
                    if (!passable)
                    {
                        v3 pq = v3add(q, {0.95f, 0, 0});
                        v3 pq_size = {0.1f, 1.0f, 0};
                        PushQuad(debug_render_list, pq, pq_size, {1,1,1,0.2f}, 1, -1);
                    }
                }
                if (y_ == 0)
                {
                    bool passable = get_passable_y(map, x_, y_);
                    if (!passable)
                    {
                        v3 pq = v3sub(q, {0, 0.05f, 0});
                        v3 pq_size = {1.0f, 0.1f, 0};
                        PushQuad(debug_render_list, pq, pq_size, {1,1,1,0.2f}, 1, -1);
                    }
                }
                {
                    bool passable = get_passable_y(map, x_, y_ + 1);
                    if (!passable)
                    {
                        v3 pq = v3add(q, {0, 0.95f, 0});
                        v3 pq_size = {1.0f, 0.1f, 0};
                        PushQuad(debug_render_list, pq, pq_size, {1,1,1,0.2f}, 1, -1);
                    }
                }

                Furniture furniture = map->furniture_tiles.get(tile);
                if (furniture.type != FurnitureType::none)
                {
                    float opacity = 1.0f;
                    if (furniture.status != furniture_status::normal)
                    {
                        opacity = 0.4f;
                    }
                    PushQuad(render_list, q, q_size, {1,1,1,opacity}, 1, _hidden_state->furniture_to_asset[(uint32_t)furniture.type]);
                }
            }
            else
            {
                int32_t texture_id = 1;
                PushQuad(render_list, q, q_size, {1,1,1,1}, 1, texture_id);
            }
        }
    }
}

void
remove_job(game_state *state, job *Job)
{
    ptrdiff_t job_number = Job - state->jobs;
    if (job_number == state->job_count - 1)
    {
        // This covers two cases:
        // a) We're the last job
        // b) We're the only job
        --state->job_count;
    }
    else
    {
        // There's another "last job"
        state->jobs[job_number] = state->jobs[state->job_count - 1];
        --state->job_count;
    }
}

v2
calculate_familiar_acceleration(game_state *state)
{
    v2 result = {};
    job *Job = 0;
    for (uint32_t i = 0; i < state->job_count; ++i)
    {
        job &PossibleJob = state->jobs[i];
        if (PossibleJob.type == job_type::build_furniture)
        {
            Job = &PossibleJob;
            break;
        }
    }
    if (!Job)
    {
        return result;
    }

    v2i target_tile = Job->furniture_data.tile;

    v2i familiar_tile = {
        MAP_WIDTH / 2 + (int32_t)floorf(state->familiar_movement.position.x),
        MAP_HEIGHT / 2 + (int32_t)floorf(state->familiar_movement.position.y),
    };

    if (v2iequal(familiar_tile, target_tile))
    {
        Job->furniture_data.remaining_build_time -= state->frame_state.delta_t;
        if (Job->furniture_data.remaining_build_time <= 0.0f)
        {
            Furniture f = {Job->furniture_data.type, furniture_status::normal};
            state->map.furniture_tiles.set(target_tile, f);
            remove_job(state, Job);
        }
    }


    v2i next_tile = target_tile;
    bool has_next_tile = true;

    auto &familiar_path = state->debug.familiar_path;
    auto familiar_window_shown = familiar_path.show;
    if (familiar_path.show)
    {
        ImGui::Begin("Familiar Path", &familiar_path.show);
    }

    next_tile = familiar_tile;
    auto &data = familiar_path.data;

    if (!v2iequal(data.end_tile, target_tile))
    {
        ImGui::Text("Selected tile %d,%d is different from astar end tile of %d,%d, restarting.",
            target_tile.x, target_tile.y, data.end_tile.x, data.end_tile.y);
        astar_start(&data, familiar_tile, target_tile);
    }

    bool single_step = false;
    bool next_step = true;
    if (familiar_window_shown)
    {
        ImGui::Checkbox("Single step", &familiar_path.single_step);

        if (familiar_path.single_step)
        {
            single_step = true;
            next_step = false;
            if (ImGui::Button("Next iteration"))
            {
                next_step = true;
            }
        }
        bool _c = data.completed;
        ImGui::Checkbox("Completed", &_c);
        bool _p = data.found_path;
        ImGui::Checkbox("Path found", &_p);
    }
    if (next_step && !data.completed)
    {
        astar(&data, &state->map, single_step);
    }

    if (familiar_window_shown)
    {
        ImGui::InputTextMultiline("##source", data.log, data.log_position, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);

        auto &queue = data.queue;
        ImGui::Text("Queue has %d of %d entries", queue.num_entries, queue.max_entries);
        for (uint32_t i = 0; i < queue.num_entries; ++i)
        {
            auto &entry = queue.entries[i];
            ImGui::Text("%d: Score %f, Tile %d, %d", i, entry.score, entry.tile_position.x, entry.tile_position.y);
        }

        ImGui::Text("Found path: %d", data.found_path);

        ImGui::Text("Path has %d entries", data.path_length);
        for (uint32_t i = 0; i < data.path_length; ++i)
        {
            v2i *current = data.path + i;
            ImGui::Text("%d: %d,%d", i, current->x, current->y);
        }
        ImGui::End();
    }

    if (v2iequal(familiar_tile, target_tile))
    {
        has_next_tile = false;
    }

    if (has_next_tile && v2iequal(familiar_path.data.end_tile, target_tile) && familiar_path.data.found_path)
    {
        auto *path_next_tile = familiar_path.data.path + familiar_path.data.path_length - 1;
        if (v2iequal(*path_next_tile, familiar_tile))
        {
            ImGui::Text("Next tile in path %d, %d is where familiar is (%d, %d), moving to next target",
                path_next_tile->x, path_next_tile->y, familiar_tile.x, familiar_tile.y);
            familiar_path.data.path_length--;
            path_next_tile--;
        }
        ImGui::Text("Next tile in path %d, %d is not where familiar is (%d, %d), keeping target",
            path_next_tile->x, path_next_tile->y, familiar_tile.x, familiar_tile.y);
        next_tile = *path_next_tile;
    }

    ImGui::Text("Has next tile: %d, next tile is %d, %d", has_next_tile, next_tile.x, next_tile.y);
    if (has_next_tile)
    {
        result = calculate_acceleration(state->familiar_movement, next_tile, 20.0f);
    }
    return result;
}

bool
furniture_terrain_compatible(map_data *map, FurnitureType furniture_type, v2i tile)
{
    terrain t = map->terrain_tiles.get(tile);
    if (t == terrain::water)
    {
        return false;
    }
    Furniture furniture = map->furniture_tiles.get(tile);
    if (furniture.type != FurnitureType::none)
    {
        return false;
    }
    return true;
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;
    _hidden_state = state;

    if (!memory->initialized)
    {
        memory->initialized = 1;
        state->asset_ids.framebuffer = add_framebuffer_asset(state, &state->framebuffer);
        state->asset_ids.shadowmap_framebuffer = add_framebuffer_depth_asset(state, &state->shadowmap_framebuffer);
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
        state->asset_ids.bottom_wall = add_asset(state, "bottom_wall");
        state->asset_ids.player = add_asset(state, "player");
        state->asset_ids.familiar_ship = add_asset(state, "familiar_ship");
        state->asset_ids.plane_mesh = add_asset(state, "plane_mesh");
        state->asset_ids.tree_mesh = add_asset(state, "tree_mesh");
        state->asset_ids.tree_texture = add_asset(state, "tree_texture");
        state->asset_ids.horse_mesh = add_asset(state, "horse_mesh", true);
        state->asset_ids.horse_texture = add_asset(state, "horse_texture");
        state->asset_ids.chest_mesh = add_asset(state, "chest_mesh");
        state->asset_ids.chest_texture = add_asset(state, "chest_texture");
        state->asset_ids.konserian_mesh = add_asset(state, "konserian_mesh");
        state->asset_ids.konserian_texture = add_asset(state, "konserian_texture");
        state->asset_ids.cactus_mesh = add_asset(state, "cactus_mesh");
        state->asset_ids.cactus_texture = add_asset(state, "cactus_texture");
        state->asset_ids.kitchen_mesh = add_asset(state, "kitchen_mesh");
        state->asset_ids.kitchen_texture = add_asset(state, "kitchen_texture");
        state->asset_ids.nature_pack_tree_mesh = add_asset(state, "nature_pack_tree_mesh");
        state->asset_ids.nature_pack_tree_texture = add_asset(state, "nature_pack_tree_texture");
        state->asset_ids.familiar = add_asset(state, "familiar");

        state->furniture_to_asset[0] = -1;
        state->furniture_to_asset[(uint32_t)FurnitureType::ship] = state->asset_ids.familiar_ship;
        state->furniture_to_asset[(uint32_t)FurnitureType::wall] = state->asset_ids.bottom_wall;

        hassert(state->asset_ids.familiar > 0);
        build_map(state, &state->map);
        RenderListBuffer(state->main_renderer.list, state->main_renderer.buffer);
        RenderListBuffer(state->debug_renderer.list, state->debug_renderer.buffer);
        RenderListBuffer(state->three_dee_renderer.list, state->three_dee_renderer.buffer);
        RenderListBuffer(state->shadowmap_renderer.list, state->shadowmap_renderer.buffer);
        RenderListBuffer(state->framebuffer_renderer.list, state->framebuffer_renderer.buffer);
        state->three_dee_renderer.list.framebuffer = &state->framebuffer;
        //state->shadowmap_framebuffer._flags.no_color_buffer = 1;
        state->shadowmap_framebuffer._flags.use_depth_texture = 1;
        state->shadowmap_renderer.list.framebuffer = &state->shadowmap_framebuffer;
        state->pixel_size = 64;
        state->familiar_movement.position = {-2, 2};

        state->acceleration_multiplier = 50.0f;

        state->debug.selected_tile = {MAP_WIDTH / 2, MAP_HEIGHT / 2};

        queue_init(
            &state->debug.priority_queue.queue,
            harray_count(state->debug.priority_queue.entries),
            state->debug.priority_queue.entries
        );
        auto &astar_data = state->debug.familiar_path.data;
        queue_init(
            &astar_data.queue,
            harray_count(astar_data.entries),
            astar_data.entries
        );

        auto &light = state->lights[(uint32_t)LightIds::sun];
        light.type = LightType::directional;
        light.direction = {1.0f, -1.0f, -1.0f};
        light.color = {1.0f, 1.0f, 1.0f};
        light.ambient_intensity = 0.2f;
        light.diffuse_intensity = 1.0f;
        light.attenuation_constant = 1.0f;
    }

    RenderListReset(&state->main_renderer.list);
    RenderListReset(&state->debug_renderer.list);
    RenderListReset(&state->three_dee_renderer.list);
    RenderListReset(&state->shadowmap_renderer.list);
    RenderListReset(&state->framebuffer_renderer.list);
    FramebufferReset(&state->framebuffer);
    state->framebuffer.size = {input->window.width, input->window.height};
    state->shadowmap_framebuffer.size = {input->window.width, input->window.height};

    state->frame_state.delta_t = input->delta_t;
    state->frame_state.mouse_position = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y)};

    if (memory->imgui_state)
    {
        ImGui::SetInternalState(memory->imgui_state);
    }
    show_debug_main_menu(state);
    show_pq_debug(&state->debug.priority_queue);

    {
        auto &light = state->lights[(uint32_t)LightIds::sun];
        light.type = LightType::directional;

        if (state->debug.show_lights) {
            ImGui::Begin("Lights", &state->debug.show_lights);
            ImGui::DragFloat3("Direction", &light.direction.x, 0.001f, -1.0f, 1.0f);
            ImGui::ColorEdit3("Colour", &light.color.x);
            ImGui::DragFloat("Ambient Intensity", &light.ambient_intensity, 0.001f, -1.0f, 1.0f);
            ImGui::DragFloat("Diffuse Intensity", &light.diffuse_intensity, 0.001f, -1.0f, 1.0f);
            ImGui::DragFloat3("Attenuation", &light.attenuation_constant, 0.001f, 0.0f, 10.0f);
            ImGui::End();
        }
    }

    ImGui::DragInt("Tile pixel size", &state->pixel_size, 1.0f, 4, 256);
    ImGui::SliderFloat2("Camera Offset", (float *)&state->camera_offset, -0.5f, 0.5f);
    float max_x = (float)input->window.width / state->pixel_size / 2.0f;
    float max_y = (float)input->window.height / state->pixel_size / 2.0f;
    state->matrices[(uint32_t)matrix_ids::pixel_projection_matrix] = m4orthographicprojection(1.0f, -1.0f, {0.0f, 0.0f}, {(float)input->window.width, (float)input->window.height});
    state->matrices[(uint32_t)matrix_ids::quad_projection_matrix] = m4orthographicprojection(1.0f, -1.0f, {-max_x + state->camera_offset.x, -max_y + state->camera_offset.y}, {max_x + state->camera_offset.x, max_y + state->camera_offset.y});

    float ratio = (float)input->window.width / (float)input->window.height;
    state->matrices[(uint32_t)matrix_ids::mesh_projection_matrix] = m4frustumprojection(1.0f, 100.0f, {-ratio, -1.0f}, {ratio, 1.0f});
    static float rotation = 0;
    rotation += state->frame_state.delta_t;
    m4 translate = m4identity();
    translate.cols[3] = {0, 0, -3.0f, 1.0f};
    m4 rotate = m4rotation({1,0,0}, rotation);
    rotate = m4identity();
    m4 local_translate = m4identity();
    local_translate.cols[3] = {0.0f, 0.0f, 0.0f, 1.0f};

    m4 scale = m4identity();
    scale.cols[0].E[0] = 2.0f;
    scale.cols[1].E[1] = 2.0f;
    scale.cols[2].E[2] = 2.0f;

    state->matrices[(uint32_t)matrix_ids::mesh_model_matrix] = m4mul(translate,m4mul(rotate, m4mul(scale, local_translate)));
    state->matrices[(uint32_t)matrix_ids::plane_model_matrix] = m4mul(translate, m4mul(scale, local_translate));

    translate.cols[3] = {0, 0, -5.0f, 1.0f};
    state->matrices[(uint32_t)matrix_ids::tree_model_matrix] = m4mul(translate,m4mul(rotate, m4mul(scale, local_translate)));
    static float horse_z = -5.0f;
    ImGui::DragFloat("Horse Z", (float *)&horse_z, -0.1f, -50.0f);
    translate.cols[3] = {-1.0f, 0, horse_z, 1.0f};
    rotate = m4rotation({0,1,0}, rotation / 2.0f);
    rotate = m4identity();
    local_translate.cols[3] = {0, -2.0f, 0.0f, 1.0f};
    state->matrices[(uint32_t)matrix_ids::horse_model_matrix] = m4mul(translate,m4mul(rotate, local_translate));

    translate.cols[3] = {-5.0f, 0, horse_z, 1.0f};
    state->matrices[(uint32_t)matrix_ids::chest_model_matrix] = m4mul(translate, m4mul(rotate, local_translate));

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

    v4 colorv4 = {
        state->clear_color[0],
        state->clear_color[1],
        state->clear_color[2],
        1.0f,
    };

    PushClear(&state->main_renderer.list, colorv4);

    PushMatrices(&state->main_renderer.list, harray_count(state->matrices), state->matrices);
    PushMatrices(&state->debug_renderer.list, harray_count(state->matrices), state->matrices);
    PushMatrices(&state->three_dee_renderer.list, harray_count(state->matrices), state->matrices);
    PushMatrices(&state->shadowmap_renderer.list, harray_count(state->matrices), state->matrices);
    PushMatrices(&state->framebuffer_renderer.list, harray_count(state->matrices), state->matrices);
    PushAssetDescriptors(&state->main_renderer.list, harray_count(state->assets), state->assets);
    PushAssetDescriptors(&state->debug_renderer.list, harray_count(state->assets), state->assets);
    PushAssetDescriptors(&state->three_dee_renderer.list, harray_count(state->assets), state->assets);
    PushAssetDescriptors(&state->shadowmap_renderer.list, harray_count(state->assets), state->assets);
    PushAssetDescriptors(&state->framebuffer_renderer.list, harray_count(state->assets), state->assets);

    LightDescriptors l = {harray_count(state->lights), state->lights};
    PushDescriptors(&state->main_renderer.list, l);
    PushDescriptors(&state->debug_renderer.list, l);
    PushDescriptors(&state->three_dee_renderer.list, l);
    PushDescriptors(&state->shadowmap_renderer.list, l);
    PushDescriptors(&state->framebuffer_renderer.list, l);

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
        game_buttons *buttons = &controller->buttons;
        switch (state->mode)
        {
            case game_mode::normal:
            {
                if (BUTTON_ENDED_DOWN(buttons->move_up))
                {
                    acceleration.y += 1.0f;
                }
                if (BUTTON_ENDED_DOWN(buttons->move_down))
                {
                    acceleration.y -= 1.0f;
                }
                if (BUTTON_ENDED_DOWN(buttons->move_left))
                {
                    acceleration.x -= 1.0f;
                }
                if (BUTTON_ENDED_DOWN(buttons->move_right))
                {
                    acceleration.x += 1.0f;
                }
                if (BUTTON_WENT_DOWN(buttons->start))
                {
                    state->mode = game_mode::movement_history;
                    state->history_playback.slice = state->player_history.current_slice;
                    state->history_playback.frame = state->player_history.slices[state->player_history.current_slice].num_data - 1;
                }
            } break;

            case game_mode::movement_history:
            {
                if (BUTTON_DOWN_REPETITIVELY(buttons->move_left, &state->repeat_count, 10))
                {
                    if (state->history_playback.frame != 0)
                    {
                        --state->history_playback.frame;
                    }
                    else
                    {
                        if (state->history_playback.slice != state->player_history.start_slice)
                        {
                            if (state->history_playback.slice == 0)
                            {
                                state->history_playback.slice = harray_count(state->player_history.slices) - 1;
                            }
                            else
                            {
                                --state->history_playback.slice;
                            }
                            state->history_playback.frame = harray_count(state->player_history.slices[0].data) - 1;
                            hassert(harray_count(state->player_history.slices[0].data) == state->player_history.slices[state->history_playback.slice].num_data);
                        }
                    }
                }

                if (BUTTON_DOWN_REPETITIVELY(buttons->move_right, &state->repeat_count, 10))
                {
                    if (state->history_playback.frame != harray_count(state->player_history.slices[0].data) - 1)
                    {
                        if (state->history_playback.slice != state->player_history.current_slice)
                        {
                            ++state->history_playback.frame;
                        }
                        else if (state->history_playback.frame != state->player_history.slices[state->player_history.current_slice].num_data - 1)
                        {
                            ++state->history_playback.frame;
                        }
                    }
                    else
                    {
                        if (state->history_playback.slice != state->player_history.current_slice)
                        {
                            ++state->history_playback.slice;
                            state->history_playback.slice %= harray_count(state->player_history.slices);
                            state->history_playback.frame = 0;
                        }
                    }
                }
                if (BUTTON_WENT_DOWN(buttons->start))
                {
                    state->mode = game_mode::normal;
                }
            } break;

            case game_mode::pathfinding:
            {

            } break;
        }

        if (BUTTON_WENT_DOWN(buttons->back))
        {
            if (state->player_movement.position.x == 0 && state->player_movement.position.y == 0)
            {
                memory->quit = true;
            }
            state->player_movement.position = {0,0};
            state->player_movement.velocity = {0,0};
            state->mode = game_mode::normal;
        }
    }

    if (BUTTON_WENT_UP(input->mouse.buttons.left))
    {
        bool mouse_free = true;
        for (uint32_t i = 0; i < state->click_targets.target_count; ++i)
        {
            rectangle2 r = state->click_targets.targets[i];
            if (point_in_rectangle(state->frame_state.mouse_position, r))
            {
                mouse_free = false;
                break;
            }
        }
        if (mouse_free)
        {
            int32_t t_x = (int32_t)floorf((input->mouse.x - input->window.width / 2.0f) / state->pixel_size);
            int32_t t_y = (int32_t)floorf((input->window.height  / 2.0f - input->mouse.y) / state->pixel_size);
            state->debug.selected_tile = {MAP_WIDTH / 2 + t_x, MAP_HEIGHT / 2 + t_y};
            v2i &tile = state->debug.selected_tile;

            if (state->selected_tool.type == ToolType::furniture)
            {
                if (furniture_terrain_compatible(&state->map, state->selected_tool.furniture_type, tile))
                {
                    add_furniture_job(state, &state->map, {MAP_WIDTH / 2 + t_x, MAP_HEIGHT / 2 + t_y}, state->selected_tool.furniture_type);
                }
            }
        }
    }

    clear_click_targets(&state->click_targets);

    ImGui::Text("Selected_tile is %d, %d", state->debug.selected_tile.x, state->debug.selected_tile.y);

    v2 familiar_acceleration = calculate_familiar_acceleration(state);

    ImGui::DragFloat("Acceleration multiplier", (float *)&state->acceleration_multiplier, 0.1f, 50.0f);
    acceleration = v2mul(acceleration, state->acceleration_multiplier);

    switch (state->mode)
    {
        case game_mode::normal:
        case game_mode::movement_history:
        {

            struct {
                entity_movement *entity_movement;
                movement_history *entity_history;
                movement_history_playback *entity_playback;
                bool *show_window;
                const char *name;
                v2 acceleration;
            } movements[] = {
                {
                    &state->player_movement,
                    &state->player_history,
                    &state->history_playback,
                    &state->debug.player_movement,
                    "player",
                    acceleration,
                },
                {
                    &state->familiar_movement,
                    &state->familiar_history,
                    &state->history_playback,
                    &state->debug.familiar_movement,
                    "familiar",
                    familiar_acceleration,
                },
            };

            for (auto &&m : movements)
            {
                movement_data m_data;
                m_data.position = m.entity_movement->position;
                m_data.velocity = m.entity_movement->velocity;
                m_data.acceleration = m.acceleration;
                m_data.delta_t = input->delta_t;

                char window_name[100];
                sprintf(window_name, "Movement of %s", m.name);
                bool opened_window = false;
                if (*m.show_window)
                {
                    ImGui::Begin(window_name, m.show_window);
                    opened_window = true;
                }

                switch (state->mode)
                {
                    case game_mode::movement_history:
                    {
                        if (*m.show_window)
                        {
                            ImGui::Text("Movement playback: Slice %d, frame %d", state->history_playback.slice, state->history_playback.frame);
                        }
                        m_data = load_movement_history(m.entity_history, m.entity_playback);
                    } break;
                    case game_mode::normal:
                    {
                        save_movement_history(m.entity_history, m_data, *m.show_window);
                    } break;
                    case game_mode::pathfinding: break;
                }
                movement_data data_out = apply_movement(&state->map, m_data, *m.show_window);
                if (opened_window)
                {
                    ImGui::End();
                }
                m.entity_movement->position = data_out.position;
                m.entity_movement->velocity = data_out.velocity;
            }
        } break;
        case game_mode::pathfinding: break;
    };

    draw_map(&state->map, &state->main_renderer.list, &state->debug_renderer.list, state->debug.selected_tile);

    v3 player_size = { 1.0f, 2.0f, 0 };
    v3 player_position = {state->player_movement.position.x - 0.5f, state->player_movement.position.y, 0};
    PushQuad(&state->main_renderer.list, player_position, player_size, {1,1,1,1}, 1, state->asset_ids.player);

    v3 player_position2 = {state->player_movement.position.x - 0.2f, state->player_movement.position.y, 0};
    v3 player_size2 = {0.4f, 0.1f, 0};
    PushQuad(&state->main_renderer.list, player_position2, player_size2, {1,1,1,0.4f}, 1, -1);

    v3 familiar_size = { 1.0f, 2.0f, 0 };
    v3 familiar_position = {state->familiar_movement.position.x - 0.5f, state->familiar_movement.position.y, 0};
    PushQuad(&state->main_renderer.list, familiar_position, familiar_size, {1,1,1,1}, 1, state->asset_ids.familiar);

    v3 familiar_position2 = {state->familiar_movement.position.x - 0.2f, state->familiar_movement.position.y, 0};
    v3 familiar_size2 = {0.4f, 0.1f, 0};
    PushQuad(&state->main_renderer.list, familiar_position2, familiar_size2, {1,1,1,0.4f}, 1, -1);

    for (uint32_t i = 0; i < (uint32_t)FurnitureType::MAX; ++i)
    {
        FurnitureType type = (FurnitureType)(i + 1);
        v3 quad_bl = { i * 96.0f + 16.0f, 16.0f, 0.0f };
        v3 quad_size = { 80.0f, 80.0f };
        rectangle2 cl = { {quad_bl.x, quad_bl.y}, {quad_size.x, quad_size.y}};
        add_click_target(&state->click_targets, cl);
        v4 opacity = {0.1f, 0.1f, 0.1f, 0.7f};
        if (point_in_rectangle(state->frame_state.mouse_position, cl))
        {
             opacity = {0.3f, 0.3f, 0.3f, 0.9f};
            if (BUTTON_WENT_UP(input->mouse.buttons.left))
            {
                state->selected_tool.type = ToolType::furniture;
                state->selected_tool.furniture_type = type;
            }
        }
        if (state->selected_tool.type == ToolType::furniture)
        {
             if (state->selected_tool.furniture_type == type)
             {
                 opacity = {0.7f, 0.7f, 0.3f, 0.9f};
             }
        }
        PushQuad(&state->main_renderer.list, quad_bl, quad_size, opacity, 0, -1);

        quad_bl = { i * 96.0f + 24.0f, 24.0f, 0.0f };
        quad_size = { 64.0f, 64.0f };
        PushQuad(&state->main_renderer.list, quad_bl, quad_size, {1,1,1,1}, 0, state->furniture_to_asset[(uint32_t)type]);
    }

    PushClear(&state->three_dee_renderer.list, {0.0f, 0.0f, 0.0f, 0.0f});
    PushMeshFromAsset(&state->three_dee_renderer.list, (uint32_t)matrix_ids::mesh_projection_matrix, (uint32_t)matrix_ids::horse_model_matrix, state->asset_ids.kitchen_mesh, state->asset_ids.kitchen_texture, 1);
    PushMeshFromAsset(&state->main_renderer.list, (uint32_t)matrix_ids::mesh_projection_matrix, (uint32_t)matrix_ids::chest_model_matrix, state->asset_ids.cactus_mesh, state->asset_ids.cactus_texture, 1);
    PushMeshFromAsset(&state->three_dee_renderer.list, (uint32_t)matrix_ids::mesh_projection_matrix, (uint32_t)matrix_ids::mesh_model_matrix, state->asset_ids.tree_mesh, state->asset_ids.tree_texture, 1);
    PushMeshFromAsset(&state->three_dee_renderer.list, (uint32_t)matrix_ids::mesh_projection_matrix, (uint32_t)matrix_ids::tree_model_matrix, state->asset_ids.nature_pack_tree_mesh, state->asset_ids.nature_pack_tree_texture, 1);

    m4 shadowmap_projection_matrix = m4orthographicprojection(0.0f, 50.0f, {-ratio * 10.0f, -10.0f}, {ratio * 10.0f, 10.0f});

    m4 shadowmap_view_matrix;
    {
        auto &light = state->lights[(uint32_t)LightIds::sun];
        v3 eye = v3sub({0,0,0}, light.direction);
        eye = v3mul(eye, 10);
        static v3 target = {0,0,0};
        ImGui::DragFloat3("Target", &target.x, 0.1f, -100.0f, 100.0f);
        v3 up = {0, 1, 0};

        shadowmap_view_matrix = m4lookat(eye, target, up);
    }
        state->matrices[(uint32_t)matrix_ids::light_projection_matrix] = m4mul(shadowmap_projection_matrix, shadowmap_view_matrix);
    //state->matrices[(uint32_t)matrix_ids::light_projection_matrix] = shadowmap_projection_matrix;

    PushClear(&state->shadowmap_renderer.list, {0.0f, 0.0f, 0.0f, 0.0f});
    PushMeshFromAsset(&state->shadowmap_renderer.list, (uint32_t)matrix_ids::light_projection_matrix, (uint32_t)matrix_ids::tree_model_matrix, state->asset_ids.nature_pack_tree_mesh, state->asset_ids.nature_pack_tree_texture, 1);

    PushMeshFromAsset(&state->framebuffer_renderer.list, (uint32_t)matrix_ids::mesh_projection_matrix, (uint32_t)matrix_ids::plane_model_matrix, state->asset_ids.plane_mesh, state->asset_ids.framebuffer, 1);

    v3 mouse_bl = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y), 0.0f};
    v3 mouse_size = {16.0f, -16.0f, 0.0f};
    PushQuad(&state->main_renderer.list, mouse_bl, mouse_size, {1,1,1,1}, 0, state->asset_ids.mouse_cursor);

    AddRenderList(memory, &state->main_renderer.list);
    if (state->debug.rendering)
    {
        AddRenderList(memory, &state->debug_renderer.list);
    }
    AddRenderList(memory, &state->three_dee_renderer.list);
    AddRenderList(memory, &state->framebuffer_renderer.list);
    AddRenderList(memory, &state->shadowmap_renderer.list);

    ImGui::Image(
        (void *)(intptr_t)state->framebuffer._texture,
        {512, 512.0f * (float)state->framebuffer.size.y / (float)state->framebuffer.size.x},
        {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
    );

    ImGui::Image(
        (void *)(intptr_t)state->shadowmap_framebuffer._texture,
        {512, 512.0f * (float)state->shadowmap_framebuffer.size.y / (float)state->shadowmap_framebuffer.size.x},
        {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
    );
    ImGui::SameLine();
    ImGui::Image(
        (void *)(intptr_t)state->shadowmap_framebuffer._renderbuffer,
        {512, 512.0f * (float)state->shadowmap_framebuffer.size.y / (float)state->shadowmap_framebuffer.size.x},
        {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
    );
}
