#pragma once

DEMO(demo_model)
{
    game_state *state = (game_state *)memory->memory;
    demo_model_state *demo_state = &state->demos.model;

    glClearColor(0.2f, 0.1f, 0.1f, 0);

    if (!demo_state->vbo)
    {
        demo_state->near_ = {5.0f};
        demo_state->far_ = {50.0f};

        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
        glGenBuffers(1, &demo_state->ibo);
        glErrorAssert();

        v3 vertices[] =
        {
            {-27.428101, 0.000000, 19.851852},
            {22.571899, 0.000000, 19.851852},
            {-27.428101, 0.000000 -10.148148},
            {22.571899, 0.000000 -10.148148},
            {-27.428101, 80.000000, 19.851852},
            {22.571899, 80.000000, 19.851852},
            {-27.428101, 80.000000 -10.148148},
            {22.571899, 80.000000 -10.148148},
        };

        face faces[] =
        {
            { { {4,9}, {2,10}, {1,7} } },
            { { {1,7}, {3,8}, {4,9} } },

            { { {8,17}, {7,18}, {5,15} } },
            { { {5,15}, {6,16}, {8,17} } },

            { { {6,6}, {5,3}, {1,2} } },
            { { {1,2}, {2,5}, {6,6} } },

            { { {8,12}, {6,6}, {2,5}, } },
            { { {2,5}, {4,11}, {8,12} } },

            { { {7,14}, {8,12}, {4,11} } },
            { { {4,11}, {3,13}, {7,14} } },

            { { {5,3}, {7,4}, {3,1} } },
            { { {3,1}, {1,2}, {5,3} } },
        };

        GLushort faces_array[harray_count(faces) * 3];
        for (uint32_t face_idx = 0;
                face_idx < harray_count(faces);
                ++face_idx)
        {
            faces_array[face_idx] = face_idx;
        }

        demo_state->ibo_length = harray_count(faces_array);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(faces_array), faces_array, GL_STATIC_DRAW);

        v3 texture_coords[] =
        {
            {1.000000,0.000000,0.000000},
            {0.814453,0.000000,0.000000},
            {0.814453,0.498047,0.000000},
            {1.000000,0.498047,0.000000},
            {0.500000,0.000000,0.000000},
            {0.500000,0.498047,0.000000},
            {0.000000,0.796875,0.000000},
            {0.000000,0.498047,0.000000},
            {0.500000,0.498047,0.000000},
            {0.500000,0.796875,0.000000},
            {0.314453,0.000000,0.000000},
            {0.314453,0.498047,0.000000},
            {0.000000,0.000000,0.000000},
            {0.000000,0.498047,0.000000},
            {0.500000,0.498047,0.000000},
            {1.000000,0.498047,0.000000},
            {1.000000,0.796875,0.000000},
            {0.500000,0.796875,0.000000},
        };

        vertex_with_style vbo_vertices[harray_count(faces) * 3];

        for (uint32_t face_idx = 0;
                face_idx < harray_count(faces);
                ++face_idx)
        {
            vertex_with_style *vbo_v1 = vbo_vertices + (3 * face_idx);
            vertex_with_style *vbo_v2 = vbo_v1 + 1;
            vertex_with_style *vbo_v3 = vbo_v2 + 1;
            face *f = faces + face_idx;
            v3 *face_v1 = vertices + (f->indices[0].vertex - 1);
            v3 *face_v2 = vertices + (f->indices[1].vertex - 1);
            v3 *face_v3 = vertices + (f->indices[2].vertex - 1);

            v3 *face_vt1 = texture_coords + (f->indices[0].texture_coord - 1);
            v3 *face_vt2 = texture_coords + (f->indices[1].texture_coord - 1);
            v3 *face_vt3 = texture_coords + (f->indices[2].texture_coord - 1);

            vbo_v1->v.position[0] = face_v1->x / 40;
            vbo_v1->v.position[1] = face_v1->y / 40;
            vbo_v1->v.position[2] = face_v1->z / 40 - 15;
            vbo_v1->v.position[3] = 1.0f;

            vbo_v2->v.position[0] = face_v2->x / 40;
            vbo_v2->v.position[1] = face_v2->y / 40;
            vbo_v2->v.position[2] = face_v2->z / 40 - 15;
            vbo_v2->v.position[3] = 1.0f;

            vbo_v3->v.position[0] = face_v3->x / 40;
            vbo_v3->v.position[1] = face_v3->y / 40;
            vbo_v3->v.position[2] = face_v3->z / 40 - 15;
            vbo_v3->v.position[3] = 1.0f;

            vbo_v1->v.color[0] = face_vt1->x;
            vbo_v1->v.color[1] = face_vt1->y;
            vbo_v1->v.color[2] = 0.0f;
            vbo_v1->v.color[3] = 1.0f;

            vbo_v1->v.color[0] += 0.3f;
            vbo_v1->v.color[1] += 0.3f;
            vbo_v1->v.color[2] += 0.3f;

            vbo_v2->v.color[0] = face_vt2->x;
            vbo_v2->v.color[1] = face_vt2->y;
            vbo_v2->v.color[2] = 0.0f;
            vbo_v2->v.color[3] = 1.0f;

            vbo_v2->v.color[0] += 0.3f;
            vbo_v2->v.color[1] += 0.3f;
            vbo_v2->v.color[2] += 0.3f;

            vbo_v3->v.color[0] = face_vt3->x;
            vbo_v3->v.color[1] = face_vt3->y;
            vbo_v3->v.color[2] = 0.0f;
            vbo_v3->v.color[3] = 1.0f;

            vbo_v3->v.color[0] += 0.3f;
            vbo_v3->v.color[1] += 0.3f;
            vbo_v3->v.color[2] += 0.3f;

            vbo_v1->style[0] = 0.0f;
            vbo_v2->style[0] = 0.0f;
            vbo_v3->style[0] = 0.0f;
        }

        for (uint32_t i = 0;
                i < harray_count(vbo_vertices);
                ++i)
        {
            vertex_with_style *v = vbo_vertices + i;
            printf("Vertex %d: ", i);
            printf("Pos: %0.2f, %0.2f, %0.2f, %0.2f   ", v->v.position[0], v->v.position[1], v->v.position[2], v->v.position[3]);
            printf("Col: %0.2f, %0.2f, %0.2f, %0.2f   ", v->v.color[0], v->v.color[1], v->v.color[2], v->v.color[3]);
            printf("\n");
        }

        glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vbo_vertices), vbo_vertices, GL_STATIC_DRAW);
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
    }

    glUseProgram(state->program_b.program);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);

    glEnableVertexAttribArray((GLuint)state->program_b.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_color_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_style_id);
    glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), 0);
    glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex, color));
    glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex_with_style, style));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ibo);

    m4 u_model = m4identity();
    v4 u_mvp_enabled = {1.0f, 0.0f, 0.0f, 0.0f};
    glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
    glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
    m4 u_view = m4identity();
    glUniformMatrix4fv(state->program_b.u_view_id, 1, false, (float *)&u_view);
    float ratio = 960.0f / 540.0f;
    m4 u_perspective = m4frustumprojection(demo_state->near_, demo_state->far_, {-ratio, -1.0f}, {ratio, 1.0f});
    glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);

    glDrawElements(GL_TRIANGLES, (GLsizei)demo_state->ibo_length, GL_UNSIGNED_SHORT, 0);
    glErrorAssert();
}
