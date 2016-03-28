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
    descriptors,

    QUADS,
    QUADS_lookup,

    mesh,
    mesh_from_asset,

    MAX = mesh_from_asset,
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
render_entry_type_QUADS
{
    render_entry_header header;
    uint32_t entry_count;
    int32_t matrix_id;
    int32_t asset_descriptor_id;
    struct {
        v3 position;
        v3 size;
        v4 color;
    } entries[2000];
};

struct
render_entry_type_matrices
{
    render_entry_header header;
    uint32_t count;
    m4 *matrices;
};

enum struct
LightType
{
    directional,
//    point,
};

struct
LightDescriptor
{
    LightType type;
    union
    {
        v3 direction;
        v3 position;
    };
    v3 color;
    float ambient_intensity;
    float diffuse_intensity;

    float attenuation_constant;
    float attenuation_linear;
    float attenuation_exponential;
};

struct
LightDescriptors
{
    uint32_t count;
    LightDescriptor *descriptors;
};

struct
render_entry_type_descriptors
{
    render_entry_header header;
    LightDescriptors lights;
};

struct
buffer
{
    void *data;
    uint32_t size;
};

struct
Mesh
{
    buffer vertices;
    buffer uvs;
    buffer normals;
    buffer indices;
    uint32_t num_triangles;
};

struct
render_entry_type_mesh
{
    render_entry_header header;
    int32_t projection_matrix_id;
    int32_t model_matrix_id;
    Mesh mesh;
    int32_t texture_asset_descriptor_id;
};

struct
render_entry_type_mesh_from_asset
{
    render_entry_header header;
    int32_t projection_matrix_id;
    int32_t model_matrix_id;
    int32_t mesh_asset_descriptor_id;
    int32_t texture_asset_descriptor_id;
    int32_t lights_mask;
};

struct FramebufferFlags
{
    unsigned int initialized:1;
    unsigned int frame_initialized:1;
};

struct
FramebufferDescriptor
{
    v2i size;

    FramebufferFlags _flags;
    uint32_t _fbo;
    uint32_t _texture;
    uint32_t _renderbuffer;
    v2i _current_size;
};

enum struct
asset_descriptor_type
{
    name,
    framebuffer,
    //renderbuffer,
};

struct
asset_descriptor
{
    asset_descriptor_type type;
    union
    {
        const char *asset_name;
        void *ptr;
        FramebufferDescriptor *framebuffer;
    };
    int32_t asset_id;
    uint32_t generation_id;
    bool debug;
    union
    {
        struct
        {
            int32_t start_face;
            int32_t end_face;
        } mesh_debug;
    };
};

struct
render_entry_type_asset_descriptors
{
    render_entry_header header;
    uint32_t count;
    asset_descriptor *descriptors;
};

struct render_entry_type_list;
struct
render_entry_type_QUADS_lookup
{
    render_entry_header header;

    render_entry_type_QUADS_lookup *next;

    uint32_t lookup_count;
    struct
    {
        render_entry_type_QUADS *QUADS;
        int32_t matrix_id;
        int32_t asset_descriptor_id;
    } lookups[20];
};

struct
render_entry_list
{
    uint32_t max_size;
    uint32_t current_size;
    uint8_t *base;
    uint32_t element_count;
    uint32_t element_counts[uint32_t(render_entry_type::MAX) + 1];

    FramebufferDescriptor *framebuffer;

    render_entry_type_QUADS_lookup *first_QUADS_lookup;

#ifndef RENDER_ENTRY_LIST_OPTIMIZED
    render_entry_header *element_pointers[50];
    render_entry_type element_types[50];
#endif
};

#define RenderListBuffer(r, b) { r.max_size = sizeof(b); r.base = b; }
inline void
RenderListReset(render_entry_list *list)
{
    list->current_size = 0;
    list->element_count = 0;
    list->first_QUADS_lookup = 0;
    for (uint32_t i = 0; i < harray_count(list->element_counts); ++i)
    {
         list->element_counts[i] = 0;
    }
#ifndef RENDER_ENTRY_LIST_OPTIMIZED
    for (uint32_t i = 0; i < harray_count(list->element_pointers); ++i)
    {
         list->element_pointers[i] = 0;
    }
    for (uint32_t i = 0; i < harray_count(list->element_types); ++i)
    {
         list->element_types[i] = (render_entry_type)0;
    }
#endif
}

inline void
FramebufferReset(FramebufferDescriptor *descriptor)
{
    descriptor->_flags.frame_initialized = 0;
}

inline bool
FramebufferInitialized(FramebufferDescriptor *descriptor)
{
    return descriptor->_flags.initialized != 0;
}

inline void
FramebufferMakeInitialized(FramebufferDescriptor *descriptor)
{
    descriptor->_flags.initialized = 1;
}

inline bool
FramebufferFrameInitialized(FramebufferDescriptor *descriptor)
{
    return descriptor->_flags.frame_initialized != 0;
}

inline void
FramebufferMakeFrameInitialized(FramebufferDescriptor *descriptor)
{
    descriptor->_flags.frame_initialized = 1;
}

inline bool
FramebufferNeedsResize(FramebufferDescriptor *descriptor)
{
    bool same = (descriptor->size.x == descriptor->_current_size.x) &&
        (descriptor->size.y == descriptor->_current_size.y);
    return !same;
}

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
#ifndef RENDER_ENTRY_LIST_OPTIMIZED
    if (list->element_count < harray_count(list->element_pointers))
    {
        list->element_pointers[list->element_count] = header;
        list->element_types[list->element_count] = type;
    }
#endif
    ++list->element_count;
    ++list->element_counts[(uint32_t)type];

    return res;
}

#define ExtractRenderElement(type, name, header) render_entry_type_##type *name = (render_entry_type_##type *)header
#define ExtractRenderElementWithSize(type, name, header, size) ExtractRenderElement(type, name, header); size = sizeof(*name)
#define ExtractRenderElementSizeOnly(type, size) size = sizeof(render_entry_type_##type)

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

render_entry_type_QUADS *
_find_QUADS_for(render_entry_list *list, int32_t matrix_id, int32_t asset_descriptor_id)
{
    render_entry_type_QUADS *result = 0;
    render_entry_type_QUADS_lookup *l;
    render_entry_type_QUADS_lookup *prev = 0;

    for(l = list->first_QUADS_lookup; l; l = l->next)
    {
        for (uint32_t i = 0; i < l->lookup_count; ++i)
        {
            auto &lookup = l->lookups[i];
            if (lookup.matrix_id == matrix_id && lookup.asset_descriptor_id == asset_descriptor_id)
            {
                if (lookup.QUADS->entry_count < harray_count(result->entries))
                {
                    return lookup.QUADS;
                }
            }
        }
        if (l->lookup_count < harray_count(l->lookups))
        {
            auto &lookup = l->lookups[l->lookup_count++];
            result = PushRenderElement(list, QUADS);
            result->entry_count = 0;
            result->matrix_id = matrix_id;
            result->asset_descriptor_id = asset_descriptor_id;
            lookup.QUADS = result;
            lookup.matrix_id = matrix_id;
            lookup.asset_descriptor_id = asset_descriptor_id;
            return result;
        }
        prev = l;
    }

    hassert(!l);
    l = PushRenderElement(list, QUADS_lookup);
    hassert(l);
    if (!list->first_QUADS_lookup)
    {
        list->first_QUADS_lookup = l;
    }
    else
    {
        hassert(prev);
        prev->next = l;
    }
    l->lookup_count = 0;
    l->next = 0;
    auto &lookup = l->lookups[l->lookup_count++];
    result = PushRenderElement(list, QUADS);
    result->entry_count = 0;
    result->matrix_id = matrix_id;
    result->asset_descriptor_id = asset_descriptor_id;
    hassert(result);
    lookup.QUADS = result;
    lookup.matrix_id = matrix_id;
    lookup.asset_descriptor_id = asset_descriptor_id;
    return result;
}

inline void
_PushQuadFast(render_entry_list *list, v3 position, v3 size, v4 color, int32_t matrix_id, int32_t asset_descriptor_id)
{
    render_entry_type_QUADS *entry = _find_QUADS_for(list, matrix_id, asset_descriptor_id);
    if (entry)
    {
        auto &quad_entry = entry->entries[entry->entry_count++];
        quad_entry.position = position;
        quad_entry.size = size;
        quad_entry.color = color;
    }
}

inline void
_PushQuadSlow(render_entry_list *list, v3 position, v3 size, v4 color, int32_t matrix_id, int32_t asset_descriptor_id)
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

#define PushQuad _PushQuadFast

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
PushDescriptors(render_entry_list *list, LightDescriptors lights)
{
     render_entry_type_descriptors *entry = PushRenderElement(list, descriptors);
     if (entry)
     {
         entry->lights = lights;
     }
}

inline void
PushMesh(render_entry_list *list, int32_t projection_matrix_id, int32_t model_matrix_id, Mesh mesh, int32_t texture_asset_descriptor_id)
{
     render_entry_type_mesh *entry = PushRenderElement(list, mesh);
     if (entry)
     {
         entry->projection_matrix_id = projection_matrix_id;
         entry->model_matrix_id = model_matrix_id;
         entry->mesh = mesh;
         entry->texture_asset_descriptor_id = texture_asset_descriptor_id;
     }
}

inline void
PushMeshFromAsset(render_entry_list *list, int32_t projection_matrix_id, int32_t model_matrix_id, int32_t mesh_asset_descriptor_id, int32_t texture_asset_descriptor_id, int32_t lights_mask)
{
     render_entry_type_mesh_from_asset *entry = PushRenderElement(list, mesh_from_asset);
     if (entry)
     {
         entry->projection_matrix_id = projection_matrix_id;
         entry->model_matrix_id = model_matrix_id;
         entry->mesh_asset_descriptor_id = mesh_asset_descriptor_id;
         entry->texture_asset_descriptor_id = texture_asset_descriptor_id;
         entry->lights_mask = lights_mask;
     }
}

inline void
AddRenderList(platform_memory *memory, render_entry_list *list)
{
    hassert(memory->render_lists_count < sizeof(memory->render_lists) - 1);
    memory->render_lists[memory->render_lists_count] = list;
    ++memory->render_lists_count;
}
