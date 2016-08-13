template<uint32_t BUFFER_SIZE>
struct
_render_list
{
    render_entry_list list;
    uint8_t buffer[BUFFER_SIZE];
};

#include "hajonta/game2/pipeline.h"

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
    int32_t nature_pack_tree_mesh;
    int32_t nature_pack_tree_texture;
    int32_t another_ground_0;
    int32_t cube_mesh;
    int32_t cube_texture;
    int32_t blocky_advanced_mesh;
    int32_t blocky_advanced_texture;
    int32_t blockfigureRigged6_mesh;
    int32_t blockfigureRigged6_texture;
    int32_t knp_palette;
    int32_t cube_bounds_mesh;
    int32_t dynamic_mesh_test;
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
    bool show_textures;
    bool cull_front;
    bool show_camera;
    bool show_nature_pack;
    struct
    {
        int32_t asset_num;
    } nature;
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
    mesh_view_matrix,
    np_projection_matrix,
    np_view_matrix,
    cube_bounds_model_matrix,
    chest_model_matrix,
    plane_model_matrix,
    tree_model_matrix,
    light_projection_matrix,
    np_model_matrix,
    mesh_model_matrix,

    MAX = mesh_model_matrix,
};

enum struct
LightIds
{
    sun,

    MAX = sun,
};

enum struct
ArmatureIds
{
    test1,

    MAX = test1,
};

struct
AssetDescriptors
{
    uint32_t count;
    asset_descriptor descriptors[128];
};

struct
FrameState
{
    float delta_t;
    v2 mouse_position;
    game_input *input;
    platform_memory *memory;
};

struct
AssetListEntry
{
    const char *pretty_name;
    const char *asset_name;
    int32_t asset_id;
};

struct
AssetClassEntry
{
    const char *name;
    uint32_t asset_start;
    uint32_t count;
};

struct
CameraState
{
    float distance;
    v3 rotation;
    v3 target;
    float near_;
    float far_;
    struct
    {
        unsigned int orthographic:1;
    };

    v3 location;
    m4 view;
    m4 projection;
};

struct game_state
{
    bool initialized;

    SelectedTool selected_tool;

    _click_targets<10> click_targets;

    FrameState frame_state;

    RenderPipeline render_pipeline;
    GamePipelineElements pipeline_elements;

    _render_list<4*1024*1024> two_dee_renderer;
    _render_list<4*1024*1024> two_dee_debug_renderer;
    _render_list<4*1024*1024> three_dee_renderer;
    _render_list<4*1024*1024> shadowmap_renderer;
    _render_list<1024*1024> framebuffer_renderer;
    _render_list<1024> multisample_renderer;
    _render_list<1024> sm_blur_x_renderer;
    _render_list<1024> sm_blur_xy_renderer;

    m4 matrices[(uint32_t)matrix_ids::MAX + 1];

    m4 np_model_matrix;
    m4 cube_bounds_model_matrix;
    m4 plane_model_matrix;
    m4 tree_model_matrix;

    LightDescriptor lights[(uint32_t)LightIds::MAX + 1];
    ArmatureDescriptor armatures[(uint32_t)ArmatureIds::MAX + 1];
    MeshBoneDescriptor bones[100];

    _asset_ids asset_ids;
    AssetDescriptors assets;


    uint32_t elements[6000 / 4 * 6];

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

    CameraState camera;
    CameraState np_camera;

    uint32_t blocks[20*20*10];

    uint32_t num_assets;
    AssetListEntry asset_lists[100];
    uint32_t num_asset_classes;
    AssetClassEntry asset_classes[10];
    const char *asset_class_names[10];

    par_shapes_mesh *par_mesh;
    Mesh test_mesh;
};

