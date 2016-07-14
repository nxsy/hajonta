#pragma once

#define STB_IMAGE_IMPLEMENTATION

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4456 4457)
#endif
#include "hajonta/thirdparty/stb_image.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "hajonta/image/dds.h"

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

bool
dds_check(
    uint8_t *source, uint32_t source_size, int32_t *x, int32_t *y,
    uint32_t *format, uint8_t **contents, uint32_t *size)
{
    if (
        (source[0] != 'D') ||
        (source[1] != 'D') ||
        (source[2] != 'S') ||
        (source[3] != ' '))
    {
        return false;
    }

    DDSURFACEDESC *surface = (DDSURFACEDESC *)(source + 4);
    hassert(surface->size == sizeof(DDSURFACEDESC));

    uint32_t block_size;
    switch (surface->format.four_cc)
    {
        case DXT1:
        {
            block_size = 8;
            *format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        } break;
        case DXT3:
        {
            block_size = 16;
            *format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        } break;
        case DXT5:
        {
            block_size = 16;
            *format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        } break;
        default:
        {
            return false;
        }
    }
    *size = ((surface->width + 3) / 4) * ((surface->height + 3) / 4) * block_size;
    *x = (int32_t)surface->width;
    *y = (int32_t)surface->height;
    *contents = source + 4 + surface->size;

    return true;
}

bool
load_image(uint8_t *source, uint32_t source_size, uint8_t *dest, uint32_t dest_size, int32_t *x, int32_t *y, uint32_t *actual_size, bool exact_size = false)
{
    int32_t comp;
    unsigned char *result = stbi_load_from_memory((unsigned char *)source, (int32_t)source_size, x, y, &comp, STBI_rgb_alpha);

    if (result == 0)
    {
        return false;
    }

    *actual_size = (uint32_t)((*x) * (*y) * 4);
    if (exact_size)
    {
        if (*actual_size != (int32_t)dest_size)
        {
            return false;
        }
    }

    if (*actual_size > (int32_t)dest_size)
    {
        return false;
    }

    memcpy(dest, result, (size_t)(*actual_size));
    stbi_image_free(result);
    return true;
}

bool
load_image(uint8_t *source, uint32_t source_size, uint8_t *dest, uint32_t dest_size)
{
    int32_t x;
    uint32_t size;
    return load_image(source, source_size, dest, dest_size, &x, &x, &size, true);
}
