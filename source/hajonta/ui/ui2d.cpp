struct ui2d_vertex_format
{
    float position[2];
    float tex_coord[2];
    uint32_t texid;
    uint32_t options;
    v3 channel_color;
};

struct ui2d_push_context
{
    ui2d_vertex_format *vertices;
    uint32_t num_vertices;
    uint32_t max_vertices;
    GLuint *elements;
    uint32_t num_elements;
    uint32_t max_elements;
    uint32_t *textures;
    uint32_t num_textures;
    uint32_t max_textures;

    uint32_t seen_textures[1024];
};

uint32_t
push_texture(ui2d_push_context *pushctx, uint32_t tex)
{
    hassert(tex < sizeof(pushctx->seen_textures));

    if (pushctx->seen_textures[tex] != 0)
    {
        return pushctx->seen_textures[tex];
    }

    hassert(pushctx->num_textures + 4 <= pushctx->max_textures);
    pushctx->textures[pushctx->num_textures++] = tex;

    // Yes, this is post-increment.  It is decremented in the shader.
    pushctx->seen_textures[tex] = pushctx->num_textures;
    return pushctx->num_textures;
}

void
push_quad(ui2d_push_context *pushctx, stbtt_aligned_quad q, uint32_t tex, uint32_t options)
{
        ui2d_vertex_format *vertices = pushctx->vertices;
        uint32_t *num_vertices = &pushctx->num_vertices;
        uint32_t *elements = pushctx->elements;
        uint32_t *num_elements = &pushctx->num_elements;

        uint32_t tex_handle = push_texture(pushctx, tex);

        hassert(pushctx->num_vertices + 4 <= pushctx->max_vertices);

        uint32_t bl_vertex = (*num_vertices)++;
        vertices[bl_vertex] =
        {
            { q.x0, q.y0 },
            { q.s0, q.t0 },
            tex_handle,
            options,
            { 1, 1, 1 },
        };
        uint32_t br_vertex = (*num_vertices)++;
        vertices[br_vertex] =
        {
            { q.x1, q.y0 },
            { q.s1, q.t0 },
            tex_handle,
            options,
            { 1, 1, 0 },
        };
        uint32_t tr_vertex = (*num_vertices)++;
        vertices[tr_vertex] =
        {
            { q.x1, q.y1 },
            { q.s1, q.t1 },
            tex_handle,
            options,
            { 1, 1, 0 },
        };
        uint32_t tl_vertex = (*num_vertices)++;
        vertices[tl_vertex] =
        {
            { q.x0, q.y1 },
            { q.s0, q.t1 },
            tex_handle,
            options,
            { 1, 1, 0 },
        };
        hassert(pushctx->num_elements + 6 <= pushctx->max_elements);
        elements[(*num_elements)++] = bl_vertex;
        elements[(*num_elements)++] = br_vertex;
        elements[(*num_elements)++] = tr_vertex;

        elements[(*num_elements)++] = tr_vertex;
        elements[(*num_elements)++] = tl_vertex;
        elements[(*num_elements)++] = bl_vertex;
}

void
ui2d_render_elements(ui2d_push_context *pushctx, ui2d_program_struct *program_ui2d, uint32_t vbo, uint32_t ibo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
            (GLsizei)(pushctx->num_vertices * sizeof(pushctx->vertices[0])),
            pushctx->vertices,
            GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            (GLsizei)(pushctx->num_elements * sizeof(pushctx->elements[0])),
            pushctx->elements,
            GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray((GLuint)program_ui2d->a_pos_id);
    glEnableVertexAttribArray((GLuint)program_ui2d->a_tex_coord_id);
    glEnableVertexAttribArray((GLuint)program_ui2d->a_texid_id);
    glEnableVertexAttribArray((GLuint)program_ui2d->a_options_id);
    glEnableVertexAttribArray((GLuint)program_ui2d->a_channel_color_id);
    glVertexAttribPointer((GLuint)program_ui2d->a_pos_id, 2, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), 0);
    glVertexAttribPointer((GLuint)program_ui2d->a_tex_coord_id, 2, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, tex_coord));
    glVertexAttribPointer((GLuint)program_ui2d->a_texid_id, 1, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, texid));
    glVertexAttribPointer((GLuint)program_ui2d->a_options_id, 1, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, options));
    glVertexAttribPointer((GLuint)program_ui2d->a_channel_color_id, 3, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, channel_color));

    GLint uniform_locations[10] = {};
    char msg[] = "tex[xx]";
    for (int idx = 0; idx < harray_count(uniform_locations); ++idx)
    {
        sprintf(msg, "tex[%d]", idx);
        uniform_locations[idx] = glGetUniformLocation(program_ui2d->program, msg);

    }
    for (uint32_t i = 0; i < pushctx->num_textures; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, pushctx->textures[i]);
        glUniform1i(
            uniform_locations[i],
            (GLint)i);
    }

    glDrawElements(GL_TRIANGLES, (GLsizei)pushctx->num_elements, GL_UNSIGNED_INT, 0);
    glErrorAssert();
}
