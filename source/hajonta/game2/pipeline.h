#pragma once

struct
RenderPipelineEntry
{
    render_entry_list *list;
    uint8_t *buffer;
    uint32_t buffer_size;

    int32_t target_framebuffer_id;
    int32_t source_framebuffer_id;

    ApplyFilterType filter_type;

    v4 clear_color;
};

struct
RenderPipelineFramebuffer
{
    FramebufferDescriptor framebuffer;
    int32_t asset_descriptor;
    int32_t depth_asset_descriptor;
    v2i size;
    v4 clear_color;
    struct {
        uint32_t fixed_size:1;
        uint32_t half_size:1;
        uint32_t multisample:1;
        uint32_t use_depth_texture:1;
        uint32_t use_rg32f_buffer:1;
        uint32_t use_texarray:1;
        uint32_t no_clear_each_frame:1;
        uint32_t use_stencil_buffer:1;
    };
};

struct
RenderPipelineFramebufferStage
{
    uint8_t framebuffer;
    uint8_t stage;
};

struct
RenderPipelineDependency
{
    uint32_t entry_depends;
    uint32_t entry_depended;
};

struct
RenderPipeline
{
    uint8_t entry_count;
    RenderPipelineEntry entries[20];
    uint8_t framebuffer_count;
    RenderPipelineFramebuffer framebuffers[10];

    uint8_t dependency_count;
    RenderPipelineDependency dependencies[10];
};

typedef uint8_t RenderPipelineFramebufferId;
typedef uint8_t RenderPipelineRendererId;

struct
GamePipelineElements
{
    RenderPipelineFramebufferId fb_main;
    RenderPipelineFramebufferId fb_multisample;
    RenderPipelineFramebufferId fb_shadowmap;
    RenderPipelineFramebufferId fb_sm_blur_x;
    RenderPipelineFramebufferId fb_sm_blur_xy;
    RenderPipelineFramebufferId fb_nature_pack_debug;
    RenderPipelineFramebufferId fb_shadowmap_texarray;
    RenderPipelineFramebufferId fb_reflection;
    RenderPipelineFramebufferId fb_refraction;

    RenderPipelineRendererId r_framebuffer;
    RenderPipelineRendererId r_multisample;
    RenderPipelineRendererId r_sky;
    RenderPipelineRendererId r_three_dee;
    RenderPipelineRendererId r_three_dee_water;
    RenderPipelineRendererId r_shadowmap;
    RenderPipelineRendererId r_shadowmap_texarray_blit;
    RenderPipelineRendererId r_sm_blur_x;
    RenderPipelineRendererId r_sm_blur_xy;
    RenderPipelineRendererId r_two_dee;
    RenderPipelineRendererId r_two_dee_debug;
    RenderPipelineRendererId r_nature_pack_debug;
    RenderPipelineRendererId r_reflection;
    RenderPipelineRendererId r_refraction;

    _render_list<1024 * 100> rl_nature_pack_debug;
    _render_list<2 * 1024 * 1024> rl_sky;
    _render_list<1024 * 100> rl_shadowmap_texarray_blit;
    _render_list<2 * 1024 * 1024> rl_reflection;
    _render_list<2 * 1024 * 1024> rl_refraction;

    _render_list<4*1024*1024> rl_two_dee;
    _render_list<4*1024*1024> rl_two_dee_debug;
    _render_list<4*1024*1024> rl_three_dee;
    _render_list<1024> rl_three_dee_water;
    _render_list<4*1024*1024> rl_shadowmap;
    _render_list<1024*1024> rl_framebuffer;
    _render_list<1024> rl_multisample;
    _render_list<1024> rl_sm_blur_x;
    _render_list<1024> rl_sm_blur_xy;

};

struct
PipelineResetData
{
    v2i window_size;
    uint32_t matrix_count;
    m4 *matrices;
    uint32_t asset_count;
    asset_descriptor *assets;
    LightDescriptors l;
    ArmatureDescriptors armatures;
};

