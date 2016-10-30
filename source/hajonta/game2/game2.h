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

#define DEMO(func_name) void func_name(platform_memory *memory, game_input *input, game_sound_output *sound_output, demo_context *context)
typedef DEMO(demo_func);

#define MAP_HEIGHT 64
#define MAP_WIDTH 64

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
    int32_t ground_plane_mesh;
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
    int32_t blocky_advanced_mesh2;
    int32_t blocky_advanced_texture;
    int32_t blockfigureRigged6_mesh;
    int32_t blockfigureRigged6_texture;
    int32_t knp_palette;
    int32_t cube_bounds_mesh;
    int32_t white_texture;
    int32_t square_texture;
    int32_t dog2_mesh;
    int32_t dog2_texture;
    int32_t water_normal;
    int32_t water_dudv;
    int32_t knp_plate_grass;
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

template<typename T>
struct
array2p
{
    T *values;
    int32_t height;
    int32_t width;

    void setall(T value);
    void set(v2i location, T value);
    T get(v2i location);
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
    array2p<T> array2p();
};

template<uint32_t H, uint32_t W, typename T>
array2p<T>
array2<H,W,T>::array2p()
{
    return { (T *)values, H, W };
}

template<uint32_t H, uint32_t W, typename T>
void
array2<H,W,T>::setall(T value)
{
    for (uint32_t x = 0; x < H * W; ++x)
    {
        values[x] = value;
    }
}

template<typename T>
void
array2p<T>::setall(T value)
{
    for (uint32_t x = 0; x < height * width; ++x)
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

template<typename T>
void
array2p<T>::set(v2i location, T value)
{
    values[location.y * width + location.x] = value;
}

template<uint32_t H, uint32_t W, typename T>
T
array2<H,W,T>::get(v2i location)
{
    return values[location.y * W + location.x];
}

template<typename T>
T
array2p<T>::get(v2i location)
{
    return values[location.y * width + location.x];
}

template<uint32_t H, uint32_t W, typename T>
T
array2<H,W,T>::get(int32_t x, int32_t y)
{
    return values[y * W + x];
}

struct
_vertexformat_1
{
    v3 position;
    v3 normal;
    v2 texcoords;
};

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
    void *map;
    bool (*astar_passable)(astar_data *data, v2i current, v2i neighbour);
};


struct
DebugProfileEventLocation
{
    char guid[256];
    uint32_t file_name_starts_at;
    uint32_t file_name_length;
    uint32_t line_number;
    uint32_t name_starts_at;
};

struct
DebugProfileEvent
{
    DebugProfileEventLocation *location;
    uint32_t depth;
    uint64_t start_cycles;
    uint64_t end_cycles;
    uint64_t cycles_self;
    uint64_t cycles_children;
};

struct
OpenGLTimerResult
{
    DebugProfileEventLocation *location;
    uint32_t result;
};

struct
DebugFrame
{
    uint32_t frame_id;

    uint64_t start_cycles;
    uint64_t end_cycles;
    float seconds_elapsed;

    uint32_t event_count;
    DebugProfileEvent events[10000];

    uint32_t opengl_timer_count;
    OpenGLTimerResult opengl_timer[20];

    uint32_t parent_count;
    uint32_t parents[10];
};

struct
DebugProfileEventLocationHash
{
    DebugProfileEventLocation locations[1024];
};

struct
DebugSystem
{
    DebugProfileEventLocationHash location_hash;

    uint32_t oldest_frame;
    uint32_t previous_frame;
    uint32_t current_frame;
    DebugFrame frames[256];

    bool show;
    char *zoom_guid;
};

inline void
frame_end(DebugSystem *debug_system, uint64_t cycles, float seconds)
{
    DebugFrame *old_frame = debug_system->frames + debug_system->current_frame;
    hassert(old_frame->parent_count == 0);
    old_frame->end_cycles = cycles;
    old_frame->seconds_elapsed = seconds;
    uint32_t next_frame = (debug_system->current_frame + 1) % harray_count(debug_system->frames);
    if (next_frame == debug_system->oldest_frame)
    {
        debug_system->oldest_frame = (debug_system->oldest_frame + 1) % harray_count(debug_system->frames);
    }
    DebugFrame *frame = debug_system->frames + next_frame;
    frame->event_count = 0;
    frame->opengl_timer_count = 0;
    frame->start_cycles = cycles;
    frame->frame_id = old_frame->frame_id + 1;
    debug_system->previous_frame = debug_system->current_frame;
    debug_system->current_frame = next_frame;
}

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
    struct
    {
        bool show;
        float scale;
        int32_t octaves;
        float persistence;
        float lacunarity;
        int32_t seed;
        v2 offset;
        float height_multiplier;

        v2 control_point_0;
        v2 control_point_1;
        float min_noise_height;
        float max_noise_height;
    } perlin;
    struct
    {
        bool show;
    } armature;

    DebugSystem debug_system;
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
    reflected_mesh_view_matrix,
    np_projection_matrix,
    np_view_matrix,
    light_projection_matrix,
    np_model_matrix,
    _unused,

    MAX = _unused,
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
    test2,

    MAX = test2,
};

struct
AssetDescriptors
{
    uint32_t count;
    asset_descriptor descriptors[512];
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
    v3 relative_location;
    m4 view;
    m4 projection;
};

enum struct
TerrainType
{
    deep_water,
    water,
    sand,
    grass,
    grass_2,
    rock,
    rock_2,
    snow,
    snow_2,

    MAX = snow_2,
};

struct
TerrainTypeInfo
{
    TerrainType type;
    const char *name;
    float height;
    v4 color;
    bool merge_with_previous;
};

struct
Landmass
{
    TerrainTypeInfo terrains[(uint32_t)TerrainType::MAX + 1];
};

struct game_state
{
    bool initialized;

    uint32_t shadowmap_size;

    SelectedTool selected_tool;

    _click_targets<10> click_targets;

    FrameState frame_state;

    v2i middle_base_location;
    v2 right_plane_location;

    RenderPipeline render_pipeline;
    GamePipelineElements pipeline_elements;

    m4 matrices[(uint32_t)matrix_ids::MAX + 1];

    m4 light_projection_matrix;

    LightDescriptor lights[(uint32_t)LightIds::MAX + 1];
    ArmatureDescriptor armatures[(uint32_t)ArmatureIds::MAX + 1];
    MeshBoneDescriptor bones[100];
    MeshBoneDescriptor bones2[100];
    m4 bone_matrices[100];
    m4 bone_matrices2[100];

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

#define MESH_SURROUND 2
#define MESH_WIDTH (1+2*MESH_SURROUND)
#define MESH_SQUARE (MESH_WIDTH*MESH_WIDTH)
    Mesh test_meshes[MESH_SQUARE];
    DynamicTextureDescriptor test_textures[MESH_SQUARE];
#define TERRAIN_MAP_CHUNK_WIDTH 64
#define TERRAIN_MAP_CHUNK_HEIGHT 64
#define NOISE_WIDTH (TERRAIN_MAP_CHUNK_WIDTH / 2 + 1)
#define NOISE_HEIGHT (TERRAIN_MAP_CHUNK_HEIGHT / 2 + 1)
    array2<NOISE_WIDTH, NOISE_HEIGHT, float> noisemaps[MESH_SQUARE];
    array2<NOISE_WIDTH, NOISE_HEIGHT, v4b> noisemap_scratches[MESH_SQUARE];
    int32_t test_mesh_descriptors[MESH_SQUARE];
    int32_t test_texture_descriptors[MESH_SQUARE];

    int32_t ui_mesh_descriptor;
    Mesh ui_mesh;

    Landmass landmass;

    struct
    {
        v2 offset;
        bool rebuild;
    } noisemaps_data[MESH_SQUARE];
    v2 noisemap_base;
};

