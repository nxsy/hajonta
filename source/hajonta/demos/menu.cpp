#pragma once

DEMO(demo_menu)
{
    game_state *state = (game_state *)memory->memory;
    demo_menu_state *demo = &state->demos.menu;

    if (!demo->vbo) {
        glGenBuffers(1, &demo->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, demo->vbo);
        float height = 14.0f / (540.0f / 2.0f);
        float width = 300.0f / (960.0f / 2.0f);
        vertex font_vertices[4] = {
            {{-width/2.0f, 0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
            {{ width/2.0f, 0, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}},
            {{ width/2.0f,-height, 0.0, 1.0}, {1.0, 0.0, 1.0, 1.0}},
            {{-width/2.0f,-height, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        };
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), font_vertices, GL_STATIC_DRAW);

        demo->menu_draw_buffer.memory = demo->menu_buffer;
        demo->menu_draw_buffer.width = menu_buffer_width;
        demo->menu_draw_buffer.height = menu_buffer_height;
        demo->menu_draw_buffer.pitch = 4 * demo->menu_draw_buffer.width;

        glGenTextures(1, &demo->texture_id);
        glBindTexture(GL_TEXTURE_2D, demo->texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            (GLsizei)demo->menu_draw_buffer.width, (GLsizei)demo->menu_draw_buffer.height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, demo->menu_buffer);
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
        if (controller->buttons.back.ended_down && !controller->buttons.back.repeat)
        {
            memory->quit = true;
        }
        if (controller->buttons.move_down.ended_down && !controller->buttons.move_down.repeat)
        {
            demo->selected_index++;
            demo->selected_index %= state->demos.number_of_demos;
        }
        if (controller->buttons.move_left.ended_down && !controller->buttons.move_left.repeat)
        {
            switch(memory->cursor_settings.mode)
            {
                case platform_cursor_mode::normal:
                {
                    memory->cursor_settings.mode = platform_cursor_mode::unlimited;
                } break;
                case platform_cursor_mode::unlimited:
                {
                    memory->cursor_settings.mode = platform_cursor_mode::normal;
                } break;
                case platform_cursor_mode::COUNT:
                default:
                {
                    hassert(!"Unknown current cursor setting");

                } break;
            }
        }
        if (controller->buttons.move_up.ended_down && !controller->buttons.move_up.repeat)
        {
            if (demo->selected_index == 0)
            {
                demo->selected_index = state->demos.number_of_demos - 1;
            }
            else
            {
                demo->selected_index--;
            }
        }
        if (controller->buttons.move_right.ended_down)
        {
            state->demos.active_demo = demo->selected_index;
        }
        if (controller->buttons.start.ended_down)
        {
            state->demos.active_demo = demo->selected_index;
        }
    }

    v2 mouse_loc = {
        (float)input->mouse.x / ((float)input->window.width / 2.0f) - 1.0f,
        ((float)input->mouse.y / ((float)input->window.height / 2.0f) - 1.0f) * -1.0f,
    };
    int32_t mouse_demo_chosen = -1;

    glClearColor(0.1f, 0.1f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, demo->vbo);
    float height = 14.0f / (540.0f / 2.0f);
    float width = 300.0f / (960.0f / 2.0f);
    for (uint32_t menu_index = 0;
            menu_index < state->demos.number_of_demos;
            ++menu_index)
    {
        float offset = -height * 2 * menu_index;
        vertex font_vertices[4] = {
            {{-width/2.0f, offset, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},
            {{ width/2.0f, offset, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}},
            {{ width/2.0f,offset-height, 0.0, 1.0}, {1.0, 0.0, 1.0, 1.0}},
            {{-width/2.0f,offset-height, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        };
        rectangle2 font_area = {
            {-width/2.0f, offset-height},
            {width, height},
        };
        if (input->mouse.is_active && point_in_rectangle(mouse_loc, font_area))
        {
            demo->selected_index = menu_index;
            mouse_demo_chosen = (int32_t)demo->selected_index;
        }
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), font_vertices, GL_STATIC_DRAW);
        memset(demo->menu_buffer, 0, sizeof(demo->menu_buffer));
        write_to_buffer(&demo->menu_draw_buffer, &state->debug_font.font, state->demos.registry[menu_index].name);
        if (menu_index == demo->selected_index)
        {
            for (uint32_t *b = (uint32_t *)demo->menu_buffer;
                    b < (uint32_t*)(demo->menu_buffer + sizeof(demo->menu_buffer));
                    ++b)
            {
                uint8_t *subpixel = (uint8_t *)b;
                uint8_t *alpha = subpixel + 3;
                if (*alpha)
                {
                    *b = 0xffff00ff;
                }
            }
        }
        font_output(state, demo->menu_draw_buffer, demo->vbo, demo->texture_id);
    }
    memset(state->fps_buffer, 0, sizeof(state->fps_buffer));
    char msg[1024];
    sprintf(msg, "MENU %03dx%03d ", input->mouse.x, input->mouse.y);
    sprintf(msg + strlen(msg), "OGL %0.2fx%0.2f ", mouse_loc.x, mouse_loc.y);
    if (input->mouse.buttons.left.ended_down)
    {
        sprintf(msg + strlen(msg), "LMB ");
        if (mouse_demo_chosen >= 0)
        {
            state->demos.active_demo = (uint32_t)mouse_demo_chosen;
        }
    }
    write_to_buffer(&state->fps_draw_buffer, &state->debug_font.font, msg);

    sound_output->samples = 0;
}

