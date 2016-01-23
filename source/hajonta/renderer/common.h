#pragma once

#include "hajonta/platform/neutral.h"
#include "hajonta/math.h"

enum struct
render_entry_type
{
    clear,
    ui2d,
    quad,
    matrices,
    asset_descriptors,
};

struct
render_entry_header
{
     render_entry_type type;
};

struct
render_entry_type_clear
{
    render_entry_header header;
    v4 color;
};

struct ui2d_push_context;
struct
render_entry_type_ui2d
{
    render_entry_header header;
    ui2d_push_context *pushctx;
};

struct
render_entry_type_quad
{
    render_entry_header header;
    v3 position;
    v3 size;
    v4 color;
    int32_t matrix_id;
    int32_t asset_descriptor_id;
};

struct
render_entry_type_matrices
{
    render_entry_header header;
    uint32_t count;
    m4 *matrices;
};

struct
asset_descriptor
{
    const char *asset_name;
    int32_t asset_id;
    uint32_t generation_id;
};

struct
render_entry_type_asset_descriptors
{
    render_entry_header header;
    uint32_t count;
    asset_descriptor *descriptors;
};

struct
render_entry_list
{
    uint32_t max_size;
    uint32_t current_size;
    uint8_t *base;
    uint32_t element_count;
};

#define RenderListBuffer(r, b) { r.max_size = sizeof(b); r.base = b; }
#define RenderListReset(r) { r.current_size = r.element_count = 0; }

#define PushRenderElement(list, type) (render_entry_type_##type *)PushRenderElement_(list, sizeof(render_entry_type_##type), render_entry_type::type)
inline void *
PushRenderElement_(render_entry_list *list, uint32_t size, render_entry_type type)
{
    void *res = 0;

    if (list->current_size + size > list->max_size)
    {
        hassert(!"render list too small");
         return res;
    }

    render_entry_header *header = (render_entry_header *)(list->base + list->current_size);
    header->type = type;
    res = header;
    list->current_size += size;
    ++list->element_count;

    return res;
}

#define ExtractRenderElement(type, name, header) render_entry_type_##type *##name = (render_entry_type_##type *)header
#define ExtractRenderElementWithSize(type, name, header, size) ExtractRenderElement(type, name, header); size = sizeof(*##name)

inline void
PushClear(render_entry_list *list, v4 color)
{
     render_entry_type_clear *entry = PushRenderElement(list, clear);
     if (entry)
     {
         entry->color = color;
     }
}

inline void
PushUi2d(render_entry_list *list, ui2d_push_context *pushctx)
{
     render_entry_type_ui2d *entry = PushRenderElement(list, ui2d);
     if (entry)
     {
         entry->pushctx = pushctx;
     }
}

inline void
PushQuad(render_entry_list *list, v3 position, v3 size, v4 color, int32_t matrix_id, int32_t asset_descriptor_id)
{
     render_entry_type_quad *entry = PushRenderElement(list, quad);
     if (entry)
     {
         entry->position = position;
         entry->size = size;
         entry->color = color;
         entry->matrix_id = matrix_id;
         entry->asset_descriptor_id = asset_descriptor_id;
     }
}

inline void
PushMatrices(render_entry_list *list, uint32_t count, m4 *matrices)
{
     render_entry_type_matrices *entry = PushRenderElement(list, matrices);
     if (entry)
     {
         entry->count = count;
         entry->matrices = matrices;
     }
}

inline void
PushAssetDescriptors(render_entry_list *list, uint32_t count, asset_descriptor *descriptors)
{
     render_entry_type_asset_descriptors *entry = PushRenderElement(list, asset_descriptors);
     if (entry)
     {
         entry->count = count;
         entry->descriptors = descriptors;
     }
}

inline void
AddRenderList(platform_memory *memory, render_entry_list *list)
{
    hassert(memory->render_lists_count < sizeof(memory->render_lists) - 1);
    memory->render_lists[memory->render_lists_count] = list;
    ++memory->render_lists_count;
}
