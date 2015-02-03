#pragma once

#pragma pack(push, 1)
struct bmp_file_header {
    char bfType[2];
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct bmp_info_header {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
    uint32_t bV4RedMask;
    uint32_t bV4GreenMask;
    uint32_t bV4BlueMask;
    uint32_t bV4AlphaMask;
};
#pragma pack(pop)

struct bmp_data {
    uint8_t *memory;

    uint8_t bits_per_pixel;
    uint32_t width;
    uint32_t height;
    bool top_to_bottom;

    uint8_t *bmp_memory;
};

inline int32_t
leading_zero_count(uint32_t mask)
{
    for(int32_t bit = 0;
        bit < 32;
        ++bit)
    {
        if(mask & (1 << bit))
        {
            return bit;
        }
    }
    return -1;
}

inline uint32_t
rotate_left(uint32_t value, int32_t rotation)
{
    rotation &= 31;
    return ((value << rotation) | (value >> (32 - rotation)));
}

bool bmp_open(bmp_data *data)
{
    bmp_file_header *h = (bmp_file_header *)data->memory;
    if (h->bfType[0] != 'B' || h->bfType[1] != 'M')
    {
        hassert(!"Incorrect file format");
        printf("Incorrect file format: %.2s, not BM", h->bfType);
        return false;
    }

    data->bmp_memory = data->memory + h->bfOffBits;

    bmp_info_header *info = (bmp_info_header *)(data->memory + sizeof(bmp_file_header));
    if (!((info->biCompression == 0) || (info->biCompression == 3)))
    {
        hassert(!"Incorrect compression");
        printf("Unsupported Compression: %i, not 0", info->biCompression);
        return false;
    }

    data->width = (uint32_t)info->biWidth;
    data->height = (uint32_t)abs(info->biHeight);
    data->top_to_bottom = info->biHeight < 0;
    data->bits_per_pixel = (uint8_t)info->biBitCount;

    if (info->biCompression == 3)
    {
        int32_t red_shift = 16 - leading_zero_count(info->bV4RedMask);
        int32_t green_shift = 8 - leading_zero_count(info->bV4GreenMask);
        int32_t blue_shift = 0 - leading_zero_count(info->bV4BlueMask);
        int32_t alpha_shift = 24 - leading_zero_count(info->bV4AlphaMask);

        uint32_t *bmp_pixel_data = (uint32_t *)data->bmp_memory;
        for (uint32_t i = 0;
                i < data->width * data->height;
                ++i)
        {
            bmp_pixel_data[i] = (rotate_left(bmp_pixel_data[i] & info->bV4RedMask, red_shift) |
                                 rotate_left(bmp_pixel_data[i] & info->bV4GreenMask, green_shift) |
                                 rotate_left(bmp_pixel_data[i] & info->bV4BlueMask, blue_shift) |
                                 rotate_left(bmp_pixel_data[i] & info->bV4AlphaMask, alpha_shift));
        }
    }

    return true;
}

struct draw_buffer {
    uint8_t *memory;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
};

static void
draw_bmp(draw_buffer *buffer, bmp_data *data, int buffer_x, int buffer_y, int bmp_x, int bmp_y, int bmp_width, int bmp_height)
{
    int buffer_max_y = buffer_y + bmp_height;

    if (buffer_max_y > (int32_t)buffer->height)
    {
        buffer_max_y = (int32_t)buffer->height;
    }

    int32_t bmp_pitch = (int32_t)(data->width * (data->bits_per_pixel / 8));

    uint8_t *bmp_first_row = data->bmp_memory;
    uint8_t *bmp_final_row = bmp_first_row + (bmp_pitch * (data->height - 1));

    uint8_t *bmp_row;
    int8_t bmp_direction;

    if (data->top_to_bottom)
    {
        bmp_direction = 1;
        bmp_row = bmp_first_row;
    }
    else
    {
        bmp_direction = -1;
        bmp_row = bmp_final_row;
    }

    bmp_row += (bmp_pitch * bmp_direction * bmp_y);

    uint8_t *buffer_row = buffer->memory + ((buffer->height - 1 - buffer_y) * buffer->pitch);
    while (buffer_y < buffer_max_y)
    {
        uint8_t *next_buffer_row = buffer_row - buffer->pitch;
        uint8_t *last_buffer_pixel = buffer_row + buffer->pitch;

        uint8_t *bmp_next_row = bmp_row + (bmp_pitch * bmp_direction);
        uint32_t *buffer_pixel = (uint32_t *)buffer_row;
        uint32_t *bmp_row_pixel = (uint32_t *)bmp_row + bmp_x;
        uint32_t *bmp_final_row_pixel = bmp_row_pixel + bmp_width;

        if (buffer_x > 0)
        {
            buffer_pixel += buffer_x;
        }
        else if (buffer_x < 0)
        {
            bmp_row_pixel -= buffer_x;
        }

        if (buffer_y >= 0) {
            while(bmp_row_pixel < bmp_final_row_pixel && buffer_pixel < (uint32_t *)last_buffer_pixel)
            {
                uint8_t *subpixel = (uint8_t *)bmp_row_pixel;
                uint8_t *alpha = subpixel + 3;
                if (*alpha == 255) {
                    *buffer_pixel = *bmp_row_pixel;
                }
                buffer_pixel++;
                bmp_row_pixel++;
            }
        }

        buffer_row = next_buffer_row;
        bmp_row = bmp_next_row;;
        ++buffer_y;
    }
}
