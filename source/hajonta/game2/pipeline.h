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
    struct {
        uint32_t fixed_size:1;
        uint32_t multisample:1;
        uint32_t use_depth_texture:1;
        uint32_t use_rg32f_buffer:1;
    };
};

struct
RenderPipelineFramebufferStage
{
    uint8_t framebuffer;
    uint8_t stage;
};

struct
RenderPipeline
{
    uint8_t entry_count;
    RenderPipelineEntry entries[10];
    uint8_t framebuffer_count;
    RenderPipelineFramebuffer framebuffers[10];
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

    RenderPipelineRendererId r_framebuffer;
    RenderPipelineRendererId r_multisample;
    RenderPipelineRendererId r_three_dee;
    RenderPipelineRendererId r_shadowmap;
    RenderPipelineRendererId r_sm_blur_x;
    RenderPipelineRendererId r_sm_blur_xy;
    RenderPipelineRendererId r_two_dee;
    RenderPipelineRendererId r_two_dee_debug;
    RenderPipelineRendererId r_nature_pack_debug;

    _render_list<1024 * 100> rl_nature_pack_debug;
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

