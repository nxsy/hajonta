#pragma once

DEMO(demo_bounce)
{
    glPointSize(5.0f);
    game_state *state = (game_state *)memory->memory;
    demo_bounce_state *demo_state = &state->bounce;

    glClearColor(0.1f, 0.0f, 0.0f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!demo_state->vbo)
    {
        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
        glErrorAssert();

        for (uint32_t ball_index = 0;
                ball_index < harray_count(demo_state->position);
                ++ball_index)
        {
            demo_state->position[ball_index] = {0.f,4.0f+1.5f*(float)ball_index};
            demo_state->velocity[ball_index] = {0,-1.0f};
        }
    }

    float delta_t = input->delta_t;
    for (uint32_t i = 0;
            i < harray_count(input->controllers);
            ++i)
    {
        if (!input->controllers[i].is_active)
        {
            continue;
        }

        game_controller_state *controller = &input->controllers[i];
        if (controller->buttons.back.ended_down)
        {
            state->active_demo = 0;
        }
    }

    float old_t = demo_state->t;

    v2 old_rotated_line_location = {cosf(old_t * (pi/4)), sinf(old_t * (pi/4))};
    old_rotated_line_location = v2normalize(old_rotated_line_location);
    old_rotated_line_location = v2mul(old_rotated_line_location, 0.75f);

    demo_state->t += delta_t;
    if (demo_state->t > 60)
    {
        demo_state->t -= 60;
    }

    v2 rotated_line_location = {cosf(demo_state->t * (pi/4)), sinf(demo_state->t * (pi/4))};
    rotated_line_location = v2normalize(rotated_line_location);
    rotated_line_location = v2mul(rotated_line_location, 0.75f);

    v2 line_centers[] =
    {
        { 2.0f, 2.0f},
        { 2.0f, 0.0f},
        { 2.0f,-2.0f},
        { 1.0f, 1.0f},
        { 1.0f,-1.0f},
        //{ 0.0f, 2.0f},
        { 0.0f, 0.0f},
        { 0.0f,-2.0f},
        {-1.0f, 1.0f},
        {-1.0f,-1.0f},
        {-2.0f, 2.0f},
        {-2.0f, 0.0f},
        {-2.0f,-2.0f},
    };

    struct line_pair {
        line2 old_line;
        line2 new_line;
    };

    line_pair lines[harray_count(line_centers)];
    for (uint32_t line_index = 0;
            line_index < harray_count(line_centers);
            ++line_index)
    {
        v2 l2c = line_centers[line_index];
        lines[line_index] = {
            {v2add(l2c, rotated_line_location), v2mul(rotated_line_location, -2)},
            {v2add(l2c, old_rotated_line_location), v2mul(old_rotated_line_location, -2)},
        };
    }


    for (uint32_t ball_index = 0;
            ball_index < harray_count(demo_state->position);
            ++ball_index)
    {
        v2 acceleration = {0,-9.8f};
        acceleration = v2normalize(acceleration);
        acceleration = v2sub(acceleration, v2mul(demo_state->velocity[ball_index], 0.05f));
        v2 movement = v2add(
                v2mul(acceleration, 0.5f * (delta_t * delta_t)),
                v2mul(demo_state->velocity[ball_index], delta_t)
        );
        demo_state->velocity[ball_index] = v2add(
                v2mul(acceleration, delta_t),
                demo_state->velocity[ball_index]
        );

        int num_intersects = 0;
        while (v2length(movement) > 0)
        {
            if (num_intersects++ > 8)
            {
                // give up after trying for so long
                demo_state->position[ball_index] = v2add(demo_state->position[ball_index], movement);
                break;
            }
            line2 movement_line = {demo_state->position[ball_index], movement};
            line2 *intersecting_line = {};
            v2 closest_intersect_point = {};
            float closest_length = -1;
            for (uint32_t i = 0;
                    i < harray_count(lines);
                    ++i)
            {
                v2 intersect_point;
                line_pair *lp = lines + i;
                if (line_intersect(movement_line, lp->old_line, &intersect_point)) {
                    float distance_to = v2length(v2sub(intersect_point, demo_state->position[ball_index]));
                    if ((closest_length < 0) || (closest_length > distance_to))
                    {
                        closest_intersect_point = intersect_point;
                        closest_length = distance_to;
                        intersecting_line = &lp->old_line;
                    }
                }
                if (line_intersect(movement_line, lp->new_line, &intersect_point)) {
                    float distance_to = v2length(v2sub(intersect_point, demo_state->position[ball_index]));
                    if ((closest_length < 0) || (closest_length > distance_to))
                    {
                        closest_intersect_point = intersect_point;
                        closest_length = distance_to;
                        intersecting_line = &lp->new_line;
                    }
                }
            }

            if (!intersecting_line)
            {
                demo_state->position[ball_index] = v2add(demo_state->position[ball_index], movement);
                break;
            }
            else
            {
                demo_state->num_collisions += 1;
                v2 k = closest_intersect_point;
                v2 used_movement = v2sub(k, demo_state->position[ball_index]);
                demo_state->position[ball_index] = v2add(demo_state->position[ball_index], used_movement);

                v2 bounce_unused_direction = v2sub(v2add(movement_line.position, movement_line.direction), k);
                v2 bouncing_projection = v2projection(intersecting_line->direction, bounce_unused_direction);

                v2 bouncing_reflection = v2sub(bouncing_projection, bounce_unused_direction);
                bouncing_reflection = v2add(bouncing_projection, bouncing_reflection);
                bouncing_reflection = v2normalize(bouncing_reflection);

                movement = v2mul(bouncing_reflection, v2length(bounce_unused_direction));
                v2 epsilon_movement = v2mul(bouncing_reflection, 0.0001f);
                demo_state->velocity[ball_index] = v2mul(bouncing_reflection, v2length(demo_state->velocity[ball_index]));

                demo_state->position[ball_index] = v2add(demo_state->position[ball_index], epsilon_movement);
                demo_state->position[ball_index] = v2add(demo_state->position[ball_index], movement);
                break;
            }
        }
    }

    glUseProgram(state->program_b.program);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);

    glEnableVertexAttribArray((GLuint)state->program_b.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_color_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_style_id);
    glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), 0);
    glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex, color));
    glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex_with_style, style));

    vertex_with_style vertices[8 + 4*harray_count(lines)] = {
        // dotted y axis +
        { { { 0, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, }, { 0.02f, 0.002f, 0.0f, 0.0f,}, },
        { { { 0, 20, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0} }, { 0.02f, 0.002f, 0.0f, 1.0f}, },

        // dotted y axis -
        { { { 0, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, }, { 0.02f, 0.002f, 0.0f, 0.0f,}, },
        { { { 0, -20, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0} }, { 0.02f, 0.002f, 0.0f, 1.0f}, },

        // dotted x axis -
        { { { 0, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, }, { 0.02f, 0.002f, 0.0f, 0.0f,}, },
        { { { -20, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0} }, { 0.02f, 0.002f, 0.0f, 1.0f}, },

        // dotted x axis +
        { { { 0, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, }, { 0.02f, 0.002f, 0.0f, 0.0f,}, },
        { { { 20, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0} }, { 0.02f, 0.002f, 0.0f, 1.0f}, },
    };

    for (uint32_t line_index = 0;
            line_index < harray_count(lines);
            ++line_index)
    {
        line_pair *lp = lines + line_index;
        line2 *l = &lp->new_line;
        vertices[8+4*line_index] = { { { l->position.x, l->position.y, 0.0, 1.0}, {1.0, 0.0, 1.0, 1.0}, }, { 0,0,0,0}, };
        vertices[8+4*line_index+1] =
            { { { l->position.x + l->direction.x, l->position.y + l->direction.y, 0.0, 1.0}, {1.0, 0.0, 1.0, 1.0} }, { 0,0,0,0}, };
        l = &lp->old_line;
        vertices[8+4*line_index+2] = { { { l->position.x, l->position.y, 0.0, 1.0}, {0.0, 0.0, 1.0, 0.5}, }, { 0,0,0,0}, };
        vertices[8+4*line_index+3] =
            { { { l->position.x + l->direction.x, l->position.y + l->direction.y, 0.0, 1.0}, {0.0, 0.0, 1.0, 0.5} }, { 0,0,0,0}, };
    }
    float ratio = 960.0f / 540.0f;
    for (uint32_t vi = 0;
            vi < harray_count(vertices);
            ++vi)
    {
        vertex_with_style *v = vertices + vi;
        v->v.position[0] = v->v.position[0] / 5.0f / ratio * 1.5f;
        v->v.position[1] = v->v.position[1] / 5.0f * 1.5f;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    float position[] = {0,0};
    glUniform2fv(state->program_b.u_offset_id, 1, (float *)position);
    glDrawArrays(GL_LINES, 0, harray_count(vertices));

    vertex_with_style point_vertices[harray_count(demo_state->position)];
    for (uint32_t ball_index = 0;
            ball_index < harray_count(demo_state->position);
            ++ball_index)
    {
        point_vertices[ball_index] = { { { demo_state->position[ball_index].x, demo_state->position[ball_index].y, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, }, { 0, 0, 0, 0}, };
    }
    for (uint32_t vi = 0;
            vi < harray_count(point_vertices);
            ++vi)
    {
        vertex_with_style *v = point_vertices + vi;
        v->v.position[0] = v->v.position[0] / 5.0f / ratio * 1.5f;
        v->v.position[1] = v->v.position[1] / 5.0f * 1.5f;
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(point_vertices), point_vertices, GL_STATIC_DRAW);
    glDrawArrays(GL_POINTS, 0, harray_count(vertices));

    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    char msg[1024];
    sprintf(msg, "C: %d", demo_state->num_collisions);
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg);

    for (uint32_t ball_index = 0;
            ball_index < harray_count(demo_state->position);
            ++ball_index)
    {
        if (demo_state->position[ball_index].y < -5)
        {
            demo_state->position[ball_index] = {0, 5+0.5f*(float)ball_index};
            demo_state->velocity[ball_index] = {0+sinf(demo_state->t + ball_index),-1};
        }
        if (demo_state->position[ball_index].x < -5)
        {
            demo_state->position[ball_index] = {0, 5+0.5f*(float)ball_index};
            demo_state->velocity[ball_index] = {0+sinf(demo_state->t + ball_index),-1};
        }
        if (demo_state->position[ball_index].x > 5)
        {
            demo_state->position[ball_index] = {0, 5+0.5f*(float)ball_index};
            demo_state->velocity[ball_index] = {0+sinf(demo_state->t + ball_index),-1};
        }
    }
}

