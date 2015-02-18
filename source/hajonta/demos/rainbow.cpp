#pragma once

DEMO(demo_rainbow)
{
    game_state *state = (game_state *)memory->memory;
    demo_rainbow_state *demo_state = &state->demos.rainbow;
    if (!demo_state->vbo)
    {
        demo_state->velocity = {0, 0};
        demo_state->position = {0, 0};

        glErrorAssert();

        glUseProgram(state->program_a.program);
        glGenBuffers(1, &demo_state->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);
        vertex vertices[4] = {
            {{-0.5, 0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
            {{ 0.5, 0.5, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
            {{ 0.5,-0.5, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
            {{-0.5,-0.5, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0}},
        };
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray((GLuint)state->program_a.a_pos_id);
        glEnableVertexAttribArray((GLuint)state->program_a.a_color_id);
        glVertexAttribPointer((GLuint)state->program_a.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
        glVertexAttribPointer((GLuint)state->program_a.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));

        glErrorAssert();


        int volume = 3000;
        demo_state->audio_buffer_data = malloc(48000 * 2 * (16 / 8));
        for (int i = 0; i < 48000 * 2;)
        {
            volume = i < 48000 ? i / 16 : abs(96000 - i) / 16;
            ((int16_t *)demo_state->audio_buffer_data)[i] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565f / 48000.0f));
            ((int16_t *)demo_state->audio_buffer_data)[i+1] = (int16_t)(volume * sinf(i * 2 * pi * 261.625565f / 48000.0f));
            i += 2;
        }
    }

    float delta_t = input->delta_t;
    glClearColor(0.1f, 0.1f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

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
        if (controller->buttons.back.ended_down)
        {
            state->demos.active_demo = 0;
        }
    }

    acceleration = v2normalize(acceleration);
    acceleration = v2sub(acceleration, v2mul(demo_state->velocity, 5.0f));
    v2 movement = v2add(
            v2mul(acceleration, 0.5f * (delta_t * delta_t)),
            v2mul(demo_state->velocity, delta_t)
    );
    demo_state->position = v2add(demo_state->position, movement);
    demo_state->velocity = v2add(
            v2mul(acceleration, delta_t),
            demo_state->velocity
    );
    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    char msg[1024];
    sprintf(msg, "P: %+.2f, %+.2f", demo_state->position.x, demo_state->position.y);
    sprintf(msg + strlen(msg), " V: %+.2f, %+.2f", demo_state->velocity.x, demo_state->velocity.y);
    sprintf(msg + strlen(msg), " A: %+.2f, %+.2f", acceleration.x, acceleration.y);
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg);

    glUseProgram(state->program_a.program);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);
    glEnableVertexAttribArray((GLuint)state->program_a.a_pos_id);
    glEnableVertexAttribArray((GLuint)state->program_a.a_color_id);
    glVertexAttribPointer((GLuint)state->program_a.a_pos_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer((GLuint)state->program_a.a_color_id, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, color));
    glUniform2fv(state->program_a.u_offset_id, 1, (float *)&demo_state->position);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


    sound_output->samples = &(((uint8_t *)demo_state->audio_buffer_data)[demo_state->audio_offset * 2 * sound_output->channels * sound_output->number_of_samples]);
    demo_state->audio_offset = (demo_state->audio_offset + 1) % 60;
}

