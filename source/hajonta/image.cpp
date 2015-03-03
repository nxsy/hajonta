#pragma once

#define STB_IMAGE_IMPLEMENTATION

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312)
#endif
#include "hajonta/thirdparty/stb_image.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

bool
load_image(uint8_t *source, uint32_t source_size, uint8_t *dest, uint32_t dest_size, int32_t *x, int32_t *y, int32_t *actual_size, bool exact_size = false)
{
    //int32_t x;
    //int32_t y;
    int32_t comp;
    unsigned char *result = stbi_load_from_memory((unsigned char *)source, (int32_t)source_size, x, y, &comp, STBI_rgb_alpha);

    if (result == 0)
    {
        return false;
    }

    *actual_size = (*x) * (*y) * (int32_t)4;
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
    int x;
    return load_image(source, source_size, dest, dest_size, &x, &x, &x, true);
}
