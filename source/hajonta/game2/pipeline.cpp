#pragma once

void
PipelineInit(RenderPipeline *pipeline, AssetDescriptors *asset_descriptors)
{
    for (uint32_t i = 0; i < pipeline->entry_count; ++i)
    {
        auto entry = pipeline->entries + i;
        RenderListBufferSize((*entry->list), entry->buffer, entry->buffer_size);
        if (entry->target_framebuffer_id >= 0)
        {
            entry->list->framebuffer = &pipeline->framebuffers[entry->target_framebuffer_id].framebuffer;
        }
    }

    for (uint32_t i = 0; i < pipeline->framebuffer_count; ++i)
    {
        auto framebuffer = pipeline->framebuffers + i;
        framebuffer->asset_descriptor = add_framebuffer_asset(asset_descriptors, &framebuffer->framebuffer);
        framebuffer->depth_asset_descriptor = add_framebuffer_depth_asset(asset_descriptors, &framebuffer->framebuffer);
        framebuffer->framebuffer._flags.use_multisample_buffer = framebuffer->multisample;
        framebuffer->framebuffer._flags.use_depth_texture = framebuffer->use_depth_texture;
        framebuffer->framebuffer._flags.use_rg32f_buffer = framebuffer->use_rg32f_buffer;
        framebuffer->framebuffer._flags.no_clear_each_frame = framebuffer->no_clear_each_frame;
        framebuffer->framebuffer.clear_color = framebuffer->clear_color;
    }
}

void
PipelineReset(game_state *state, RenderPipeline *pipeline, PipelineResetData *data)
{
    v2i window_size = data->window_size;
    for (uint32_t i = 0; i < pipeline->framebuffer_count; ++i)
    {
        auto framebuffer = pipeline->framebuffers + i;
        FramebufferReset(&framebuffer->framebuffer);
        if (framebuffer->fixed_size)
        {
            framebuffer->framebuffer.size = framebuffer->size;
        }
        else
        {
            framebuffer->framebuffer.size = window_size;
        }
        framebuffer->framebuffer._flags.cleared_this_frame = false;
    }

    for (uint32_t i = 0; i < pipeline->entry_count; ++i)
    {
        auto entry = pipeline->entries + i;
        RenderListReset(entry->list);
        RegisterRenderList(state->frame_state.memory, entry->list);
        PushMatrices(entry->list, data->matrix_count, data->matrices);
        PushAssetDescriptors(entry->list, data->asset_count, data->assets);
        PushDescriptors(entry->list, data->l, data->armatures);

        if ((entry->target_framebuffer_id >= 0) && (entry->source_framebuffer_id >= 0))
        {
            auto source_framebuffer = pipeline->framebuffers[entry->source_framebuffer_id];
            if (entry->filter_type != ApplyFilterType::none) {
                PushApplyFilter(entry->list, entry->filter_type, source_framebuffer.asset_descriptor);
            }
            else
            {
                PushFramebufferBlit(entry->list, source_framebuffer.asset_descriptor);
            }
        }
    }

    for (uint32_t i = 0; i < pipeline->dependency_count; ++i)
    {
        auto dependency = pipeline->dependencies + i;
        auto entry_depends = pipeline->entries + dependency->entry_depends;
        auto entry_depended = pipeline->entries + dependency->entry_depended;
        RenderListDependsOn(entry_depends->list, entry_depended->list);
    }
}

RenderPipelineFramebufferId
RenderPipelineAddFramebuffer(RenderPipeline *pipeline)
{
    return pipeline->framebuffer_count++;
}

RenderPipelineRendererId
RenderPipelineAddRenderer(RenderPipeline *pipeline)
{
    return pipeline->entry_count++;
}

void
PipelineAddDependency(RenderPipeline *pipeline, uint32_t entry_depends, uint32_t entry_depended)
{
    hassert(pipeline->dependency_count < harray_count(pipeline->dependencies));
    auto dependency = pipeline->dependencies + pipeline->dependency_count++;
    dependency->entry_depends = entry_depends;
    dependency->entry_depended = entry_depended;
}

void
CreatePipeline(game_state *state)
{
    auto &pipeline_elements = state->pipeline_elements;
    auto pipeline = &state->render_pipeline;
    *pipeline = {};
    pipeline_elements.fb_main = RenderPipelineAddFramebuffer(pipeline);
    auto &fb_main = pipeline->framebuffers[pipeline_elements.fb_main];
    fb_main.use_depth_texture = 1;
    fb_main.no_clear_each_frame = 1;
    pipeline_elements.fb_multisample = RenderPipelineAddFramebuffer(pipeline);
    auto &fb_multisample = pipeline->framebuffers[pipeline_elements.fb_multisample];
    fb_multisample.multisample = 1;
    fb_multisample.use_depth_texture = 1;

    pipeline_elements.fb_shadowmap = RenderPipelineAddFramebuffer(pipeline);
    auto &fb_shadowmap = pipeline->framebuffers[pipeline_elements.fb_shadowmap];
    fb_shadowmap.use_depth_texture = 1;
    fb_shadowmap.use_rg32f_buffer = 1;
    fb_shadowmap.size = {
        (int32_t)state->shadowmap_size,
        (int32_t)state->shadowmap_size
    };
    fb_shadowmap.fixed_size = 1;
    fb_shadowmap.clear_color = {1.0f, 1.0f, 0.0f, 1.0f};

    pipeline_elements.fb_sm_blur_x = RenderPipelineAddFramebuffer(pipeline);
    auto &fb_sm_blur_x = pipeline->framebuffers[pipeline_elements.fb_sm_blur_x];
    fb_sm_blur_x.use_rg32f_buffer = 1;
    fb_sm_blur_x.size = fb_shadowmap.size;
    fb_sm_blur_x.fixed_size = 1;
    fb_sm_blur_x.no_clear_each_frame = 1;

    pipeline_elements.fb_sm_blur_xy = RenderPipelineAddFramebuffer(pipeline);
    auto &fb_sm_blur_xy = pipeline->framebuffers[pipeline_elements.fb_sm_blur_xy];
    fb_sm_blur_xy.use_rg32f_buffer = 1;
    fb_sm_blur_xy.size = fb_shadowmap.size;
    fb_sm_blur_xy.fixed_size = 1;
    fb_sm_blur_xy.no_clear_each_frame = 1;

    pipeline_elements.fb_nature_pack_debug = RenderPipelineAddFramebuffer(pipeline);
    auto &fb_nature_pack_debug = pipeline->framebuffers[pipeline_elements.fb_nature_pack_debug];
    fb_nature_pack_debug.size = { 64, 64 };
    fb_nature_pack_debug.fixed_size = 1;

    pipeline_elements.r_framebuffer = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *framebuffer = pipeline->entries + pipeline_elements.r_framebuffer;
    state->framebuffer_renderer.list.name = DEBUG_NAME("framebuffer");
    *framebuffer = {
        &state->framebuffer_renderer.list,
        state->framebuffer_renderer.buffer,
        sizeof(state->framebuffer_renderer.buffer),
        -1,
        -1,
    };

    pipeline_elements.r_sky = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *sky = pipeline->entries + pipeline_elements.r_sky;
    state->pipeline_elements.rl_sky.list.name = DEBUG_NAME("sky");
    *sky = {
        &state->pipeline_elements.rl_sky.list,
        state->pipeline_elements.rl_sky.buffer,
        sizeof(state->pipeline_elements.rl_sky.buffer),
        -1,
        -1,
    };

    pipeline_elements.r_multisample = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *multisample = pipeline->entries + pipeline_elements.r_multisample;
    state->multisample_renderer.list.name = DEBUG_NAME("multisample");
    *multisample = {
        &state->multisample_renderer.list,
        state->multisample_renderer.buffer,
        sizeof(state->multisample_renderer.buffer),
        pipeline_elements.fb_main,
        pipeline_elements.fb_multisample,
    };
    pipeline_elements.r_three_dee = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *three_dee = pipeline->entries + pipeline_elements.r_three_dee;
    state->three_dee_renderer.list.name = DEBUG_NAME("three_dee");
    *three_dee = {
        &state->three_dee_renderer.list,
        state->three_dee_renderer.buffer,
        sizeof(state->three_dee_renderer.buffer),
        pipeline_elements.fb_multisample,
        -1,
    };

    pipeline_elements.r_shadowmap = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *shadowmap = pipeline->entries + pipeline_elements.r_shadowmap;
    state->shadowmap_renderer.list.name = DEBUG_NAME("shadowmap");
    *shadowmap = {
        &state->shadowmap_renderer.list,
        state->shadowmap_renderer.buffer,
        sizeof(state->shadowmap_renderer.buffer),
        pipeline_elements.fb_shadowmap,
        -1,
    };

    pipeline_elements.r_sm_blur_x = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *sm_blur_x = pipeline->entries + pipeline_elements.r_sm_blur_x;
    state->sm_blur_x_renderer.list.name = DEBUG_NAME("sm_blur_x");
    *sm_blur_x = {
        &state->sm_blur_x_renderer.list,
        state->sm_blur_x_renderer.buffer,
        sizeof(state->sm_blur_x_renderer.buffer),
        pipeline_elements.fb_sm_blur_x,
        pipeline_elements.fb_shadowmap,
        ApplyFilterType::gaussian_7x1_x,
    };

    pipeline_elements.r_sm_blur_xy = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *sm_blur_xy = pipeline->entries + pipeline_elements.r_sm_blur_xy;
    state->sm_blur_xy_renderer.list.name = DEBUG_NAME("sm_blur_xy");
    *sm_blur_xy = {
        &state->sm_blur_xy_renderer.list,
        state->sm_blur_xy_renderer.buffer,
        sizeof(state->sm_blur_xy_renderer.buffer),
        pipeline_elements.fb_sm_blur_xy,
        pipeline_elements.fb_sm_blur_x,
        ApplyFilterType::gaussian_7x1_y,
    };

    pipeline_elements.r_two_dee = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *two_dee = pipeline->entries + pipeline_elements.r_two_dee;
    state->two_dee_renderer.list.name = DEBUG_NAME("two_dee");
    *two_dee = {
        &state->two_dee_renderer.list,
        state->two_dee_renderer.buffer,
        sizeof(state->two_dee_renderer.buffer),
        -1,
        -1,
    };

    pipeline_elements.r_two_dee_debug = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *two_dee_debug = pipeline->entries + pipeline_elements.r_two_dee_debug;
    state->two_dee_debug_renderer.list.name = DEBUG_NAME("two_dee_debug");
    *two_dee_debug = {
        &state->two_dee_debug_renderer.list,
        state->two_dee_debug_renderer.buffer,
        sizeof(state->two_dee_debug_renderer.buffer),
        -1,
        -1,
    };

    pipeline_elements.r_nature_pack_debug = RenderPipelineAddRenderer(pipeline);
    RenderPipelineEntry *nature_pack_debug = pipeline->entries + pipeline_elements.r_nature_pack_debug;
    pipeline_elements.rl_nature_pack_debug.list.name = DEBUG_NAME("nature_pack_debug");
    *nature_pack_debug = {
        &pipeline_elements.rl_nature_pack_debug.list,
        pipeline_elements.rl_nature_pack_debug.buffer,
        sizeof(pipeline_elements.rl_nature_pack_debug.buffer),
        pipeline_elements.fb_nature_pack_debug,
        -1,
    };

    PipelineAddDependency(pipeline, pipeline_elements.r_three_dee, pipeline_elements.r_sm_blur_xy);
    PipelineAddDependency(pipeline, pipeline_elements.r_sm_blur_xy, pipeline_elements.r_sm_blur_x);
    PipelineAddDependency(pipeline, pipeline_elements.r_sm_blur_x, pipeline_elements.r_shadowmap);
    PipelineAddDependency(pipeline, pipeline_elements.r_multisample, pipeline_elements.r_three_dee);
    PipelineAddDependency(pipeline, pipeline_elements.r_framebuffer, pipeline_elements.r_sky);
    PipelineAddDependency(pipeline, pipeline_elements.r_sky, pipeline_elements.r_multisample);
}

