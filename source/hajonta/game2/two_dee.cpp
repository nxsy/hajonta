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

            PushQuad(&_hidden_state->pipeline_elements.rl_two_dee_debug.list, pq, pq_size, {1,0,1,0.5f}, 1, -1);

            v2 n = v2normalize({-l->direction.y,  l->direction.x});
            if (v2dot(movement, n) > 0)
            {
                n = v2normalize({ l->direction.y, -l->direction.x});
            }
            v3 nq = v3add(q, v3mul({l->direction.x, l->direction.y, 0}, 0.5f));

            v3 npq = v3sub(nq, {0.05f, 0.05f, 0});
            v3 npq_size = {n.x, n.y, 0};
            npq_size = v3add(npq_size, {0.05f, 0.05f, 0});

            PushQuad(&_hidden_state->pipeline_elements.rl_two_dee_debug.list, npq, npq_size, {1,1,0,0.5f}, 1, -1);

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
                    PushQuad(&_hidden_state->pipeline_elements.rl_two_dee_debug.list, q, q_size, {0,1,0,0.5f}, 1, -1);

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

