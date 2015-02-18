#pragma once

DEMO(demo_collision)
{
    game_state *state = (game_state *)memory->memory;
    demo_collision_state *demo_state = &state->collision;

    glClearColor(0.1f, 0.0f, 0.0f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!demo_state->vbo)
    {
        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
        glErrorAssert();

        demo_state->line_starting = {-0.5f, -0.5f};
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
        if (controller->buttons.back.ended_down)
        {
            state->active_demo = 0;
        }
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

    float delta_t = input->delta_t;
    acceleration = v2normalize(acceleration);
    acceleration = v2sub(acceleration, v2mul(demo_state->line_velocity, 5.0f));
    v2 movement = v2add(
            v2mul(acceleration, 0.5f * (delta_t * delta_t)),
            v2mul(demo_state->line_velocity, delta_t)
    );
    demo_state->line_starting = v2add(demo_state->line_starting, movement);
    demo_state->line_velocity = v2add(
            v2mul(acceleration, delta_t),
            demo_state->line_velocity
    );

    glUseProgram(state->program_b.program);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);

    glEnableVertexAttribArray((GLuint)state->program_b.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_color_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_style_id);
    glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), 0);
    glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex, color));
    glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex_with_style, style));

    v2 *lv = &demo_state->line_starting;
    v2 lvn = v2normalize({lv->x-0.1f, lv->y + 0.1f});
    v2 lve = v2sub(*lv, lvn);

    line2 x_axis = { {-2, 0}, {4, 0} };
    line2 y_axis = { {0, -2}, {0, 4} };
    line2 line = {*lv, {-lvn.x,-lvn.y}};

    line2 bouncing_line = { {-1, -2}, {2, 4} };

    v2 i = {};
    bool x_intercepts = line_intersect(line, x_axis, &i);

    v2 j = {};
    bool y_intercepts = line_intersect(line, y_axis, &j);

    v2 k = {};
    line_intersect(line, bouncing_line, &k);

    v2 x_axis_reflection = v2sub(v2add(line.position, line.direction), i);
    x_axis_reflection.y *= -1;
    x_axis_reflection = v2add(i, x_axis_reflection);

    v2 y_axis_reflection = v2sub(v2add(line.position, line.direction), j);
    y_axis_reflection.x *= -1;
    y_axis_reflection = v2add(j, y_axis_reflection);

    v2 bounce_unused_direction = v2sub(v2add(line.position, line.direction), k);
    v2 bouncing_projection = v2projection(bouncing_line.direction, bounce_unused_direction);

    v2 bouncing_reflection = v2sub(bouncing_projection, v2add(line.position, line.direction));
    bouncing_reflection = v2add(bouncing_projection, bouncing_reflection);
    bouncing_reflection = v2normalize(bouncing_reflection);
    bouncing_reflection = v2mul(bouncing_reflection, v2length(bounce_unused_direction));

    vertex_with_style vertices[] = {
        // dotted y axis +
        { { { 0, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, }, { 0.02f, 0.002f, 0.0f, 0.0f,}, },
        { { { 0, 2, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0} }, { 0.02f, 0.002f, 0.0f, 1.0f}, },

        // dotted y axis -
        { { { 0, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, }, { 0.02f, 0.002f, 0.0f, 0.0f,}, },
        { { { 0, -2, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0} }, { 0.02f, 0.002f, 0.0f, 1.0f}, },

        // dotted x axis -
        { { { 0, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, }, { 0.02f, 0.002f, 0.0f, 0.0f,}, },
        { { { -2, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0} }, { 0.02f, 0.002f, 0.0f, 1.0f}, },
        // dotted x axis +
        { { { 0, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, }, { 0.02f, 0.002f, 0.0f, 0.0f,}, },
        { { { 2, 0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0} }, { 0.02f, 0.002f, 0.0f, 1.0f}, },

        // line
        { { { lv->x, lv->y, 0.0, 1.0}, {1.0, 0.0, 1.0, 1.0}, }, { 0,0,0,0}, },
        { { { lve.x, lve.y, 0.0, 1.0}, {1.0, 0.0, 1.0, 1.0} }, { 0,0,0,0}, },

        // line to x axis
        { { { lv->x, lv->y, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}, }, { 0.02f,0.014f,0,0}, },
        { { { i.x, i.y, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0} }, { 0.02f,0.014f,0,1.0}, },

        // line to y axis
        { { { lv->x, lv->y, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}, }, { 0.02f,0.007f,0,0}, },
        { { { j.x, j.y, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0} }, { 0.02f,0.007f,0,1.0}, },

        // reflection of x axis
        { { { i.x, i.y, 0.0, 1.0}, {1.0, 0.0, 0.0, x_intercepts ? 1.0f : 0.0f}, }, { 0.02f,0.014f,0,0}, },
        { { { x_axis_reflection.x, x_axis_reflection.y, 0.0, 1.0}, {1.0, 0.0, 0.0, x_intercepts ? 1.0f : 0.0f} }, { 0.02f,0.014f,0,1.0}, },

        // reflection of y axis
        { { { j.x, j.y, 0.0, 1.0}, {1.0, 0.5, 0.0, y_intercepts ? 1.0f : 0.0f}, }, { 0.02f,0.014f,0,0}, },
        { { { y_axis_reflection.x, y_axis_reflection.y, 0.0, 1.0}, {1.0, 0.5, 0.0, y_intercepts ? 1.0f : 0.0f} }, { 0.02f,0.014f,0,1.0}, },

        // bouncing_line
        { { { bouncing_line.position.x, bouncing_line.position.y, 0.0, 1.0}, {1.0, 0.5, 0.5, 1.0f}, }, { 0.02f,0.014f,0,0}, },
        { { { bouncing_line.position.x + bouncing_line.direction.x, bouncing_line.position.y + bouncing_line.direction.y, 0.0, 1.0}, {1.0, 0.5, 0.5, 1.0f} }, { 0.02f,0.014f,0,1.0}, },

        // bouncing_line projection
        { { { k.x, k.y, 0.0, 1.0}, {0.5, 0.5, 1.0, 1.0f}, }, { 0.02f,0.014f,0,0}, },
        { { { k.x+bouncing_projection.x, k.y+bouncing_projection.y, 0.0, 1.0}, {0.5, 0.5, 1.0, 1.0f} }, { 0.02f,0.014f,0,1.0}, },

        // bouncing_line projection
        { { { k.x, k.y, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0f}, }, { 0.02f,0.014f,0,0}, },
        { { { k.x+bouncing_reflection.x, k.y+bouncing_reflection.y, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0f} }, { 0.02f,0.014f,0,1.0}, },
    };

    float ratio = 960.0f / 540.0f;
    for (uint32_t vi = 0;
            vi < harray_count(vertices);
            ++vi)
    {
        vertex_with_style *v = vertices + vi;
        v->v.position[0] = v->v.position[0] / ratio * 1.5f;
        v->v.position[1] = v->v.position[1] * 1.5f;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    float position[] = {0,0};
    glUniform2fv(state->program_b.u_offset_id, 1, (float *)position);
    glDrawArrays(GL_LINES, 0, harray_count(vertices));
}
