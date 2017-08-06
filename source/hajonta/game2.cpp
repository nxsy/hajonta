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

DEMO(demo_imgui)
{
    game_state *state = (game_state *)memory->game_block->base;
    ImGui::ShowTestWindow(&state->imgui.show_window);
}

template<uint32_t SIZE>
int32_t
add_asset(AssetDescriptors<SIZE> *asset_descriptors, const char *name, asset_descriptor_source_type source_type, bool debug)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        asset_descriptors->descriptors[asset_descriptors->count].source_type = asset_descriptor_source_type::name;
        asset_descriptors->descriptors[asset_descriptors->count].asset_name = name;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

template<uint32_t SIZE>
int32_t
add_asset(AssetDescriptors<SIZE> *asset_descriptors, const char *name, bool debug = false)
{
    return add_asset(asset_descriptors, name, asset_descriptor_source_type::name, debug);
}

template<uint32_t SIZE>
int32_t
add_dynamic_mesh_asset(AssetDescriptors<SIZE> *asset_descriptors, Mesh *ptr, bool debug = false)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        hassert(ptr->dynamic);
        asset_descriptors->descriptors[asset_descriptors->count].source_type = asset_descriptor_source_type::dynamic_mesh;
        asset_descriptors->descriptors[asset_descriptors->count].ptr = ptr;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

template<uint32_t SIZE>
int32_t
add_dynamic_texture_asset(AssetDescriptors<SIZE> *asset_descriptors, DynamicTextureDescriptor *ptr, bool debug = false)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        asset_descriptors->descriptors[asset_descriptors->count].source_type = asset_descriptor_source_type::dynamic_texture;
        asset_descriptors->descriptors[asset_descriptors->count].dynamic_texture_descriptor = ptr;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

template<uint32_t SIZE>
int32_t
add_framebuffer_asset(AssetDescriptors<SIZE> *asset_descriptors, FramebufferDescriptor *framebuffer, bool debug = false)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        asset_descriptors->descriptors[asset_descriptors->count].source_type = asset_descriptor_source_type::framebuffer;
        asset_descriptors->descriptors[asset_descriptors->count].framebuffer = framebuffer;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

template<uint32_t SIZE>
int32_t
add_framebuffer_depth_asset(AssetDescriptors<SIZE> *asset_descriptors, FramebufferDescriptor *framebuffer, bool debug = false)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        asset_descriptors->descriptors[asset_descriptors->count].source_type = asset_descriptor_source_type::framebuffer_depth;
        asset_descriptors->descriptors[asset_descriptors->count].framebuffer = framebuffer;
        result = (int32_t)asset_descriptors->count;
        ++asset_descriptors->count;
    }
    return result;
}

#include "hajonta/game2/pipeline.cpp"

void
show_debug_main_menu(demo_cowboy_state *state)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Debug"))
        {
            if (ImGui::BeginMenu("General"))
            {
                ImGui::MenuItem("Debug rendering", "", &state->debug->rendering);
                ImGui::MenuItem("Profiling", "", &state->debug->debug_system->show);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Movement"))
            {
                ImGui::MenuItem("Player", "", &state->debug->player_movement);
                ImGui::MenuItem("Familiar", "", &state->debug->familiar_movement);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Pathfinding"))
            {
                ImGui::MenuItem("Priority Queue", "", &state->debug->priority_queue.show);
                ImGui::MenuItem("Familiar Path", "", &state->debug->familiar_path.show);
                ImGui::EndMenu();
            }
            ImGui::MenuItem("Show lights", "", &state->debug->show_lights);
            ImGui::MenuItem("Show textures", "", &state->debug->show_textures);
            ImGui::MenuItem("Show camera", "", &state->debug->show_camera);
            ImGui::MenuItem("Show nature pack", "", &state->debug->show_nature_pack);
            ImGui::MenuItem("Show perlin", "", &state->debug->perlin.show);
            ImGui::MenuItem("Show armature", "", &state->debug->armature.show);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Game"))
        {
            ImGui::MenuItem("Memory arena", "", &state->debug->show_arena);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void
update_camera(CameraState *camera, float aspect_ratio, bool invert = false)
{
    float rho = camera->distance;
    float theta = camera->rotation.y;
    float phi = camera->rotation.x;

    float sign = invert ? -1.0f : 1.0f;

    camera->relative_location = {
        sinf(theta) * cosf(phi) * rho,
        sign * sinf(phi) * rho,
        cosf(theta) * cosf(phi) * rho,
    };

    v3 target = camera->target;
    target.y *= sign;

    camera->location = v3add(target, camera->relative_location);
    v3 up;
    if (camera->rotation.x < h_halfpi && camera->rotation.x > -h_halfpi)
    {
        up = {0, 1, 0};
    }
    else
    {
        up = {0, -1, 0};
    }

    camera->view = m4lookat(camera->location, target, up);
    if (camera->orthographic)
    {
        camera->projection = m4orthographicprojection(camera->near_, camera->far_, {-aspect_ratio * rho, -1.0f * rho}, {aspect_ratio * rho, 1.0f * rho});
    }
    else
    {
        if (camera->frustum)
        {
            static float foo = 1.0f;
            ImGui::DragFloat("Foo", &foo, 0.01f, 0.01f, 100.0f);
            float height = tan(camera->fov * 0.5f / (180.0f / h_pi)) * foo;
            float width = height * aspect_ratio;
            v2 dimension[] =
            {
                { width, height  },
                {-width,-height  },
            };
            camera->projection = m4frustumprojection(
                camera->near_,
                camera->far_,
                dimension[1],
                dimension[0]);
        }
        else
        {
            float fov_radians = camera->fov / (180.0f / h_pi);
            camera->projection = m4perspectiveprojection(
                camera->near_,
                camera->far_,
                aspect_ratio,
                fov_radians);
        }
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

struct
MeshProxy
{
    _vertexformat_1 *vertices;
    v3i *triangles;
    uint32_t *vertex_index;
    uint32_t *num_indices;
};

void
meshp_reset(const MeshProxy &meshp)
{
    *meshp.vertex_index = 0;
    *meshp.num_indices = 0;
}

rectangle2
uv_square = {
    { 64 / 512.0f, 64 / 512.0f },
    { 384 / 512.0f , 384  / 512.0f }
};

rectangle2
uv_line =
{
    { 128 / 512.0f, 65 / 512.0f },
    { 16 / 512.0f, 14 / 512.0f },
};

void
append_square(const MeshProxy &meshp, rectangle2 rect)
{
    int32_t base_vertex_index = (int32_t)*meshp.vertex_index;
    meshp.vertices[(*meshp.vertex_index)++] =
    {
        { rect.x,0,rect.y },
        { 0,1,0 },
        { uv_square.x, uv_square.y },
    };
    meshp.vertices[(*meshp.vertex_index)++] =
    {
        { rect.x,0, rect.y + rect.height },
        { 0,1,0 },
        { uv_square.x, uv_square.y + uv_square.height },
    };
    meshp.vertices[(*meshp.vertex_index)++] =
    {
        { rect.x + rect.width,0,rect.y + rect.height },
        { 0,1,0 },
        { uv_square.x + uv_square.width, uv_square.y + uv_square.height },
    };
    meshp.vertices[(*meshp.vertex_index)++] =
    {
        { rect.x + rect.width,0,rect.y },
        { 0,1,0 },
        { uv_square.x + uv_square.width, uv_square.y },
    };

    meshp.triangles[*meshp.num_indices] = {
        base_vertex_index,
        base_vertex_index + 1,
        base_vertex_index + 2,
    };

    (*meshp.num_indices) += 3;

    meshp.triangles[*meshp.num_indices] = {
        base_vertex_index + 2,
        base_vertex_index + 3,
        base_vertex_index,
    };

    (*meshp.num_indices) += 3;
}

void
append_line(const MeshProxy &meshp, line2 line, float width)
{
    v2 direction = line.direction;
    v2 normal = v2mul(v2normalize(v2{-direction.y, direction.x}), width);

    v2 from = line.position;
    v2 to = v2add(line.position, line.direction);

    v2 bl = v2sub(from, normal);
    v2 tl = v2add(from, normal);
    v2 br = v2sub(to, normal);
    v2 tr = v2add(to, normal);


    int32_t base_vertex_index = (int32_t)*meshp.vertex_index;
    meshp.vertices[(*meshp.vertex_index)++] =
    {
        { bl.x, 0, bl.y },
        { 0,1,0 },
        { uv_line.x, uv_line.y },
    };
    meshp.vertices[(*meshp.vertex_index)++] =
    {
        { tl.x,0, tl.y },
        { 0,1,0 },
        { uv_line.x, uv_line.y + uv_line.height },
    };
    meshp.vertices[(*meshp.vertex_index)++] =
    {
        { tr.x,0,tr.y },
        { 0,1,0 },
        { uv_line.x + uv_line.width, uv_line.y + uv_line.height },
    };
    meshp.vertices[(*meshp.vertex_index)++] =
    {
        { br.x,0, br.y},
        { 0,1,0 },
        { uv_line.x + uv_line.width, uv_line.y },
    };

    meshp.triangles[*meshp.num_indices] = {
        base_vertex_index,
        base_vertex_index + 1,
        base_vertex_index + 2,
    };
    (*meshp.num_indices) += 3;

    meshp.triangles[*meshp.num_indices] = {
        base_vertex_index + 2,
        base_vertex_index + 3,
        base_vertex_index,
    };
    (*meshp.num_indices) += 3;

}

void
append_arrow(const MeshProxy &meshp, line2 line, float width)
{
    append_line(meshp, line, width);

    v2 tip = v2add(line.position, line.direction);
    v2 tip_start = v2sub(
        tip,
        v2mul(v2normalize(line.direction), width * 3)
    );

    v2 normal = v2mul(v2normalize(v2{-line.direction.y, line.direction.x}), width * 3);

    v2 tip_left_start = v2add(tip_start, normal);
    v2 tip_left_direction = v2sub(tip, tip_left_start);
    float tip_left_length = v2length(tip_left_direction);
    tip_left_direction = v2mul(
        v2normalize(tip_left_direction),
        tip_left_length + width
    );
    line2 tip_left = {
        tip_left_start,
        tip_left_direction,
    };
    append_line(meshp, tip_left, width);

    v2 tip_right_start = v2sub(tip_start, normal);
    line2 tip_right = {
        tip_right_start,
        v2sub(tip, tip_right_start),
    };
    append_line(meshp, tip_right, width);
}

void
test_append(const MeshProxy &meshp, v2i target_tile)
{
    /*
    rectangle2 rect = { {-0.5, -0.5 }, {1, 1} };
    append_square(meshp, rect);
    rect = { {-1.5, -1.5 }, {1, 1} };
    append_square(meshp, rect);
    rect = { { 0.5,  0.5 }, {1, 1} };
    append_square(meshp, rect);
    */

    /*
    append_arrow(meshp, {{0,0}, {-0.8f,0.8f}}, 0.1f);
    append_arrow(meshp, {{-1,1}, {0,0.8f}}, 0.1f);
    append_arrow(meshp, {{-1,2}, {0,0.8f}}, 0.1f);
    append_arrow(meshp, {{-1,3}, {0,0.8f}}, 0.1f);
    append_arrow(meshp, {{-1,4}, {0,0.8f}}, 0.1f);
    append_arrow(meshp, {{-1,5}, {-0.8f,0.8f}}, 0.1f);
    append_arrow(meshp, {{-2,6}, {-0.8f,0.8f}}, 0.1f);
    append_arrow(meshp, {{-3,7}, {0,0.8f}}, 0.1f);
    */


    v2 target_tile_f = {
        (float)target_tile.x - TERRAIN_MAP_CHUNK_WIDTH/2 + 0.5f,
        (float)target_tile.y - TERRAIN_MAP_CHUNK_WIDTH/2 + 0.5f,
    };
    rectangle2 r = {
        target_tile_f,
        {1,1},
    };
    ImGui::Text("BBB: %.2f, %.2f", r.position.x, r.position.y);
    append_square(meshp, {{-3.5, 7.5}, {1,1}});
    append_square(meshp, r);
    append_square(meshp, {{-1.5, 7.5}, {1,1}});
    append_square(meshp, {{-3.5, 9.5}, {1,1}});
}


void
append_block_mesh(
    _vertexformat_1 *vertices,
    v3i *triangles,
    int32_t *vertex_index,
    uint32_t *num_triangles,
    v3 base_location,
    CornerHeights ch,
    v2 uv_base,
    v2 uv_size
    )
{
    v3 upper_top_left = { -0.5f, 0.1f + ch.top_left, 0.5f };
    v3 upper_top_right= { 0.5f, 0.1f + ch.top_right, 0.5f };
    v3 upper_bottom_left = { -0.5f, 0.1f + ch.bottom_left, -0.5f };
    v3 upper_bottom_right = { 0.5f, 0.1f + ch.bottom_right, -0.5f };

    v3 lower_top_left = { -0.5f, -10.f + ch.top_left, 0.5f };
    v3 lower_top_right = { 0.5f, -10.f + ch.top_right, 0.5f };
    v3 lower_bottom_left = { -0.5f, -10.0f + ch.bottom_left, -0.5f };
    v3 lower_bottom_right = { 0.5f, -10.0f + ch.bottom_right, -0.5f };

    triangle3 tris[] =
    {
        {
            upper_bottom_left,
            upper_top_left,
            upper_top_right,
        },
        {
            upper_bottom_left,
            upper_top_right,
            upper_bottom_right,
        },
    };

    for (uint32_t i = 0; i < harray_count(tris); ++i)
    {
        int32_t tri_base_vertex_index = *vertex_index;
        triangle3 &tri = tris[i];
        v3 normal = winded_triangle_normal(tri);

        for (uint32_t j = 0; j < 3; ++j)
        {
            v3 &offset = tri.p[j];
            v3 position = v3add(base_location, offset);
            v2 uv_offset = {
                (uv_base.x + offset.x * 0.1f) / uv_size.x,
                (uv_base.y - offset.z * 0.1f) / uv_size.y,
            };
            vertices[(*vertex_index)++] = {
                position,
                normal,
                uv_offset,
            };
        }
        triangles[(*num_triangles)++] = {
            tri_base_vertex_index,
            tri_base_vertex_index + 1,
            tri_base_vertex_index + 2,
        };

        tri_base_vertex_index = *vertex_index;
        for (uint32_t j = 0; j < 3; ++j)
        {
            v3 &offset = tri.p[j];
            v3 position = v3add(base_location, offset);
            position = v3add(position, {0,-0.00001f,0});
            v2 uv_offset = {
                (uv_base.x + offset.x * 0.1f) / uv_size.x,
                (uv_base.y - offset.z * 0.1f) / uv_size.y,
            };
            vertices[(*vertex_index)++] = {
                position,
                normal,
                uv_offset,
            };
        }
        triangles[(*num_triangles)++] = {
            tri_base_vertex_index + 2,
            tri_base_vertex_index + 1,
            tri_base_vertex_index + 0,
        };
    }
}

float
perturb_raw(float raw)
{
    return raw;
    //return fmod(raw, 0.001f) * 100.0f;
}

void
generate_heightmap(array2p<float> map, array2p<float> heights, float height_multiplier, v2 cp0, v2 cp1)
{
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
            heights.set({x,y}, base_height);
        }
    }
}


void
generate_terrain_mesh3(array2p<float> map, Mesh *mesh)
{
    v2 top_left = {
        -(map.width - 1.0f),
        (map.height - 1.0f),
    };
    v2 uv_size = {
        (float)map.width,
        (float)map.height,
    };

    _vertexformat_1 *vertices = (_vertexformat_1 *)mesh->vertices.data;
    v3i *triangles = (v3i *)mesh->indices.data;
    int32_t vertex_index = 0;
    uint32_t num_triangles = 0;

    for (int32_t y = 0; y < map.height - 1; ++y)
    {
        for (int32_t x = 0; x < map.width - 1; ++x)
        {
            v2 uv_base = {x + 0.5f, y + 0.5f};
            float base_height = map.get({x,y});

            CornerHeights ch = {};

            v3 base_location =
            {
                top_left.x + (2*x),
                base_height,
                top_left.y - (2*y),
            };

            append_block_mesh(
                vertices,
                triangles,
                &vertex_index,
                &num_triangles,
                base_location,
                ch,
                uv_base,
                uv_size);

            {
                float base_height_x1 = map.get({x+1,y});
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
                v2 x_uv_base = uv_base;
                x_uv_base.x += 0.25f;
                append_block_mesh(
                    vertices,
                    triangles,
                    &vertex_index,
                    &num_triangles,
                    x_base_location,
                    ch,
                    x_uv_base,
                    uv_size);
            }

            {
                float base_height_y1 = map.get({x,y+1});
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
                v2 y_uv_base = uv_base;
                y_uv_base.y += 0.25f;

                append_block_mesh(
                    vertices,
                    triangles,
                    &vertex_index,
                    &num_triangles,
                    y_base_location,
                    ch,
                    y_uv_base,
                    uv_size);
            }

            {
                ch = {
                    map.get({x,y}),
                    map.get({x+1,y}),
                    map.get({x,y+1}),
                    map.get({x+1,y+1}),
                };

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
                v2 xy_uv_base = uv_base;
                xy_uv_base.x += 0.25f;
                xy_uv_base.y += 0.25f;

                append_block_mesh(
                    vertices,
                    triangles,
                    &vertex_index,
                    &num_triangles,
                    xy_base_location,
                    ch,
                    xy_uv_base,
                    uv_size);
            }
        }
    }

    mesh->v3.vertex_count = (uint32_t)vertex_index;
    mesh->num_indices = 3 * num_triangles;
    mesh->reload = true;
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

    float max_height = 0;
    float max_amplitude = 1.0f;
    for (uint32_t i = 0; i < octaves; ++i)
    {
        max_height += max_amplitude;
        max_amplitude *= persistence;
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
                float sample_x = (x - half_width + offsets[i].x) / scale * frequency;
                float sample_y = (y - half_height + offsets[i].y) / scale * frequency;

                float perlin_value = stb_perlin_noise3(sample_x, sample_y, 0, 256, 256, 256);
                height += perlin_value * amplitude;

                amplitude *= persistence;
                frequency *= lacunarity;
            }
            height /= max_height;
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
            if (value < 0.0f)
            {
                value -= 0.05f;
            }
            if (value > 0.0f)
            {
                value += 0.05f;
            }
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
                    TerrainTypeInfo *terrain = 0; // terrains;
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
                    /*
                    if (terrain->merge_with_previous)
                    {
                        TerrainTypeInfo *previous_terrain = terrain - 1;

                        float t = (height - previous_terrain->height) / (terrain->height - previous_terrain->height);
                        color = lerp(previous_terrain->color, terrain->color, t);
                    }
                    */
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

MeshBoneDescriptor
blend_transforms(uint32_t num_transforms, MeshBoneDescriptor *transforms, float *weights)
{
    if (num_transforms == 1)
    {
        return transforms[0];
    }

    float total_weight = 0;
    for (uint32_t i = 0; i < num_transforms; ++i)
    {
        total_weight += weights[i];
    }

    float my_weight = weights[0] / total_weight;
    MeshBoneDescriptor result = transforms[0];
    MeshBoneDescriptor rest = blend_transforms(num_transforms - 1, transforms + 1, weights + 1);

    result.scale = lerp(rest.scale, result.scale, my_weight);
    result.translate = lerp(rest.translate, result.translate, my_weight);
    result.q = quatmix(rest.q, result.q, my_weight);

    return result;
}

MeshBoneDescriptor
transform_for_tick(V3Animation *animation, float tick, uint32_t num_bones, int32_t bone)
{
    float t0 = floorf(tick);
    float t1 = ceilf(tick);
    uint32_t t0i = (uint32_t)t0 % animation->num_ticks;
    uint32_t t1i = (uint32_t)t1 % animation->num_ticks;
    MeshBoneDescriptor ds[] = {
        animation->animation_ticks[t0i * num_bones + bone].transform,
        animation->animation_ticks[t1i * num_bones + bone].transform,
    };
    float weights[] = {
        t1 - tick,
        tick - t0,
    };
    return blend_transforms(2, &ds[0], &weights[0]);
}

void
advance_armature(asset_descriptor *asset, ArmatureDescriptor *armature, float delta_t, float speed, float time_to_idle, bool *debug = 0)
{
    if (asset->load_state != 2)
    {
        return;
    }

    bool interpolate = false;
    float interp_weight = 0;

    auto &v3bones = *asset->v3bones;

    if (speed == 0 || time_to_idle < 0.3f)
    {
        if (armature->stance != v3bones.idle_animation)
        {
            armature->previous_stance = armature->stance;
            armature->previous_stance_weight = 1;
            armature->previous_stance_tick = armature->tick;
        }
        armature->stance = v3bones.idle_animation;

        if (armature->previous_stance_weight || time_to_idle < 0.3f)
        {
            interpolate = true;
            interp_weight = armature->previous_stance_weight;

            if (time_to_idle < 0.3f)
            {
                armature->previous_stance_weight = time_to_idle / 0.3f;
            }
            else
            {
                armature->previous_stance_weight = max(0, armature->previous_stance_weight - 5 * delta_t);
            }
        }
    }
    else
    {
        if (armature->stance != v3bones.walk_animation)
        {
            armature->previous_stance = armature->stance;
            armature->previous_stance_weight = 1;
            armature->tick = 12;
        }
        armature->stance = v3bones.walk_animation;
        if (armature->previous_stance_weight)
        {
            interpolate = true;
            interp_weight = armature->previous_stance_weight;
            armature->previous_stance_weight = max(0, armature->previous_stance_weight - 5 * delta_t);
        }
    }

    auto &animation = v3bones.animations[armature->stance];
    V3Animation *interp_animation = 0;
    if (interpolate)
    {
        interp_animation = v3bones.animations + armature->previous_stance;
    }

    ImGui::Begin("Animation");
    ImGui::Text("Animation: %s", v3bones.animation_names[armature->stance]);
    ImGui::Text("Time to idle: %f", time_to_idle);
    if (interpolate)
    {
        ImGui::Text("Interp Animation: %s", v3bones.animation_names[armature->previous_stance]);
        ImGui::Text("Interp Weight: %f", interp_weight);
    }
    ImGui::End();

    bool opened_debug = debug ? *debug : false;
    if (opened_debug)
    {
        char label[100];
        sprintf(label, "Armature Animation##%p", armature);
        ImGui::Begin(label, debug);

        ImGui::Checkbox("Animation proceed", &armature->halt_time);
        if (ImGui::Button("Reset to start"))
        {
            armature->tick = 0;
        }
        ImGui::Text("Tick %d of %d",
            (uint32_t)armature->tick % animation.num_ticks,
            animation.num_ticks - 1);
    }
    if (!armature->halt_time)
    {
        armature->tick += delta_t * 24.0f * speed;
        if (interpolate)
        {
            armature->previous_stance_tick += delta_t * 24.0f * speed;
        }
    }
    int32_t first_bone = 0;
    for (uint32_t i = 0; i < v3bones.num_bones; ++i)
    {
        if (v3bones.bone_parents[i] == -1)
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

        int32_t parent_bone = v3bones.bone_parents[bone];
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
            sprintf(label, "%*s %s", parent_list_location * 2, "", v3bones.bone_names[bone].name);
            auto size = ImGui::CalcTextSize("%s", label);
            ImGui::PushItemWidth(size.x);
            ImGui::Text("%s", label);
            ImGui::PopItemWidth();
            ImGui::NextColumn();
        }

        MeshBoneDescriptor &d = armature->bone_descriptors[bone];

        if (!armature->halt_time)
        {
            d = transform_for_tick(&animation, armature->tick, v3bones.num_bones, bone);
            if (interpolate)
            {
                MeshBoneDescriptor ds[] =
                {
                    d,
                    transform_for_tick(interp_animation, armature->previous_stance_tick, v3bones.num_bones, bone),
                };
                float weights[] =
                {
                    1 - armature->previous_stance_weight,
                    armature->previous_stance_weight,
                };
                d = blend_transforms(2, &ds[0], &weights[0]);
            }
        }
        if (d.scale.x == 0)
        {
            d.scale = v3bones.default_transforms[bone].scale;
            d.translate = v3bones.default_transforms[bone].translate;
            d.q = v3bones.default_transforms[bone].q;
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
            m4 bone_offset = v3bones.bone_offsets[bone];
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

        armature->bones[bone] = m4mul(global_transform, v3bones.bone_offsets[bone]);

        for (uint32_t i = 0; i < v3bones.num_bones; ++i)
        {
            if (v3bones.bone_parents[i] == bone)
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

Ray
screen_to_ray(demo_cowboy_state *state)
{
    auto &window = state->frame_state.input->window;
    auto &mouse = state->frame_state.input->mouse;
    v4 clip_coordinates =
    {
        2.0f * (mouse.x) / window.width - 1.0f,
        1.0f - 2.0f * (mouse.y) / window.height,
        -1.0f,
        0.0f,
    };

    v4 eyespace_ray = m4mul(m4inverse(state->camera.projection), clip_coordinates);
    eyespace_ray.z = -1.0f;
    eyespace_ray.w = 0.0f;

    v4 worldspace_ray4 = m4mul(m4inverse(state->camera.view), eyespace_ray);
    v3 worldspace_ray3 = v3normalize({
        worldspace_ray4.x,
        worldspace_ray4.y,
        worldspace_ray4.z,
    });

    return {
        state->camera.relative_location,
        worldspace_ray3,
    };
}

v2
world_to_screen(demo_cowboy_state *state, v3 pos)
{
    v4 pos4 = {pos.x, pos.y, pos.z, 1};
    v4 view_space = m4mul(state->camera.view, pos4);
    v4 clip = m4mul(state->camera.projection, view_space);
    clip = v4div(clip, clip.w);

    v2 clip2 = v2add({0.5f,0.5f}, v2mul({clip.x, -clip.y}, 0.5f));

    auto &window = state->frame_state.input->window;
    v2 screen_coords = {
        clip2.x * window.width,
        clip2.y * window.height,
    };
    return screen_coords;
}

void
add_mesh_to_render_lists(demo_cowboy_state *state, m4 model, int32_t mesh, int32_t texture, int32_t armature)
{
    MeshFromAssetFlags shadowmap_mesh_flags = {};
    shadowmap_mesh_flags.cull_front = state->debug->cull_front ? (uint32_t)1 : (uint32_t)0;
    MeshFromAssetFlags three_dee_mesh_flags = {};
    three_dee_mesh_flags.attach_shadowmap = 1;
    three_dee_mesh_flags.attach_shadowmap_color = 1;

    PushMeshFromAsset(
        &state->pipeline_elements.rl_three_dee.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::mesh_view_matrix,
        model,
        mesh,
        texture,
        1,
        armature,
        three_dee_mesh_flags,
        ShaderType::standard
    );

    PushMeshFromAsset(
        &state->pipeline_elements.rl_shadowmap.list,
        (uint32_t)matrix_ids::light_projection_matrix,
        -1,
        model,
        mesh,
        texture,
        0,
        armature,
        shadowmap_mesh_flags,
        ShaderType::variance_shadow_map
    );

    PushMeshFromAsset(
        &state->pipeline_elements.rl_reflection.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::reflected_mesh_view_matrix,
        model,
        mesh,
        texture,
        1,
        armature,
        three_dee_mesh_flags,
        ShaderType::standard
    );

    PushMeshFromAsset(
        &state->pipeline_elements.rl_refraction.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::mesh_view_matrix,
        model,
        mesh,
        texture,
        1,
        armature,
        three_dee_mesh_flags,
        ShaderType::standard
    );
}

float
height_for(astar_data *data, v2i tile)
{
    array2<NOISE_WIDTH, NOISE_HEIGHT, float> *map = (array2<NOISE_WIDTH, NOISE_HEIGHT, float> *)data->map;
    v2 noise_location_f = {
        (float)tile.x / 2,
        NOISE_HEIGHT - (float)tile.y / 2 - 1,
    };

    v2i noise_location = {
        (int32_t)floorf(noise_location_f.x),
        (int32_t)floorf(noise_location_f.y),
    };

    bool lerp_x = fmod(noise_location_f.x, 1.0f) > 0.0f;
    bool lerp_y = fmod(noise_location_f.y, 1.0f) > 0.0f;

    float height = map->get(noise_location);
    uint32_t num_heights = 1;

    if (lerp_x)
    {
        v2i noise_location_x = v2add(noise_location, {1,0});
        height += map->get(noise_location_x);
        ++num_heights;
    }

    if (lerp_y)
    {
        v2i noise_location_y = v2add(noise_location, {0,1});
        height += map->get(noise_location_y);
        ++num_heights;
    }

    if (lerp_x && lerp_y)
    {
        v2i noise_location_xy = v2add(noise_location, {1,1});
        height += map->get(noise_location_xy);
        ++num_heights;
    }

    height /= num_heights;
    height += 0.1f;
    return height;
}

bool
noise_passable(astar_data *data, v2i current, v2i neighbour)
{
#if 0
    return true;
#else
    float height = height_for(data, neighbour);
    return height >= 0;
#endif
}

float
initial_cost_estimate(astar_data *data, v2i tile_start, v2i tile_end)
{
    v2 movement = {(float)(tile_start.x - tile_end.x), (float)(tile_start.y - tile_end.y)};
    return v2length(movement);
}

float
neighbour_cost_estimate(astar_data *data, v2i tile_start, v2i tile_end)
{
    v2 movement = {(float)(tile_start.x - tile_end.x), (float)(tile_start.y - tile_end.y)};
    float length = v2length(movement);

    float current_height = height_for(data, tile_start);
    float neighbour_height = height_for(data, tile_end);

    float diff = abs(current_height - neighbour_height);
    return pow(length + diff * 2.0f, 3.0f);
}

void
do_path_stuff(demo_cowboy_state *state, Pathfinding *path, v2i cowboy_location2, v2i target_tile)
{
    if (!path->initialized || !v2iequal(path->data.end_tile, target_tile))
    {
        path->initialized = true;
        path->data.astar_passable = noise_passable;
        path->data.initial_cost_estimate = initial_cost_estimate;
        path->data.neighbour_cost_estimate = neighbour_cost_estimate;
        queue_init(
            &path->data.queue,
            harray_count(path->data.entries),
            path->data.entries
        );
        astar_start(&path->data, &state->heightmaps[MESH_SQUARE / 2], cowboy_location2, target_tile);
    }

    bool next_step = true;
    bool single_step = false;
    if (path->show)
    {
        ImGui::Begin("Path", &path->show);

        ImGui::Text("Current location: %d, %d", cowboy_location2.x, cowboy_location2.y);
        ImGui::Text("Target location: %d, %d", target_tile.x, target_tile.y);

        if (ImGui::Button("Restart"))
        {
            path->initialized = false;
        }

        ImGui::Checkbox("Single step", &path->single_step);

        if (path->single_step)
        {
            single_step = true;
            next_step = false;
            if (ImGui::Button("Next iteration"))
            {
                next_step = true;
            }
        }
        bool _c = path->data.completed;
        ImGui::Checkbox("Completed", &_c);
        bool _p = path->data.found_path;
        ImGui::Checkbox("Path found", &_p);
        ImGui::End();
    }

    if (next_step && !path->data.completed)
    {
        astar(&path->data, single_step);
    }

    if (path->show)
    {
        char label[64];
        sprintf(label, "Path##%p", path);
        ImGui::Begin(label, &path->show);
        ImGui::InputTextMultiline("##source", path->data.log, path->data.log_position, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);

        auto &queue = path->data.queue;
        ImGui::Text("Queue has %d of %d entries", queue.num_entries, queue.max_entries);
        for (uint32_t i = 0; i < queue.num_entries; ++i)
        {
            auto &entry = queue.entries[i];
            ImGui::Text("%d: Score %f, Tile %d, %d", i, entry.score, entry.tile_position.x, entry.tile_position.y);
        }

        ImGui::Text("Found path: %d", path->data.found_path);

        ImGui::Text("Path has %d entries", path->data.path_length);
        for (uint32_t i = 0; i < path->data.path_length; ++i)
        {
            v2i *current = path->data.path + i;
            ImGui::Text("%d: %d,%d", i, current->x, current->y);
        }
        ImGui::End();
    }

    ImGuiWindowFlags imgui_flags = 0 |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoInputs |
        0;

    if (path->show)
    {
        static bool lt = true;
        static float threshold = 0.0f;
        ImGui::Checkbox("Less than", &lt);
        ImGui::DragFloat("Threshold", &threshold, 0.05f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
        /*
        for (int32_t y = 0; y < TERRAIN_MAP_CHUNK_HEIGHT; ++y)
        {
            for (int32_t x = 0; x < TERRAIN_MAP_CHUNK_WIDTH; ++x)
            {
                v2 window_location = world_to_screen(
                    state,
                    {
                        (float)x - 32,
                        0,
                        (float)y - 32
                    });

                auto &window = state->frame_state.input->window;
                float height = height_for(&path->data, {x, y});
                //height += 0.1f;
                if ((height > threshold) ? lt : !lt)
                {
                    continue;
                }
                if (window_location.x < 0)
                {
                    continue;
                }
                if (window_location.x >= window.width)
                {
                    continue;
                }
                if (window_location.y < 0)
                {
                    continue;
                }
                if (window_location.y >= window.height)
                {
                    continue;
                }
                ImVec2 text_size = ImGui::CalcTextSize("11,11: -0.02");
                text_size *= 1.1f;
                ImVec2 window_position = {
                    window_location.x - text_size.x / 2.0f,
                    window_location.y - text_size.y / 2.0f + 10,
                };
                ImGui::SetNextWindowPos(window_position);
                ImGui::SetNextWindowSize(text_size);
                int32_t i = 5000 + (y * TERRAIN_MAP_CHUNK_WIDTH + x);
                ImGui::PushID((void *)(intptr_t)i);
                char label[100];
                sprintf(label, "Overlay coord##%d", i);
                ImGui::Begin(label, 0, imgui_flags);
                ImGui::Text("%d,%d: %0.2f", x, y, height);
                ImGui::End();
                ImGui::PopID();
            }
        }
        */

        auto &queue = path->data.queue;
        for (uint32_t i = 0; i < queue.num_entries; ++i)
        {
            auto &entry = queue.entries[i];
            v2 window_location = world_to_screen(
                state,
                {
                    (float)entry.tile_position.x - 32,
                    0,
                    (float)entry.tile_position.y - 32
                });

            ImVec2 text_size = ImGui::CalcTextSize("11, 11: 11.11\n11: 11.11");
            text_size *= 1.1f;
            ImVec2 window_position = {
                window_location.x - text_size.x / 2.0f,
                window_location.y - text_size.y / 2.0f - 10,
            };
            ImGui::SetNextWindowPos(window_position);
            ImGui::SetNextWindowSize(text_size);
            ImGui::PushID((void *)(uintptr_t)i);
            char label[100];
            sprintf(label, "Overlay##%d", i);
            ImGui::Begin(label, 0, imgui_flags);
            ImGui::Text("%d,%d: %.2f\n%d: %.2f",
                entry.tile_position.x, entry.tile_position.y, height_for(&path->data, entry.tile_position),
                i, entry.score);
            ImGui::End();
            ImGui::PopID();
        }
        //float height = ImGui::GetItemsLineHeightWithSpacing();

        ImGui::PopStyleColor();
    }

    auto &mesh = state->ui_mesh;
    MeshProxy meshp = {
        (_vertexformat_1 *)mesh.vertices.data,
        (v3i *)mesh.indices.data,
        &mesh.v3.vertex_count,
        &mesh.num_indices,
    };
    m4 model = m4identity();
    MeshFromAssetFlags flags = {};
    flags.depth_disabled = 1;
    meshp_reset(meshp);

    v2 target_tile_f = {
        (float)target_tile.x - TERRAIN_MAP_CHUNK_WIDTH/2 - 0.5f,
        (float)target_tile.y - TERRAIN_MAP_CHUNK_WIDTH/2 - 0.5f,
    };
    rectangle2 r = {
        target_tile_f,
        {1,1},
    };
    append_square(meshp, r);

    if (path->data.found_path)
    {
        v2 previous_location = {
            (float)cowboy_location2.x - TERRAIN_MAP_CHUNK_WIDTH/2,
            (float)cowboy_location2.y - TERRAIN_MAP_CHUNK_HEIGHT/2,
        };
        auto *path_next_tile = path->data.path + path->data.path_length - 1;
        while (path_next_tile >= path->data.path)
        {
            ImGui::Text("Next tile in path: %d, %d", path_next_tile->x, path_next_tile->y);
            v2 next_location = {
                (float)path_next_tile->x - TERRAIN_MAP_CHUNK_WIDTH/2,
                (float)path_next_tile->y - TERRAIN_MAP_CHUNK_HEIGHT/2,
            };
            v2 direction = v2mul(v2sub(next_location, previous_location), 0.8f);
            line2 line = {previous_location, direction};
            append_arrow(meshp, line, 0.1f);
            --path_next_tile;
            previous_location = next_location;
        }
    }

    meshp_reset(meshp);
    mesh.reload = true;

    PushMeshFromAsset(
        &state->pipeline_elements.rl_three_dee_debug.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::mesh_view_matrix,
        model,
        state->ui_mesh_descriptor,
        state->asset_ids->square_texture,
        0,
        -1,
        flags,
        ShaderType::standard
    );

}

struct
Motion
{
    v2 movement;
    float rotation;
};

Motion
direction_to_motion(v2 direction, float current_rotation, float speed)
{
    v2 normalized_direction = v2normalize(direction);
    v2 movement = v2mul(normalized_direction, speed);
    float target_rotation = -atan2(normalized_direction.x, normalized_direction.y);

    float rotation_diff = target_rotation - current_rotation;

    while (rotation_diff > h_pi)
    {
        rotation_diff -= h_twopi;
    }
    while (rotation_diff < -h_pi)
    {
        rotation_diff += h_twopi;
    }

    if (abs(rotation_diff) > h_halfpi)
    {
        rotation_diff /= 20;
        movement = v2mul(movement, 0);
    }
    else if (abs(rotation_diff) > h_pi / 3)
    {
        rotation_diff /= 15;
        movement = v2mul(movement, 0.50f);
    }
    else if (abs(rotation_diff) > h_pi / 6)
    {
        rotation_diff /= 15;
        movement = v2mul(movement, 0.75f);
    }
    else if (abs(rotation_diff) > h_pi / 12)
    {
        rotation_diff /= 10;
        movement = v2mul(movement, 0.90f);
    }
    else if (abs(rotation_diff) > h_pi / 24)
    {
        rotation_diff /= 5;
        movement = v2mul(movement, 0.95f);
    }
    return Motion{movement, rotation_diff};
}

void
arena_debug(game_state *state, bool *show)
{
    if (ImGui::Begin("Game memory arenas", show))
    {
        ImGui::Columns(3, "mycolumns");
        ImGui::Separator();
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("Size"); ImGui::NextColumn();
        ImGui::Text("Code"); ImGui::NextColumn();
        ImGui::Separator();
        MemoryBlockDebugList *list = state->arena.debug_list;
        while (list)
        {
            for (uint32_t i = 0; i < list->num_data; ++i)
            {
                MemoryBlockDebugData *data = list->data + i;
                ImGui::Text("%s", data->label);
                ImGui::NextColumn();
                ImGui::Text("% 12d", data->size);
                ImGui::NextColumn();
                ImGui::Text("%64s", data->guid);
                ImGui::NextColumn();
            }
            list = list->next;
        }
        ImGui::Columns(1);
        ImGui::Separator();
    }
    ImGui::End();
}

void
apply_path(
    demo_cowboy_state *state,
    Pathfinding *path,
    v2i *cowboy_location2,
    v3 *cowboy_location,
    float *rotation,
    v2i *target_tile,
    int32_t mesh_id,
    ArmatureIds armature_id,
    float delta_t
    )
{
    float cowboy_speed = 0;
    float time_to_idle = 0;
    static float playback_speed = 1.0f;
    ImGui::DragFloat("Playback speed", &playback_speed, 0.01f, 0.01f, 10.0f, "%.2f");

    if (path->data.found_path)
    {
        v2 previous_location = {
            (float)cowboy_location2->x - TERRAIN_MAP_CHUNK_WIDTH/2,
            (float)cowboy_location2->y - TERRAIN_MAP_CHUNK_HEIGHT/2,
        };

        auto *path_next_tile = path->data.path + path->data.path_length - 1;
        v2 next_location = {};
        bool have_next_location = false;
        while (path_next_tile >= path->data.path)
        {
            next_location = {
                (float)path_next_tile->x - TERRAIN_MAP_CHUNK_WIDTH/2,
                (float)path_next_tile->y - TERRAIN_MAP_CHUNK_HEIGHT/2,
            };
            if (v2equal(previous_location, next_location))
            {
                --path->data.path_length;
                --path_next_tile;
                continue;
            }
            have_next_location = true;
            break;
        }

        if (have_next_location)
        {
            v2 direction = v2sub(
                next_location,
                {cowboy_location->x, cowboy_location->z});

            Motion motion = direction_to_motion(
                direction,
                *rotation,
                delta_t * playback_speed * 3);
            *cowboy_location = v3add(
                *cowboy_location,
                {
                    motion.movement.x,
                    0,
                    motion.movement.y
                });
            *rotation += motion.rotation;
            cowboy_speed = v2length(motion.movement) / (delta_t * playback_speed);
            time_to_idle = FLT_MAX;
        }
    }
    if (!path->data.found_path)
    {
        *target_tile = *cowboy_location2;
    }
    if (v2iequal(*cowboy_location2, *target_tile))
    {
        v2 direction = v2sub(
            v2{
                (float)target_tile->x - TERRAIN_MAP_CHUNK_WIDTH/2,
                (float)target_tile->y - TERRAIN_MAP_CHUNK_HEIGHT/2,
            },
            v2{cowboy_location->x, cowboy_location->z}
        );
        float direction_length = v2length(direction);
        if (direction_length > 0.1)
        {
            Motion motion = direction_to_motion(
                direction,
                *rotation,
                delta_t * playback_speed * 2);
            *cowboy_location = v3add(
                *cowboy_location,
                {
                    motion.movement.x,
                    0,
                    motion.movement.y
                });
            *rotation += motion.rotation;
            float movement_length = v2length(motion.movement);
            cowboy_speed = movement_length / (delta_t * playback_speed);
            time_to_idle = direction_length / movement_length * delta_t * playback_speed;
        }
    }

    advance_armature(state->assets->descriptors + mesh_id, state->armatures + (uint32_t)armature_id, delta_t * playback_speed, cowboy_speed, time_to_idle);
}


struct _ObjectData
{
    v3 position;
    v3 rotation;
    v3 scale;
    uint32_t depth_disabled;
    int32_t model_id;
    int32_t material;
    int32_t object_id;
};

struct
demo_a_state
{
    MemoryArena arena;
    bool initialized;

    FrameState frame_state;

    AssetDescriptors<64> assets;

    RenderPipeline render_pipeline;

    RenderPipelineRendererId r_m;
    _render_list<2 * 1024 * 1024> rl_m;

    RenderPipelineRendererId r_mouse;
    _render_list<2 * 1024 * 1024> rl_mouse;

    RenderPipelineRendererId r_mousepicking;
    _render_list<2 * 1024 * 1024> rl_mousepicking;

    RenderPipelineFramebufferId fb_mousepicking;

    RenderPipelineRendererId r_outline;
    _render_list<2 * 1024 * 1024> rl_outline;

    RenderPipelineRendererId r_outline_blur_x;
    _render_list<2 * 1024 * 1024> rl_outline_blur_x;

    RenderPipelineRendererId r_outline_blur_y;
    _render_list<2 * 1024 * 1024> rl_outline_blur_y;

    RenderPipelineRendererId r_outline_sobel;
    _render_list<2 * 1024 * 1024> rl_outline_sobel;

    RenderPipelineRendererId r_outline_draw;
    _render_list<2 * 1024 * 1024> rl_outline_draw;

    RenderPipelineFramebufferId fb_outline;
    RenderPipelineFramebufferId fb_outline_blur_x;
    RenderPipelineFramebufferId fb_outline_blur_y;
    RenderPipelineFramebufferId fb_outline_sobel;

    bool show_camera;
    CameraState camera;
    m4 matrices[3];

    bool show_lights;
    LightDescriptor lights[1];
    v2i middle_base_location;
    v2i right_base_location;
    v2 sun_rotation;

    int32_t ground_plane;
    int32_t diffuse46;
    int32_t normal46;
    int32_t specular46;

    int32_t barrel_mesh;
    int32_t barrel_diffuse;
    int32_t barrel_normal;
    int32_t barrel_specular;

    int32_t metal_barrel_mesh;
    int32_t metal_barrel_diffuse;
    int32_t metal_barrel_normal;
    int32_t metal_barrel_specular;

    int32_t cactus_mesh;
    int32_t cactus_diffuse;

    int32_t modular_building_brick_door_mesh;
    int32_t modular_building_brick_wall_mesh;
    int32_t modular_building_brick_small_window_mesh;
    int32_t modular_building_diffuse;
    int32_t modular_building_normal;
    int32_t modular_building_specular;

    int32_t mouse_cursor;

    Material materials[5];
    ReadPixelResult readpixel_result;

#define DEMO_A_TEST_MESH_COUNT 1
    Mesh test_meshes[DEMO_A_TEST_MESH_COUNT];
    int32_t test_mesh_descriptors[DEMO_A_TEST_MESH_COUNT];

    _ObjectData objects[25];
    uint32_t num_objects;

};

DEMO(demo_a)
{
    game_state *gs = (game_state *)memory->game_block->base;
    if (!gs->demo_a_block)
    {
        gs->demo_a_block = _platform->allocate_memory("demo_a", 256 * 1024 * 1024);
        bootstrap_memory_arena(gs->demo_a_block, demo_a_state, arena);
    }
    demo_a_state *state = (demo_a_state *)gs->demo_a_block->base;
    auto &light = state->lights[0];
    if (!state->initialized)
    {
        state->initialized = 1;
        state->r_m = RenderPipelineAddRenderer(&state->render_pipeline);
        RenderPipelineEntry *m = state->render_pipeline.entries + state->r_m;
        *m = {
            &state->rl_m.list,
            state->rl_m.buffer,
            sizeof(state->rl_m.buffer),
            -1,
            -1,
        };
        state->rl_m.list.name = DEBUG_NAME("m");

        state->fb_outline = RenderPipelineAddFramebuffer(&state->render_pipeline);
        state->fb_outline_blur_x = RenderPipelineAddFramebuffer(&state->render_pipeline);
        state->fb_outline_blur_y = RenderPipelineAddFramebuffer(&state->render_pipeline);
        state->fb_outline_sobel = RenderPipelineAddFramebuffer(&state->render_pipeline);
        auto *fb_outline = state->render_pipeline.framebuffers + state->fb_outline;
        fb_outline->half_size = 1;
        auto *fb_outline_blur_x = state->render_pipeline.framebuffers + state->fb_outline_blur_x;
        fb_outline_blur_x->half_size = 1;
        auto *fb_outline_blur_y = state->render_pipeline.framebuffers + state->fb_outline_blur_y;
        fb_outline_blur_y->half_size = 1;
        auto *fb_outline_sobel = state->render_pipeline.framebuffers + state->fb_outline_sobel;
        fb_outline_sobel->half_size = 1;
        fb_outline_sobel->clear_color = { 1, 1, 1, 0 };

        state->r_outline = RenderPipelineAddRenderer(&state->render_pipeline);
        RenderPipelineEntry *outline = state->render_pipeline.entries + state->r_outline;
        *outline = {
            &state->rl_outline.list,
            state->rl_outline.buffer,
            sizeof(state->rl_outline.buffer),
            state->fb_outline,
            -1,
        };
        state->rl_outline.list.name = DEBUG_NAME("outline");
        state->rl_outline.list.config.use_color = 1;
        state->rl_outline.list.config.color = {1.0f, 0.2f, 0.2f, 1.0f};

        ApplyFilterArgs args = {};

        state->r_outline_sobel = RenderPipelineAddRenderer(&state->render_pipeline);
        RenderPipelineEntry *outline_sobel = state->render_pipeline.entries + state->r_outline_sobel;
        args = {};
        *outline_sobel = {
            &state->rl_outline_sobel.list,
            state->rl_outline_sobel.buffer,
            sizeof(state->rl_outline_sobel.buffer),
            state->fb_outline_sobel,
            state->fb_outline,
            ApplyFilterType::sobel,
            args,
        };
        state->rl_outline_sobel.list.name = DEBUG_NAME("outline_sobel");
        PipelineAddDependency(&state->render_pipeline, state->r_outline_sobel, state->r_outline);


        state->r_outline_blur_x = RenderPipelineAddRenderer(&state->render_pipeline);
        RenderPipelineEntry *outline_blur_x = state->render_pipeline.entries + state->r_outline_blur_x;
        args = {};
        args.gaussian.blur_scale_divisor = input->window.width / 0.25f;
        *outline_blur_x = {
            &state->rl_outline_blur_x.list,
            state->rl_outline_blur_x.buffer,
            sizeof(state->rl_outline_blur_x.buffer),
            state->fb_outline_blur_x,
            state->fb_outline_sobel,
            ApplyFilterType::gaussian_7x1_x,
            args,
        };
        state->rl_outline_blur_x.list.name = DEBUG_NAME("outline_blur_x");
        PipelineAddDependency(&state->render_pipeline, state->r_outline_blur_x, state->r_outline_sobel);

        args.gaussian.blur_scale_divisor = input->window.height / 0.25f;
        state->r_outline_blur_y = RenderPipelineAddRenderer(&state->render_pipeline);
        RenderPipelineEntry *outline_blur_y = state->render_pipeline.entries + state->r_outline_blur_y;
        *outline_blur_y = {
            &state->rl_outline_blur_y.list,
            state->rl_outline_blur_y.buffer,
            sizeof(state->rl_outline_blur_y.buffer),
            state->fb_outline_blur_y,
            state->fb_outline_blur_x,
            ApplyFilterType::gaussian_7x1_y,
            args,
        };
        state->rl_outline_blur_y.list.name = DEBUG_NAME("outline_blur_y");
        PipelineAddDependency(&state->render_pipeline, state->r_outline_blur_y, state->r_outline_blur_x);


        state->r_outline_draw = RenderPipelineAddRenderer(&state->render_pipeline);
        RenderPipelineEntry *outline_draw = state->render_pipeline.entries + state->r_outline_draw;
        *outline_draw = {
            &state->rl_outline_draw.list,
            state->rl_outline_draw.buffer,
            sizeof(state->rl_outline_draw.buffer),
            -1,
            -1,
        };
        state->rl_outline_draw.list.name = DEBUG_NAME("outline_draw");
        PipelineAddDependency(&state->render_pipeline, state->r_outline_draw, state->r_outline_blur_y);
        state->r_mouse = RenderPipelineAddRenderer(&state->render_pipeline);
        RenderPipelineEntry *mouse = state->render_pipeline.entries + state->r_mouse;
        *mouse = {
            &state->rl_mouse.list,
            state->rl_mouse.buffer,
            sizeof(state->rl_mouse.buffer),
            -1,
            -1,
        };
        state->rl_mouse.list.name = DEBUG_NAME("mouse");
        PipelineAddDependency(&state->render_pipeline, state->r_mouse, state->r_m);

        state->fb_mousepicking = RenderPipelineAddFramebuffer(&state->render_pipeline);

        state->r_mousepicking = RenderPipelineAddRenderer(&state->render_pipeline);
        RenderPipelineEntry *mousepicking = state->render_pipeline.entries + state->r_mousepicking;
        *mousepicking = {
            &state->rl_mousepicking.list,
            state->rl_mousepicking.buffer,
            sizeof(state->rl_mousepicking.buffer),
            state->fb_mousepicking,
            -1,
        };
        state->rl_mousepicking.list.name = DEBUG_NAME("mousepicking");
        state->rl_mousepicking.list.config.show_object_identifier = 1;

        RenderPipelineFramebuffer *fb_mousepicking = state->render_pipeline.framebuffers + state->fb_mousepicking;
        fb_mousepicking->no_clear_each_frame = 1;
        PipelineInit(&state->render_pipeline, &state->assets);
        state->ground_plane = add_asset(&state->assets, "ground_plane_mesh");
        state->diffuse46 = add_asset(&state->assets, "pattern_46_diffuse");
        state->normal46 = add_asset(&state->assets, "pattern_46_normal");
        state->specular46 = add_asset(&state->assets, "pattern_46_specular");
        state->mouse_cursor = add_asset(&state->assets, "mouse_cursor");

        state->barrel_diffuse = add_asset(&state->assets, "barrel_diffuse");
        state->barrel_normal = add_asset(&state->assets, "barrel_normal");
        state->barrel_specular = add_asset(&state->assets, "barrel_specular");
        state->barrel_mesh = add_asset(&state->assets, "barrel_mesh");

        state->metal_barrel_diffuse = add_asset(&state->assets, "metal_barrel_diffuse");
        state->metal_barrel_normal = add_asset(&state->assets, "metal_barrel_normal");
        state->metal_barrel_specular = add_asset(&state->assets, "metal_barrel_specular");
        state->metal_barrel_mesh = add_asset(&state->assets, "metal_barrel_mesh");

        state->modular_building_diffuse = add_asset(&state->assets, "modular_building_diffuse");
        state->modular_building_normal = add_asset(&state->assets, "modular_building_normal");
        state->modular_building_specular = add_asset(&state->assets, "modular_building_specular");
        state->modular_building_brick_door_mesh = add_asset(&state->assets, "modular_building_brick_door_mesh");
        state->modular_building_brick_wall_mesh = add_asset(&state->assets, "modular_building_brick_wall_mesh");
        state->modular_building_brick_small_window_mesh = add_asset(&state->assets, "modular_building_brick_small_window_mesh");

        state->cactus_diffuse = add_asset(&state->assets, "cactus_diffuse");
        state->cactus_mesh = add_asset(&state->assets, "cactus_mesh");

        state->materials[0] = {
            state->diffuse46,
            state->normal46,
            state->specular46,
        };

        state->materials[1] = {
            state->barrel_diffuse,
            state->barrel_normal,
            state->barrel_specular,
        };

        state->materials[2] = {
            state->metal_barrel_diffuse,
            state->metal_barrel_normal,
            state->metal_barrel_specular,
        };

        state->materials[3] = {
            state->modular_building_diffuse,
            state->modular_building_normal,
            state->modular_building_specular,
        };

        state->materials[4] = {
            state->cactus_diffuse,
            -1,
            -1,
        };

        state->sun_rotation = {-2.525f,-0.92f};
        light.type = LightType::directional;
        light.direction = {0.323f, -0.794f, -0.516f};
        light.color = {1.0f, 1.0f, 1.0f};
        light.ambient_intensity = 0.298f;
        light.diffuse_intensity = 1.0f;
        light.attenuation_constant = 1.0f;

        state->camera.distance = 3.0f;
        state->camera.near_ = 0.1f;
        state->camera.far_ = 2000.0f * 1.1f;
        state->camera.fov = 60;
        state->camera.target = {2, 1.0, 0};
        state->camera.rotation = {-0.1f, -0.8f, 0};
        float ratio = (float)input->window.width / (float)input->window.height;
        update_camera(&state->camera, ratio);

        for (uint32_t i = 0; i < harray_count(state->test_meshes); ++i)
        {
            auto &mesh = state->test_meshes[i];
            mesh.dynamic = true;
            mesh.vertexformat = VertexFormat::just_vertices;
            mesh.dynamic_max_vertices = 1000;
            mesh.dynamic_max_indices = 2 * 1000;
            mesh.mesh_format = MeshFormat::just_vertices;
            mesh.vertices.size = sizeof(vertexformat_just_vertices) * mesh.dynamic_max_vertices;
            mesh.vertices.data = malloc(mesh.vertices.size);
            mesh.indices.size = 4 * mesh.dynamic_max_indices;
            mesh.indices.data = malloc(mesh.indices.size);

            state->test_mesh_descriptors[i] = add_dynamic_mesh_asset(&state->assets, state->test_meshes + i);
        }

        state->objects[0] =
            {
                {0,0,0},
                {0,0,0},
                {5,5,5},
                1,
                state->ground_plane,
                0,
                1,
            };
        state->objects[1] =
            {
                {0,0,0},
                {0,0,0},
                {0.01f, 0.01f, 0.01f},
                0,
                state->barrel_mesh,
                1,
                2,
            };
        state->objects[2] =
            {
                {1, 0, 0},
                {0, 0, 0},
                {0.01f, 0.01f, 0.01f},
                0,
                state->metal_barrel_mesh,
                2,
                3,
            };
        state->objects[3] =
            {
                {0, 0, 5},
                {0, h_pi, 0},
                {0.01f, 0.01f, 0.01f},
                0,
                state->modular_building_brick_door_mesh,
                3,
                4,
            };
        state->objects[4] =
            {
                {2.5f, 0, 5},
                {0, h_pi, 0},
                {0.01f, 0.01f, 0.01f},
                0,
                state->modular_building_brick_wall_mesh,
                3,
                5,
            };
        state->objects[5] =
            {
                {-2.5f, 0, 5},
                {0, h_pi, 0},
                {0.01f, 0.01f, 0.01f},
                0,
                state->modular_building_brick_small_window_mesh,
                3,
                6,
            };
        state->objects[6] =
            {
                {-1, 0, 0},
                {0, 0, 0},
                {0.01f, 0.01f, 0.01f},
                0,
                state->cactus_mesh,
                4,
                7,
            };
        state->num_objects = 7;
    }

    ImGui::Begin("Debug");
    static bool progress = true;
    ImGui::Checkbox("Progress", &progress);
    ImGui::End();

    if (!progress)
    {
        return;
    }
    ClearRenderLists();

    state->frame_state.delta_t = input->delta_t;
    state->frame_state.mouse_position = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y)};
    state->frame_state.input = input;
    state->frame_state.memory = memory;

    {
        MouseInput *mouse = &input->mouse;
        if (mouse->vertical_wheel_delta)
        {
            state->camera.distance -= mouse->vertical_wheel_delta * 0.1f;
        }

        if (BUTTON_WENT_DOWN(mouse->buttons.middle))
        {
            state->middle_base_location = {mouse->x, mouse->y};
        }

        if (BUTTON_STAYED_DOWN(mouse->buttons.middle))
        {
            v2i diff = v2sub(state->middle_base_location, {mouse->x, mouse->y});
            if (abs(diff.x) > 1 || abs(diff.y) > 1)
            {
                if (abs(diff.x) > abs(diff.y))
                {
                    state->camera.rotation.y += (diff.x / 25.0f);
                }
                else
                {
                    state->camera.rotation.x -= (diff.y / 40.0f);
                }
                state->middle_base_location = {mouse->x, mouse->y};
            }
        }

        if (BUTTON_WENT_DOWN(mouse->buttons.right))
        {
            state->right_base_location = {mouse->x, mouse->y};
        }

        if (BUTTON_STAYED_DOWN(mouse->buttons.right))
        {
            v2i diff = v2sub(state->right_base_location, {mouse->x, mouse->y});
            if (abs(diff.x) > 1 || abs(diff.y) > 1)
            {
                if (abs(diff.x) > abs(diff.y))
                {
                    state->sun_rotation.y += (diff.x / 25.0f);
                }
                else
                {
                    state->sun_rotation.x -= (diff.y / 40.0f);
                }
                state->right_base_location = {mouse->x, mouse->y};
            }
        }
    }


    if (state->show_camera) {
        ImGui::Begin("Camera", &state->show_camera);
        ImGui::DragFloat3("Rotation", &state->camera.rotation.x, 0.1f);
        ImGui::DragFloat3("Target", &state->camera.target.x);
        ImGui::DragFloat("Distance", &state->camera.distance, 0.1f, 0.1f, 100.0f);
        ImGui::DragFloat("Near", &state->camera.near_, 0.1f, 0.1f, 5.0f);
        ImGui::DragFloat("Far", &state->camera.far_, 0.1f, 5.0f, 2000.0f);
        ImGui::DragFloat("Field of View", &state->camera.fov, 1, 30, 150);
        bool orthographic = state->camera.orthographic;
        bool frustum = state->camera.frustum;
        ImGui::Checkbox("Orthographic", &orthographic);
        ImGui::Checkbox("Frustum", &frustum);
        state->camera.orthographic = (uint32_t)orthographic;
        state->camera.frustum = (uint32_t)frustum;

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
    state->camera.target = {0.0f, 1.0f, 0.0f};
    float ratio = (float)input->window.width / (float)input->window.height;
    update_camera(&state->camera, ratio);
    state->matrices[0] = state->camera.projection;
    state->matrices[1] = state->camera.view;
    state->matrices[2] = m4orthographicprojection(1.0f, -1.0f, {0.0f, 0.0f}, {(float)input->window.width, (float)input->window.height});

    float theta = state->sun_rotation.y;
    float phi = state->sun_rotation.x;
    light.direction = {
        sinf(theta) * cosf(phi),
        sinf(phi),
        cosf(theta) * cosf(phi),
    };

    if (state->show_lights) {
        ImGui::Begin("Lights", &state->show_lights);
        ImGui::DragFloat3("Direction", &light.direction.x, 0.001f, -1.0f, 1.0f);
        ImGui::Text("Sun rotation is %f, %f", state->sun_rotation.x, state->sun_rotation.y);
        static bool animate_sun = false;
        if (ImGui::Button("Normalize"))
        {
            light.direction = v3normalize(light.direction);
        }
        ImGui::Checkbox("Animate Sun", &animate_sun);
        if (animate_sun)
        {
            static float animation_speed_per_second = 0.05f;
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

    LightDescriptors lights = {harray_count(state->lights), state->lights};

    ArmatureDescriptors armatures = {
        0,
        0,
    };

    PipelineResetData prd = {
        { input->window.width, input->window.height },
        harray_count(state->matrices), state->matrices,
        harray_count(state->assets.descriptors), (asset_descriptor *)&state->assets.descriptors,
        lights,
        armatures,
        MaterialDescriptors{harray_count(state->materials), state->materials},
    };

    PipelineReset(
        &state->render_pipeline,
        &prd
    );

    {
        bool foo = state->rl_m.list.config.normal_map_disabled;
        ImGui::Checkbox("Disable normal map", &foo);
        state->rl_m.list.config.normal_map_disabled = (uint32_t)foo;
    }

    {
        bool foo = state->rl_m.list.config.show_normals;
        ImGui::Checkbox("Show normals", &foo);
        state->rl_m.list.config.show_normals = (uint32_t)foo;
    }

    {
        bool foo = state->rl_m.list.config.show_normalmap;
        ImGui::Checkbox("Show normal map", &foo);
        state->rl_m.list.config.show_normalmap = (uint32_t)foo;
    }

    {
        bool foo = state->rl_m.list.config.specular_map_disabled;
        ImGui::Checkbox("Disable specular map", &foo);
        state->rl_m.list.config.specular_map_disabled = (uint32_t)foo;
    }

    {
        bool foo = state->rl_m.list.config.show_specular;
        ImGui::Checkbox("Show specular", &foo);
        state->rl_m.list.config.show_specular = (uint32_t)foo;
    }

    {
        bool foo = state->rl_m.list.config.show_specularmap;
        ImGui::Checkbox("Show specular map", &foo);
        state->rl_m.list.config.show_specularmap = (uint32_t)foo;
    }

    {
        bool foo = state->rl_m.list.config.show_tangent;
        ImGui::Checkbox("Show tangent", &foo);
        state->rl_m.list.config.show_tangent = (uint32_t)foo;
    }

    {
        bool foo = state->rl_m.list.config.show_object_identifier;
        ImGui::Checkbox("Show object_identifier", &foo);
        state->rl_m.list.config.show_object_identifier = (uint32_t)foo;
    }

    {
        bool foo = state->rl_m.list.config.show_texcontainer_index;
        ImGui::Checkbox("Show texcontainer_index", &foo);
        state->rl_m.list.config.show_texcontainer_index = (uint32_t)foo;
    }

    state->rl_m.list.config.camera_position = state->camera.location;
    PushClear(&state->rl_m.list, {0,0,0,1});

    if (BUTTON_WENT_DOWN(input->mouse.buttons.left))
    {
        state->readpixel_result.pixel_returned = 0;
        PushReadPixel(
            &state->rl_mousepicking.list,
            {
                input->mouse.x,
                input->window.height - input->mouse.y
            },
            &state->readpixel_result);
    }
    auto &pixel = state->readpixel_result.pixel;
    int32_t selected_object_id = pixel[0] + pixel[1] * 256 + pixel[2] * 256 * 256;
    ImGui::Text("Mouse picked object_id %d", selected_object_id);
    PushClear(&state->rl_mousepicking.list, {0,0,0,1});
    PushClear(&state->rl_outline.list, {0,0,0,0});

    render_entry_list *lists[] =
    {
        &state->rl_m.list,
        &state->rl_mousepicking.list,
        0,
    };

    _ObjectData *selected_object = 0;
    m4 selected_model_matrix = {};


    for (uint32_t object_index = 0; object_index < state->num_objects; ++object_index)
    {
        auto &object = state->objects[object_index];
        m4 model_matrix = m4mul(
            m4translate(object.position),
            m4mul(
                m4scale(object.scale),
                m4rotation(QuaternionFromEulerZYX(object.rotation))
            )
        );
        MeshFromAssetFlags flags = {};
        flags.depth_disabled = object.depth_disabled;
        flags.use_materials = 1;
        for (render_entry_list **li = lists; *li; ++li)
        {
            render_entry_list *l = *li;
            PushMeshFromAsset(
                l,
                0, // projection matrix - matrices[0]
                1, // view materix - matrices[1]
                model_matrix,
                object.model_id,
                object.material, // material 0
                0, // light 0
                -1, // no armature
                flags,
                ShaderType::standard,
                object.object_id // object_id
            );
        }
        if (object.object_id == selected_object_id)
        {
            render_entry_list *l = &state->rl_outline.list;
            PushMeshFromAsset(
                l,
                0, // projection matrix - matrices[0]
                1, // view materix - matrices[1]
                model_matrix,
                object.model_id,
                object.material, // material 0
                0, // light 0
                -1, // no armature
                flags,
                ShaderType::standard,
                object.object_id // object_id
            );
            selected_object = state->objects + object_index;
            selected_model_matrix = model_matrix;
        }
    }


    {
        PushSky(&state->rl_m.list,
            0,
            1,
            v3normalize(light.direction));
    }

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Debug"))
        {
            ImGui::MenuItem("Show lights", "", &state->show_lights);
            ImGui::MenuItem("Show camera", "", &state->show_camera);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    v3 mouse_bl = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y), 0.0f};
    v3 mouse_size = {16.0f, -16.0f, 0.0f};
    PushQuad(
        &state->rl_m.list,
        mouse_bl,
        mouse_size,
        {1,1,1,1},
        2,
        state->mouse_cursor);

    {
        v3 screen_pos = { input->window.width / 2.0f, input->window.height / 2.0f, 0 };
        m4 rotate = m4identity();
        v3 screen_pos_x = v3add(screen_pos, {100, 0, 0});
        v3 screen_pos_y = v3add(screen_pos, {0, 100, 0});
        v3 screen_pos_z = v3add(screen_pos, {0, 0, 100});
        if (selected_object)
        {
            ImGui::Begin("Selected Object");
            ImGui::DragFloat3("Position", (float *)&selected_object->position, 0.001f);
            ImGui::DragFloat3("Rotation", (float *)&selected_object->rotation, 0.001f);
            ImGui::DragFloat3("Scale", (float *)&selected_object->scale, 0.001f);
            ImGui::End();
            v4 world_pos = m4mul(
                selected_model_matrix,
                m4mul(
                    m4rotation(QuaternionFromEulerZYX(selected_object->rotation)),
                    v4{0, 0, 0, 1}
                )
            );
            v4 view_space = m4mul(state->camera.view, world_pos);
            v4 clip = m4mul(state->camera.projection, view_space);
            clip = v4div(clip, clip.w);

            v2 clip2 = v2add({0.5f,0.5f}, v2mul({clip.x, clip.y}, 0.5f));

            screen_pos = {
                clip2.x * input->window.width,
                clip2.y * input->window.height,
                0,
            };

            v4 world_pos_x = v4add(
                world_pos,
                m4mul(
                    m4rotation(QuaternionFromEulerZYX(selected_object->rotation)),
                    v4{1, 0, 0, 0}
                )
            );

            v4 view_space_x = m4mul(state->camera.view, world_pos_x);
            v4 clip_space_x = m4mul(state->camera.projection, view_space_x);
            clip_space_x = v4div(clip_space_x, clip_space_x.w);

            v2 clip2_x = v2add({0.5f,0.5f}, v2mul({clip_space_x.x, clip_space_x.y}, 0.5f));

            screen_pos_x = {
                clip2_x.x * input->window.width,
                clip2_x.y * input->window.height,
                0,
            };


            v4 world_pos_y = v4add(
                world_pos,
                m4mul(
                    m4rotation(QuaternionFromEulerZYX(selected_object->rotation)),
                    v4{0, 1, 0, 0}
                )
            );
            v4 view_space_y = m4mul(state->camera.view, world_pos_y);
            v4 clip_space_y = m4mul(state->camera.projection, view_space_y);
            clip_space_y = v4div(clip_space_y, clip_space_y.w);

            v2 clip2_y = v2add({0.5f,0.5f}, v2mul({clip_space_y.x, clip_space_y.y}, 0.5f));

            screen_pos_y = {
                clip2_y.x * input->window.width,
                clip2_y.y * input->window.height,
                0,
            };

            v4 world_pos_z = v4add(
                world_pos,
                m4mul(
                    m4rotation(QuaternionFromEulerZYX(selected_object->rotation)),
                    v4{0, 0, 1, 0}
                )
            );
            v4 view_space_z = m4mul(state->camera.view, world_pos_z);
            v4 clip_space_z = m4mul(state->camera.projection, view_space_z);
            clip_space_z = v4div(clip_space_z, clip_space_z.w);

            v2 clip2_z = v2add({0.5f,0.5f}, v2mul({clip_space_z.x, clip_space_z.y}, 0.5f));

            screen_pos_z = {
                clip2_z.x * input->window.width,
                clip2_z.y * input->window.height,
                0,
            };
        }
        ImGui::Begin("Lines");
        auto *mesh = state->test_meshes + 0;
        vertexformat_just_vertices *vertices = (vertexformat_just_vertices *)mesh->vertices.data;
        uint32_t vertex_count = 0;
        uint32_t num_indices = 0;
        const uint32_t num_segments = 32;
        v3 last_pos = { 0, 1, 0 };
        uint32_t *indices = (uint32_t *)mesh->indices.data;
        for (uint32_t i = 0; i < num_segments; ++i)
        {
            float angle = (i + 1.0f) / (float)num_segments * h_twopi;
            v3 pos = { sin(angle), cos(angle), 0 };
            ImGui::Text("From %f, %f to %f, %f",
                last_pos.x, last_pos.y,
                pos.x, pos.y);
            vertices[vertex_count++].position = v3add(screen_pos, v3mul(last_pos, 100));
            vertices[vertex_count++].position = v3add(screen_pos, v3mul(pos, 100));
            last_pos = pos;
            indices[num_indices++] = num_indices;
            indices[num_indices++] = num_indices;
        }
        vertices[vertex_count++].position = screen_pos;
        vertices[vertex_count++].position = screen_pos_x;
        indices[num_indices++] = num_indices;
        indices[num_indices++] = num_indices;

        vertices[vertex_count++].position = screen_pos;
        vertices[vertex_count++].position = screen_pos_y;
        indices[num_indices++] = num_indices;
        indices[num_indices++] = num_indices;

        vertices[vertex_count++].position = screen_pos;
        vertices[vertex_count++].position = screen_pos_z;
        indices[num_indices++] = num_indices;
        indices[num_indices++] = num_indices;
        mesh->v3.vertex_count = vertex_count;
        mesh->num_indices = num_indices;
        mesh->primitive_type = PrimitiveType::lines;
        mesh->reload = true;
        MeshFromAssetFlags flags = {};
        flags.depth_disabled = 1;
        if (selected_object)
        {
            PushMeshFromAsset(
                &state->rl_m.list,
                2, // projection matrix - matrices[0]
                -1, // view matrix - matrices[1]
                m4identity(),
                state->test_mesh_descriptors[0],
                -1, // no material
                -1, // no lights
                -1, // no armature
                flags,
                ShaderType::standard,
                -1
            );
        }
        ImGui::End();
    }

    {
        auto &rfb = state->render_pipeline.framebuffers[state->fb_mousepicking];
        ImGui::Image(
            (void *)(intptr_t)rfb.framebuffer._texture,
            {512, 512.0f * (float)rfb.framebuffer.size.y / (float)rfb.framebuffer.size.x},
            {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
        );
    }

    {
        auto &rfb = state->render_pipeline.framebuffers[state->fb_outline];
        ImGui::Image(
            (void *)(intptr_t)rfb.framebuffer._texture,
            {1024, 1024.0f * (float)rfb.framebuffer.size.y / (float)rfb.framebuffer.size.x},
            {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
        );
    }

    {
        auto &rfb = state->render_pipeline.framebuffers[state->fb_outline_sobel];
        ImGui::Image(
            (void *)(intptr_t)rfb.framebuffer._texture,
            {1024, 1024.0f * (float)rfb.framebuffer.size.y / (float)rfb.framebuffer.size.x},
            {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
        );
    }

    {
        auto &rfb = state->render_pipeline.framebuffers[state->fb_outline_blur_x];
        ImGui::Image(
            (void *)(intptr_t)rfb.framebuffer._texture,
            {1024, 1024.0f * (float)rfb.framebuffer.size.y / (float)rfb.framebuffer.size.x},
            {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
        );
    }

    {
        auto &rfb = state->render_pipeline.framebuffers[state->fb_outline_blur_y];
        ImGui::Image(
            (void *)(intptr_t)rfb.framebuffer._texture,
            {1024, 1024.0f * (float)rfb.framebuffer.size.y / (float)rfb.framebuffer.size.x},
            {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
        );
    }

    {
        auto *fb_outline_blur_y = state->render_pipeline.framebuffers + state->fb_outline_blur_y;
        PushQuad(
            &state->rl_outline_draw.list,
            {0,0},
            {(float)input->window.width, (float)input->window.height},
            {1,1,1,1},
            2,
            fb_outline_blur_y->asset_descriptor
        );
    }
}

void
initialize(platform_memory *memory, demo_cowboy_state *state)
{
    state->shadowmap_size = memory->shadowmap_size;
    CreatePipeline(state);
    PipelineInit(&state->render_pipeline, state->assets);

    state->pixel_size = 64;
    state->familiar_movement.position = {-2, 2};

    state->acceleration_multiplier = 50.0f;

    queue_init(
        &state->debug->priority_queue.queue,
        harray_count(state->debug->priority_queue.entries),
        state->debug->priority_queue.entries
    );
    auto &astar_data = state->debug->familiar_path.data;
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

    state->camera.distance = 3.0f;
    state->camera.near_ = 0.1f;
    state->camera.far_ = 2000.0f * 1.1f;
    state->camera.fov = 60;
    state->camera.target = {2, 1.0, 0};
    state->camera.rotation = {-0.1f, -0.8f, 0};
    {
        float ratio = (float)state->frame_state.input->window.width / (float)state->frame_state.input->window.height;
        update_camera(&state->camera, ratio);
    }

    state->np_camera.distance = 2.0f;
    state->np_camera.near_ = 1.0f;
    state->np_camera.far_ = 100.0f;
    state->np_camera.fov = 60;
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
            l.asset_id = add_asset(state->assets, l.asset_name);
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
            l.asset_id = add_asset(state->assets, l.asset_name);
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
            l.asset_id = add_asset(state->assets, l.asset_name);
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
            l.asset_id = add_asset(state->assets, l.asset_name);
        }
    }

    for (uint32_t i = 0; i < state->num_asset_classes; ++i)
    {
        state->asset_class_names[i] = state->asset_classes[i].name;
    }

    state->num_squares = MESH_SQUARE;
    state->test_meshes = PushArray(
        "test_meshes",
        state->arena,
        Mesh,
        state->num_squares);
    state->test_textures = PushArray(
        "test_textures",
        state->arena,
        DynamicTextureDescriptor,
        state->num_squares);
    state->noisemaps = PushArray(
        "noisemaps",
        state->arena,
        noise_float_array,
        state->num_squares);
    state->heightmaps = PushArray(
        "heightmaps",
        state->arena,
        noise_float_array,
        state->num_squares);
    state->noisemap_scratches = PushArray(
        "noisemap_scratches",
        state->arena,
        noise_v4b_array,
        state->num_squares);

    for (uint32_t i = 0; i < state->num_squares; ++i)
    {
        auto &mesh = state->test_meshes[i];
        mesh.dynamic = true;
        mesh.vertexformat = VertexFormat::v1;
        mesh.dynamic_max_vertices = 120000;
        mesh.dynamic_max_indices = 3 * 120000;
        mesh.mesh_format = MeshFormat::v3_boneless;
        mesh.vertices.size = sizeof(_vertexformat_1) * mesh.dynamic_max_vertices;
        mesh.vertices.data = malloc(mesh.vertices.size);
        mesh.indices.size = 4 * mesh.dynamic_max_indices;
        mesh.indices.data = malloc(mesh.indices.size);
        mesh.primitive_type = PrimitiveType::triangles;

        state->test_mesh_descriptors[i] = add_dynamic_mesh_asset(state->assets, state->test_meshes + i);
        state->test_texture_descriptors[i] = add_dynamic_texture_asset(state->assets, state->test_textures + i);
    }

    {
        auto &mesh = state->ui_mesh;
        mesh.dynamic = true;
        mesh.vertexformat = VertexFormat::v1;
        mesh.dynamic_max_vertices = 10000;
        mesh.dynamic_max_indices = 3 * 10000;
        mesh.mesh_format = MeshFormat::v3_boneless;
        mesh.vertices.size = sizeof(_vertexformat_1) * mesh.dynamic_max_vertices;
        mesh.vertices.data = malloc(mesh.vertices.size);
        mesh.indices.size = 4 * mesh.dynamic_max_indices;
        mesh.indices.data = malloc(mesh.indices.size);
        mesh.primitive_type = PrimitiveType::triangles;
        state->ui_mesh_descriptor = add_dynamic_mesh_asset(state->assets, &mesh);
    }

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
                ti->color = {0.45f, 0.45f, 0.1f, 1.0f};
            } break;
            case TerrainType::water:
            {
                ti->name = "water";
                ti->height = -0.005f;
                ti->color = {0.55f, 0.55f, 0.15f, 1.0f};
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
                ti->height = 0.35f;
                ti->color = {0.0f, 0.6f, 0.0f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::rock:
            {
                ti->name = "rock";
                ti->height = 0.65f;
                ti->color = {0.65f, 0.25f, 0.25f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::rock_2:
            {
                ti->name = "rock 2";
                ti->height = 0.8f;
                ti->color = {0.5f, 0.2f, 0.2f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::snow:
            {
                ti->name = "snow";
                ti->height = 0.85f;
                ti->color = {0.8f, 0.8f, 0.8f, 1.0f};
                ti->merge_with_previous = true;
            } break;
            case TerrainType::snow_2:
            {
                ti->name = "snow_2";
                ti->height = 5.0f;
                ti->color = {0.9f, 0.9f, 0.9f, 1.0f};
                ti->merge_with_previous = true;
            } break;
        }
    }

    for (int32_t y = -MESH_SURROUND; y < MESH_SURROUND+1; ++y)
    {
        for (int32_t x = -MESH_SURROUND; x < MESH_SURROUND+1; ++x)
        {
            state->noisemaps_data[(y+MESH_SURROUND)*MESH_WIDTH+x+MESH_SURROUND] = { {(float)x * (TERRAIN_MAP_CHUNK_WIDTH / 2), (float)y * (TERRAIN_MAP_CHUNK_HEIGHT/2)}, true };
        }
    }
    auto &perlin = state->debug->perlin;
    perlin.seed = 95;
    perlin.offset = {-3.0f, -23.0f};
    perlin.scale = 27.6f;
    //perlin.scale = 22.320f;
    //perlin.scale = 54.830f;
    perlin.show = false;
    perlin.octaves = 4;
    perlin.persistence = 0.3f;
    perlin.lacunarity = 3.33f;
    perlin.height_multiplier = 253.0f;
    perlin.min_noise_height = FLT_MAX;
    perlin.max_noise_height = FLT_MIN;
    //auto &path = state->cowboy_path;
}


DEMO(demo_cowboy)
{
    game_state *gs = (game_state *)memory->game_block->base;
    if (!gs->demo_cowboy_block)
    {
        gs->demo_cowboy_block = _platform->allocate_memory("demo_cowboy", sizeof(demo_cowboy_state) + 128);
        bootstrap_memory_arena(gs->demo_cowboy_block, demo_cowboy_state, arena);
        demo_cowboy_state *state = (demo_cowboy_state *)gs->demo_cowboy_block->base;

        state->debug = &gs->debug;
        state->asset_ids = &gs->asset_ids;
        state->assets = &gs->assets;
        state->arena = &gs->arena;

        state->frame_state.delta_t = input->delta_t;
        state->frame_state.mouse_position = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y)};
        state->frame_state.input = input;
        state->frame_state.memory = memory;


        initialize(memory, state);
    }
    demo_cowboy_state *state = (demo_cowboy_state *)gs->demo_cowboy_block->base;
    state->frame_state.delta_t = input->delta_t;
    state->frame_state.mouse_position = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y)};
    state->frame_state.input = input;
    state->frame_state.memory = memory;
    {
        MouseInput *mouse = &input->mouse;
        if (mouse->vertical_wheel_delta)
        {
            state->camera.distance -= mouse->vertical_wheel_delta * 0.1f;
        }

        if (BUTTON_WENT_DOWN(mouse->buttons.middle))
        {
            state->middle_base_location = {mouse->x, mouse->y};
        }

        if (BUTTON_STAYED_DOWN(mouse->buttons.middle))
        {
            v2i diff = v2sub(state->middle_base_location, {mouse->x, mouse->y});
            if (abs(diff.x) > 1 || abs(diff.y) > 1)
            {
                if (abs(diff.x) > abs(diff.y))
                {
                    state->camera.rotation.y += (diff.x / 25.0f);
                }
                else
                {
                    state->camera.rotation.x -= (diff.y / 40.0f);
                }
                state->middle_base_location = {mouse->x, mouse->y};
            }
        }

        /*
        if (BUTTON_ENDED_DOWN(mouse->buttons.right))
        {
            Plane p = {
                {0,1,0},
                -state->camera.target.y,
            };
            Ray r = screen_to_ray(state);
            //ImGui::Text("Ray: %f, %f, %f", r.direction.x, r.direction.y, r.direction.z);
            float t;
            v3 q;
            if (ray_plane_intersect(r, p, &t, &q))
            {
                //ImGui::Text("Intersect");
                v2 plane_location = {q.x, q.z};
                if (BUTTON_STAYED_DOWN(mouse->buttons.right))
                {
                    if (t < 60.0f && t > -60.0f)
                    {
                        v2 diff = v2sub(plane_location, state->right_plane_location);
                        v3 diff3 = {diff.x, 0, diff.y};
                        if (t < 0)
                        {
                            diff3 = v3sub({}, diff3);
                        }
                        state->camera.target = v3sub(state->camera.target, diff3);
                    }
                }
                state->right_plane_location = plane_location;
            }
            //ImGui::Text("t / q: %f / %f, %f, %f", t, q.x, q.y, q.z);
        }
        */
    }
    //ImGui::Text("Hello");

    show_debug_main_menu(state);
    show_pq_debug(&state->debug->priority_queue);

    {
        static float rebuild_time = 1.0f;
        rebuild_time += input->delta_t;
        //bool rebuild = rebuild_time > 1.0f;
        bool rebuild = false;
        auto &perlin = state->debug->perlin;
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
            rebuild |= ImGui::DragFloat2("Offset", &perlin.offset.x, 1.00f, -100.0f, 100.0f, "%0.2f");
            rebuild |= ImGui::DragFloat("Height Multiplier", &perlin.height_multiplier, 0.01f, 0.01f, 1000.0f, "%0.2f");

            ImGui::DragFloat2("Control point 0", &perlin.control_point_0.x, 0.01f, 0.0f, 1.0f, "%.02f");
            ImGui::DragFloat2("Control point 1", &perlin.control_point_1.x, 0.01f, 0.0f, 1.0f, "%.02f");

            float values[100];
            v2 p1 = perlin.control_point_0;
            v2 p2 = perlin.control_point_1;
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
            ImGui::End();
        }
        if (rebuild)
        {
            rebuild_time = 0;
        }

        v2 new_noisemap_base = {
            roundf(state->camera.target.x / TERRAIN_MAP_CHUNK_WIDTH) * (TERRAIN_MAP_CHUNK_WIDTH/2),
            -roundf(state->camera.target.z / TERRAIN_MAP_CHUNK_WIDTH) * (TERRAIN_MAP_CHUNK_HEIGHT/2),
        };

        if (!(new_noisemap_base == state->noisemap_base))
        {
            for (int32_t y = -MESH_SURROUND; y < MESH_SURROUND+1; ++y)
            {
                for (int32_t x = -MESH_SURROUND; x < MESH_SURROUND+1; ++x)
                {
                    state->noisemaps_data[(y+MESH_SURROUND)*MESH_WIDTH+x+MESH_SURROUND] = {
                        v2add(new_noisemap_base, {(float)x * (TERRAIN_MAP_CHUNK_WIDTH/2), (float)y * (TERRAIN_MAP_CHUNK_HEIGHT/2)}),
                        true
                    };
                }
            }
            state->noisemap_base = new_noisemap_base;
        }

        /*
        uint32_t noisemaps_free[harray_count(state->noisemaps_data)];
        uint32_t num_noisemaps_free = {};
        */

        for (uint32_t i = 0; i < harray_count(state->noisemaps_data); ++i)
        {
            auto &noisemap_data = state->noisemaps_data[i];
            if (!rebuild && !noisemap_data.rebuild)
            {
                continue;
            }
            noisemap_data.rebuild = false;
            auto &noisemap_id = i;
            auto &offset = noisemap_data.offset;

            array2p<float> noisemap = state->noisemaps[noisemap_id].array2p();
            array2p<float> heightmap = state->heightmaps[noisemap_id].array2p();

            array2p<v4b> noisemap_scratch = state->noisemap_scratches[noisemap_id].array2p();
            Mesh *test_mesh = state->test_meshes + noisemap_id;
            DynamicTextureDescriptor *test_texture = state->test_textures + noisemap_id;

            generate_noise_map(
                noisemap,
                (uint32_t)perlin.seed,
                perlin.scale,
                (uint32_t)perlin.octaves,
                perlin.persistence,
                perlin.lacunarity,
                v2add(offset, perlin.offset),
                &perlin.min_noise_height,
                &perlin.max_noise_height
            );

            generate_heightmap(
                noisemap,
                heightmap,
                perlin.height_multiplier,
                perlin.control_point_0,
                perlin.control_point_1);

            noise_map_to_texture<TerrainMode::color>(
                noisemap,
                noisemap_scratch,
                test_texture,
                state->landmass.terrains);

            generate_terrain_mesh3(
                heightmap,
                test_mesh);
        }

        if (perlin.show)
        {
            ImGui::Begin("Perlin", &perlin.show);
            ImGui::Text("Minimum noise height: %0.02f", perlin.min_noise_height);
            ImGui::Text("Maximum noise height: %0.02f", perlin.max_noise_height);
            /*
            ImGui::Image(
                (void *)(intptr_t)state->test_texture._texture,
                {512, 512},
                {0,1}, {1,0}, {1,1,1,1}, {0.5f, 0.5f, 0.5f, 0.5f}
            );
            */
            ImGui::End();
        }
    }

    {
        auto &light = state->lights[(uint32_t)LightIds::sun];
        light.type = LightType::directional;

        if (state->debug->show_lights) {
            ImGui::Begin("Lights", &state->debug->show_lights);
            ImGui::DragFloat3("Direction", &light.direction.x, 0.001f, -1.0f, 1.0f);
            static bool animate_sun = false;
            if (ImGui::Button("Normalize"))
            {
                light.direction = v3normalize(light.direction);
            }
            ImGui::Checkbox("Animate Sun", &animate_sun);
            if (animate_sun)
            {
                static float animation_speed_per_second = 0.05f;
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
    //state->matrices[(uint32_t)matrix_ids::mesh_projection_matrix] = m4frustumprojection(1.0f, 100.0f, {-ratio, -1.0f}, {ratio, 1.0f});

    static v3 cowboy_location = {};
    static v3 dog_location = { 2, 2 };
    if (state->debug->show_camera) {
        ImGui::Begin("Camera", &state->debug->show_camera);
        ImGui::DragFloat3("Rotation", &state->camera.rotation.x, 0.1f);
        ImGui::DragFloat3("Target", &state->camera.target.x);
        ImGui::DragFloat("Distance", &state->camera.distance, 0.1f, 0.1f, 100.0f);
        ImGui::DragFloat("Near", &state->camera.near_, 0.1f, 0.1f, 5.0f);
        ImGui::DragFloat("Far", &state->camera.far_, 0.1f, 5.0f, 2000.0f);
        ImGui::DragFloat("Field of View", &state->camera.fov, 1, 30, 150);
        bool orthographic = state->camera.orthographic;
        bool frustum = state->camera.frustum;
        ImGui::Checkbox("Orthographic", &orthographic);
        ImGui::Checkbox("Frustum", &frustum);
        state->camera.orthographic = (uint32_t)orthographic;
        state->camera.frustum = (uint32_t)frustum;

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
    state->camera.target = {cowboy_location.x, 1.0f, cowboy_location.z};
    update_camera(&state->camera, ratio);
    CameraState inverted_camera = state->camera;
    update_camera(&inverted_camera, ratio, true);
    ImGui::Text("Camera location: %.2f, %.2f, %.2f", state->camera.location.x, state->camera.location.y, state->camera.location.z);
    ImGui::Text("Reflection camera location: %.2f, %.2f, %.2f",
            inverted_camera.location.x,
            inverted_camera.location.y,
            inverted_camera.location.z);

    if (state->debug->show_camera) {
        ImGui::Begin("Camera", &state->debug->show_camera);
        ImGui::Separator();
        ImGui::Text("View Matrix");
        for (uint32_t i = 0; i < 4; ++i)
        {
            ImGui::Text("%2.2f %2.2f %2.2f %2.2f",
                state->camera.view.cols[0].E[i],
                state->camera.view.cols[1].E[i],
                state->camera.view.cols[2].E[i],
                state->camera.view.cols[3].E[i]);
        }
        ImGui::Separator();
        m4 inverse_view = m4inverse(state->camera.view);
        ImGui::Text("Inverse View Matrix");
        for (uint32_t i = 0; i < 4; ++i)
        {
            ImGui::Text("%2.2f %2.2f %2.2f %2.2f",
                inverse_view.cols[0].E[i],
                inverse_view.cols[1].E[i],
                inverse_view.cols[2].E[i],
                inverse_view.cols[3].E[i]);
        }
        ImGui::Separator();
        ImGui::Text("Projection Matrix");
        for (uint32_t i = 0; i < 4; ++i)
        {
            ImGui::Text("%2.2f %2.2f %2.2f %2.2f",
                state->camera.projection.cols[0].E[i],
                state->camera.projection.cols[1].E[i],
                state->camera.projection.cols[2].E[i],
                state->camera.projection.cols[3].E[i]);
        }
        ImGui::End();
    }
    state->np_camera.rotation.y += 0.4f * input->delta_t;
    update_camera(&state->np_camera, ratio);
    state->matrices[(uint32_t)matrix_ids::mesh_projection_matrix] = state->camera.projection;
    state->matrices[(uint32_t)matrix_ids::mesh_view_matrix] = state->camera.view;

    state->matrices[(uint32_t)matrix_ids::reflected_mesh_view_matrix] = inverted_camera.view;

    state->matrices[(uint32_t)matrix_ids::np_projection_matrix] = state->np_camera.projection;
    state->matrices[(uint32_t)matrix_ids::np_view_matrix] = state->np_camera.view;

    LightDescriptors l = {harray_count(state->lights), state->lights};
    ArmatureDescriptors armatures = {
        harray_count(state->armatures),
        state->armatures + 0
    };

    PipelineResetData prd = {
        { input->window.width, input->window.height },
        harray_count(state->matrices), state->matrices,
        harray_count(state->assets->descriptors), (asset_descriptor *)&state->assets->descriptors,
        l,
        armatures,
    };

    PipelineReset(
        &state->render_pipeline,
        &prd
    );
    state->pipeline_elements.rl_three_dee.list.config.camera_position = state->camera.location;
    m4 shadowmap_projection_matrix;

    m4 shadowmap_view_matrix;
    {
        auto &light = state->lights[(uint32_t)LightIds::sun];
        v3 eye = v3sub({0,0,0}, v3normalize(light.direction));
        static float eye_distance_base = 100;
        ImGui::DragFloat("Light distance", &eye_distance_base, 0.1f, 5.0f, 100.0f);
        float eye_distance = eye_distance_base + state->camera.distance;
        shadowmap_projection_matrix = m4orthographicprojection(1.0f, 500.0f + eye_distance, {-eye_distance, -eye_distance}, {eye_distance, eye_distance});
        eye = v3mul(eye, eye_distance);
        v3 target = state->camera.target;
        eye = v3add(eye, target);
        ImGui::Text("Sun is at %f, %f, %f", eye.x, eye.y, eye.z);
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
        light.shadowmap_color_texaddress_asset_descriptor_id = fb_sm_blur_xy.asset_descriptor;
       // auto fb_shadowmap_texarray = state->render_pipeline.framebuffers[state->pipeline_elements.fb_shadowmap_texarray];
       // light.shadowmap_color_texaddress_asset_descriptor_id = fb_shadowmap_texarray.asset_descriptor;
    }

    MeshFromAssetFlags shadowmap_mesh_flags = {};
    ImGui::Checkbox("Cull front", &state->debug->cull_front);
    shadowmap_mesh_flags.cull_front = state->debug->cull_front ? (uint32_t)1 : (uint32_t)0;
    MeshFromAssetFlags three_dee_mesh_flags = {};
    three_dee_mesh_flags.attach_shadowmap = 1;
    three_dee_mesh_flags.attach_shadowmap_color = 1;

    MeshFromAssetFlags three_dee_mesh_flags_debug = three_dee_mesh_flags;
    //three_dee_mesh_flags_debug.debug = 1;

    v2i dog_location2 = {
        (int32_t)roundf(dog_location.x) + TERRAIN_MAP_CHUNK_WIDTH / 2,
        (int32_t)roundf(dog_location.z) + TERRAIN_MAP_CHUNK_HEIGHT / 2,
    };
    v2i cowboy_location2 = {
        (int32_t)roundf(cowboy_location.x) + TERRAIN_MAP_CHUNK_WIDTH / 2,
        (int32_t)roundf(cowboy_location.z) + TERRAIN_MAP_CHUNK_HEIGHT / 2,
    };
    static v2i target_tile = {
        TERRAIN_MAP_CHUNK_WIDTH / 2 + 1,
        TERRAIN_MAP_CHUNK_HEIGHT / 2 + 1,
    };
    static v2i target_tile2 = {
        TERRAIN_MAP_CHUNK_WIDTH / 2 - 1,
        TERRAIN_MAP_CHUNK_HEIGHT / 2 - 1,
    };
    {
        MouseInput *mouse = &input->mouse;

        if (BUTTON_ENDED_DOWN(mouse->buttons.left))
        {
            Plane p = {
                {0,1,0},
                0,
            };
            Ray r = screen_to_ray(state);
            r.location = state->camera.location;

            v3 b = v3mul(r.direction, 1000);
            float t;
            v3 q;
            if (segment_plane_intersect(r.location, b, p, &t, &q))
            {
                ImGui::Text("v3 plane location: %0.2f, %0.2f, %0.2f",
                    q.x, q.y, q.z);
                v2 plane_location = {q.x, q.z};
                ImGui::Text("Plane location: %0.2f, %0.2f", plane_location.x, plane_location.y);
                target_tile = {
                    TERRAIN_MAP_CHUNK_WIDTH / 2 + (int32_t)roundf(plane_location.x),
                    TERRAIN_MAP_CHUNK_HEIGHT / 2 + (int32_t)roundf(plane_location.y),
                };
            }
        }

        if (BUTTON_ENDED_DOWN(mouse->buttons.right))
        {
            Plane p = {
                {0,1,0},
                0,
            };
            Ray r = screen_to_ray(state);
            r.location = state->camera.location;

            v3 b = v3mul(r.direction, 1000);
            float t;
            v3 q;
            if (segment_plane_intersect(r.location, b, p, &t, &q))
            {
                ImGui::Text("v3 plane location: %0.2f, %0.2f, %0.2f",
                    q.x, q.y, q.z);
                v2 plane_location = {q.x, q.z};
                ImGui::Text("Plane location: %0.2f, %0.2f", plane_location.x, plane_location.y);
                target_tile2 = {
                    TERRAIN_MAP_CHUNK_WIDTH / 2 + (int32_t)roundf(plane_location.x),
                    TERRAIN_MAP_CHUNK_HEIGHT / 2 + (int32_t)roundf(plane_location.y),
                };
            }
        }
    }

    auto path = &state->cowboy_path;
    do_path_stuff(state, path, cowboy_location2, target_tile);
    do_path_stuff(state, &state->dog_path, dog_location2, target_tile2);

    apply_path(
        state,
        path,
        &cowboy_location2,
        &cowboy_location,
        &state->cowboy_rotation,
        &target_tile,
        state->asset_ids->blocky_advanced_mesh,
        ArmatureIds::test1,
        input->delta_t
        );

    apply_path(
        state,
        &state->dog_path,
        &dog_location2,
        &dog_location,
        &state->dog_rotation,
        &target_tile2,
        state->asset_ids->dog2_mesh,
        ArmatureIds::test2,
        input->delta_t
        );

    //advance_armature(state, state->assets.descriptors + state->asset_ids.blocky_advanced_mesh2, state->armatures + (uint32_t)ArmatureIds::test2, input->delta_t * 3, dog_speed, dog_time_to_idle);


    {
        array2p<float> noise2p = state->noisemaps[12].array2p();
        v2i middle = {
            (int32_t)(noise2p.width / 2),
            (int32_t)(noise2p.height / 2),
        };
        float height = roundf(
            cubic_bezier(
                state->debug->perlin.control_point_0,
                state->debug->perlin.control_point_1,
                noise2p.get(middle)).y * state->debug->perlin.height_multiplier) / 4.0f + 0.1f;


        m4 model = m4mul(
            m4translate({cowboy_location.x,height,cowboy_location.z}),
            m4rotation({0,1,0}, state->cowboy_rotation)
        );
        add_mesh_to_render_lists(
            state,
            model,
            state->asset_ids->blocky_advanced_mesh,
            state->asset_ids->blocky_advanced_texture,
            (int32_t)ArmatureIds::test1);

        /*
        model = m4translate({0, height + 0.5f, 0});
        MeshFromAssetFlags cube_flags = {};
        cube_flags.depth_disabled = 1;
        PushMeshFromAsset(
            &state->pipeline_elements.rl_three_dee.list,
            (uint32_t)matrix_ids::mesh_projection_matrix,
            (uint32_t)matrix_ids::mesh_view_matrix,
            model,
            state->asset_ids.cube_bounds_mesh,
            state->asset_ids.white_texture,
            0,
            -1,
            cube_flags,
            ShaderType::standard
        );
        */
        /*
        {
            m4 model = m4scale(0.5);
            MeshFromAssetFlags flags = {};
            flags.depth_disabled = 1;
            PushMeshFromAsset(
                &state->pipeline_elements.rl_three_dee_debug.list,
                (uint32_t)matrix_ids::mesh_projection_matrix,
                (uint32_t)matrix_ids::mesh_view_matrix,
                model,
                state->asset_ids.ground_plane_mesh,
                state->asset_ids.square_texture,
                0,
                -1,
                flags,
                ShaderType::standard
            );
        }
        */

        v2i noise_location = {
            (int32_t)(noise2p.width / 2) + 1,
            (int32_t)(noise2p.height / 2 - 1)
        };
        height = roundf(
            cubic_bezier(
                state->debug->perlin.control_point_0,
                state->debug->perlin.control_point_1,
                noise2p.get(noise_location)).y * state->debug->perlin.height_multiplier) / 4.0f + 0.1f;
        model = m4mul(
            m4translate({dog_location.x,height,dog_location.z}),
            m4rotation({0,1,0}, state->dog_rotation)
        );
        add_mesh_to_render_lists(
            state,
            model,
            state->asset_ids->dog2_mesh,
            state->asset_ids->dog2_texture,
            (int32_t)ArmatureIds::test2);

        /*
        model = m4translate({2,height + 0.5f,2});
        PushMeshFromAsset(
            &state->pipeline_elements.rl_three_dee.list,
            (uint32_t)matrix_ids::mesh_projection_matrix,
            (uint32_t)matrix_ids::mesh_view_matrix,
            model,
            state->asset_ids.cube_bounds_mesh,
            state->asset_ids.white_texture,
            0,
            -1,
            cube_flags,
            ShaderType::standard
        );
        */
    }


    for (uint32_t i = 0; i < harray_count(state->noisemaps_data); ++i)
    {
        auto &noisemap_data = state->noisemaps_data[i];
        auto &noisemap_id = i;
        auto &offset = noisemap_data.offset;

        v3 translate = {
            2 * offset.x,
            0,
            -2 * offset.y,
        };

        m4 model = m4translate(translate);

        add_mesh_to_render_lists(state, model, state->test_mesh_descriptors[noisemap_id], state->test_texture_descriptors[noisemap_id], -1);
    }
    {
        state->pipeline_elements.rl_three_dee_water.list.config.reflection_asset_descriptor = state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_reflection].asset_descriptor;
        state->pipeline_elements.rl_three_dee_water.list.config.refraction_asset_descriptor = state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_refraction].asset_descriptor;
        state->pipeline_elements.rl_three_dee_water.list.config.refraction_depth_asset_descriptor = state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_refraction].depth_asset_descriptor;
        state->pipeline_elements.rl_three_dee_water.list.config.dudv_map_asset_descriptor = state->asset_ids->water_dudv;
        state->pipeline_elements.rl_three_dee_water.list.config.normal_map_asset_descriptor = state->asset_ids->water_normal;
        state->pipeline_elements.rl_three_dee_water.list.config.camera_position = state->camera.location;
        state->pipeline_elements.rl_three_dee_water.list.config.near_ = state->camera.near_;
        state->pipeline_elements.rl_three_dee_water.list.config.far_ = state->camera.far_;
        m4 model = m4mul(
            m4scale({500,0.1f,500}),
            m4translate({-0,-1.1f,-1})
        );

        MeshFromAssetFlags mesh_flags = {};
        mesh_flags.attach_shadowmap = 1;
        mesh_flags.attach_shadowmap_color = 1;
        PushMeshFromAsset(
            &state->pipeline_elements.rl_three_dee_water.list,
            (uint32_t)matrix_ids::mesh_projection_matrix,
            (uint32_t)matrix_ids::mesh_view_matrix,
            model,
            state->asset_ids->cube_mesh,
            state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_refraction].asset_descriptor,
            //state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_reflection].asset_descriptor,
            1,
            -1,
            mesh_flags,
            ShaderType::water
        );
    }

    {
        PushClear(&state->pipeline_elements.rl_sky.list, {1.0f, 1.0f, 0.4f, 1.0f});
        PushSky(&state->pipeline_elements.rl_sky.list,
            (int32_t)matrix_ids::mesh_projection_matrix,
            (int32_t)matrix_ids::mesh_view_matrix,
            v3normalize(light.direction));
    }

    PushSky(&state->pipeline_elements.rl_reflection.list,
        (int32_t)matrix_ids::mesh_projection_matrix,
        (int32_t)matrix_ids::reflected_mesh_view_matrix,
        v3normalize(light.direction));

    PushQuad(&state->pipeline_elements.rl_framebuffer.list, {0,0},
            {(float)input->window.width, (float)input->window.height},
            {1,1,1,1}, 0,
            state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_main].asset_descriptor
    );

#if 0
    {
        int32_t descriptors_to_show[] =
        {
            state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_refraction].asset_descriptor,
            state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_refraction].depth_asset_descriptor,
            state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_reflection].asset_descriptor,
        };

        float width = 200;
        float gap = 100;
        float start_x = 0;

        for (uint32_t i = 0; i < harray_count(descriptors_to_show); ++i)
        {
            PushQuad(&state->pipeline_elements.rl_framebuffer.list, {start_x + gap,100},
                    {width, 200},
                    {1,1,1,1}, 0,
                    descriptors_to_show[i]
            );
            start_x += gap + width;
        }
    }
#endif

    v3 mouse_bl = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y), 0.0f};
    v3 mouse_size = {16.0f, -16.0f, 0.0f};

    PushQuad(&state->pipeline_elements.rl_framebuffer.list, mouse_bl, mouse_size, {1,1,1,1}, 0, state->asset_ids->mouse_cursor);

    if (state->debug->show_textures) {
        ImGui::Begin("Textures", &state->debug->show_textures);

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

    if (state->debug->show_arena) {
        arena_debug(_hidden_state, &state->debug->show_arena);
    }
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    _platform = memory->platform_api;
    GlobalDebugTable = memory->debug_table;
    TIMED_FUNCTION();

    if (memory->imgui_state)
    {
        ImGui::SetCurrentContext((ImGuiContext *)memory->imgui_state);
    }

    if (!memory->game_block)
    {
        memory->game_block = _platform->allocate_memory("game", 256 * 1024 * 1024);
        bootstrap_memory_arena(memory->game_block, game_state, arena);
    }

    game_state *state = (game_state *)memory->game_block->base;
    _hidden_state = state;
    if (!state->initialized)
    {
        state->initialized = 1;
        //initialize(memory, state);
        auto *asset_descriptors = &state->assets;
        //state->asset_ids.mouse_cursor = add_asset(asset_descriptors, "mouse_cursor");
        state->asset_ids.plane_mesh = add_asset(asset_descriptors, "plane_mesh");
        state->asset_ids.ground_plane_mesh = add_asset(asset_descriptors, "ground_plane_mesh");
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
        state->asset_ids.square_texture = add_asset(asset_descriptors, "square_texture");
        state->asset_ids.knp_plate_grass = add_asset(asset_descriptors, "knp_Plate_Grass_01");
        state->asset_ids.dog2_mesh = add_asset(asset_descriptors, "dog2_mesh");
        state->asset_ids.dog2_texture = add_asset(asset_descriptors, "dog2_texture");

        state->asset_ids.water_normal = add_asset(asset_descriptors, "water_normal");
        state->asset_ids.water_dudv = add_asset(asset_descriptors, "water_dudv");

        state->asset_ids.familiar = add_asset(asset_descriptors, "familiar");

        state->debug.show_textures = 0;
        state->debug.show_lights = 0;
        state->debug.cull_front = 1;
        state->debug.show_nature_pack = 0;
        state->debug.show_camera = 0;

        state->debug.debug_system = PushStruct("debug_system", &state->arena, DebugSystem);

        hassert(state->asset_ids.familiar > 0);

        state->active_demo = 0;
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

    demo_data demoes[] = {
        {
            "Simple Scene",
            &demo_a,
        },
        {
            "Cowboy in terrain",
            &demo_cowboy,
        },
        {
            "ImGui demo",
            &demo_imgui,
        },
    };

    int32_t previous_demo = state->active_demo;
    for (int32_t i = 0; i < harray_count(demoes); ++i)
    {
        ImGui::RadioButton(demoes[i].name, &state->active_demo, i);
    }

    demo_context demo_ctx = {};
    demo_ctx.switched = previous_demo != state->active_demo;
    demoes[state->active_demo].func(memory, input, sound_output, &demo_ctx);
    return;
}
