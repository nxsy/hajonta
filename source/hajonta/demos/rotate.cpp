#pragma once

DEMO(demo_rotate)
{
    game_state *state = (game_state *)memory->memory;
    demo_rotate_state *demo_state = &state->demos.rotate;

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!demo_state->vbo)
    {
        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
        glGenBuffers(1, &demo_state->ibo);
        GLushort cube_elements[] = {
            // 0. front top right
            // 1. front top left
            // 2. front bottom right
            // 3. front bottom left
            // 4. back top right
            // 5. back top left
            // 6. back bottom right
            // 7. back bottom left
            //
            // front
            0, 1, 3,
            3, 2, 0,
            // bottom
            3, 2, 6,
            6, 7, 3,
            // left
            5, 1, 3,
            3, 7, 5,
            // top
            5, 4, 0,
            0, 1, 5,
            // right
            0, 4, 6,
            6, 2, 0,
            // back
            5, 4, 6,
            6, 7, 5,
        };
        demo_state->ibo_length = harray_count(cube_elements);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);

        glGenBuffers(1, &demo_state->line_ibo);
        GLushort line_elements[] = {
            0, 1, 1, 3,
            3, 2, 2, 0,
            // bottom
            3, 2, 2, 6,
            6, 7, 7, 3,
            // left
            5, 1, 1, 3,
            3, 7, 7, 5,
            // top
            5, 4, 4, 0,
            0, 1, 1, 5,
            // right
            0, 4, 4, 6,
            6, 2, 2, 0,
            // back
            5, 4, 4, 6,
            6, 7, 7, 5,
        };
        demo_state->line_ibo_length = harray_count(line_elements);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->line_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(line_elements), line_elements, GL_STATIC_DRAW);
        glErrorAssert();
        demo_state->near_ = {5.0f};
        demo_state->far_ = {50.0f};
        demo_state->spacing = {0.0f};
        demo_state->back_size = {1.0f};

        demo_state->matrix_draw_buffer.memory = demo_state->matrix_buffer;
        demo_state->matrix_draw_buffer.width = matrix_buffer_width;
        demo_state->matrix_draw_buffer.height = matrix_buffer_height;
        demo_state->matrix_draw_buffer.pitch = 4 * demo_state->matrix_draw_buffer.width;

        glGenTextures(1, &demo_state->texture_id);
        glBindTexture(GL_TEXTURE_2D, demo_state->texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            (GLsizei)demo_state->matrix_draw_buffer.width, (GLsizei)demo_state->matrix_draw_buffer.height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, demo_state->matrix_buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
        if (controller->buttons.back.ended_down)
        {
            state->demos.active_demo = 0;
        }
        if (controller->buttons.start.ended_down && !controller->buttons.start.repeat)
        {
            demo_state->frustum_mode ^= true;
        }
        if (demo_state->frustum_mode)
        {
            if (controller->buttons.move_up.ended_down)
            {
                demo_state->far_ += 1.0f;
            }
            if (controller->buttons.move_down.ended_down)
            {
                demo_state->far_ -= 1.0f;
            }
            if (controller->buttons.move_left.ended_down)
            {
                demo_state->back_size -= 0.1f;
            }
            if (controller->buttons.move_right.ended_down)
            {
                demo_state->back_size += 0.1f;
            }
        }
        else
        {
            if (controller->buttons.move_up.ended_down)
            {
                demo_state->spacing += 0.02f;
            }
            if (controller->buttons.move_down.ended_down)
            {
                demo_state->spacing -= 0.02f;
            }
            if (controller->buttons.move_left.ended_down)
            {
                demo_state->near_ -= 0.01f;
            }
            if (controller->buttons.move_right.ended_down)
            {
                demo_state->near_ += 0.01f;
            }
        }
    }
    if (demo_state->spacing < 0.00001f)
    {
        demo_state->spacing = 0.00001f;
    }
    if (demo_state->spacing > 8.0f)
    {
        demo_state->spacing = 8.0f;
    }

    if (demo_state->near_ > 4.99999f)
    {
        demo_state->near_ = 4.99999f;
    }

    demo_state->delta_t += input->delta_t;

    glUseProgram(state->program_b.program);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);

    glEnableVertexAttribArray((GLuint)state->program_b.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_color_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_style_id);
    glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), 0);
    glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex, color));
    glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex_with_style, style));

    struct mdtm{
        float s; // size_multiplier;
        float t; // time_multiplier;
    };
    mdtm multipliers[] = {
        {-0.16f, 0.5f},
        {-0.32f,-0.5f},
        {-0.48f, 1.0f},
    };
    float ratio = 960.0f / 540.0f;
    for (uint32_t circle_idx = 0;
            circle_idx < harray_count(multipliers);
            ++circle_idx)
    {
        mdtm *m = multipliers + circle_idx;
        vertex_with_style vertices[] = {
            { { {-m->s, m->s, m->s * demo_state->spacing - 5.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            { { { m->s, m->s, m->s * demo_state->spacing - 5.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            { { { m->s,-m->s, m->s * demo_state->spacing - 5.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            { { {-m->s,-m->s, m->s * demo_state->spacing - 5.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
        };

        glErrorAssert();
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glErrorAssert();
        v2 position = {0,0};
        glUniform2fv(state->program_b.u_offset_id, 1, (float *)&position);
        glErrorAssert();
        v4 u_mvp_enabled = {1.0f, 0.0f, 0.0f, 0.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        glErrorAssert();
        v3 axis = {0.0f, 0.0f, -1.0f};
        m4 u_model = m4rotation(axis, demo_state->delta_t * m->t);
        glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
        m4 u_view = m4identity();
        glUniformMatrix4fv(state->program_b.u_view_id, 1, false, (float *)&u_view);
        m4 u_perspective = m4frustumprojection(demo_state->near_, demo_state->far_, {-ratio * demo_state->back_size, -1.0f * demo_state->back_size}, {ratio * demo_state->back_size, 1.0f * demo_state->back_size});
        glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);
        glErrorAssert();
        glDrawArrays(GL_TRIANGLE_FAN, 0, harray_count(vertices));
        glErrorAssert();
    }

    for (uint32_t circle_idx = 0;
            circle_idx < harray_count(multipliers);
            ++circle_idx)
    {
        mdtm *m = multipliers + circle_idx;
        vertex_with_style vertices[] = {
            // front top right
            { { { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // front top left
            { { {-1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // front bottom right
            { { { 1.0f,-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // front bottom left
            { { {-1.0f,-1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },

            // back top right
            { { { 1.0f, 1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // back top left
            { { {-1.0f, 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // back bottom right
            { { { 1.0f,-1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
            // back bottom left
            { { {-1.0f,-1.0f,-1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, }, { 0.0f, 0.0f, 0.0f, 0.0f }, },
        };
        v3 axis = {0.1f, 0.5f, -1.0f};
        axis = v3normalize(axis);
        m4 rotate = m4rotation(axis, demo_state->delta_t);
        m4 scale = m4identity();
        m4 translate = m4identity();
        translate.cols[3] = v4{7.0f * m->t, 0.0f, -10.0f * m->t - 20.0f, 1.0f};

        m4 a = scale;
        m4 b = rotate;
        m4 c = translate;

        m4 u_model = m4mul(c, m4mul(a, b));

        v4 u_mvp_enabled = {1.0f, 0.0f, 0.0f, 0.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
        glErrorAssert();
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glErrorAssert();
        m4 u_perspective = m4frustumprojection(demo_state->near_, demo_state->far_, {-ratio * demo_state->back_size, -1.0f * demo_state->back_size}, {ratio * demo_state->back_size, 1.0f * demo_state->back_size});
        glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ibo);
        glDrawElements(GL_TRIANGLES, (GLsizei)demo_state->ibo_length, GL_UNSIGNED_SHORT, 0);

        u_mvp_enabled = {1.0f, 0.0f, 0.0f, 1.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->line_ibo);
        glDrawElements(GL_LINES, (GLsizei)demo_state->line_ibo_length, GL_UNSIGNED_SHORT, 0);
    }

    for (uint32_t circle_idx = 0;
            circle_idx < harray_count(multipliers);
            ++circle_idx)
    {
        mdtm *m = multipliers + circle_idx;
        // circle
        vertex_with_style circle_vertices[128+1];
        for (uint32_t idx = 0;
                idx < harray_count(circle_vertices);
                ++idx)
        {
            vertex_with_style *v = circle_vertices + idx;
            float a = idx * (2.0f * pi) / (harray_count(circle_vertices) - 1);
            *v = {
                {
                    {sinf(a) * m->s, cosf(a) * m->s, m->s, 1.0f},
                    {1.0f, 1.0f, 1.0f, 0.5f},
                },
                {0.0f, 0.0f, 0.0f, 0.0f},
            };
        }
        v4 u_mvp_enabled = {1.0f, 0.0f, 0.0f, 0.0f};
        glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
        m4 u_model = m4identity();
        glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
        glErrorAssert();
        glBufferData(GL_ARRAY_BUFFER, sizeof(circle_vertices), circle_vertices, GL_STATIC_DRAW);
        glErrorAssert();
        m4 u_perspective = m4identity();
        u_perspective.cols[0].E[0] = 1 / ratio;
        glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_LINE_STRIP, 0, harray_count(circle_vertices));
    }

    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    char msg[1024];
    sprintf(msg, "NEAR: %+.2f SPACING: %+.2f", demo_state->near_, demo_state->spacing);
    sprintf(msg + strlen(msg), " FAR: %+.2f BACK_SIZE: %+.2f", demo_state->far_, demo_state->back_size);
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg);
}
