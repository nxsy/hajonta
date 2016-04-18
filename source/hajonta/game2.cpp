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

#include "hajonta/game2/game2.h"

game_state *_hidden_state;

#include "hajonta/game2/two_dee.cpp"

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
        asset_descriptors->descriptors[asset_descriptors->count].debug = debug;
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
add_framebuffer_asset(AssetDescriptors *asset_descriptors, FramebufferDescriptor *framebuffer, bool debug = false)
{
    int32_t result = -1;
    if (asset_descriptors->count < harray_count(asset_descriptors->descriptors))
    {
        asset_descriptors->descriptors[asset_descriptors->count].type = asset_descriptor_type::framebuffer;
        asset_descriptors->descriptors[asset_descriptors->count].framebuffer = framebuffer;
        asset_descriptors->descriptors[asset_descriptors->count].debug = debug;
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
        asset_descriptors->descriptors[asset_descriptors->count].debug = debug;
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
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    game_state *state = (game_state *)memory->memory;
    _hidden_state = state;

    if (!memory->initialized)
    {
        CreatePipeline(state);
        memory->initialized = 1;
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
        state->asset_ids.familiar = add_asset(asset_descriptors, "familiar");

        state->furniture_to_asset[0] = -1;
        state->furniture_to_asset[(uint32_t)FurnitureType::ship] = state->asset_ids.familiar_ship;
        state->furniture_to_asset[(uint32_t)FurnitureType::wall] = state->asset_ids.bottom_wall;

        hassert(state->asset_ids.familiar > 0);
        build_map(state, &state->map);

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
        light.direction = {1.0f, -1.0f, -1.0f};
        light.color = {1.0f, 1.0f, 1.0f};
        light.ambient_intensity = 0.2f;
        light.diffuse_intensity = 1.0f;
        light.attenuation_constant = 1.0f;

        state->debug.show_textures = 1;
        state->debug.show_lights = 1;
        state->debug.cull_front = 1;
    }

    state->frame_state.delta_t = input->delta_t;
    state->frame_state.mouse_position = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y)};
    state->frame_state.input = input;
    state->frame_state.memory = memory;

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
    static float plane_rotation = 0.0f;
    ImGui::DragFloat("Plane rotation", &plane_rotation, 0.01f, 0, 3.0f);
    rotate = m4identity();

    float _scale = 5.0f;
    translate.cols[3] = {0, -3.0f, -5.0f, 1.0f};
    scale.cols[0].E[0] = _scale;
    scale.cols[1].E[1] = 1.0f;
    scale.cols[2].E[2] = _scale;
    state->matrices[(uint32_t)matrix_ids::plane_model_matrix] = m4mul(translate, m4mul(rotate, m4mul(scale, local_translate)));
    rotate = m4identity();

    scale.cols[0].E[0] = 2.0f;
    scale.cols[1].E[1] = 2.0f;
    scale.cols[2].E[2] = 2.0f;
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
    PipelineResetData prd = {
        { input->window.width, input->window.height },
        harray_count(state->matrices), state->matrices,
        harray_count(state->assets.descriptors), (asset_descriptor *)&state->assets.descriptors,
        l,
    };

    PipelineReset(
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

    draw_map(&state->map, &state->two_dee_renderer.list, &state->two_dee_debug_renderer.list, state->debug.selected_tile);

    v3 player_size = { 1.0f, 2.0f, 0 };
    v3 player_position = {state->player_movement.position.x - 0.5f, state->player_movement.position.y, 0};
    PushQuad(&state->two_dee_renderer.list, player_position, player_size, {1,1,1,1}, 1, state->asset_ids.player);

    v3 player_position2 = {state->player_movement.position.x - 0.2f, state->player_movement.position.y, 0};
    v3 player_size2 = {0.4f, 0.1f, 0};
    PushQuad(&state->two_dee_renderer.list, player_position2, player_size2, {1,1,1,0.4f}, 1, -1);

    v3 familiar_size = { 1.0f, 2.0f, 0 };
    v3 familiar_position = {state->familiar_movement.position.x - 0.5f, state->familiar_movement.position.y, 0};
    PushQuad(&state->two_dee_renderer.list, familiar_position, familiar_size, {1,1,1,1}, 1, state->asset_ids.familiar);

    v3 familiar_position2 = {state->familiar_movement.position.x - 0.2f, state->familiar_movement.position.y, 0};
    v3 familiar_size2 = {0.4f, 0.1f, 0};
    PushQuad(&state->two_dee_renderer.list, familiar_position2, familiar_size2, {1,1,1,0.4f}, 1, -1);

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
        PushQuad(&state->two_dee_renderer.list, quad_bl, quad_size, opacity, 0, -1);

        quad_bl = { i * 96.0f + 24.0f, 24.0f, 0.0f };
        quad_size = { 64.0f, 64.0f };
        PushQuad(&state->two_dee_renderer.list, quad_bl, quad_size, {1,1,1,1}, 0, state->furniture_to_asset[(uint32_t)type]);
    }

    MeshFromAssetFlags mesh_flags = {};
    PushMeshFromAsset(
        &state->two_dee_renderer.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::chest_model_matrix,
        state->asset_ids.cactus_mesh,
        state->asset_ids.cactus_texture,
        1,
        mesh_flags,
        ShaderType::standard
    );

    m4 shadowmap_projection_matrix = m4orthographicprojection(0.1f, 20.0f, {-ratio * 5.0f, -5.0f}, {ratio * 5.0f, 5.0f});

    m4 shadowmap_view_matrix;
    {
        auto &light = state->lights[(uint32_t)LightIds::sun];
        v3 eye = v3sub({0,0,0}, light.direction);
        eye = v3mul(eye, 5);
        static v3 target = {0,0,0};
        ImGui::DragFloat3("Target", &target.x, 0.1f, -100.0f, 100.0f);
        v3 up = {0, 1, 0};

        shadowmap_view_matrix = m4lookat(eye, target, up);
    }
    state->matrices[(uint32_t)matrix_ids::light_projection_matrix] = m4mul(shadowmap_projection_matrix, shadowmap_view_matrix);

    auto &light = state->lights[(uint32_t)LightIds::sun];
    light.shadowmap_matrix_id = (uint32_t)matrix_ids::light_projection_matrix;
    {
        auto fb_shadowmap = state->render_pipeline.framebuffers[state->pipeline_elements.fb_shadowmap];
        light.shadowmap_asset_descriptor_id = fb_shadowmap.depth_asset_descriptor;
        light.shadowmap_color_asset_descriptor_id = fb_shadowmap.asset_descriptor;
    }

    MeshFromAssetFlags shadowmap_mesh_flags = {};
    ImGui::Checkbox("Cull front", &state->debug.cull_front);
    shadowmap_mesh_flags.cull_front = state->debug.cull_front ? (uint32_t)1 : (uint32_t)0;
    MeshFromAssetFlags three_dee_mesh_flags = {};
    three_dee_mesh_flags.attach_shadowmap = 1;
    three_dee_mesh_flags.attach_shadowmap_color = 1;

    PushMeshFromAsset(
        &state->three_dee_renderer.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::tree_model_matrix,
        state->asset_ids.tree_mesh,
        state->asset_ids.tree_texture,
        1,
        three_dee_mesh_flags,
        ShaderType::standard
    );

    PushMeshFromAsset(
        &state->shadowmap_renderer.list,
        (uint32_t)matrix_ids::light_projection_matrix,
        (uint32_t)matrix_ids::tree_model_matrix,
        state->asset_ids.tree_mesh,
        state->asset_ids.tree_texture,
        0,
        shadowmap_mesh_flags,
        ShaderType::variance_shadow_map
    );

    PushMeshFromAsset(&state->three_dee_renderer.list,
        (uint32_t)matrix_ids::mesh_projection_matrix,
        (uint32_t)matrix_ids::plane_model_matrix,
        state->asset_ids.cube_mesh,
        state->asset_ids.cube_texture,
        1,
        three_dee_mesh_flags,
        ShaderType::standard
    );
    PushMeshFromAsset(&state->shadowmap_renderer.list,
        (uint32_t)matrix_ids::light_projection_matrix,
        (uint32_t)matrix_ids::plane_model_matrix,
        state->asset_ids.cube_mesh,
        state->asset_ids.cube_texture,
        0,
        shadowmap_mesh_flags,
        ShaderType::variance_shadow_map
    );

    v3 mouse_bl = {(float)input->mouse.x, (float)(input->window.height - input->mouse.y), 0.0f};
    v3 mouse_size = {16.0f, -16.0f, 0.0f};
    PushQuad(&state->two_dee_renderer.list, mouse_bl, mouse_size, {1,1,1,1}, 0, state->asset_ids.mouse_cursor);

    PushQuad(&state->framebuffer_renderer.list, {0,0},
            {(float)input->window.width, (float)input->window.height},
            {1,1,1,1}, 0,
            state->render_pipeline.framebuffers[(uint32_t)state->pipeline_elements.fb_main].asset_descriptor
    );
    PushQuad(&state->framebuffer_renderer.list, mouse_bl, mouse_size, {1,1,1,1}, 0, state->asset_ids.mouse_cursor);

    PipelineRender(state, &state->render_pipeline);

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
            if (rfb.framebuffer._flags.use_depth_texture)
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
}
