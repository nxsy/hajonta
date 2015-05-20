#include <stdint.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct vec4u8
{
    uint8_t x;
    uint8_t y;
    uint8_t z;
    uint8_t w;
};

struct vec3f
{
    float x;
    float y;
    float z;
};

vec3f
v3cross(vec3f p, vec3f q)
{
    vec3f result = {
        p.y * q.z - p.z * q.y,
        -(p.x * q.z - p.z * q.x),
        p.x * q.y - p.y * q.x
    };

    return result;
}

float
v3length(vec3f v)
{
    float result = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return result;
}

vec3f
v3normalize(vec3f v)
{
    float length = v3length(v);
    if (length == 0)
    {
        return v;
    }

    vec3f result = {v.x / length, v.y / length, v.z / length};
    return result;
}

vec3f
v3add(vec3f left, vec3f right)
{
    vec3f result = {left.x + right.x, left.y + right.y, left.z + right.z};
    return result;
}

vec3f
v3div(vec3f v, float divisor)
{
    vec3f result = {v.x / divisor, v.y / divisor, v.z / divisor};
    return result;
}

int
main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "%s takes two arguments - a source file and a target one\n", argv[0]);
        return 1;
    }

    int width, height, n;

    char *source = argv[1];

    vec4u8 *data = (vec4u8 *)stbi_load(source, &width, &height, &n, 4);

    if (!data)
    {
        fprintf(stderr, "Failed to load file: %s\n", source);
        return 1;
    }

    vec4u8 *normal_data = (vec4u8 *)malloc(width * height * 4);

    vec4u8 *normal_pixel = normal_data;
    vec4u8 *data_pixel = data;
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
#define X(xoff) ((x + xoff) % width)
#define Y(yoff) ((y + yoff) % height)
#define HEIGHT(xoff,yoff) (data + Y(yoff) * width + X(xoff))
            float dY = 0.0f;
            dY += HEIGHT(-1,-1)->x / 255.0f * -1.0f;
            dY += HEIGHT( 0,-1)->x / 255.0f * -2.0f;
            dY += HEIGHT( 1,-1)->x / 255.0f * -1.0f;
            dY += HEIGHT(-1, 1)->x / 255.0f *  1.0f;
            dY += HEIGHT( 0, 1)->x / 255.0f *  2.0f;
            dY += HEIGHT( 1, 1)->x / 255.0f *  1.0f;

            float dX = 0.0f;
            dX += HEIGHT(-1, 1)->x / 255.0f * -1.0f;
            dX += HEIGHT(-1, 0)->x / 255.0f * -2.0f;
            dX += HEIGHT(-1,-1)->x / 255.0f * -1.0f;
            dX += HEIGHT( 1, 1)->x / 255.0f *  1.0f;
            dX += HEIGHT( 1, 0)->x / 255.0f *  2.0f;
            dX += HEIGHT( 1,-1)->x / 255.0f *  1.0f;

            float length = sqrtf(dX*dX + dY*dY + 1);
            dX /= -length;
            dY /= -length;
            float dZ = 1.0f / length;

            *normal_pixel = {
                (uint8_t)((1.0f + dX) / 2.0f * 255.0f),
                (uint8_t)((1.0f + dY) / 2.0f * 255.0f),
                (uint8_t)((1.0f + dZ) / 2.0f * 255.0f),
                255,
            };

            normal_pixel++;
            data_pixel++;
        }
    }

    char *dest = argv[2];
    bool success = stbi_write_png(dest, width, height, 4, normal_data, width * 4);
    stbi_image_free(data);

    if (!success)
    {
        fprintf(stderr, "Failed to write file: %s\n", dest);
        return 1;
    }
}
