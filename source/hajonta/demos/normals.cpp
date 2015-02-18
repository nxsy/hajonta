#pragma once

DEMO(demo_normals)
{
    game_state *state = (game_state *)memory->memory;
    demo_normals_state *demo_state = &state->demos.normals;

    glClearColor(0.1f, 0.0f, 0.0f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!demo_state->vbo)
    {
        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
        glErrorAssert();

        demo_state->line_ending = {1.0f, 1.0f};
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
            state->demos.active_demo = 0;
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
    demo_state->line_ending = v2add(demo_state->line_ending, movement);
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

    v2 *lv = &demo_state->line_ending;

    float ratio = 960.0f / 540.0f;

    v2 rhn = v2normalize({-lv->y,  lv->x});
    v2 lhn = v2normalize({ lv->y, -lv->x});

    v2 nlv = v2normalize(*lv);
    v2 nlvrh = v2add(nlv, v2mul(rhn, 0.05f));
    v2 nlvlh = v2add(nlv, v2mul(lhn, 0.05f));

    v2 xproj = v2projection(v2{1,0}, *lv);
    v2 yproj = v2projection(v2{0,1}, *lv);

    vertex_with_style vertices[] = {
        {{{ 0, 0.0, 0.0, 1.0}, {0.5, 0.5, 0.5, 0.5}}, { 0.02f, 0.003f, 0.0f, 0.0f}},
        {{{ -10, 0, 0.0, 1.0}, {0.5, 0.5, 0.5, 0.5}}, { 0.02f, 0.003f, 0.0f, 1.0f} },

        {{{ 0, 0.0, 0.0, 1.0}, {0.5, 0.5, 0.5, 0.5}}, { 0.02f, 0.003f, 0.0f, 0.0f}},
        {{{ 10, 0, 0.0, 1.0}, {0.5, 0.5, 0.5, 0.5}}, { 0.02f, 0.003f, 0.0f, 1.0f} },

        {{{ 0, 0, 0.0, 1.0}, {0.5, 0.5, 0.5, 0.5}}, { 0.02f, 0.003f, 0.0f, 0.0f}},
        {{{ 0, 10, 0.0, 1.0}, {0.5, 0.5, 0.5, 0.5}}, { 0.02f, 0.003f, 0.0f, 1.0f} },

        {{{ 0, 0, 0.0, 1.0}, {0.5, 0.5, 0.5, 0.5}}, { 0.02f, 0.003f, 0.0f, 0.0f}},
        {{{ 0, -10, 0.0, 1.0}, {0.5, 0.5, 0.5, 0.5}}, { 0.02f, 0.003f, 0.0f, 1.0f} },

        {{{ 0.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}}},
        {{{ rhn.x, rhn.y, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}}},

        {{{ 0.0, 0.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}}},
        {{{ lhn.x, lhn.y, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}}},

        {{{ nlvrh.x, nlvrh.y, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}}},
        {{{ nlvlh.x, nlvlh.y, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}}},

        {{{ 0.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 1.0, 1.0}}},
        {{{ xproj.x, xproj.y, 0.0, 1.0}, {0.0, 1.0, 1.0, 1.0}}},

        {{{ 0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}}},
        {{{ yproj.x, yproj.y, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}}},

        {{{ 0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}}},
        {{{ lv->x, lv->y, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}}},
    };

    char msg[1024];
    sprintf(msg, "L: %+.2f, %+.2f", demo_state->line_ending.x, demo_state->line_ending.y);
    sprintf(msg + strlen(msg), "NL: %+.2f, %+.2f", nlv.x, nlv.y);
    sprintf(msg + strlen(msg), "RHN: %+.2f, %+.2f", rhn.x, rhn.y);
    sprintf(msg + strlen(msg), "LHN: %+.2f, %+.2f", lhn.x, lhn.y);
    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg);
    for (uint32_t vi = 0;
            vi < harray_count(vertices);
            ++vi)
    {
        vertex_with_style *v = vertices + vi;
        v->v.position[0] = v->v.position[0] / ratio * 0.3f;
        v->v.position[1] = v->v.position[1] * 0.3f;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    float position[] = {0,0};
    glUniform2fv(state->program_b.u_offset_id, 1, (float *)position);
    glDrawArrays(GL_LINES, 0, harray_count(vertices));
}

