
#pragma pack(push, 1)
struct zfi_symbol
{
    int32_t id;
    int16_t page;
    int16_t _padding;
    uint8_t width;
    uint8_t height;

    int32_t shift_x;
    int32_t shift_y;
    int32_t shift_p;
    v2 tex_coords[4];
};

struct zfi_header
{
  char magic[13];
  uint16_t pages;
  uint16_t characters;
  int32_t max_height;
  int32_t max_shift_y;
  uint8_t padding[4];

  zfi_symbol symbols[1];
};
#pragma pack(pop)

struct font_data
{
    bmp_data data;
    zfi_header *header;
    zfi_symbol *symbols[256];
};

bool
load_font(uint8_t *zfi, uint8_t *bmp, font_data *font, hajonta_thread_context *ctx, platform_memory *memory)
{
    zfi_header *h = font->header = (zfi_header *)zfi;
    font->data.memory = bmp;
    if (!bmp_open(&font->data))
    {
        hassert(!"Failed to understand bmp file");
        memory->platform_fail(ctx, "Failed to understand bmp file");
        return false;
    }

    zfi_symbol *symbol = h->symbols;
    for(uint32_t i = 0;
            i < h->characters;
            ++i)
    {
        if (symbol->id >= sizeof(font->symbols))
        {
            continue;
        }

        font->symbols[symbol->id] = symbol;
        symbol++;
    }
    return true;
}

bool
write_to_buffer(draw_buffer *buffer, font_data *font, char *message)
{
    int buffer_x = 0;
    int buffer_y = 0;

    for (uint32_t i = 0;
            message[i];
            ++i)
    {
        zfi_symbol *s = font->symbols[message[i]];

        v2 min = s->tex_coords[0];
        v2 max = s->tex_coords[2];

        int bmp_x = (int)(min.x * font->data.width);
        int bmp_y = (int)((1-min.y) * font->data.height);
        int bmp_width = (int)((max.x - min.x) * font->data.width);
        int bmp_height = (int)((min.y - max.y) * font->data.height);

        draw_bmp(buffer, &font->data, buffer_x, buffer_y - s->shift_y, bmp_x, bmp_y, bmp_width, bmp_height);
        buffer_x += s->width + 1;
    }

    return true;
}
