#include "hajonta/platform/common.h"
#include "hajonta/renderer/common.h"

#if defined(_MSC_VER)
#pragma warning(push, 3)
#pragma warning(disable: 4365 4312 4456 4457 4774 4577 4244 4242 4838 4305 4018)
#endif
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_demo.cpp"

#define PAR_SHAPES_T uint32_t
#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "hajonta/math.cpp"

#include "hajonta/game2/pq.h"
#include "hajonta/game2/pq.cpp"
#include "hajonta/game2/pq_debug.cpp"

#include "hajonta/game2/game2.h"

game_state *_hidden_state;

#include "hajonta/game2/two_dee.cpp"
#include "hajonta/game2/debug.cpp"

#include <random>

static float h_pi = 3.14159265358979f;
static float h_halfpi = h_pi / 2.0f;
static float h_twopi = h_pi * 2.0f;

DebugTable *GlobalDebugTable;

DEMO(demo_b)
{
    game_state *state = (game_state *)memory->memory;
    ImGui::ShowTestWindow(&state->b.show_window);

}

int32_t
add_asset(AssetDescriptors *asset_descriptors, const char *name, asset_descriptor_type type, bool debug)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        asset_descriptors->descriptors[asset_descriptors->count].type = asset_descriptor_type::name;
        asset_descriptors->descriptors[asset_descriptors->count].asset_name = name;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

int32_t
add_asset(AssetDescriptors *asset_descriptors, const char *name, bool debug = false)
{
    return add_asset(asset_descriptors, name, asset_descriptor_type::name, debug);
}

int32_t
add_dynamic_mesh_asset(AssetDescriptors *asset_descriptors, Mesh *ptr, bool debug = false)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        hassert(ptr->dynamic);
        asset_descriptors->descriptors[asset_descriptors->count].type = asset_descriptor_type::dynamic_mesh;
        asset_descriptors->descriptors[asset_descriptors->count].ptr = ptr;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

int32_t
add_dynamic_texture_asset(AssetDescriptors *asset_descriptors, DynamicTextureDescriptor *ptr, bool debug = false)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        asset_descriptors->descriptors[asset_descriptors->count].type = asset_descriptor_type::dynamic_texture;
        asset_descriptors->descriptors[asset_descriptors->count].dynamic_texture_descriptor = ptr;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

int32_t
add_framebuffer_asset(AssetDescriptors *asset_descriptors, FramebufferDescriptor *framebuffer, bool debug = false)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        asset_descriptors->descriptors[asset_descriptors->count].type = asset_descriptor_type::framebuffer;
        asset_descriptors->descriptors[asset_descriptors->count].framebuffer = framebuffer;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

int32_t
add_framebuffer_depth_asset(AssetDescriptors *asset_descriptors, FramebufferDescriptor *framebuffer, bool debug = false)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        asset_descriptors->descriptors[asset_descriptors->count].type = asset_descriptor_type::framebuffer_depth;
        asset_descriptors->descriptors[asset_descriptors->count].framebuffer = framebuffer;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

#include "hajonta/game2/pipeline.cpp"

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
                ImGui::MenuItem("Profiling", "", &state->debug.debug_system.show);
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
            ImGui::MenuItem("Show textures", "", &state->debug.show_textures);
            ImGui::MenuItem("Show camera", "", &state->debug.show_camera);
            ImGui::MenuItem("Show nature pack", "", &state->debug.show_nature_pack);
            ImGui::MenuItem("Show perlin", "", &state->debug.perlin.show);
            ImGui::MenuItem("Show armature", "", &state->debug.armature.show);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void
update_camera(CameraState *camera, float aspect_ratio)
{
    float rho = camera->distance;
    float theta = camera->rotation.y;
    float phi = camera->rotation.x;

    camera->location = {
        sinf(theta) * cosf(phi) * rho,
        sinf(phi) * rho,
        cosf(theta) * cosf(phi) * rho,
    };

    v3 eye = v3add(camera->target, camera->location);
    v3 up;
    if (camera->rotation.x < h_halfpi && camera->rotation.x > -h_halfpi)
    {
        up = {0, 1, 0};
    }
    else
    {
        up = {0, -1, 0};
    }

    camera->view = m4lookat(eye, camera->target, up);
    if (camera->orthographic)
    {
        camera->projection = m4orthographicprojection(camera->near_, camera->far_, {-aspect_ratio * rho, -1.0f * rho}, {aspect_ratio * rho, 1.0f * rho});
    }
    else
    {
        camera->projection = m4frustumprojection(camera->near_, camera->far_, {-aspect_ratio, -1.0f}, {aspect_ratio, 1.0f});
    }
}

struct
AssetPlusTransform
{
    int32_t asset_id;
    m4 transform;
};

AssetPlusTransform
tile_asset_and_transform(int32_t y, int32_t x, int32_t x_width, int32_t z, int32_t z_width, int32_t centre_asset_id, int32_t side_asset_id, int32_t corner_asset_id)
{
    AssetPlusTransform result = {};
    auto &&asset_id = result.asset_id;
    auto &&transform = result.transform;
    transform = m4translate({(float)x, float(y), (float)z});
    asset_id = centre_asset_id;
    m4 rotation = m4identity();
    if (abs(x) == x_width && abs(z) == z_width)
    {
        asset_id = corner_asset_id;
        float rotate_amount = 0;
        if (x < 0 && z > 0)
        {
            rotate_amount = -h_halfpi;
        }
        else if (x < 0 && z < 0)
        {
            rotate_amount = 0;
        }
        else if (x > 0 && z > 0)
        {
            rotate_amount = h_pi;
        }
        else if (x > 0 && z < 0)
        {
            rotate_amount = h_halfpi;
        }
        rotation = m4rotation({0, 1, 0}, rotate_amount);
    }
    else if (abs(x) == x_width)
    {
        asset_id = side_asset_id;
        float rotate_amount = h_pi;
        if (x < 0) {
            rotate_amount = 0;
        }
        rotation = m4rotation({0, 1, 0}, rotate_amount);
    }
    else if (abs(z) == z_width)
    {
        asset_id = side_asset_id;
        float rotate_amount = -h_halfpi;
        if (z < 0) {
            rotate_amount = -rotate_amount;
        }
        rotation = m4rotation({0, 1, 0}, rotate_amount);
    }
    transform = m4mul(transform, rotation);
    m4 np_translate = m4translate({0.5f,0.5f,0.5f});
    transform = m4mul(np_translate, transform);
    return result;
}


void
initialize(platform_memory *memory, game_state *state)
{
    state->shadowmap_size = memory->shadowmap_size;
    CreatePipeline(state);
    AssetDescriptors *asset_descriptors = &state->assets;
    state->asset_ids.mouse_cursor = add_asset(asset_descriptors, "mouse_cursor");
    state->asset_ids.sea_0 = add_asset(asset_descriptors, "sea_0");
    state->asset_ids.ground_0 = add_asset(asset_descriptors, "ground_0");
    state->asset_ids.sea_ground_br = add_asset(asset_descriptors, "sea_ground_br");
    state->asset_ids.sea_ground_bl = add_asset(asset_descriptors, "sea_ground_bl");
    state->asset_ids.sea_ground_tr = add_asset(asset_descriptors, "sea_ground_tr");
    state->asset_ids.sea_ground_tl = add_asset(asset_descriptors, "sea_ground_tl");
    state->asset_ids.sea_ground_r = add_asset(asset_descriptors, "sea_ground_r");
    state->asset_ids.sea_ground_l = add_asset(asset_descriptors, "sea_ground_l");
    state->asset_ids.sea_ground_t = add_asset(asset_descriptors, "sea_ground_t");
    state->asset_ids.sea_ground_b = add_asset(asset_descriptors, "sea_ground_b");
    state->asset_ids.sea_ground_t_l = add_asset(asset_descriptors, "sea_ground_t_l");
    state->asset_ids.sea_ground_t_r = add_asset(asset_descriptors, "sea_ground_t_r");
    state->asset_ids.sea_ground_b_l = add_asset(asset_descriptors, "sea_ground_b_l");
    state->asset_ids.sea_ground_b_r = add_asset(asset_descriptors, "sea_ground_b_r");
    state->asset_ids.bottom_wall = add_asset(asset_descriptors, "bottom_wall");
    state->asset_ids.player = add_asset(asset_descriptors, "player");
    state->asset_ids.familiar_ship = add_asset(asset_descriptors, "familiar_ship");
    state->asset_ids.plane_mesh = add_asset(asset_descriptors, "plane_mesh");
    state->asset_ids.tree_mesh = add_asset(asset_descriptors, "tree_mesh");
    state->asset_ids.tree_texture = add_asset(asset_descriptors, "tree_texture");
    state->asset_ids.horse_mesh = add_asset(asset_descriptors, "horse_mesh", true);
    state->asset_ids.horse_texture = add_asset(asset_descriptors, "horse_texture");
    state->asset_ids.chest_mesh = add_asset(asset_descriptors, "chest_mesh");
    state->asset_ids.chest_texture = add_asset(asset_descriptors, "chest_texture");
    state->asset_ids.konserian_mesh = add_asset(asset_descriptors, "konserian_mesh");
    state->asset_ids.konserian_texture = add_asset(asset_descriptors, "konserian_texture");
    state->asset_ids.cactus_mesh = add_asset(asset_descriptors, "cactus_mesh");
    state->asset_ids.cactus_texture = add_asset(asset_descriptors, "cactus_texture");
    state->asset_ids.kitchen_mesh = add_asset(asset_descriptors, "kitchen_mesh");
    state->asset_ids.kitchen_texture = add_asset(asset_descriptors, "kitchen_texture");
    state->asset_ids.nature_pack_tree_mesh = add_asset(asset_descriptors, "nature_pack_tree_mesh");
    state->asset_ids.nature_pack_tree_texture = add_asset(asset_descriptors, "nature_pack_tree_texture");
    state->asset_ids.another_ground_0 = add_asset(asset_descriptors, "another_ground_0");
    state->asset_ids.cube_mesh = add_asset(asset_descriptors, "cube_mesh");
    state->asset_ids.cube_texture = add_asset(asset_descriptors, "cube_texture");

    state->asset_ids.blocky_advanced_mesh = add_asset(asset_descriptors, "kenney_blocky_advanced_mesh");
    state->asset_ids.blocky_advanced_mesh2 = add_asset(asset_descriptors, "kenney_blocky_advanced_mesh2");
    state->asset_ids.blocky_advanced_texture = add_asset(asset_descriptors, "kenney_blocky_advanced_cowboy_texture");

    state->asset_ids.blockfigureRigged6_mesh = add_asset(asset_descriptors, "blockfigureRigged6_mesh");
    state->asset_ids.blockfigureRigged6_texture = add_asset(asset_descriptors, "blockfigureRigged6_texture");

    state->asset_ids.knp_palette = add_asset(asset_descriptors, "knp_palette");
    state->asset_ids.cube_bounds_mesh = add_asset(asset_descriptors, "cube_bounds_mesh");
    state->asset_ids.white_texture = add_asset(asset_descriptors, "white_texture");
    state->asset_ids.knp_plate_grass = add_asset(asset_descriptors, "knp_Plate_Grass_01");

    state->asset_ids.familiar = add_asset(asset_descriptors, "familiar");

    state->furniture_to_asset[0] = -1;
    state->furniture_to_asset[(uint32_t)FurnitureType::ship] = state->asset_ids.familiar_ship;
    state->furniture_to_asset[(uint32_t)FurnitureType::wall] = state->asset_ids.bottom_wall;

    hassert(state->asset_ids.familiar > 0);

    PipelineInit(&state->render_pipeline, asset_descriptors);

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
    light.direction = {1.0f, -0.66f, -0.288f};
    light.color = {1.0f, 0.75f, 0.75f};
    light.ambient_intensity = 0.298f;
    light.diffuse_intensity = 1.0f;
    light.attenuation_constant = 1.0f;

    {
        auto &armature = state->armatures[(uint32_t)ArmatureIds::test1];
        armature.count = harray_count(state->bones);
        armature.bone_descriptors = state->bones;
        armature.bones = state->bone_matrices;
    }

    {
        auto &armature = state->armatures[(uint32_t)ArmatureIds::test2];
        armature.count = harray_count(state->bones2);
        armature.bone_descriptors = state->bones2;
        armature.bones = state->bone_matrices2;
    }

    state->debug.show_textures = 0;
    state->debug.show_lights = 0;
    state->debug.cull_front = 1;
    state->debug.show_nature_pack = 0;
    state->debug.show_camera = 0;
    state->debug.debug_system.show = 0;

    state->camera.distance = 7.0f;
    state->camera.near_ = 1.0f;
    state->camera.far_ = 10000.0f * 1.1f;
    state->camera.target = {2, 2.5, 0};
    state->camera.rotation = {-0.1f, -0.8f, 0};

    state->np_camera.distance = 2.0f;
    state->np_camera.near_ = 1.0f;
    state->np_camera.far_ = 100.0f;
    state->np_camera.target = {0.5f, 0.5f, 0.5f};
    state->np_camera.rotation.x = 0.5f * h_halfpi;
    state->np_camera.orthographic = 1;

    {
        AssetClassEntry &f = state->asset_classes[state->num_asset_classes++];
        f.name = "Brown Cliff";
        struct
        {
            const char *pretty_name;
            const char *asset_name;
        } _assets[] =
        {
            { "01", "knp_Brown_Cliff_01" },
            { "Bottom_01", "knp_Brown_Cliff_Bottom_01" },
            { "Bottom_Corner_01", "knp_Brown_Cliff_Bottom_Corner_01" },
            { "Bottom_Corner_Green_Top_01", "knp_Brown_Cliff_Bottom_Corner_Green_Top_01" },
            { "Bottom_Green_Top_01", "knp_Brown_Cliff_Bottom_Green_Top_01" },
            { "Corner_01", "knp_Brown_Cliff_Corner_01" },
            { "Corner_Green_Top_01", "knp_Brown_Cliff_Corner_Green_Top_01" },
            { "End_01", "knp_Brown_Cliff_End_01" },
            { "End_Green_Top_01", "knp_Brown_Cliff_End_Green_Top_01" },
            { "Green_Top_01", "knp_Brown_Cliff_Green_Top_01" },
            { "Top_01", "knp_Brown_Cliff_Top_01" },
            { "Top_Corner_01", "knp_Brown_Cliff_Top_Corner_01" },
            { "Waterfall_01", "knp_Brown_Waterfall_01" },
            { "Waterfall_Top_01", "knp_Brown_Waterfall_Top_01" },
        };
        f.asset_start = state->num_assets;
        f.count = harray_count(_assets);
        for (uint32_t i = 0; i < harray_count(_assets); ++i)
        {
            auto &a = _assets[i];
            AssetListEntry &l = state->asset_lists[state->num_assets++];
            l.pretty_name = a.pretty_name;
            l.asset_name = a.asset_name;
            l.asset_id = add_asset(asset_descriptors, l.asset_name);
        }
    }

    {
        AssetClassEntry &f = state->asset_classes[state->num_asset_classes++];
        f.name = "Grey Cliff";
        struct
        {
            const char *pretty_name;
            const char *asset_name;
        } _assets[] =
        {
            { "01", "knp_Grey_Cliff_01" },
            { "Bottom_01", "knp_Grey_Cliff_Bottom_01" },
            { "Bottom_Corner_01", "knp_Grey_Cliff_Bottom_Corner_01" },
            { "Bottom_Corner_Green_Top_01", "knp_Grey_Cliff_Bottom_Corner_Green_Top_01" },
            { "Bottom_Green_Top_01", "knp_Grey_Cliff_Bottom_Green_Top_01" },
            { "Corner_01", "knp_Grey_Cliff_Corner_01" },
            { "Corner_Green_Top_01", "knp_Grey_Cliff_Corner_Green_Top_01" },
            { "End_01", "knp_Grey_Cliff_End_01" },
            { "End_Green_Top_01", "knp_Grey_Cliff_End_Green_Top_01" },
            { "Green_Top_01", "knp_Grey_Cliff_Green_Top_01" },
            { "Top_01", "knp_Grey_Cliff_Top_01" },
            { "Top_Corner_01", "knp_Grey_Cliff_Top_Corner_01" },
            { "Waterfall_01", "knp_Grey_Waterfall_01" },
            { "Waterfall_Top_01", "knp_Grey_Waterfall_Top_01" },
        };
        f.asset_start = state->num_assets;
        f.count = harray_count(_assets);
        for (uint32_t i = 0; i < harray_count(_assets); ++i)
        {
            auto &a = _assets[i];
            AssetListEntry &l = state->asset_lists[state->num_assets++];
            l.pretty_name = a.pretty_name;
            l.asset_name = a.asset_name;
            l.asset_id = add_asset(asset_descriptors, l.asset_name);
        }
    }

    {
        AssetClassEntry &f = state->asset_classes[state->num_asset_classes++];
        f.name = "Ground";
        struct
        {
            const char *pretty_name;
            const char *asset_name;
        } _assets[] =
        {
            { "Grass", "knp_Plate_Grass_01" },
            { "Grass Dirt", "knp_Plate_Grass_Dirt_01" },
            { "River", "knp_Plate_River_01" },
            { "River_Corner", "knp_Plate_River_Corner_01" },
            { "River Corner Dirt", "knp_Plate_River_Corner_Dirt_01" },
            { "River Dirt", "knp_Plate_River_Dirt_01" },
        };
        f.asset_start = state->num_assets;
        f.count = harray_count(_assets);
        for (uint32_t i = 0; i < harray_count(_assets); ++i)
        {
            auto &a = _assets[i];
            AssetListEntry &l = state->asset_lists[state->num_assets++];
            l.pretty_name = a.pretty_name;
            l.asset_name = a.asset_name;
            l.asset_id = add_asset(asset_descriptors, l.asset_name);
        }
    }

    {
        AssetClassEntry &f = state->asset_classes[state->num_asset_classes++];
        f.name = "Trees";
        struct
        {
            const char *pretty_name;
            const char *asset_name;
        } _assets[] =
        {
            { "Large Oak Dark", "knp_Large_Oak_Dark_01" },
            { "Large Oak Fall", "knp_Large_Oak_Fall_01" },
            { "Large Oak Green", "knp_Large_Oak_Green_01" },
        };
        f.asset_start = state->num_assets;
        f.count = harray_count(_assets);
        for (uint32_t i = 0; i < harray_count(_assets); ++i)
        {
            auto &a = _assets[i];
            AssetListEntry &l = state->asset_lists[state->num_assets++];
            l.pretty_name = a.pretty_name;
            l.asset_name = a.asset_name;
            l.asset_id = add_asset(asset_descriptors, l.asset_name);
        }
    }

    for (uint32_t i = 0; i < state->num_asset_classes; ++i)
    {
        state->asset_class_names[i] = state->asset_classes[i].name;
    }

    state->test_mesh.dynamic = true;
    state->asset_ids.dynamic_mesh_test = add_dynamic_mesh_asset(asset_descriptors, &state->test_mesh);
    state->asset_ids.dynamic_texture_test = add_dynamic_texture_asset(asset_descriptors, &state->test_texture);

    for (uint32_t i = 0; i < (uint32_t)TerrainType::MAX + 1; ++i)
    {
        TerrainTypeInfo *ti = state->landmass.terrains + i;
        ti->type = (TerrainType)i;
        switch(ti->type)
        {
            case TerrainType::deep_water:
            {
                ti->name = "deep water";
                ti->height = -0.5f;
                ti->color = {0, 0, 0.8f, 1.0f};
            } break;
            case TerrainType::water:
            {
                ti->name = "water";
                ti->height = -0.005f;
                ti->color = {0.1f, 0.1f, 0.8f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::sand:
            {
                ti->name = "sand";
                ti->height = 0.1f;
                ti->color = {0.65f, 0.65f, 0.2f, 1.0f};
            } break;
            case TerrainType::grass:
            {
                ti->name = "grass";
                ti->height = 0.15f;
                ti->color = {0.1f, 0.7f, 0.0f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::grass_2:
            {
                ti->name = "grass 2";
                ti->height = 0.3f;
                ti->color = {0.0f, 0.6f, 0.0f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::rock:
            {
                ti->name = "rock";
                ti->height = 0.6f;
                ti->color = {0.65f, 0.25f, 0.25f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::rock_2:
            {
                ti->name = "rock 2";
                ti->height = 0.75f;
                ti->color = {0.5f, 0.2f, 0.2f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::snow:
            {
                ti->name = "snow";
                ti->height = 0.8f;
                ti->color = {0.8f, 0.8f, 0.8f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::snow_2:
            {
                ti->name = "snow_2";
                ti->height = 2.0f;
                ti->color = {0.9f, 0.9f, 0.9f, 1.0f};
                ti->merge_with_previous = true;
            } break;
        }
    }
}

v2
cubic_bezier(v2 p1, v2 p2, float t)
{
    float sign = t > 0 ? 1.0f : -1.0f;
    t *= sign;

    v2 ap0 = v2mul(p1, t);
    v2 p1_p2 = v2sub(p2, p1);
    v2 ap1 = v2add(p1, v2mul(p1_p2, t));
    v2 p2_p3 = v2sub({1.0f, 1.0f}, p2);
    v2 ap2 = v2add(p2, v2mul(p2_p3, t));

    v2 ap0_ap1 = v2sub(ap1, ap0);
    v2 bp0 = v2add(ap0, v2mul(ap0_ap1, t));
    v2 ap1_ap2 = v2sub(ap2, ap1);
    v2 bp1 = v2add(ap1, v2mul(ap1_ap2, t));

    v2 bp0_bp1 = v2sub(bp1, bp0);
    return v2mul(v2add(bp0, v2mul(bp0_bp1, t)), sign);
}

/*
void
generate_terrain_mesh(array2p<float> map, TerrainMeshDataP mesh_data_p, Mesh *mesh, float height_multiplier, v2 cp0, v2 cp1)
{
    v2 top_left = {
        -(map.width - 1) / 2.0f,
        (map.height - 1) / 2.0f,
    };

    int32_t vertex_index = 0;
    for (int32_t y = 0; y < map.height; ++y)
    {
        for (int32_t x = 0; x < map.width; ++x)
        {
            mesh_data_p.vertices.values[vertex_index] =
            {
                {
                    top_left.x + x,
                    cubic_bezier(cp0, cp1, map.get({x,y})).y * height_multiplier,
                    top_left.y - y,
                },
                {
                    0,
                    1,
                    0,
                },
                {
                    x/(float)map.width,
                    y/(float)map.height
                },
            };

            if (x < map.width - 1 && y < map.height -1)
            {
                mesh_data_p.add_triangle({vertex_index, vertex_index + map.width + 1, vertex_index + map.width});
                mesh_data_p.add_triangle({vertex_index + map.width + 1, vertex_index, vertex_index + 1});
            }

            ++vertex_index;
        }
    }

    mesh_data_p.update_mesh(mesh);
}
*/

union
CornerHeights
{
    struct
    {
        float top_left;
        float top_right;
        float bottom_left;
        float bottom_right;
    };
    float E[4];
};

BlockVertices
generate_block_vertices(
    TerrainMeshDataP mesh_data_p,
    int32_t vertex_index,
    v3 base_location,
    CornerHeights ch,
    v2 uv_base,
    v2 uv_size
    )
{
    BlockVertices bv = {};

    v3 upper_top_left = { -0.5f, 0.1f + ch.top_left, 0.5f };
    v3 upper_top_right= { 0.5f, 0.1f + ch.top_right, 0.5f };
    v3 upper_bottom_left = { -0.5f, 0.1f + ch.bottom_left, -0.5f };
    v3 upper_bottom_right = { 0.5f, 0.1f + ch.bottom_right, -0.5f };

    v3 lower_top_left = { -0.5f, -0.1f + ch.top_left, 0.5f };
    v3 lower_top_right = { 0.5f, -0.1f + ch.top_right, 0.5f };
    v3 lower_bottom_left = { -0.5f, -0.1f + ch.bottom_left, -0.5f };
    v3 lower_bottom_right = { 0.5f, -0.1f + ch.bottom_right, -0.5f };

    struct
    {
        v3 corner_offsets[4];
        v3 normal;
    } faces_data[] =
    {
        {
            {
                upper_bottom_left,
                upper_top_left,
                upper_top_right,
                upper_bottom_right,
            },
            { 0, 1, 0 },
        },
        {
            {
                lower_bottom_right,
                lower_top_right,
                lower_top_left,
                lower_bottom_left,
            },
            { 0, -1, 0 },
        },
        {
            {
                lower_bottom_left,
                lower_top_left,
                upper_top_left,
                upper_bottom_left,
            },
            { -1, 0, 0 },
        },
        {
            {
                upper_top_right,
                lower_top_right,
                lower_bottom_right,
                upper_bottom_right,
            },
            { 1, 0, 0 },
        },
        {
            {
                lower_bottom_left,
                upper_bottom_left,
                upper_bottom_right,
                lower_bottom_right,
            },
            { 0, 0, -1 },
        },
        {
            {
                upper_top_right,
                upper_top_left,
                lower_top_left,
                lower_top_right,
            },
            { 0, 0,  1 },
        },
    };

    for (uint32_t i = 0; i < harray_count(faces_data); ++i)
    {
        FaceVertices &fv = bv.faces[i];

        auto &face_data = faces_data[i];

        v3i triangles_indices[] = {
            { 0, 1, 2 },
            { 0, 2, 3 },
        };

        for (uint32_t j = 0; j < 2; ++j)
        {
            int32_t base_vertex_index = vertex_index;
            v3i &triangle_indices = triangles_indices[j];
            auto &triangle_data = fv.triangles[j];
            triangle3 tri = {
                face_data.corner_offsets[triangle_indices.E[0]],
                face_data.corner_offsets[triangle_indices.E[1]],
                face_data.corner_offsets[triangle_indices.E[2]],
            };
            v3 normal = winded_triangle_normal(tri);
            for (uint32_t k = 0; k < 3; ++k)
            {
                _vertexformat_1 &vf1 = triangle_data.vertices[k];
                v3 &offset = face_data.corner_offsets[triangle_indices.E[k]];
                v2 uv_offset = {
                    (uv_base.x + offset.x * 0.1f) / uv_size.x,
                    (uv_base.y - offset.z * 0.1f) / uv_size.y,
                };
                v3 position = v3add(base_location, offset);
                vf1 =
                {
                    position,
                    normal,
                    uv_offset,
                };
                ++vertex_index;
            }
            mesh_data_p.add_triangle(
                {
                    base_vertex_index,
                    base_vertex_index + 1,
                    base_vertex_index + 2,
                });
        }
    }

    return bv;
}

float
perturb_raw(float raw)
{
    return fmod(raw, 0.001f) * 100.0f;
}

void
generate_terrain_mesh2(array2p<float> map, TerrainMeshDataP mesh_data_p, Mesh *mesh, float height_multiplier, v2 cp0, v2 cp1)
{
    mesh_data_p.reset();
    v2 top_left = {
        -(map.width - 1.0f),
        (map.height - 1.0f),
    };
    v2 uv_size = {
        (float)map.width,
        (float)map.height,
    };

    for (int32_t y = 0; y < map.height; ++y)
    {
        for (int32_t x = 0; x < map.width; ++x)
        {
            v2 uv_base = {x + 0.5f, y + 0.5f};
            float raw_base_height = cubic_bezier(
                cp0,
                cp1,
                map.get({x,y})).y;
            float base_height = roundf(raw_base_height * height_multiplier) / 4 +
                perturb_raw(raw_base_height);

            CornerHeights ch = {};

            v3 base_location =
            {
                top_left.x + (2*x),
                base_height,
                top_left.y - (2*y),
            };

            int32_t vertex_index = mesh_data_p.vertex_index({x*2, y*2});

            BlockVertices bv = generate_block_vertices(
                mesh_data_p,
                vertex_index,
                base_location,
                ch,
                uv_base,
                uv_size
                );
            mesh_data_p.vertices.set({x*2,y*2}, bv);

            if (x < map.width - 1)
            {
                float raw_base_height_x1 = cubic_bezier(
                    cp0,
                    cp1,
                    map.get({x+1,y})).y;
                float base_height_x1 = roundf(raw_base_height_x1 * height_multiplier) / 4 +
                    perturb_raw(raw_base_height_x1);

                float min_height = min(base_height, base_height_x1);

                ch = {
                    base_height - min_height,
                    base_height_x1 - min_height,
                    base_height - min_height,
                    base_height_x1 - min_height,
                };

                v3 x_base_location = base_location;
                x_base_location.x++;
                x_base_location.y = min_height;
                vertex_index = mesh_data_p.vertex_index({x*2 + 1, y*2});
                v2 x_uv_base = uv_base;
                x_uv_base.x += 0.25f;
                bv = generate_block_vertices(
                    mesh_data_p,
                    vertex_index,
                    x_base_location,
                    ch,
                    x_uv_base,
                    uv_size
                    );
                mesh_data_p.vertices.set({x*2+1,y*2}, bv);
            }

            if (y < map.height - 1)
            {
                float raw_base_height_y1 = cubic_bezier(
                    cp0,
                    cp1,
                    map.get({x,y+1})).y;

                float base_height_y1 = roundf(raw_base_height_y1 * height_multiplier) / 4 +
                    perturb_raw(raw_base_height_y1);

                float min_height = min(base_height, base_height_y1);

                ch = {
                    base_height - min_height,
                    base_height - min_height,
                    base_height_y1 - min_height,
                    base_height_y1 - min_height,
                };

                v3 y_base_location = base_location;
                y_base_location.z--;
                y_base_location.y = min_height;
                vertex_index = mesh_data_p.vertex_index({x*2, y*2+1});
                v2 y_uv_base = uv_base;
                y_uv_base.y += 0.25f;
                bv = generate_block_vertices(
                    mesh_data_p,
                    vertex_index,
                    y_base_location,
                    ch,
                    y_uv_base,
                    uv_size
                    );
                mesh_data_p.vertices.set({x*2,y*2+1}, bv);
            }

            if (y < map.height - 1 && x < map.width - 1)
            {
                ch = {
                    cubic_bezier(
                        cp0,
                        cp1,
                        map.get({x,y})).y,
                    cubic_bezier(
                        cp0,
                        cp1,
                        map.get({x+1,y})).y,
                    cubic_bezier(
                        cp0,
                        cp1,
                        map.get({x,y+1})).y,
                    cubic_bezier(
                        cp0,
                        cp1,
                        map.get({x+1,y+1})).y,
                };

                for (uint32_t i = 0; i < 4; ++i)
                {
                    ch.E[i] = roundf(ch.E[i] * height_multiplier) / 4 +
                        perturb_raw(ch.E[i]);
                }

                float min_height = min(
                    min(ch.E[0], ch.E[1]),
                    min(ch.E[2], ch.E[3])
                );
                for (uint32_t i = 0; i < 4; ++i)
                {
                    ch.E[i] -= min_height;
                }

                v3 xy_base_location = base_location;
                xy_base_location.x++;
                xy_base_location.z--;
                xy_base_location.y = min_height;
                vertex_index = mesh_data_p.vertex_index({x*2+1, y*2+1});
                v2 xy_uv_base = uv_base;
                xy_uv_base.x += 0.25f;
                xy_uv_base.y += 0.25f;
                bv = generate_block_vertices(
                    mesh_data_p,
                    vertex_index,
                    xy_base_location,
                    ch,
                    xy_uv_base,
                    uv_size
                    );
                mesh_data_p.vertices.set({x*2+1,y*2+1}, bv);
            }
        }
    }

    mesh_data_p.update_mesh(mesh);
}

void
generate_noise_map(
    array2p<float> map,
    uint32_t seed,
    float scale,
    uint32_t octaves,
    float persistence,
    float lacunarity,
    v2 offset,
    float *min_noise_height,
    float *max_noise_height
    )
{
    if (scale <= 0) {
        scale = 0.0001f;
    }

    std::mt19937 prng;
    prng.seed(seed);
    std::uniform_int_distribution<int32_t> distribution(-100000, 100000);

    hassert(octaves <= 10);
    v2 offsets[10];
    for (uint32_t i = 0; i < octaves; ++i)
    {
        offsets[i] = {
            distribution(prng) + offset.x,
            distribution(prng) + offset.y
        };
    }

    float half_width = map.width / 2.0f;
    float half_height = map.height / 2.0f;

    for (int32_t y = 0; y < map.height; ++y)
    {
        for (int32_t x = 0; x < map.width; ++x)
        {
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float height = 0.0f;

            for (uint32_t i = 0; i < octaves; ++i)
            {
                float sample_x = (x - half_width) / scale * frequency + offsets[i].x;
                float sample_y = (y - half_height) / scale * frequency + offsets[i].y;

                float perlin_value = stb_perlin_noise3(sample_x, sample_y, 0, 256, 256, 256);
                height += perlin_value * amplitude;

                amplitude *= persistence;
                frequency *= lacunarity;
            }
            if (height > *max_noise_height)
            {
                *max_noise_height = height;
            }
            if (height < *min_noise_height)
            {
                *min_noise_height = height;
            }
            map.set({x, y}, height);
        }
    }

    for (int32_t y = 0; y < map.height; y++)
    {
        for (int32_t x = 0; x < map.width; x++)
        {
            float value = map.get({x, y});
            //value -= min_noise_height;
            // value /= (max_noise_height - min_noise_height);
            //value  /= 3.0f;
            map.set({x,y}, value);
        }
    }
}

enum struct
TerrainMode
{
    gray,
    color,
};

template<TerrainMode mode>
void
noise_map_to_texture(array2p<float> map, array2p<v4b> scratch, DynamicTextureDescriptor *texture, TerrainTypeInfo *terrains)
{
    texture->size = {map.width, map.height};
    for (int32_t y = 0; y < map.height; ++y)
    {
        for (int32_t x = 0; x < map.width; ++x)
        {
            switch(mode)
            {
                case TerrainMode::gray:
                {
                    uint8_t gray = (uint8_t)(255.0f * map.get({x,y}));
                    scratch.set(
                        {x,y},
                        {
                            gray,
                            gray,
                            gray,
                            255,
                        }
                    );
                } break;
                case TerrainMode::color:
                {
                    TerrainTypeInfo *terrain = terrains;
                    float height = map.get({x,y});
                    for (uint32_t i = 0; i < (uint32_t)TerrainType::MAX + 1; ++i)
                    {
                        TerrainTypeInfo *ti = terrains + i;
                        if (height <= ti->height)
                        {
                            terrain = ti;
                            break;
                        }
                    }
                    v4 color = terrain->color;
                    if (terrain->merge_with_previous)
                    {
                        TerrainTypeInfo *previous_terrain = terrain - 1;

                        float t = (height - previous_terrain->height) / (terrain->height - previous_terrain->height);
                        color = lerp(previous_terrain->color, terrain->color, t);
                    }
                    scratch.set(
                        {x,y},
                        {
                            (uint8_t)(255.0f * color.x),
                            (uint8_t)(255.0f * color.y),
                            (uint8_t)(255.0f * color.z),
                            255,
                        }
                    );
                } break;
            }
        }
    }
    texture->data = scratch.values;
    texture->reload = true;
}

void
advance_armature(game_state *state, asset_descriptor *asset, ArmatureDescriptor *armature, float delta_t)
{
    if (asset->load_state != 2)
    {
        return;
    }

    auto mesh_data = asset->mesh_data;

    static float playback_speed = 1.0f;
    bool opened_debug = state->debug.armature.show;
    if (opened_debug)
    {
        char label[100];
        sprintf(label, "Armature Animation##%p", armature);
        ImGui::Begin(label);

        ImGui::Checkbox("Animation proceed", &armature->proceed_time);
        ImGui::DragFloat("Playback speed", &playback_speed, 0.01f, 0.01f, 10.0f, "%.2f");
        if (ImGui::Button("Reset to start"))
        {
            armature->tick = 0;
        }
        ImGui::Text("Tick %d of %d",
            (uint32_t)armature->tick % mesh_data.num_ticks,
            mesh_data.num_ticks - 1);
    }
    if (armature->proceed_time)
    {
        armature->tick += 1.0f / 60.0f * 24.0f * playback_speed;
    }
    int32_t first_bone = 0;
    for (uint32_t i = 0; i < mesh_data.num_bones; ++i)
    {
        if (mesh_data.bone_parents[i] == -1)
        {
            first_bone = (int32_t)i;
            break;
        }
    }

    int32_t stack_location = 0;

    int32_t stack[100];
    stack[0] = first_bone;
    struct
    {
        int32_t bone_id;
        m4 transform;
    } parent_list[100];
    int32_t parent_list_location = 0;

    if (opened_debug)
    {
        ImGui::Separator();
        if (ImGui::Button("Armature reset"))
        {
            MeshBoneDescriptor b = {
                {1,1,1},
                {0,0,0},
                {0,0,0,0},
            };
            for (uint32_t i = 0; i < armature->count; ++i)
            {
                armature->bone_descriptors[i] = b;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Armature to mesh defaults"))
        {
            MeshBoneDescriptor b = {
                {0,0,0},
                {0,0,0},
                {0,0,0,0},
            };
            for (uint32_t i = 0; i < armature->count; ++i)
            {
                armature->bone_descriptors[i] = b;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Armature to scale"))
        {
            for (uint32_t i = 0; i < armature->count; ++i)
            {
                armature->bone_descriptors[i].scale = {1,1,1};
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("No rotation"))
        {
            for (uint32_t i = 0; i < armature->count; ++i)
            {
                armature->bone_descriptors[i].q = {0, 0, 0, 0};
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
        ImGui::Columns(4);
        ImGui::Separator();

    }

    while (stack_location >= 0)
    {
        int32_t bone = stack[stack_location];

        int32_t parent_bone = mesh_data.bone_parents[bone];
        --stack_location;
        while (parent_list[parent_list_location].bone_id != parent_bone && parent_list_location >= 0)
        {
            --parent_list_location;
        }
        ++parent_list_location;

        m4 parent_matrix = m4identity();
        if (parent_list_location)
        {
            parent_matrix = parent_list[parent_list_location - 1].transform;
        }

        if (opened_debug)
        {
            char label[200];
            sprintf(label, "%*s %s", parent_list_location * 2, "", mesh_data.bone_names + (bone * mesh_data.num_bonename_chars));
            auto size = ImGui::CalcTextSize("%s", label);
            ImGui::PushItemWidth(size.x);
            ImGui::Text("%s", label);
            ImGui::PopItemWidth();
            ImGui::NextColumn();
        }

        MeshBoneDescriptor &d = armature->bone_descriptors[bone];

        if (armature->proceed_time)
        {
            d = mesh_data.animation_ticks[((uint32_t)armature->tick % mesh_data.num_ticks) * mesh_data.num_animtick_bones + bone].transform;
        }
        if (d.scale.x == 0)
        {
            d.scale = mesh_data.default_transforms[bone].scale;
            d.translate = mesh_data.default_transforms[bone].translate;
            d.q = mesh_data.default_transforms[bone].q;
        }
        if (opened_debug)
        {
            char label[100];
            sprintf(label, "Scale##%d", bone);
            ImGui::DragFloat(label, &d.scale.x, 0.01f, 0.01f, 100.0f, "%.2f");
            d.scale.y = d.scale.x;
            d.scale.z = d.scale.x;
            ImGui::NextColumn();
            sprintf(label, "Translation##%d", bone);
            ImGui::DragFloat3(label, &d.translate.x, 0.01f, -100.0f, 100.0f, "%.4f");
            ImGui::NextColumn();
            sprintf(label, "Rotation##%d", bone);
            ImGui::DragFloat4(label, &d.q.x, 0.01f, -100.0f, 100.0f, "%.4f");
            ImGui::NextColumn();
        }

        m4 scale = m4identity();

        scale.cols[0].E[0] = d.scale.x;
        scale.cols[1].E[1] = d.scale.y;
        scale.cols[2].E[2] = d.scale.z;

        m4 translate = m4identity();
        translate.cols[3].x = d.translate.x;
        translate.cols[3].y = d.translate.y;
        translate.cols[3].z = d.translate.z;

        m4 rotate = m4rotation(d.q);

        m4 local_matrix = m4mul(translate,m4mul(rotate, scale));

        m4 global_transform = m4mul(parent_matrix, local_matrix);

        parent_list[parent_list_location] = {
            bone,
            global_transform,
        };

        if (opened_debug)
        {
            m4 bone_offset = mesh_data.bone_offsets[bone];
            for (uint32_t i = 0; i < 4; ++i)
            {
                ImGui::Text("%0.3f %0.3f %0.3f %0.3f",
                    bone_offset.cols[0].E[i],
                    bone_offset.cols[1].E[i],
                    bone_offset.cols[2].E[i],
                    bone_offset.cols[3].E[i]);

            }
            ImGui::NextColumn();
            ImGui::NextColumn();
            ImGui::NextColumn();
            ImGui::NextColumn();
        }

        armature->bones[bone] = m4mul(global_transform, mesh_data.bone_offsets[bone]);

        for (uint32_t i = 0; i < mesh_data.num_bones; ++i)
        {
            if (mesh_data.bone_parents[i] == bone)
            {
                stack_location++;
                stack[stack_location] = (int32_t)i;
            }
        }
    }

    if (opened_debug)
    {
        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
        ImGui::End();
    }
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    _platform_get_thread_id = memory->platform_get_thread_id;
    GlobalDebugTable = memory->debug_table;
    TIMED_FUNCTION();
    game_state *state = (game_state *)memory->memory;
    _hidden_state = state;

    if (!memory->initialized)
    {
        memory->initialized = 1;
        initialize(memory, state);

        //state->par_mesh = par_shapes_create_dodecahedron();
        state->par_mesh = par_shapes_create_hemisphere(10, 10);
        par_shapes_unweld(state->par_mesh, true);
        par_shapes_compute_normals(state->par_mesh);
        //par_shapes_scale(state->par_mesh, 10, 10, 10);
        //par_shapes_translate(state->par_mesh, 0, 3.0f, 0);
        auto &mesh = state->test_mesh;
        mesh.vertexformat = 1;
        mesh.dynamic = true;
        mesh.dynamic_max_vertices = 50000;
        mesh.dynamic_max_triangles = 50000;
        mesh.mesh_format = MeshFormat::v3_boneless;
        auto &par_mesh = *state->par_mesh;

        size_t vertex_size = sizeof(_vertexformat_1) * mesh.dynamic_max_vertices;
        _vertexformat_1 *vf1_p = (_vertexformat_1 *)malloc(vertex_size);

        for (int32_t i = 0; i < par_mesh.npoints / 3; ++i)
        {
            vf1_p[i] = {
                {
                    par_mesh.points[3 * i],
                    par_mesh.points[3 * i + 1],
                    par_mesh.points[3 * i + 2],
                },
                {
                    par_mesh.normals[3 * i],
                    par_mesh.normals[3 * i + 1],
                    par_mesh.normals[3 * i + 2],
                },
                {
                    par_mesh.tcoords[2 * i],
                    par_mesh.tcoords[2 * i + 1],
                },
            };
        }

        size_t index_size = 4 * 3 * mesh.dynamic_max_triangles;
        uint32_t *indices = (uint32_t *)malloc(index_size);
        memcpy(indices, par_mesh.triangles, (size_t)(4 * 3 * par_mesh.ntriangles));
        mesh.vertices = {
            (void *)vf1_p,
            (uint32_t)vertex_size,
        };
        mesh.indices = {
            (void *)indices,
            (uint32_t)index_size,
        };
        mesh.num_triangles = (uint32_t)par_mesh.ntriangles;
        mesh.v3_boneless.vertex_count = (uint32_t)par_mesh.npoints;
        auto &perlin = state->debug.perlin;
        perlin.seed = 71;
        perlin.offset = {0.06f, -0.71f};
        //perlin.scale = 27.6f;
        perlin.scale = 22.320f;
        perlin.show = false;
        perlin.octaves = 4;
        perlin.persistence = 0.5f;
        perlin.lacunarity = 2.0f;
        perlin.height_multiplier = 113.0f;
        perlin.min_noise_height = FLT_MAX;
        perlin.max_noise_height = FLT_MIN;
    }

    state->frame_state.delta_t = input->delta_t;
    state->frame_state.mouse_position = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y)};
    state->frame_state.input = input;
    state->frame_state.memory = memory;

    if (memory->imgui_state)
    {
        ImGui::SetCurrentContext((ImGuiContext *)memory->imgui_state);
    }
    show_debug_main_menu(state);
    show_pq_debug(&state->debug.priority_queue);

    {
        static float rebuild_time = 1.0f;
        rebuild_time += input->delta_t;
        bool rebuild = rebuild_time > 1.0f;
        auto &perlin = state->debug.perlin;
        if (perlin.scale <= 0.0f)
        {
            perlin.scale = 5.0f;
        }
        if (perlin.octaves <= 0)
        {
            perlin.octaves = 1;
        }
        if (perlin.persistence <= 0.0f)
        {
            perlin.persistence = 0.5f;
        }
        if (perlin.lacunarity <= 0.0f)
        {
            perlin.lacunarity = 2.0f;
        }
        if (perlin.show)
        {
            ImGui::Begin("Perlin", &perlin.show);
            rebuild |= ImGui::DragInt("Seed", &perlin.seed, 1, 0, INT32_MAX);
            rebuild |= ImGui::DragFloat("Scale", &perlin.scale, 0.01f, 0.001f, 1000.0f, "%0.3f");
            rebuild |= ImGui::DragInt("Octaves", &perlin.octaves, 1.0f, 1, 10);
            rebuild |= ImGui::DragFloat("Persistence", &perlin.persistence, 0.001f, 0.001f, 1000.0f, "%0.3f");
            rebuild |= ImGui::DragFloat("Lacunarity", &perlin.lacunarity, 0.001f, 0.001f, 1000.0f, "%0.3f");
            rebuild |= ImGui::DragFloat2("Offset", &perlin.offset.x, 0.01f, -100.0f, 100.0f, "%0.2f");
            rebuild |= ImGui::DragFloat("Height Multiplier", &perlin.height_multiplier, 0.01f, 0.01f, 1000.0f, "%0.2f");
            ImGui::End();
        }
        if (rebuild)
        {
            rebuild_time = 0;
            generate_noise_map(
                state->noisemap.array2p(),
                (uint32_t)perlin.seed,
                perlin.scale,
                (uint32_t)perlin.octaves,
                perlin.persistence,
                perlin.lacunarity,
                perlin.offset,
                &perlin.min_noise_height,
                &perlin.max_noise_height
            );
            noise_map_to_texture<TerrainMode::color>(state->noisemap.array2p(), state->noisemap_scratch.array2p(), &state->test_texture, state->landmass.terrains);
            generate_terrain_mesh2(
                state->noisemap.array2p(),
                state->terrain_mesh_data.mesh_data_p(),
                &state->test_mesh,
                perlin.height_multiplier,
                perlin.control_point_0,
                perlin.control_point_1);
        }

        if (perlin.show)
        {
            ImGui::Begin("Perlin", &perlin.show);
            ImGui::Text("Minimum noise height: %0.02f", perlin.min_noise_height);
            ImGui::Text("Maximum noise height: %0.02f", perlin.max_noise_height);
            ImGui::Image(
                (void *)(intptr_t)state->test_texture._texture,
                {512, 512},
                {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
            );
            ImGui::End();
        }
    }

    {
        auto &light = state->lights[(uint32_t)LightIds::sun];
        light.type = LightType::directional;

        if (state->debug.show_lights) {
            ImGui::Begin("Lights", &state->debug.show_lights);
            ImGui::DragFloat3("Direction", &light.direction.x, 0.001f, -1.0f, 1.0f);
            static bool animate_sun = false;
            if (ImGui::Button("Normalize"))
            {
                light.direction = v3normalize(light.direction);
            }
            ImGui::Checkbox("Animate Sun", &animate_sun);
            if (animate_sun)
            {
                static float animation_speed_per_second = 0.1f;
                ImGui::DragFloat("  Animation Speed", &animation_speed_per_second, 0.001f, 0.0f, 1.0f);
                if (light.direction.y < -0.5)
                {
                    light.direction.y = 0.5;
                }
                light.direction.y -= input->delta_t * animation_speed_per_second;
            }
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

    if (state->debug.show_camera) {
        ImGui::Begin("Camera", &state->debug.show_camera);
        ImGui::DragFloat3("Rotation", &state->camera.rotation.x, 0.1f);
        ImGui::DragFloat3("Target", &state->camera.target.x);
        ImGui::DragFloat("Distance", &state->camera.distance);
        ImGui::DragFloat("Near", &state->camera.near_);
        ImGui::DragFloat("Far", &state->camera.far_);
        bool orthographic = state->camera.orthographic;
        ImGui::Checkbox("Orthographic", &orthographic);
        state->camera.orthographic = (uint32_t)orthographic;

        if (ImGui::Button("Top"))
        {
            state->camera.rotation.x = h_halfpi;
            state->camera.rotation.y = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Front"))
        {
            state->camera.rotation.x = 0;
            state->camera.rotation.y = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Right"))
        {
            state->camera.rotation.x = 0;
            state->camera.rotation.y = h_halfpi;
        }

        ImGui::Text("Camera location: %.2f, %.2f, %.2f", state->camera.location.x, state->camera.location.y, state->camera.location.z);
        ImGui::End();
    }
    update_camera(&state->camera, ratio);
    state->np_camera.rotation.y += 0.4f * input->delta_t;
    update_camera(&state->np_camera, ratio);
    state->matrices[(uint32_t)matrix_ids::mesh_projection_matrix] = state->camera.projection;
    state->matrices[(uint32_t)matrix_ids::mesh_view_matrix] = state->camera.view;
    state->matrices[(uint32_t)matrix_ids::np_projection_matrix] = state->np_camera.projection;
    state->matrices[(uint32_t)matrix_ids::np_view_matrix] = state->np_camera.view;

    m4 translate = m4identity();
    translate.cols[3] = {0, 0, -3.0f, 1.0f};
    m4 rotate = m4identity();
    m4 local_translate = m4identity();
    local_translate.cols[3] = {0.0f, 0.0f, 0.0f, 1.0f};

    m4 scale = m4identity();
    scale.cols[0].E[0] = 2.0f;
    scale.cols[1].E[1] = 2.0f;
    scale.cols[2].E[2] = 2.0f;

    state->matrices[(uint32_t)matrix_ids::mesh_model_matrix] = m4mul(translate,m4mul(rotate, m4mul(scale, local_translate)));
    static float plane_rotation = 0.0f;
    ImGui::DragFloat("Plane rotation", &plane_rotation, 0.01f, 0, 3.0f);
    rotate = m4identity();

    float _scale = 500.0f;
    translate.cols[3] = {0, -500.0f, -500.0f, 1.0f};
    scale.cols[0].E[0] = _scale;
    scale.cols[1].E[1] = _scale;
    scale.cols[2].E[2] = _scale;
    state->plane_model_matrix = m4mul(translate, m4mul(rotate, m4mul(scale, local_translate)));

    rotate = m4identity();
    //rotate = m4rotation({1,0,0}, IM_PI / 2.0f);
    scale.cols[0].E[0] = 1.0f;
    scale.cols[1].E[1] = 1.0f;
    scale.cols[2].E[2] = 1.0f;
    rotate = m4rotation({0,1,0}, h_halfpi/3.0f);
    array2p<float> noise2p = state->noisemap.array2p();
    v2i middle = {(int32_t)(noise2p.width / 2.0f), (int32_t)(noise2p.height / 2.0f)};
    local_translate.cols[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    //float height = cubic_bezier(state->debug.perlin.control_point_0, state->debug.perlin.control_point_1, noise2p.get(middle)).y * state->debug.perlin.height_multiplier;
    float height = roundf(
        cubic_bezier(
            state->debug.perlin.control_point_0,
            state->debug.perlin.control_point_1,
            noise2p.get(middle)).y * state->debug.perlin.height_multiplier) / 4.0f + 0.1f;
    translate.cols[3] = {0.5f, height, 0.5f, 1.0f};
    state->tree_model_matrix = m4mul(translate,m4mul(rotate, m4mul(scale, local_translate)));
    translate.cols[3] = {1.5f, height, 1.5f, 1.0f};
    state->tree_model_matrix2 = m4mul(translate,m4mul(rotate, m4mul(scale, local_translate)));

    local_translate = m4identity();
    rotate = m4identity();
    translate = m4identity();
    translate.cols[3] = {0.5f,2.1f,0.5f,1};
    scale = m4identity();
    state->cube_bounds_model_matrix = m4mul(translate,m4mul(rotate, local_translate));
    translate.cols[3] = {-0.5f,2.1f,-0.5f,1};
    state->cube_bounds_model_matrix_2 = m4mul(translate,m4mul(rotate, local_translate));

    local_translate = m4identity();
    rotate = m4identity();
    translate = m4translate({0.5f,0.5f,0.5f});
    scale = m4identity();
    state->np_model_matrix = m4mul(translate,m4mul(rotate, m4mul(scale, local_translate)));
    translate = m4identity();
    state->dynamic_mesh_model_matrix = m4mul(translate,m4mul(rotate, m4mul(scale, local_translate)));

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

    LightDescriptors l = {harray_count(state->lights), state->lights};
    ArmatureDescriptors armatures = {
        harray_count(state->armatures),
        state->armatures + 0
    };

    PipelineResetData prd = {
        { input->window.width, input->window.height },
        harray_count(state->matrices), state->matrices,
        harray_count(state->assets.descriptors), (asset_descriptor *)&state->assets.descriptors,
        l,
        armatures,
    };

    PipelineReset(
        state,
        &state->render_pipeline,
        &prd
    );

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
        if (BUTTON_WENT_DOWN(buttons->back))
        {
            memory->quit = true;
        }
    }

    m4 shadowmap_projection_matrix;

    m4 shadowmap_view_matrix;
    {
        auto &light = state->lights[(uint32_t)LightIds::sun];
        v3 eye = v3sub({0,0,0}, v3normalize(light.direction));
        static float eye_distance = 20;
        shadowmap_projection_matrix = m4orthographicprojection(0.1f, 20.0f + eye_distance, {-eye_distance, -eye_distance}, {eye_distance, eye_distance});
        ImGui::DragFloat("Light distance", &eye_distance, 0.1f, 5.0f, 40.0f);
        eye = v3mul(eye, eye_distance);
        static v3 target = {0,0,0};
        ImGui::DragFloat3("Target", &target.x, 0.1f, -100.0f, 100.0f);
        v3 up = {0, 1, 0};

        shadowmap_view_matrix = m4lookat(eye, target, up);
    }
    state->matrices[(uint32_t)matrix_ids::light_projection_matrix] = m4mul(shadowmap_projection_matrix, shadowmap_view_matrix);
    state->light_projection_matrix = m4mul(shadowmap_projection_matrix, shadowmap_view_matrix);

    auto &light = state->lights[(uint32_t)LightIds::sun];
    //light.shadowmap_matrix_id = (uint32_t)matrix_ids::light_projection_matrix;
    light.shadowmap_matrix = state->light_projection_matrix;
    {
        auto fb_shadowmap = state->render_pipeline.framebuffers[state->pipeline_elements.fb_shadowmap];
        light.shadowmap_asset_descriptor_id = fb_shadowmap.depth_asset_descriptor;
        auto fb_sm_blur_xy = state->render_pipeline.framebuffers[state->pipeline_elements.fb_sm_blur_xy];
        light.shadowmap_color_asset_descriptor_id = fb_sm_blur_xy.asset_descriptor;
        auto fb_shadowmap_texarray = state->render_pipeline.framebuffers[state->pipeline_elements.fb_shadowmap_texarray];
        light.shadowmap_color_texaddress_asset_descriptor_id = fb_shadowmap_texarray.asset_descriptor;
    }

    MeshFromAssetFlags shadowmap_mesh_flags = {};
    ImGui::Checkbox("Cull front", &state->debug.cull_front);
    shadowmap_mesh_flags.cull_front = state->debug.cull_front ? (uint32_t)1 : (uint32_t)0;
    MeshFromAssetFlags three_dee_mesh_flags = {};
    three_dee_mesh_flags.attach_shadowmap = 1;
    three_dee_mesh_flags.attach_shadowmap_color = 1;

    if (state->debug.show_nature_pack) {
        ImGui::Begin("Nature Pack", &state->debug.show_nature_pack);

        ImGui::DragInt("Asset number", &state->debug.nature.asset_num, 1, 0, 10);
        int32_t asset_id = state->asset_lists[state->debug.nature.asset_num].asset_id;
        MeshFromAssetFlags nature_pack_flags = {};
        PushMeshFromAsset(
            &state->pipeline_elements.rl_nature_pack_debug.list,
            (uint32_t)matrix_ids::np_projection_matrix,
            (uint32_t)matrix_ids::np_view_matrix,
            state->np_model_matrix,
            asset_id,
            state->asset_ids.knp_palette,
            0,
            -1,
            nature_pack_flags,
            ShaderType::standard
        );
        static int32_t nature_pack_class = -1;
        static int32_t nature_pack_item = -1;

        if (ImGui::Combo("Category", &nature_pack_class, state->asset_class_names, (int32_t)state->num_asset_classes))
        {
            nature_pack_item = -1;
        }

        if (nature_pack_class >= 0)
        {
            auto &asset_class = state->asset_classes[nature_pack_class];
            const char *items[20];
            hassert(harray_count(items) >= asset_class.count);
            for (uint32_t i = 0; i < asset_class.count; ++i)
            {
                items[i] = state->asset_lists[asset_class.asset_start + i].pretty_name;
            }
            if (ImGui::Combo("Item", &nature_pack_item, items, (int32_t)asset_class.count))
            {
                state->debug.nature.asset_num = (int32_t)asset_class.asset_start + nature_pack_item;
            }
        }

        /*
        PushMeshFromAsset(
            &state->three_dee_renderer.list,
            (uint32_t)matrix_ids::mesh_projection_matrix,
            (uint32_t)matrix_ids::mesh_view_matrix,
            state->np_model_matrix,
            asset_id,
            state->asset_ids.knp_palette,
            1,
            -1,
            three_dee_mesh_flags,
            ShaderType::standard
        );
        PushMeshFromAsset(
            &state->shadowmap_renderer.list,
            (uint32_t)matrix_ids::light_projection_matrix,
            -1,
            state->np_model_matrix,
            asset_id,
            state->asset_ids.knp_palette,
            0,
            -1,
            shadowmap_mesh_flags,
            ShaderType::variance_shadow_map
        );
        */

        auto fb_nature_pack_debug = state->render_pipeline.framebuffers[state->pipeline_elements.fb_nature_pack_debug];
        ImGui::Image(
            (void *)(intptr_t)fb_nature_pack_debug.framebuffer._texture,
            {256, 256.0f * (float)fb_nature_pack_debug.framebuffer.size.y / (float)fb_nature_pack_debug.framebuffer.size.x},
            {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
        );
        ImGui::End();
    }

    MeshFromAssetFlags three_dee_mesh_flags_debug = three_dee_mesh_flags;
    //three_dee_mesh_flags_debug.debug = 1;
    //
    advance_armature(state, state->assets.descriptors + state->asset_ids.blocky_advanced_mesh, state->armatures + (uint32_t)ArmatureIds::test1, input->delta_t);
    advance_armature(state, state->assets.descriptors + state->asset_ids.blocky_advanced_mesh2, state->armatures + (uint32_t)ArmatureIds::test2, input->delta_t);

    PushMeshFromAsset(
        &state->three_dee_renderer.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::mesh_view_matrix,
        state->tree_model_matrix,
        state->asset_ids.blocky_advanced_mesh,
        state->asset_ids.blocky_advanced_texture,
        1,
        (int32_t)ArmatureIds::test1,
        three_dee_mesh_flags_debug,
        ShaderType::standard
    );

    PushMeshFromAsset(
        &state->shadowmap_renderer.list,
        (uint32_t)matrix_ids::light_projection_matrix,
        -1,
        state->tree_model_matrix,
        state->asset_ids.blocky_advanced_mesh,
        state->asset_ids.blocky_advanced_texture,
        0,
        (int32_t)ArmatureIds::test1,
        shadowmap_mesh_flags,
        ShaderType::variance_shadow_map
    );

    PushMeshFromAsset(
        &state->three_dee_renderer.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::mesh_view_matrix,
        state->tree_model_matrix2,
        state->asset_ids.blocky_advanced_mesh2,
        state->asset_ids.blocky_advanced_texture,
        1,
        (int32_t)ArmatureIds::test2,
        three_dee_mesh_flags_debug,
        ShaderType::standard
    );

    PushMeshFromAsset(
        &state->shadowmap_renderer.list,
        (uint32_t)matrix_ids::light_projection_matrix,
        -1,
        state->tree_model_matrix2,
        state->asset_ids.blocky_advanced_mesh2,
        state->asset_ids.blocky_advanced_texture,
        0,
        (int32_t)ArmatureIds::test2,
        shadowmap_mesh_flags,
        ShaderType::variance_shadow_map
    );

    {
        m4 model = m4mul(
            m4translate({0, 0, 0}),
            m4scale(1.0f)
        );

        PushMeshFromAsset(
            &state->three_dee_renderer.list,
            (uint32_t)matrix_ids::mesh_projection_matrix,
            (uint32_t)matrix_ids::mesh_view_matrix,
            model,
            state->asset_ids.dynamic_mesh_test,
            state->asset_ids.dynamic_texture_test,
            1,
            -1,
            three_dee_mesh_flags_debug,
            ShaderType::standard
        );

        PushMeshFromAsset(
            &state->shadowmap_renderer.list,
            (uint32_t)matrix_ids::light_projection_matrix,
            -1,
            model,
            state->asset_ids.dynamic_mesh_test,
            state->asset_ids.dynamic_texture_test,
            0,
            -1,
            shadowmap_mesh_flags,
            ShaderType::variance_shadow_map
        );
    }

    /*
    array2p<float> map = state->noisemap.array2p();

    v2 top_left = {
        -(map.width - 1) / 2.0f,
        (map.height - 1) / 2.0f,
    };

    auto &perlin = state->debug.perlin;
    for (int32_t y = 0; y < map.height; y++)
    {
        for (int32_t x = 0; x < map.width; x++)
        {
            m4 transform = m4translate(
                {
                    top_left.x + (float)x,
                    2 + roundf(cubic_bezier(
                        perlin.control_point_0,
                        perlin.control_point_1,
                        map.get({x,y})).y * perlin.height_multiplier) / 4,
                    top_left.y - (float)y,
                });
            m4 model = m4mul(transform, state->np_model_matrix);
            PushMeshFromAsset(&state->three_dee_renderer.list,
                (uint32_t)matrix_ids::mesh_projection_matrix,
                (uint32_t)matrix_ids::mesh_view_matrix,
                model,
                state->asset_ids.knp_plate_grass,
                state->asset_ids.knp_palette,
                1, // lights_mask
                -1, // armature
                three_dee_mesh_flags,
                ShaderType::standard
            );
            PushMeshFromAsset(&state->shadowmap_renderer.list,
                (uint32_t)matrix_ids::light_projection_matrix,
                -1,
                model,
                state->asset_ids.knp_plate_grass,
                state->asset_ids.knp_palette,
                0, // lights_mask
                -1, // armature
                shadowmap_mesh_flags,
                ShaderType::variance_shadow_map
            );
        }
    }
    */

    /*
    int32_t plate_grass_asset_id = -1;
    int32_t cliff_top_asset_id = -1;
    int32_t cliff_top_corner_asset_id = -1;
    int32_t cliff_asset_id = -1;
    int32_t cliff_corner_asset_id = -1;
    int32_t cliff_bottom_asset_id = -1;
    int32_t cliff_bottom_corner_asset_id = -1;
    int32_t large_oak_dark_asset_id = -1;
    int32_t large_oak_fall_asset_id = -1;
    int32_t large_oak_green_asset_id = -1;

    struct
    {
        const char *asset_name;
        int32_t *asset_id;
    } asset_names[] = {
        {
            "knp_Plate_Grass_01",
            &plate_grass_asset_id,
        },
        {
            "knp_Brown_Cliff_Top_01",
            &cliff_top_asset_id,
        },
        {
            "knp_Brown_Cliff_Top_Corner_01",
            &cliff_top_corner_asset_id,
        },
        {
            "knp_Brown_Cliff_01",
            &cliff_asset_id,
        },
        {
            "knp_Brown_Cliff_Corner_01",
            &cliff_corner_asset_id,
        },
        {
            "knp_Brown_Cliff_Bottom_01",
            &cliff_bottom_asset_id,
        },
        {
            "knp_Brown_Cliff_Bottom_Corner_01",
            &cliff_bottom_corner_asset_id,
        },
        {
            "knp_Large_Oak_Dark_01",
            &large_oak_dark_asset_id,
        },
        {
            "knp_Large_Oak_Fall_01",
            &large_oak_fall_asset_id,
        },
        {
            "knp_Large_Oak_Green_01",
            &large_oak_green_asset_id,
        },
    };

    for (uint32_t i = 0; i < state->assets.count; ++i)
    {
        for (uint32_t j = 0; j < harray_count(asset_names); ++j)
        {
            if (strcmp(state->assets.descriptors[i].asset_name, asset_names[j].asset_name) == 0)
            {
                 *(asset_names[j].asset_id) = (int32_t)i;
                 break;
            }
        }
    }

    const int32_t x_width = 3;
    int32_t x_start = -x_width;
    int32_t x_end = x_width;
    const int32_t z_width = 5;
    int32_t z_start = -z_width;
    int32_t z_end = z_width;

    const uint32_t num_foos = (
        (x_width * 2 + 1) * (z_width * 2 + 1) // top
        +
        (x_width * 2 + 1) * 2 + (z_width * 2 + 1) * 2 - 4 // cliff
        +
        (x_width * 2 + 1) * 2 + (z_width * 2 + 1) * 2 - 4 // bottom of cliff
        +
        5
    );
    AssetPlusTransform foo[num_foos];
    uint32_t foo_count = 0;

    for (int32_t x = x_start; x <= x_end; ++x)
    {
        for (int32_t z = z_start; z <= z_end; ++z)
        {
            auto &f = foo[foo_count++];
            f = tile_asset_and_transform(2, x, x_width, z, z_width, plate_grass_asset_id, cliff_top_asset_id, cliff_top_corner_asset_id);
        }
    }

    for (int32_t x = x_start; x <= x_end; ++x)
    {
        for (int32_t z = z_start; z <= z_end; ++z)
        {
            if (abs(x) == x_width || abs(z) == z_width)
            {
                auto &f = foo[foo_count++];
                f = tile_asset_and_transform(1, x, x_width, z, z_width, plate_grass_asset_id, cliff_asset_id, cliff_corner_asset_id);
            }
        }
    }

    for (int32_t x = x_start; x <= x_end; ++x)
    {
        for (int32_t z = z_start; z <= z_end; ++z)
        {
            if (abs(x) == x_width || abs(z) == z_width)
            {
                auto &f = foo[foo_count++];
                f = tile_asset_and_transform(0, x, x_width, z, z_width, plate_grass_asset_id, cliff_bottom_asset_id, cliff_bottom_corner_asset_id);
            }
        }
    }

    {
        auto &f = foo[foo_count++];
        float x = -x_width + 1.0f;
        float y = 2;
        float z = -z_width + 1.0f;
        m4 transform = m4translate({x, y, z});
        m4 np_translate = m4translate({0.5f,0.5f,0.5f});
        f.transform = m4mul(np_translate, transform);
        f.asset_id = large_oak_dark_asset_id;
    }

    {
        auto &f = foo[foo_count++];
        float x = x_width - 1.5f;
        float y = 2;
        float z = -z_width + 2.5f;
        m4 transform = m4translate({x, y, z});
        m4 a_scale = m4scale({1.0f, 1.5f, 1.0f});
        m4 np_translate = m4translate({0.5f,0.75f,0.5f});
        f.transform = m4mul(np_translate, m4mul(transform, a_scale));
        f.asset_id = large_oak_fall_asset_id;
    }

    {
        auto &f = foo[foo_count++];
        float x = x_width - 2.25f;
        float y = 2;
        float z = -z_width + 2.40f;
        m4 transform = m4translate({x, y, z});
        m4 a_scale = m4scale({0.75f, 1.0f, 0.75f});
        m4 np_translate = m4translate({0.375f,0.5f,0.375f});
        f.transform = m4mul(np_translate, m4mul(transform, a_scale));
        f.asset_id = large_oak_fall_asset_id;
    }

    {
        auto &f = foo[foo_count++];
        float x = x_width - 1.95f;
        float y = 2;
        float z = -z_width + 3.25f;
        m4 transform = m4translate({x, y, z});
        m4 a_scale = m4scale({0.75f, 1.0f, 0.75f});
        m4 np_translate = m4translate({0.375f,0.5f,0.375f});
        f.transform = m4mul(np_translate, m4mul(transform, a_scale));
        f.asset_id = large_oak_fall_asset_id;
    }

    {
        auto &f = foo[foo_count++];
        float x = x_width - 2.5f;
        float y = 2;
        float z = z_width - 1.5f;
        m4 transform = m4translate({x, y, z});
        m4 a_scale = m4scale({1.5f, 1.0f, 1.5f});
        m4 np_translate = m4translate({0.75f,0.5f,0.75f});
        f.transform = m4mul(np_translate, m4mul(transform, a_scale));
        f.asset_id = large_oak_green_asset_id;
    }

    for (uint32_t i = 0; i < foo_count; ++i)
    {
        auto &&asset_and_loc = foo[i];
        //m4 model = m4mul(asset_and_loc.transform, state->np_model_matrix);
        PushMeshFromAsset(&state->three_dee_renderer.list,
            (uint32_t)matrix_ids::mesh_projection_matrix,
            (uint32_t)matrix_ids::mesh_view_matrix,
            asset_and_loc.transform,
            asset_and_loc.asset_id,
            state->asset_ids.knp_palette,
            1,
            -1,
            three_dee_mesh_flags,
            ShaderType::standard
        );
        PushMeshFromAsset(&state->shadowmap_renderer.list,
            (uint32_t)matrix_ids::light_projection_matrix,
            -1,
            asset_and_loc.transform,
            asset_and_loc.asset_id,
            state->asset_ids.knp_palette,
            0,
            -1,
            shadowmap_mesh_flags,
            ShaderType::variance_shadow_map
        );
    }

    PushDebugTextureLoad(
        &state->three_dee_renderer.list,
        state->asset_ids.dynamic_texture_test
    );
    PushMeshFromAsset(&state->three_dee_renderer.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::mesh_view_matrix,
        state->plane_model_matrix,
        state->asset_ids.cube_mesh,
        state->asset_ids.dynamic_texture_test,
        1,
        -1,
        three_dee_mesh_flags,
        ShaderType::standard
    );
    PushMeshFromAsset(&state->shadowmap_renderer.list,
        (uint32_t)matrix_ids::light_projection_matrix,
        -1,
        state->plane_model_matrix,
        state->asset_ids.cube_mesh,
        state->asset_ids.cube_texture,
        0,
        -1,
        shadowmap_mesh_flags,
        ShaderType::variance_shadow_map
    );
    */

    {
        PushClear(&state->pipeline_elements.rl_sky.list, {1.0f, 1.0f, 0.4f, 1.0f});
        PushSky(&state->pipeline_elements.rl_sky.list,
            (int32_t)matrix_ids::mesh_projection_matrix,
            (int32_t)matrix_ids::mesh_view_matrix,
            v3normalize(light.direction));
    }

    PushQuad(&state->framebuffer_renderer.list, {0,0},
            {(float)input->window.width, (float)input->window.height},
            {1,1,1,1}, 0,
            state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_main].asset_descriptor
    );
    PushQuad(&state->framebuffer_renderer.list, {100,100},
            {200, 200},
            {1,1,1,1}, 0,
            state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_shadowmap].asset_descriptor
    );

    PushQuad(&state->framebuffer_renderer.list, {400,100},
            {200, 200},
            {1,1,1,1}, 0,
            state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_shadowmap_texarray].asset_descriptor
    );

    v3 mouse_bl = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y), 0.0f};
    v3 mouse_size = {16.0f, -16.0f, 0.0f};
    PushQuad(&state->framebuffer_renderer.list, mouse_bl, mouse_size, {1,1,1,1}, 0, state->asset_ids.mouse_cursor);

    /*
    {
        MeshFromAssetFlags flags = {};
        flags.depth_disabled = 1;
        PushMeshFromAsset(
            &state->three_dee_renderer.list,
            (uint32_t)matrix_ids::mesh_projection_matrix,
            (uint32_t)matrix_ids::mesh_view_matrix,
            state->cube_bounds_model_matrix,
            state->asset_ids.cube_bounds_mesh,
            state->asset_ids.white_texture,
            0,
            -1,
            flags,
            ShaderType::standard
        );
    }

    {
        MeshFromAssetFlags flags = {};
        flags.depth_disabled = 1;
        PushMeshFromAsset(
            &state->three_dee_renderer.list,
            (uint32_t)matrix_ids::mesh_projection_matrix,
            (uint32_t)matrix_ids::mesh_view_matrix,
            state->cube_bounds_model_matrix_2,
            state->asset_ids.cube_bounds_mesh,
            state->asset_ids.white_texture,
            0,
            -1,
            flags,
            ShaderType::standard
        );
    }
    */

    if (state->debug.show_textures) {
        ImGui::Begin("Textures", &state->debug.show_textures);

        auto &pipeline = state->render_pipeline;
        for (uint32_t i = 0; i < pipeline.framebuffer_count; ++i)
        {
            auto &rfb = pipeline.framebuffers[i];
            bool sameline = false;
            if (!rfb.framebuffer._flags.no_color_buffer && !rfb.framebuffer._flags.use_multisample_buffer)
            {
                ImGui::Image(
                    (void *)(intptr_t)rfb.framebuffer._texture,
                    {256, 256.0f * (float)rfb.framebuffer.size.y / (float)rfb.framebuffer.size.x},
                    {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
                );
                sameline = true;
            }
            if (rfb.framebuffer._flags.use_depth_texture && !rfb.framebuffer._flags.use_multisample_buffer)
            {
                if (sameline)
                {
                    ImGui::SameLine();
                }
                ImGui::Image(
                    (void *)(intptr_t)rfb.framebuffer._renderbuffer,
                    {256, 256.0f * (float)rfb.framebuffer.size.y / (float)rfb.framebuffer.size.x},
                    {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
                );
            }
        }
        ImGui::End();
    }

    {
        ImGui::DragFloat2(
            "Control point 0",
            &state->debug.perlin.control_point_0.x,
            0.01f,
            0.0f,
            1.0f,
            "%.02f"
        );
        ImGui::DragFloat2(
            "Control point 1",
            &state->debug.perlin.control_point_1.x,
            0.01f,
            0.0f,
            1.0f,
            "%.02f"
        );
        float values[100];
        v2 p1 = state->debug.perlin.control_point_0;
        v2 p2 = state->debug.perlin.control_point_1;
        for (uint32_t i = 0; i < harray_count(values); ++i)
        {
            float t = 1.0f / (harray_count(values) - 1) * i;
            values[i] = cubic_bezier(p1, p2, t).y;
        }
        ImGui::PlotLines(
            (const char *)"Perlin Cubic bezier",
            (const float *)values,
            100, // number of values
            0, // offset of first value
            (const char *)0,
            0.0f, // scale_min
            FLT_MAX, // scale_max
            ImVec2(600,600), // size
            sizeof(float)
        );
    }
}
