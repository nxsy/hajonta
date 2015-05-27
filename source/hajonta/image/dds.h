#pragma once

#define DDS_FOURCC      0x00000004
#define MAKE_FOURCC(ch0, ch1, ch2, ch3) \
    (uint32_t)( \
        (uint32_t)((ch3) << 24) | \
        (uint32_t)((ch2) << 16) | \
        (uint32_t)((ch1) << 8) | \
        (uint32_t)(ch0) \
    )
#define DXT1 MAKE_FOURCC('D', 'X', 'T', '1')
#define DXT3 MAKE_FOURCC('D', 'X', 'T', '3')
#define DXT5 MAKE_FOURCC('D', 'X', 'T', '5')

struct DDSCAPS2
{
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    union
    {
        uint32_t caps4;
        uint32_t volume_depth;
    };
};

struct DDCOLORKEY
{
    uint32_t low;
    uint32_t high;
};

struct DDPIXELFORMAT
{
    uint32_t size;
    uint32_t flags;
    uint32_t four_cc;
    union
    {
        uint32_t rgb_bit_count;
        uint32_t yuv_bit_count;
        uint32_t zbuffer_bit_depth;
        uint32_t alpha_bit_depth;
        uint32_t luminance_bit_count;
        uint32_t bump_bit_count;
        uint32_t private_format_bit_count;
    };
    union
    {
        uint32_t r_bit_mask;
        uint32_t y_bit_mask;
        uint32_t stencil_bit_depth;
        uint32_t luminance_bit_mask;
        uint32_t bump_du_bit_mask;
        uint32_t operations;
    };
    union
    {
        uint32_t g_bit_mask;
        uint32_t u_bit_mask;
        uint32_t z_bit_mask;
        uint32_t bump_dv_bit_mask;
        struct {
            uint16_t flip_ms_types;
            uint16_t blt_ms_types;
        } MultiSampleCaps;
    };
    union
    {
        uint32_t b_bit_mask;
        uint32_t v_bit_mask;
        uint32_t stencil_bit_mask;
        uint32_t bump_luminance_bit_mask;
    };
    union
    {
        uint32_t rgb_alpha_bit_mask;
        uint32_t yuv_alpha_bit_mask;
        uint32_t luminance_alpha_bit_mask;
        uint32_t rgb_z_bit_mask;
        uint32_t yuv_z_bit_mask;
    };
};

struct DDSURFACEDESC
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    union
    {
        int32_t pitch;
        uint32_t linear_size;
    };
    union
    {
        uint32_t back_buffer_count;
        uint32_t depth;
    };
    union
    {
        uint32_t mip_map_count;
        uint32_t refresh_rate;
    };
    uint32_t alpha_bit_depth;
    uint32_t _reserved0;
    uint32_t surface;

    DDCOLORKEY dest_overlay;
    DDCOLORKEY dest_blt;
    DDCOLORKEY src_overlay;
    DDCOLORKEY src_blt;
    DDPIXELFORMAT format;
    DDSCAPS2     caps;
    uint32_t    texture_stage;
};
