#pragma once

#include "hajonta/image.cpp"

DEMO(demo_model)
{
    game_state *state = (game_state *)memory->memory;
    demo_model_state *demo_state = &state->demos.model;

    glClearColor(0.2f, 0.1f, 0.1f, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!demo_state->vbo)
    {
        demo_state->near_ = {5.0f};
        demo_state->far_ = {50.0f};

        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
        glGenBuffers(1, &demo_state->ibo);
        glGenBuffers(1, &demo_state->line_ibo);
        glErrorAssert();

        v3 vertices[] =
        {
            {-27.428101f,-40.000000f, 19.851852f},
            { 22.571899f,-40.000000f, 19.851852f},
            {-27.428101f,-40.000000f,-10.148148f},
            { 22.571899f,-40.000000f,-10.148148f},
            {-27.428101f, 40.000000f, 19.851852f},
            { 22.571899f, 40.000000f, 19.851852f},
            {-27.428101f, 40.000000f,-10.148148f},
            { 22.571899f, 40.000000f,-10.148148f},
        };

        v3 texture_coords[] =
        {
            {1.000000f, 0.000000f ,0.000000f},
            {0.814453f, 0.000000f, 0.000000f},
            {0.814453f, 0.498047f, 0.000000f},
            {1.000000f, 0.498047f, 0.000000f},
            {0.500000f, 0.000000f, 0.000000f},
            {0.500000f, 0.498047f, 0.000000f},
            {0.000000f, 0.796875f, 0.000000f},
            {0.000000f, 0.498047f, 0.000000f},
            {0.500000f, 0.498047f, 0.000000f},
            {0.500000f, 0.796875f, 0.000000f},
            {0.314453f, 0.000000f, 0.000000f},
            {0.314453f, 0.498047f, 0.000000f},
            {0.000000f, 0.000000f, 0.000000f},
            {0.000000f, 0.498047f, 0.000000f},
            {0.500000f, 0.498047f, 0.000000f},
            {1.000000f, 0.498047f, 0.000000f},
            {1.000000f, 0.796875f, 0.000000f},
            {0.500000f, 0.796875f, 0.000000f},
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
                face_idx < harray_count(faces_array);
                ++face_idx)
        {
            faces_array[face_idx] = (GLushort)face_idx;
        }

        GLushort lines_array[harray_count(faces) * 6];
        int num_line_elements = 0;
        for (uint32_t face_array_idx = 0;
                face_array_idx < harray_count(faces_array);
                face_array_idx += 3)
        {
            lines_array[num_line_elements++] = face_array_idx;
            lines_array[num_line_elements++] = (GLushort)(face_array_idx + 1);
            lines_array[num_line_elements++] = (GLushort)(face_array_idx + 1);
            lines_array[num_line_elements++] = (GLushort)(face_array_idx + 2);
            lines_array[num_line_elements++] = (GLushort)(face_array_idx + 2);
            lines_array[num_line_elements++] = face_array_idx;
        }
        demo_state->line_ibo_length = harray_count(lines_array);

        demo_state->ibo_length = harray_count(faces_array);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(faces_array), faces_array, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->line_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lines_array), lines_array, GL_STATIC_DRAW);

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

            vbo_v1->v.position[0] = face_v1->x;
            vbo_v1->v.position[1] = face_v1->y;
            vbo_v1->v.position[2] = face_v1->z;
            vbo_v1->v.position[3] = 1.0f;

            vbo_v2->v.position[0] = face_v2->x;
            vbo_v2->v.position[1] = face_v2->y;
            vbo_v2->v.position[2] = face_v2->z;
            vbo_v2->v.position[3] = 1.0f;

            vbo_v3->v.position[0] = face_v3->x;
            vbo_v3->v.position[1] = face_v3->y;
            vbo_v3->v.position[2] = face_v3->z;
            vbo_v3->v.position[3] = 1.0f;

            vbo_v1->v.color[0] = face_vt1->x;
            vbo_v1->v.color[1] = 1 - face_vt1->y;
            vbo_v1->v.color[2] = 0.0f;
            vbo_v1->v.color[3] = 1.0f;

            vbo_v2->v.color[0] = face_vt2->x;
            vbo_v2->v.color[1] = 1 - face_vt2->y;
            vbo_v2->v.color[2] = 0.0f;
            vbo_v2->v.color[3] = 1.0f;

            vbo_v3->v.color[0] = face_vt3->x;
            vbo_v3->v.color[1] = 1 - face_vt3->y;
            vbo_v3->v.color[2] = 0.0f;
            vbo_v3->v.color[3] = 1.0f;

            vbo_v1->style[0] = 2.0f;
            vbo_v2->style[0] = 2.0f;
            vbo_v3->style[0] = 2.0f;
        }

        glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vbo_vertices), vbo_vertices, GL_STATIC_DRAW);

        struct file_and_size
        {
            char *filename;
            uint32_t size;
        };

        file_and_size f_s[] = {
            {"models/minimalist-dudes/m0.jpg", 27970},
            {"models/minimalist-dudes/m1.jpg", 34060},
            {"models/minimalist-dudes/m2.jpg", 45223},
            {"models/minimalist-dudes/m3.jpg", 58295},
        };

        glGenTextures(4, demo_state->texture_ids);
        for (uint32_t file_idx = 0;
                file_idx < harray_count(f_s);
                ++file_idx)
        {
            file_and_size *f = f_s + file_idx;
            uint8_t jpg[100000];
            if (!memory->platform_load_asset(ctx, f->filename, f->size, jpg)) {
                memory->platform_fail(ctx, "Could not load models/minimalist-dudes/m0.jpg");
                memory->quit = true;
                return;
            }
            demo_state->model_buffer.memory = demo_state->model_bitmap;
            demo_state->model_buffer.width = 512;
            demo_state->model_buffer.height = 512;
            demo_state->model_buffer.pitch = 512;
            load_image(jpg, f->size, demo_state->model_bitmap, sizeof(demo_state->model_bitmap));

            glBindTexture(GL_TEXTURE_2D, demo_state->texture_ids[file_idx]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                (GLsizei)demo_state->model_buffer.width, (GLsizei)demo_state->model_buffer.height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, demo_state->model_buffer.memory);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
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
        if (controller->buttons.move_up.ended_down && !controller->buttons.move_up.repeat)
        {
            demo_state->current_texture_idx++;
        }
        if (controller->buttons.move_down.ended_down && !controller->buttons.move_down.repeat)
        {
            demo_state->current_texture_idx--;
        }
        if (controller->buttons.move_right.ended_down && !controller->buttons.move_right.repeat)
        {
            demo_state->model_mode = (demo_state->model_mode + 1) % 2;
        }
    }

    uint32_t posted_idx = 0;
    char posted[MAX_KEYBOARD_INPUTS] = {};
    keyboard_input *base = input->keyboard_inputs;
    for (uint32_t i = 0;
            i < MAX_KEYBOARD_INPUTS;
            ++i)
    {
        keyboard_input *ki = base + i;
        if (ki->type == keyboard_input_type::ASCII)
        {
            char msg[100];
            sprintf(msg, "char %c\n", ki->ascii);
            memory->platform_debug_message(ctx, msg);
            posted[posted_idx++] = ki->ascii;
        }
    }
    if (posted[0])
    {
        char msg[100];
        sprintf(msg, "posted: %s\n", posted);
        memory->platform_debug_message(ctx, msg);
    }

    demo_state->current_texture_idx %= harray_count(demo_state->texture_ids);

    demo_state->delta_t += input->delta_t;

    glUseProgram(state->program_b.program);
    glBindTexture(GL_TEXTURE_2D, demo_state->texture_ids[demo_state->current_texture_idx]);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);

    glEnableVertexAttribArray((GLuint)state->program_b.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_color_id);
    glEnableVertexAttribArray((GLuint)state->program_b.a_style_id);
    glVertexAttribPointer((GLuint)state->program_b.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), 0);
    glVertexAttribPointer((GLuint)state->program_b.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex, color));
    glVertexAttribPointer((GLuint)state->program_b.a_style_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_with_style), (void *)offsetof(vertex_with_style, style));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ibo);

    v3 axis = {0.0f, 1.0f, 0.0f};
    m4 rotate = m4rotation(axis, demo_state->delta_t);
    m4 scale = m4identity();
    scale.cols[0].E[0] = 0.025f;
    scale.cols[1].E[1] = 0.025f;
    scale.cols[2].E[2] = 0.025f;
    m4 translate = m4identity();
    translate.cols[3] = v4{0.0f, 0.0f, -6.0f, 1.0f};

    m4 a = scale;
    m4 b = rotate;
    m4 c = translate;

    m4 u_model = m4mul(c, m4mul(a, b));

    v4 u_mvp_enabled = {1.0f, 0.0f, 0.0f, 0.0f};
    glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
    glUniformMatrix4fv(state->program_b.u_model_id, 1, false, (float *)&u_model);
    m4 u_view = m4identity();
    glUniformMatrix4fv(state->program_b.u_view_id, 1, false, (float *)&u_view);
    float ratio = 960.0f / 540.0f;
    m4 u_perspective = m4frustumprojection(demo_state->near_, demo_state->far_, {-ratio, -1.0f}, {ratio, 1.0f});
    //m4 u_perspective = m4identity();
    glUniformMatrix4fv(state->program_b.u_perspective_id, 1, false, (float *)&u_perspective);

    glUniform1i(state->program_b.u_model_mode_id, demo_state->model_mode);
    glDrawElements(GL_TRIANGLES, (GLsizei)demo_state->ibo_length, GL_UNSIGNED_SHORT, 0);
    glErrorAssert();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->line_ibo);
    u_mvp_enabled = {1.0f, 0.0f, 0.0f, 1.0f};
    glUniform4fv(state->program_b.u_mvp_enabled_id, 1, (float *)&u_mvp_enabled);
    glUniform1i(state->program_b.u_model_mode_id, 0);
    glDrawElements(GL_LINES, (GLsizei)demo_state->line_ibo_length, GL_UNSIGNED_SHORT, 0);
}
