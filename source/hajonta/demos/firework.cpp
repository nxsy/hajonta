#pragma once

DEMO(demo_firework)
{
    game_state *state = (game_state *)memory->memory;
    demo_firework_state *demo_state = &state->demos.firework;

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!demo_state->vbo)
    {
        glErrorAssert();
        glGenBuffers(1, &demo_state->vbo);
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

    demo_state->delta_t += input->delta_t;

    glUseProgram(state->program_b.program);
    glBindBuffer(GL_ARRAY_BUFFER, demo_state->vbo);
}
