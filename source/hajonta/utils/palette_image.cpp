#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4456 4457)
#endif
#include "stb_image_write.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

struct vec4u8
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

int
main(int argc, const char **argv)
{
    if (argc < 5)
    {
        printf("usage: %s outfile color width height ...\n", argv[0]);
        return 1;
    }
    const char *outfile = argv[1];
    int32_t width = strtol(argv[2], 0, 10);
    int32_t height = strtol(argv[3], 0, 10);
    printf("Size: %ldx%ld\n", width, height);

    uint32_t num_colors = argc - 4;
    const char **colors = argv + 4;

    printf("Number of colors: %d:\n", num_colors);
    uint32_t squares_per_line = 1;
    while ((squares_per_line * squares_per_line) < num_colors)
    {
        ++squares_per_line;
    }
    uint32_t square_width = width / squares_per_line;
    uint32_t square_height = height / squares_per_line;
    printf("Number of squares: %d\n", squares_per_line * squares_per_line);
    for (uint32_t i = 0; i < num_colors; ++i)
    {
        printf("  %s\n", colors[i]);
    }

    vec4u8 *buffer = (vec4u8 *)malloc(4 * width * height);

    uint32_t color_id = 0;
    for (uint32_t row = 0; row < squares_per_line; ++row)
    {
        uint32_t offset_y = row * square_height;
        for (uint32_t col = 0; col < squares_per_line; ++col)
        {
            if (color_id >= num_colors)
            {
                break;
            }
            uint32_t offset_x = col * square_width;
            uint32_t color = strtol(colors[color_id], 0, 16);
            for (uint32_t y = 0; y < square_height; ++y)
            {
                for (uint32_t x = 0; x < square_width; ++x)
                {
                    vec4u8 *p = buffer + ((offset_y + y) * width + x + offset_x);
                    *p = {
                        (uint8_t)(color >> 16 & 0xff),
                        (uint8_t)(color >> 8 & 0xff),
                        (uint8_t)(color & 0xff),
                        255
                    };
                }
            }
            ++color_id;
        }
        if (color_id >= num_colors)
        {
            break;
        }
    }
    stbi_write_png(outfile, width, height, 4, buffer, width * 4);
    return 0;
}
