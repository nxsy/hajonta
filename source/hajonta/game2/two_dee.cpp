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
astar_start(astar_data *data, void *map, v2i start_tile, v2i end_tile)
{
    _astar_init(data);
    float cost = data->initial_cost_estimate(data, start_tile, end_tile);

    auto *open_set_queue = &data->queue;
    queue_add(open_set_queue, { cost, start_tile });

    data->open_set.set(start_tile, true);
    data->g_score.set(start_tile, 0);
    data->end_tile = end_tile;
    data->start_tile = start_tile;
    data->map = map;
}

void
astar(astar_data *data, bool one_step = false)
{
    auto &open_set_queue = data->queue;
    auto &open_set = data->open_set;
    auto &closed_set = data->open_set;
    auto &end_tile = data->end_tile;
    auto &g_score = data->g_score;
    auto &came_from = data->came_from;

    v2i result = {};
    while (!data->completed && open_set_queue.num_entries)
    {
        auto current = queue_pop(&open_set_queue);
        open_set.set(current.tile_position, false);
        closed_set.set(current.tile_position, true);

        if (v2iequal(current.tile_position, end_tile))
        {
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

                if (!data->astar_passable(data, current.tile_position, neighbour_position))
                {
                    continue;

                }
                if (closed_set.get(neighbour_position))
                {
                    continue;
                }
                float tentative_g_score = g_score.get(current.tile_position) + data->neighbour_cost_estimate(data, current.tile_position, neighbour_position);
                if (open_set.get(neighbour_position))
                {
                    if (tentative_g_score >= g_score.get(neighbour_position))
                    {
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
