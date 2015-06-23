#pragma once

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4505)
#endif
#define STB_RECT_PACK_IMPLEMENTATION
#include "hajonta/thirdparty/stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "hajonta/thirdparty/stb_truetype.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "hajonta/programs/ui2d.h"
#include "hajonta/ui/ui2d.cpp"

#include <time.h>

enum struct shader_mode
{
    standard,
    diffuse_texture,
    normal_texture,
    ao_texture,
    emit_texture,
    blinn_phong,
    specular_exponent_texture,
};

float lerp(float a, float b, float s)
{
    return a + (b - a) * s;
}

float
randfloat10()
{
    return (float)rand() / RAND_MAX;
}

void
create_particle(firework_behaviour *behaviours, firework_particle *fp, firework_type type, v3 initial_position, v3 initial_velocity)
{
    firework_behaviour *fb = behaviours + (int32_t)type;

    if (initial_position.y > 0)
    {
        fp->position = initial_position;
    }
    else
    {
        fp->position.x = lerp(-20, 20, randfloat10());
        fp->position.y = 0;
        fp->position.z = lerp(-20, 20, randfloat10());
    }

    fp->velocity = initial_velocity;
    fp->velocity.x += lerp(fb->min_velocity.x, fb->max_velocity.x, randfloat10());
    fp->velocity.y += lerp(fb->min_velocity.y, fb->max_velocity.x, randfloat10());
    fp->velocity.z += lerp(fb->min_velocity.z, fb->max_velocity.z, randfloat10());

    fp->constant_acceleration = {0.0f, -9.8f, 0.0f};
    fp->type = type;

    fp->ttl = lerp(fb->ttl_range.x, fb->ttl_range.y, randfloat10());

    float color_rand = randfloat10();
    if (color_rand < 0.33f)
    {
        fp->color = fb->colors[0];
    }
    else if (color_rand < 0.66f)
    {
        fp->color = fb->colors[1];
    }
    else
    {
        fp->color = fb->colors[2];
    }
}

void
create_particle(firework_behaviour *behaviours, firework_particle *fp, firework_type type)
{
    create_particle(behaviours, fp, type, v3{0,0,0}, v3{0,0,0});
}

DEMO(demo_firework)
{
    glErrorAssert();

    game_state *state = (game_state *)memory->memory;
    demo_firework_state *demo_state = &state->demos.firework;

    if (!demo_state->firework_vbo)
    {
        srand((uint32_t)(time(0)));
        glErrorAssert();
        glGenBuffers(1, &demo_state->firework_vbo);

        struct
        {
            float position[4];
        } vertices[] = {
            { 0.000f,  0.000f,  1.000f, 1.000f},
            { 0.894f,  0.000f,  0.447f, 1.000f},
            { 0.276f,  0.851f,  0.447f, 1.000f},
            {-0.724f,  0.526f,  0.447f, 1.000f},
            {-0.724f, -0.526f,  0.447f, 1.000f},
            { 0.276f, -0.851f,  0.447f, 1.000f},
            { 0.724f,  0.526f, -0.447f, 1.000f},
            {-0.276f,  0.851f, -0.447f, 1.000f},
            {-0.894f,  0.000f, -0.447f, 1.000f},
            {-0.276f, -0.851f, -0.447f, 1.000f},
            { 0.724f, -0.526f, -0.447f, 1.000f},
            { 0.000f,  0.000f, -1.000f, 1.000f},
        };
        glBindBuffer(GL_ARRAY_BUFFER, demo_state->firework_vbo);
        glBufferData(GL_ARRAY_BUFFER,
                sizeof(vertices),
                vertices,
                GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        struct
        {
            uint8_t vertex_ids[3];
        } faces[] =
        {
            { 0, 1, 2},
            { 0, 2, 3},
            { 0, 3, 4},
            { 0, 4, 5},
            { 0, 5, 1},
            {11, 6, 7},
            {11, 7, 8},
            {11, 8, 9},
            {11, 9,10},
            {11,10, 6},
            { 1, 2, 6},
            { 2, 3, 7},
            { 3, 4, 8},
            { 4, 5, 9},
            { 5, 1,10},
            { 6, 7, 2},
            { 7, 8, 3},
            { 8, 9, 4},
            { 9,10, 5},
            {10, 6, 1},
        };
        demo_state->firework_num_faces = harray_count(faces);
        glGenBuffers(1, &demo_state->firework_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->firework_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                sizeof(faces),
                faces,
                GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glGenTextures((GLsizei)firework_color::NUMBER_FIREWORK_COLORS, demo_state->firework_textures);
        struct
        {
            firework_color color;
            uint32_t color32;
        } color_data[] =
        {
            { firework_color::purple, 0xffff00ff },
            { firework_color::white, 0xffffffff },
            { firework_color::green, 0xff00ff00 },
            { firework_color::blue, 0xffff0000 },
        };
        uint32_t num_color_data = harray_count(color_data);
        hassert(num_color_data == (uint32_t)firework_color::NUMBER_FIREWORK_COLORS);
        for (uint32_t idx = 0; idx < (uint32_t)firework_color::NUMBER_FIREWORK_COLORS; ++idx)
        {
            glBindTexture(GL_TEXTURE_2D, demo_state->firework_textures[idx]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color_data[idx].color32);
        }
        glErrorAssert();

        {
            glGenBuffers(1, &demo_state->ground_vbo);
            struct
            {
                float position[4];
            } vertices[] = {
                {-200.000f, 0.000f,-100.000f, 1.000f},
                {-200.000f, 0.000f, 100.000f, 1.000f},
                { 200.000f, 0.000f, 100.000f, 1.000f},
                { 200.000f, 0.000f,-100.000f, 1.000f},
            };
            glBindBuffer(GL_ARRAY_BUFFER, demo_state->ground_vbo);
            glBufferData(GL_ARRAY_BUFFER,
                    sizeof(vertices),
                    vertices,
                    GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            struct
            {
                uint8_t vertex_ids[3];
            } faces[] =
            {
                { 0, 1, 2},
                { 0, 2, 3},
            };
            demo_state->ground_num_faces = harray_count(faces);
            glGenBuffers(1, &demo_state->ground_ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ground_ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    sizeof(faces),
                    faces,
                    GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            glGenTextures(1, &demo_state->ground_texture);
            glBindTexture(GL_TEXTURE_2D, demo_state->ground_texture);
            uint32_t color = 0xff202020;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);

            glErrorAssert();
        }

        uint8_t num_behaviours_configured = 0;
        demo_state->behaviours[(int)firework_type::basic_initial] = {
            {0.75f, 1.4f},
            {-5, 25,-5},
            { 5, 27, 5},
            {
                {3, 12, firework_type::basic_second_part},
            },
            {
                firework_color::white,
                firework_color::white,
                firework_color::white,
            },
        };
        num_behaviours_configured++;
        demo_state->behaviours[(int)firework_type::basic_second_part] = {
            {0.7f, 1.2f},
            {-10,-10,-10},
            { 10, 10, 10},
            {
            },
            {
                firework_color::purple,
                firework_color::green,
                firework_color::blue,
            },
        };
        num_behaviours_configured++;
        hassert(num_behaviours_configured == (uint8_t)firework_type::NUMBER_FIREWORK_TYPES);
    }

    if (demo_state->num_particles < harray_count(demo_state->particles) / 2)
    {
        if (randfloat10() < 0.05f) {
            create_particle(demo_state->behaviours, demo_state->particles + demo_state->num_particles++, firework_type::basic_initial);
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
    }

    glErrorAssert();

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glErrorAssert();

    demo_state->delta_t += input->delta_t;

    glUseProgram(state->program_c.program);
    glUniform1i(state->program_c.u_shader_mode_id, (int)shader_mode::diffuse_texture);

    glBindBuffer(GL_ARRAY_BUFFER, demo_state->firework_vbo);
    glEnableVertexAttribArray((GLuint)state->program_c.a_pos_id);
    glVertexAttribPointer((GLuint)state->program_c.a_pos_id, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->firework_ibo);

    glUniform1i(
        glGetUniformLocation(state->program_c.program, "tex"),
        0);
    glActiveTexture(GL_TEXTURE0);

    glErrorAssert();

    m4 view = m4identity();
    view.cols[3] = v4{0,-6,-30,1};
    float ratio = (float)input->window.width / (float)input->window.height;
    m4 projection = m4frustumprojection(1.0f, 100.0f, {-ratio, -1.0f}, {ratio, 1.0f});

    glUniformMatrix4fv(state->program_c.u_view_id, 1, false, (float *)&view);
    glUniformMatrix4fv(state->program_c.u_projection_id, 1, false, (float *)&projection);

    glErrorAssert();

    for (uint32_t idx = 0; idx < demo_state->num_particles; ++idx)
    {
        firework_particle *fp = demo_state->particles + idx;

        v3 acceleration = fp->constant_acceleration;
        acceleration = v3sub(acceleration, v3mul(fp->velocity, 0.05f));
        v3 movement = v3add(
            v3mul(acceleration, 0.5f * (input->delta_t * input->delta_t)),
            v3mul(fp->velocity, input->delta_t)
        );
        fp->position = v3add(fp->position, movement);
        fp->velocity = v3add(
            v3mul(acceleration, input->delta_t),
            fp->velocity
        );
        fp->ttl -= input->delta_t;

        if (fp->position.y < 0.0f || fp->ttl <= 0.0f)
        {
            if (fp->ttl <= 0.0f)
            {
                firework_behaviour *fb = demo_state->behaviours + (int32_t)fp->type;
                for (uint32_t idx = 0; idx < harray_count(fb->payload); ++idx)
                {
                    firework_payload *payload = fb->payload + idx;
                    if (payload->max_num_fireworks == 0)
                    {
                        break;
                    }
                    for (uint32_t payload_idx = 0; payload_idx < payload->max_num_fireworks; ++payload_idx)
                    {
                        create_particle(demo_state->behaviours,
                                demo_state->particles + demo_state->num_particles++,
                                payload->type,
                                fp->position,
                                fp->velocity);
                    }
                }
            }

            firework_particle *last_particle = demo_state->particles + demo_state->num_particles - 1;
            *fp = *last_particle;
            demo_state->num_particles--;
            idx--;
            continue;
        }

        m4 model = m4identity();
        m4 translate = m4identity();
        translate.cols[3] = v4{fp->position.x, fp->position.y, fp->position.z, 1.000f};
        model = m4mul(model, translate);

        glUniformMatrix4fv(state->program_c.u_model_id, 1, false, (float *)&model);
        glBindTexture(GL_TEXTURE_2D, demo_state->firework_textures[(uint32_t)fp->color]);
        glDrawElements(GL_TRIANGLES, (GLsizei)(demo_state->firework_num_faces * 3), GL_UNSIGNED_BYTE, 0);
    }

    glErrorAssert();

    glBindBuffer(GL_ARRAY_BUFFER, demo_state->ground_vbo);
    glEnableVertexAttribArray((GLuint)state->program_c.a_pos_id);
    glVertexAttribPointer((GLuint)state->program_c.a_pos_id, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, demo_state->ground_ibo);

    glUniform1i(
        glGetUniformLocation(state->program_c.program, "tex"),
        0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, demo_state->ground_texture);

    {
        m4 model = m4identity();
        glUniformMatrix4fv(state->program_c.u_model_id, 1, false, (float *)&model);
        glDrawElements(GL_TRIANGLES, (GLsizei)(demo_state->firework_num_faces * 3), GL_UNSIGNED_BYTE, 0);
    }

    glErrorAssert();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glErrorAssert();
}
