#include "hajonta/thirdparty/glcorearb.h"

#include <stdio.h>

#include <chrono>

#include "hajonta/platform/common.h"
#include "hajonta/renderer/common.h"

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4456 4457 4774 4577 4244 4242 4838 4305 4018 4389 4267)
#endif
#include "hajonta/renderer/opengl_setup.h"
#define PAR_SHAPES_T uint32_t
#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "hajonta/programs/imgui.h"
#include "hajonta/programs/filters/filter_gaussian_7x1.h"
#include "hajonta/programs/filters/filter_sobel.h"
#include "hajonta/programs/sky.h"
#include "hajonta/programs/texarray_1.h"
#include "hajonta/programs/texarray_1_vsm.h"
#include "hajonta/programs/texarray_1_water.h"

#include "stb_truetype.h"
#include "hajonta/image.cpp"
#include "hajonta/math.cpp"
#include "hajonta/algo/fnv1a.cpp"
#include "hajonta/assets.cpp"

#ifndef HAJONTA_QUERY_TIMER
#define HAJONTA_QUERY_TIMER 0
#endif

#ifndef HAJONTA_ERROR_ASSERT
#define HAJONTA_ERROR_ASSERT 0
#endif

static platform_memory *_platform_memory;

static float h_pi = 3.14159265358979f;
static float h_halfpi = h_pi / 2.0f;

DebugTable *GlobalDebugTable;

typedef struct {
    GLuint count;
    GLuint primCount;
    GLuint firstIndex;
    GLint  baseVertex;
    GLuint baseInstance;
} DrawElementsIndirectCommand;

struct TextureAddress
{
    GLuint container_index;
    GLfloat layer;
    // std140 -> arrays aligned at 16 bytes
    GLint reserved[2];
};

struct
CommandKey
{
    uint32_t count;
    ShaderType shader_type;
    VertexFormat vertexformat;
    uint32_t vertex_buffer;
    uint32_t index_buffer;
    uint32_t primitive_type;

    bool operator==(const CommandKey &other) const
    {
        return
            (count == other.count) &&
            (shader_type == other.shader_type) &&
            (vertexformat == other.vertexformat) &&
            (vertex_buffer == other.vertex_buffer) &&
            (index_buffer == other.index_buffer) &&
            (primitive_type == other.primitive_type) &&
            1;
    }
};

struct
BufferKey
{
    VertexFormat vertexformat;

    bool operator==(const BufferKey &other) const
    {
        return
            (vertexformat == other.vertexformat);
    }
};

struct DrawData
{
    m4 projection;
    // 4 * 4 * 4 = 64
    m4 view;
    // 4 * 4 * 4 = 64
    m4 model;
    // 4 * 4 * 4 = 64

    int32_t texture_texaddress_index;
    int32_t shadowmap_texaddress_index;
    int32_t shadowmap_color_texaddress_index;
    int32_t light_index;
    // 4 + 4 + 4 + 4 = 16

    v3 camera_position;
    int32_t bone_offset;
    // 4 * 3 + 4 = 16
    //
    int32_t normal_texaddress_index;
    int32_t specular_texaddress_index;
    int32_t object_identifier;
    int32_t pad[1];
};

struct DrawDataList
{
    DrawData data[100];
};

struct
BoneDataList
{
    m4 data[100];
};

struct Light {
    v4 position_or_direction;
    // 4
    //
    m4 lightspace_matrix;
    // 64

    v3 color;
    float ambient_intensity;
    // 3 + 1

    float diffuse_intensity;
    float attenuation_constant;
    float attenuation_linear;
    float attenuation_exponential;
    // 1+1+1+1
};

struct LightList
{
    Light lights[32];
};

struct TexContainerIndexItem
{
    uint32_t container_index;
    // 3 bytes wasted due to std140 layout rules
    uint32_t reserved[3];
};

struct
TexContainerSamplerMapping
{
    uint32_t generation;
    uint32_t texcontainer_index;
    // 2 bytes wasted due to std140 layout rules
    uint32_t reserved[2];
};

#define NUM_TEXARRAY_TEXTURES 32
#define NUM_CONTAINER_TEXARRAY_TEXTURES 8
struct
CommandList
{
    uint32_t generation;
    uint32_t num_commands;
    DrawElementsIndirectCommand commands[100];
    bool _vao_initialized;
    uint32_t vao;
    union
    {
        DrawDataList draw_data_list;
    };
    uint32_t num_bones;
    BoneDataList bone_data_list;
    uint32_t num_texcontainer_indices;
    TexContainerIndexItem texcontainer_index[NUM_CONTAINER_TEXARRAY_TEXTURES];
    TexContainerSamplerMapping texcontainer_sampler_mapping[NUM_TEXARRAY_TEXTURES];
};

struct
CommandLists
{
    uint32_t num_command_lists;
    CommandKey keys[40];
    CommandList command_lists[40];
};


inline void
hglErrorAssert(bool skip = false)
{
#if HAJONTA_ERROR_ASSERT
    GLenum error = hglGetError();
    if (skip)
    {
         return;
    }
    switch(error)
    {
        case GL_NO_ERROR:
        {
            return;
        } break;
        case GL_INVALID_ENUM:
        {
            hassert(!"Invalid enum");
        } break;
        case GL_INVALID_VALUE:
        {
            hassert(!"Invalid value");
        } break;
        case GL_INVALID_OPERATION:
        {
            hassert(!"Invalid operation");
        } break;
#if !defined(_WIN32)
        case GL_INVALID_FRAMEBUFFER_OPERATION:
        {
            hassert(!"Invalid framebuffer operation");
        } break;
#endif
        case GL_OUT_OF_MEMORY:
        {
            hassert(!"Out of memory");
        } break;
#if !defined(__APPLE__) && !defined(NEEDS_EGL)
        case GL_STACK_UNDERFLOW:
        {
            hassert(!"Stack underflow");
        } break;
        case GL_STACK_OVERFLOW:
        {
            hassert(!"Stack overflow");
        } break;
#endif
        default:
        {
            hassert(!"Unknown error");
        } break;
    }
#else
    return;
#endif
}

#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4456 4457 4774 4577)
#endif
#include "imgui.cpp"
#include "imgui_draw.cpp"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

struct asset_file_to_texture
{
    int32_t asset_file_id;
    int32_t texture_offset;
};

struct asset_file_to_texaddress_index
{
    int32_t asset_file_id;
    int32_t texaddress_index;
};

struct asset_file_to_mesh
{
    int32_t asset_file_id;
    int32_t mesh_offset;
};

struct
asset_file
{
    char asset_file_path[1024];
};

enum struct
RendererAssetType
{
    texture,
    mesh,
};

#define TEXTURE_FORMATS \
    X(rgba8, GL_RGBA8, GL_RGBA) \
    X(srgba, GL_SRGB8_ALPHA8, GL_RGBA) \
    X(rg32f, GL_RG32F, GL_RG) \
    X(rgba16f, GL_RGBA16F, GL_RGBA) \
    X(depth24stencil8, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL) \


enum struct
TextureFormat
{
#define X(a, ...) a,
TEXTURE_FORMATS
#undef X
};

struct
TextureFormatConfig
{
    const char *name;
    uint32_t internal_format;
} texture_format_configs[] =
{
#define X(name, internal_format, ...) { #name, internal_format },
TEXTURE_FORMATS
#undef X
};

struct
asset
{
    RendererAssetType type;
    uint32_t asset_id;
    char asset_name[100];
    int32_t asset_file_id;
    v2 st0;
    v2 st1;
    TextureFormat format;
};

struct GLSetupCountersHistory
{
    GLSetupCounters entries[200];

    int32_t first;
    int32_t last;
};

struct Skybox
{
    v3 vertices[2145];
    v3i faces[4032];

    uint32_t vbo;
    uint32_t ibo;
};

struct
TimerQueryData
{
    uint32_t query_count[2];
    uint32_t query_ids[2][40];
    const char *guid[2][40];
    uint32_t buffer;
};

inline void
EndTimerQuery()
{
#if HAJONTA_QUERY_TIMER
    hglEndQuery(GL_TIME_ELAPSED);
#endif
}

inline void
RecordTimerQuery(TimerQueryData *timer_query_data, const char *guid)
{
#if HAJONTA_QUERY_TIMER
    auto &buffer = timer_query_data->buffer;
    auto &query_count = timer_query_data->query_count[buffer];
    auto &query_id = timer_query_data->query_ids[buffer][query_count];
    timer_query_data->guid[buffer][query_count] = guid;
    hglBeginQuery(GL_TIME_ELAPSED, query_id);
    ++query_count;
#endif
}

inline void
CollectAndSwapTimers(TimerQueryData *timer_query_data)
{
#if HAJONTA_QUERY_TIMER
    auto &buffer = timer_query_data->buffer;
    buffer = (uint32_t)!buffer;

    for (uint32_t i = 0; i < timer_query_data->query_count[buffer]; ++i)
    {
        const char *guid = timer_query_data->guid[buffer][i];
        uint32_t query_id = timer_query_data->query_ids[buffer][i];
        uint32_t time_taken;
        hglGetQueryObjectuiv(query_id, GL_QUERY_RESULT, &time_taken);
        OPENGL_TIMER_RESULT(guid, time_taken);
    }
    timer_query_data->query_count[buffer] = 0;
#endif
}

struct
VertexBuffer
{
    uint32_t vbo;
    uint32_t max_vertices;
    uint32_t vertex_count;
    uint32_t vertex_size;
    // state -> draining, ~3 frames to fully drain?
};

struct
IndexBuffer
{
    uint32_t ibo;
    uint32_t max_indices;
    uint32_t index_count;
    // state -> draining, ~3 frames to fully drain?
};

struct
TextureAddressBuffer
{
    // type, uniform, ssbo?
    union
    {
        struct
        {
            uint32_t ubo;
            uint32_t max_addresses;
            uint32_t address_count;
        } uniform_buffer;
    };
};

struct
DrawDataBuffer
{
    uint32_t ubo;
    uint32_t max_data;
    uint32_t data_count;
};

struct
BoneDataBuffer
{
    uint32_t ubo;
    uint32_t max_data;
    uint32_t data_count;
};

struct
IndirectBuffer
{
    uint32_t ubo;
    uint32_t max_data;
    uint32_t data_count;
};

struct
TexContainerSamplerMappingBuffer
{
    uint32_t ubo;
    uint32_t max_data;
    uint32_t data_count;
};

struct
TexArray1ShaderConfigFlags
{
    unsigned int normal_map_disabled:1;
    unsigned int show_normals:1;
    unsigned int show_normalmap:1;
    unsigned int specular_map_disabled:1;
    unsigned int show_specular:1;
    unsigned int show_specularmap:1;
    unsigned int show_tangent:1;
    unsigned int show_object_identifier:1;
    unsigned int show_texcontainer_index:1;
    unsigned int use_color:1;
};

struct
TexArray1ShaderConfig
{
    Plane clipping_plane;
    TexArray1ShaderConfigFlags flags;
    v4 color;
};

struct
SkyShaderConfig
{
    v3 v3InvWavelength;
    float kr;
    float km;
    float sun_brightness;
    float fKrESun;
    float fKmESun;
    float pi;
    float fKr4PI;
    float fKm4PI;
    float fInnerRadius;
    float fOuterRadius;
    float fScale;
    float fScaleDepth;
    float fScaleOverScaleDepth;
};

struct
TexArray1WaterShaderConfig
{
    v3 camera_position;
    int reflection_texaddress_index;
    int refraction_texaddress_index;
    int refraction_depth_texaddress_index;
    int dudv_map_texaddress_index;
    int normal_map_texaddress_index;
    float tiling;

    float wave_strength;
    float move_factor;
    float minimum_variance;
    float bias;
    float lightbleed_compensation;
    float near_;
    float far_;
};

struct
FilterSobelShaderConfig
{
    int texaddress_index;
    v2 texture_scale;
};

union
ShaderConfig
{
    TexArray1ShaderConfig texarray_1_shaderconfig;
    TexArray1WaterShaderConfig texarray_1_water_shaderconfig;
    SkyShaderConfig sky_shaderconfig;
    FilterSobelShaderConfig filter_sobel_shaderconfig;
};

struct
ShaderConfigBuffer
{
    uint32_t ubo;
    uint32_t max_data;
    uint32_t data_count;
};

struct
LightsBuffer
{
    uint32_t ubo;
};

struct
TextureConfig
{
    int32_t width;
    int32_t height;
    TextureFormat format;

    inline
    bool operator==(const TextureConfig &other) const
    {
        if (width != other.width)
        {
            return false;
        }
        if (height != other.height)
        {
            return false;
        }
        if (format != other.format)
        {
            return false;
        }
        return true;
    };
};

struct
TextureLayerInfo
{
    int32_t max_layers;
    int32_t num_layers_used;
};

struct
CommandState
{
    uint32_t buffers_used;
    VertexBuffer vertex_buffers[10];
    IndexBuffer index_buffers[10];
    BufferKey buffer_keys[10];

    TextureAddressBuffer texture_address_buffers[10];
    // new, draining?

    DrawDataBuffer draw_data_buffer;
    BoneDataBuffer bone_data_buffer;
    TexContainerSamplerMappingBuffer texcontainer_sampler_mapping_buffer;
    IndirectBuffer indirect_buffer;
    ShaderConfigBuffer shaderconfig_buffer;
    LightList light_list;
    LightsBuffer lights_buffer;
    CommandLists lists;

    uint32_t textures[NUM_TEXARRAY_TEXTURES];
    TextureConfig texture_configs[NUM_TEXARRAY_TEXTURES];
    TextureLayerInfo texture_layer_info[NUM_TEXARRAY_TEXTURES];

    uint32_t texture_address_last_buffer_count;
    uint32_t texture_address_count;
    TextureAddress texture_addresses[100];
    uint32_t vao;
};

struct renderer_state
{
    MemoryArena arena;
    bool initialized;
    CommandState *command_state;
    AssetManagementState asset_state;

    AssetPack asset_pack;
    AssetPack asset_pack_file;

    TimerQueryData timer_query_data;

    imgui_program_struct imgui_program;
    int32_t imgui_texcontainer;
    int32_t imgui_tex;
    uint32_t imgui_cb0;
    texarray_1_program_struct texarray_1_program;
    uint32_t texarray_1_cb0;
    uint32_t texarray_1_cb1;
    uint32_t texarray_1_cb2;
    uint32_t texarray_1_cb3;
    uint32_t texarray_1_cb4;
    uint32_t texarray_1_shaderconfig;
    int32_t texarray_1_texcontainer;
    int32_t texarray_1_shadowmap_tex_id;
    int32_t texarray_1_shadowmap_color_tex_id;

    texarray_1_vsm_program_struct texarray_1_vsm_program;
    uint32_t texarray_1_vsm_cb0;
    uint32_t texarray_1_vsm_cb1;
    uint32_t texarray_1_vsm_cb3;
    uint32_t texarray_1_vsm_cb4;
    int32_t texarray_1_vsm_texcontainer;

    texarray_1_water_program_struct texarray_1_water_program;
    uint32_t texarray_1_water_cb0;
    uint32_t texarray_1_water_cb1;
    uint32_t texarray_1_water_cb2;
    uint32_t texarray_1_water_cb4;
    uint32_t texarray_1_water_shaderconfig;
    int32_t texarray_1_water_texcontainer;

    filter_gaussian_7x1_program_struct filter_gaussian_7x1_program;
    uint32_t filter_gaussian_7x1_cb0;
    int32_t filter_gaussian_7x1_texcontainer;

    filter_sobel_program_struct filter_sobel_program;
    uint32_t filter_sobel_cb0;
    uint32_t filter_sobel_shaderconfig;
    int32_t filter_sobel_texcontainer;

    sky_program_struct sky_program;
    uint32_t sky_shaderconfig;

    uint32_t vbo;
    uint32_t ibo;
    uint32_t QUADS_ibo;
    uint32_t mesh_vertex_vbo;
    uint32_t mesh_uvs_vbo;
    uint32_t mesh_normals_vbo;
    uint32_t mesh_bone_ids_vbo;
    uint32_t mesh_bone_weights_vbo;
    uint32_t vao;
    int32_t tex_id;
    uint32_t font_texture;
    uint32_t white_texture;

    uint32_t rdl_vbo;
    uint32_t rdl_ibo;
    uint32_t rdl_vao;

    //char bitmap_scratch[4096 * 4096 * 4];
    //uint8_t asset_scratch[4096 * 4096 * 4];
    char *bitmap_scratch;
    uint8_t *asset_scratch;
    uint32_t bitmap_scratch_size;
    uint32_t asset_scratch_size;
    game_input *input;

    uint32_t textures[32];
    uint32_t texture_count;
    uint32_t texaddresses[32];
    uint32_t texaddress_count;
    asset_file_to_texture texture_lookup[32];
    uint32_t texture_lookup_count;
    asset_file_to_texaddress_index texaddress_index_lookup[32];
    uint32_t texaddress_index_lookup_count;

    Mesh meshes[16];
    uint32_t mesh_count;
    asset_file_to_mesh mesh_lookup[16];
    uint32_t mesh_lookup_count;

    asset_file asset_files[256];
    uint32_t asset_file_count;
    asset assets[256];
    uint32_t asset_count;

    uint32_t generation_id;

    MouseInput original_mouse_input;

    uint32_t vertices_count;
    struct
    {
        v2 pos;
        v2 uv;
        uint32_t col;
    } vertices_scratch[4 * 2000];
    uint32_t indices_scratch[6 * 2000];

    int32_t shadow_mode;
    int32_t poisson_spread;
    float shadowmap_bias;
    int32_t pcf_distance;
    int32_t poisson_samples;
    float poisson_position_granularity;
    float vsm_minimum_variance;
    float vsm_lightbleed_compensation;
    float blur_scale_divisor;

    m4 m4identity;

    bool show_debug;
    bool show_animation_debug;
    bool show_gl_debug;
    bool show_arena;
    bool gl_debug_toggles[harray_count(counter_names_and_offsets)];

    struct
    {
        v3 plane_vertices[4];
        v2 plane_uvs[4];
        uint16_t plane_indices[6];
    } debug_mesh_data;

    Skybox skybox;

    GLSetupCounters glsetup_counters;
    GLSetupCountersHistory glsetup_counters_history;

    const char *gl_vendor;
    const char *gl_renderer;
    const char *gl_version;
    const char *gl_extension_strings[1000];
    int gl_uniform_buffer_offset_alignment;
    int gl_max_uniform_block_size;
    int gl_max_texture_image_units;

    int uniform_block_size;

    struct
    {
        bool gl_arb_multi_draw_indirect;
        bool gl_arb_draw_indirect;
    } gl_extensions;

    bool multisample_disabled;
    float framebuffer_scale;
    bool overdraw;
    bool flush_for_profiling;
    bool crash_on_gl_errors;
};

renderer_state *_state;

int32_t
lookup_asset_file_to_texaddress_index(renderer_state *state, int32_t asset_file_id)
{
    int32_t result = -1;
    for (uint32_t i = 0; i < state->texaddress_index_lookup_count; ++i)
    {
        asset_file_to_texaddress_index *lookup = state->texaddress_index_lookup + i;
        if (lookup->asset_file_id == asset_file_id)
        {
            result = lookup->texaddress_index;
            break;
        }
    }
    return result;
}

int32_t
lookup_asset_file_to_mesh(renderer_state *state, int32_t asset_file_id)
{
    int32_t result = -1;
    for (uint32_t i = 0; i < state->mesh_lookup_count; ++i)
    {
        asset_file_to_mesh *lookup = state->mesh_lookup + i;
        if (lookup->asset_file_id == asset_file_id)
        {
            result = lookup->mesh_offset;
            break;
        }
    }
    return result;
}

void
add_asset_file_texture_lookup(renderer_state *state, int32_t asset_file_id, int32_t texture_offset)
{
    hassert(state->texture_lookup_count < harray_count(state->texture_lookup));
    asset_file_to_texture *lookup = state->texture_lookup + state->texture_lookup_count;
    ++state->texture_lookup_count;
    lookup->asset_file_id = asset_file_id;
    lookup->texture_offset = texture_offset;
}

void
add_asset_file_texaddress_index_lookup(renderer_state *state, int32_t asset_file_id, int32_t texaddress_index)
{
    hassert(state->texaddress_index_lookup_count < harray_count(state->texaddress_index_lookup));
    asset_file_to_texaddress_index *lookup = state->texaddress_index_lookup + state->texaddress_index_lookup_count;
    ++state->texaddress_index_lookup_count;
    lookup->asset_file_id = asset_file_id;
    lookup->texaddress_index = texaddress_index;
}

void
add_asset_file_mesh_lookup(renderer_state *state, int32_t asset_file_id, int32_t mesh_offset)
{
    hassert(state->mesh_lookup_count < harray_count(state->mesh_lookup));
    asset_file_to_mesh *lookup = state->mesh_lookup + state->mesh_lookup_count;
    ++state->mesh_lookup_count;
    lookup->asset_file_id = asset_file_id;
    lookup->mesh_offset = mesh_offset;
}

uint32_t
container_index_for_size(CommandState *_command_state, int32_t x, int32_t y, TextureFormat format)
{
    TextureConfig ts = {x, y, format};
    TextureConfig uts = {0, 0, (TextureFormat)0};
    auto &command_state = *_command_state;
    enum struct
    _SearchResult
    {
        exhausted,
        found,
        notfound,
    };
    uint32_t container_index = 0;
    _SearchResult search_result = _SearchResult::exhausted;
    for (uint32_t i = 0; i < harray_count(command_state.texture_configs); ++i)
    {
        container_index = i;
        if (ts == command_state.texture_configs[i])
        {
            search_result = _SearchResult::found;
            break;
        }
        if (uts == command_state.texture_configs[i])
        {
            search_result = _SearchResult::notfound;
            break;
        }
    }

    uint32_t tex = command_state.textures[container_index];
    switch (search_result)
    {
        case _SearchResult::exhausted:
        {
            hassert(!"Not enough space for new texture size");
            // return false;
        } break;
        case _SearchResult::found:
        {
        } break;
        case _SearchResult::notfound:
        {
            hglGenTextures(1, &tex);
            command_state.textures[container_index] = tex;
            command_state.texture_configs[container_index] = ts;
            hglBindTexture(GL_TEXTURE_2D_ARRAY, tex);
            int32_t xy = x * y;
            const int32_t target_size = 4096 * 4096 * 10;
            int32_t texture_space_max = min(target_size / xy, 100);
            command_state.texture_layer_info[container_index].max_layers = texture_space_max;
            hglTexStorage3D(
                GL_TEXTURE_2D_ARRAY,
                1,
                texture_format_configs[(uint32_t)format].internal_format,
                x,
                y,
                texture_space_max);
        } break;
    }
    return container_index;
}

uint32_t
new_texaddress(CommandState *command_state, int32_t x, int32_t y, TextureFormat format)
{
    uint32_t container_index = container_index_for_size(command_state, x, y, format);
    auto &layer = command_state->texture_layer_info[container_index].num_layers_used;

    auto &ta_count = command_state->texture_address_count;
    TextureAddress &ta = command_state->texture_addresses[ta_count];
    ta.container_index = container_index;
    ta.layer = (float)layer;

    uint32_t texaddress_index = ta_count;
    ++layer;
    ++ta_count;
    return texaddress_index;
}

bool
load_texture_array_asset(
    renderer_state *state,
    uint8_t *image_buffer,
    uint32_t image_size,
    TextureFormat format,
    int32_t *x,
    int32_t *y,
    uint32_t *texaddress_index
)
{
    uint32_t actual_size;
    load_image(image_buffer, image_size, (uint8_t *)state->bitmap_scratch, state->bitmap_scratch_size,
            x, y, &actual_size);

    auto &command_state = *state->command_state;
    uint32_t container_index = container_index_for_size(&command_state, *x, *y, format);
    auto &tex = command_state.textures[container_index];
    GLuint lod = 0;

    auto &layer = command_state.texture_layer_info[container_index].num_layers_used;
    hglBindTexture(GL_TEXTURE_2D_ARRAY, tex);
    hglTexSubImage3D(
        GL_TEXTURE_2D_ARRAY,
        lod,
        0,
        0,
        layer,
        *x,
        *y,
        1,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        state->bitmap_scratch
    );
    auto &ta_count = command_state.texture_address_count;
    TextureAddress &ta = command_state.texture_addresses[ta_count];
    ta.container_index = container_index;
    ta.layer = (float)layer;

    *texaddress_index = ta_count;
    ++layer;
    ++ta_count;
    hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return true;
}

void
load_texture_asset_failed(
    const char *filename
)
{
    char msg[1024];
    sprintf(msg, "Could not load %s\n", filename);
    _platform->fail(msg);
    _platform->quit = true;
}

V3Bones *
build_base_v3bones(MemoryArena *parent_arena, uint32_t size, uint32_t num_bones, uint32_t num_animations)
{
    V3Bones *result = create_sub_arena("v3_bones_arena", parent_arena, size, V3Bones, arena);
    auto arena = &result->arena;
    result->num_animations = num_animations;
    result->num_bones = num_bones;
    result->animation_names = PushArray("animation_names", arena, V3Name, num_animations);
    result->animations = PushArray("animations", arena, V3Animation, num_animations);

    result->bone_names = PushArray("bone_names", arena, V3Name, num_bones);
    result->bone_parents = PushArray("bone_parents", arena, int32_t, num_bones);
    result->bone_offsets = PushArray("bone_offsets", arena, m4, num_bones);
    result->default_bones = PushArray("default_bones", arena, m4, num_bones);
    result->default_transforms = PushArray("default_transforms", arena, MeshBoneDescriptor, num_bones);

    return result;
}

bool
load_mesh_asset(
    renderer_state *state,
    const char *filename,
    uint8_t *mesh_buffer,
    uint32_t mesh_buffer_size,
    Mesh *mesh
)
{
    uint32_t actual_file_size = {};
    if (!_platform->load_asset(filename, mesh_buffer_size, mesh_buffer, &actual_file_size)) {
        return false;
    }

    union _header
    {
        uint32_t format;
        char format_string[4];
    } *header = (_header *)mesh_buffer;

    hassert(header->format_string[0] == 'H');

    mesh_buffer += 4;

    struct _version
    {
        uint32_t version;
    } *version = (_version *)mesh_buffer;

    mesh_buffer += 4;

    struct format_data
    {
        uint32_t vertices_offset;
        uint32_t vertices_size;
        uint32_t indices_offset;
        uint32_t indices_size;
        uint32_t num_triangles;
        uint32_t num_bones;
        uint32_t bone_names_offset;
        uint32_t bone_names_size;
        uint32_t bone_ids_offset;
        uint32_t bone_ids_size;
        uint32_t bone_weights_offset;
        uint32_t bone_weights_size;
        uint32_t bone_parent_offset;
        uint32_t bone_parent_size;
        uint32_t bone_offsets_offset;
        uint32_t bone_offsets_size;
        uint32_t bone_default_transform_offset;
        uint32_t bone_default_transform_size;
        uint32_t animation_offset;
        uint32_t animation_size;
    } format = {};

    if (version->version == 3)
    {
        struct binary_format_v3_boneless
        {
            uint32_t header_size;
            uint32_t vertexformat;
            uint32_t vertices_offset;
            uint32_t vertices_size;
            uint32_t indices_offset;
            uint32_t indices_size;
            uint32_t num_triangles;
            uint32_t num_vertices;
        } *v3_format = (binary_format_v3_boneless *)mesh_buffer;
        format.vertices_offset = v3_format->vertices_offset;
        format.vertices_size = v3_format->vertices_size;
        format.indices_offset = v3_format->indices_offset;
        format.indices_size = v3_format->indices_size;
        format.num_triangles = v3_format->num_triangles;
        mesh->mesh_format = MeshFormat::v3_boneless;
        mesh->vertexformat = (VertexFormat)v3_format->vertexformat;
        mesh->primitive_type = PrimitiveType::triangles;
        mesh->v3.vertex_count = v3_format->num_vertices;
        mesh_buffer += v3_format->header_size;
        vertexformat_1 *vf = (vertexformat_1 *)mesh_buffer;
        vertexformat_1 vf0 = vf[0];
        vertexformat_1 vf1 = vf[1];
        vertexformat_1 vf2 = vf[2];
        vertexformat_1 vf3 = vf[3];
        vertexformat_1 vf4 = vf[4];
        vertexformat_1 vf5 = vf[5];
        vertexformat_1 vf6 = vf[6];
        mesh->num_indices = 3 * format.num_triangles;
        mesh->num_bones = format.num_bones;
    }
    else if (version->version == 4)
    {
        struct binary_format_v3_bones
        {
            uint32_t header_size;
            uint32_t vertexformat;
            uint32_t vertices_offset;
            uint32_t vertices_size;
            uint32_t indices_offset;
            uint32_t indices_size;
            uint32_t num_triangles;
            uint32_t num_vertices;

            uint32_t num_bones;
            uint32_t bone_names_offset;
            uint32_t bone_names_size;
            uint32_t bone_parent_offset;
            uint32_t bone_parent_size;
            uint32_t bone_offsets_offset;
            uint32_t bone_offsets_size;
            uint32_t bone_default_transform_offset;
            uint32_t bone_default_transform_size;

            uint32_t num_animations;
            uint32_t animation_names_offset;
            uint32_t animation_names_size;
            uint32_t animation_type_offsets_offset;
            uint32_t animation_type_offsets_size;
            uint32_t animations_offset;
            uint32_t animations_size;
        } *v3_format = (binary_format_v3_bones *)mesh_buffer;
        format.vertices_offset = v3_format->vertices_offset;
        format.vertices_size = v3_format->vertices_size;
        format.indices_offset = v3_format->indices_offset;
        format.indices_size = v3_format->indices_size;
        format.num_triangles = v3_format->num_triangles;
        mesh->mesh_format = MeshFormat::v3_bones;
        mesh->vertexformat = (VertexFormat)v3_format->vertexformat;
        mesh->primitive_type = PrimitiveType::triangles;
        mesh->v3.vertex_count = v3_format->num_vertices;

        format.num_bones = v3_format->num_bones;
        format.bone_names_offset = v3_format->bone_names_offset;
        format.bone_names_size = v3_format->bone_names_size;
        format.bone_parent_offset = v3_format->bone_parent_offset;
        format.bone_parent_size = v3_format->bone_parent_size;
        format.bone_offsets_offset = v3_format->bone_offsets_offset;
        format.bone_offsets_size = v3_format->bone_offsets_size;
        format.bone_default_transform_offset = v3_format->bone_default_transform_offset;
        format.bone_default_transform_size = v3_format->bone_default_transform_size;
        format.animation_offset = v3_format->animations_offset;
        format.animation_size = v3_format->animations_size;

        mesh_buffer += v3_format->header_size;
        mesh->num_indices = 3 * format.num_triangles;
        mesh->num_bones = format.num_bones;

        uint8_t *base_offset = mesh_buffer;

        uint32_t memory_size = format.vertices_size + format.indices_size;
        memory_size += format.bone_ids_size + format.bone_weights_size;
        uint8_t *m = (uint8_t *)malloc(memory_size);

        uint32_t offset = 0;
        memcpy((void *)(m + offset), base_offset + format.vertices_offset, format.vertices_size);
        mesh->vertices = {
            (void *)m,
            format.vertices_size,
        };
        offset += format.vertices_size;

        memcpy((void *)(m + offset), base_offset + format.indices_offset, format.indices_size);
        mesh->indices = {
            (void *)(m + offset),
            format.indices_size,
        };
        offset += format.indices_size;

        uint32_t v3bones_size = 1024 * 1024; // TODO: calculate this
        mesh->v3.v3bones = build_base_v3bones(
            &state->arena,
            v3bones_size,
            v3_format->num_bones,
            v3_format->num_animations
        );
        auto &v3bones = *mesh->v3.v3bones;

        int32_t *bone_parents = (int32_t *)(base_offset + format.bone_parent_offset);
        m4 *bone_offsets = (m4 *)(base_offset + format.bone_offsets_offset);
        MeshBoneDescriptor *default_transforms = (MeshBoneDescriptor *)(base_offset + format.bone_default_transform_offset);
        uint8_t *bone_name = base_offset + format.bone_names_offset;
        for (uint32_t i = 0; i < format.num_bones; ++i)
        {
            v3bones.bone_parents[i] = bone_parents[i];
            v3bones.bone_offsets[i] = bone_offsets[i];
            v3bones.default_transforms[i] = default_transforms[i];

            uint8_t bone_name_length = *bone_name;
            v3bones.bone_names[i].name = PushStringWithLength("bone_name", &v3bones.arena, bone_name[0], (char *)(bone_name + 1));

            bone_name += bone_name_length + 1;
        }

        AnimationTypeOffset *atos = (AnimationTypeOffset *)(base_offset + v3_format->animation_type_offsets_offset);
        uint8_t *animation_name = base_offset + v3_format->animation_names_offset;
        bool found_idle = false;
        bool found_walk = false;
        for (uint32_t k = 0; k < v3_format->num_animations; ++k)
        {
            uint8_t animation_name_length = *animation_name;
            v3bones.animation_names[k].name = PushStringWithLength("animation_name", &v3bones.arena, animation_name[0], (char *)(animation_name + 1));
            if (!found_idle && strstr(v3bones.animation_names[k].name, "Idle"))
            {
                v3bones.idle_animation = k;
                found_idle = true;
            }
            if (!found_idle && strstr(v3bones.animation_names[k].name, "Rest"))
            {
                v3bones.idle_animation = k;
                found_idle = true;
            }
            if (!found_walk && strstr(v3bones.animation_names[k].name, "Walk"))
            {
                v3bones.walk_animation = k;
                found_walk = true;
            }
            animation_name += animation_name_length + 1;

            AnimationTypeOffset &ato = atos[k];
            BoneAnimationHeader *bone_animation_header = (BoneAnimationHeader *)(base_offset + v3_format->animations_offset + ato.animation_offset);
            AnimTick *anim_tick = (AnimTick *)(bone_animation_header + 1);

            uint32_t total_ticks = format.num_bones * (uint32_t)bone_animation_header->num_ticks;
            v3bones.animations[k].animation_ticks = PushArray("animation_ticks", &v3bones.arena, AnimTick, total_ticks);
            AnimTick *dest_anim_tick = v3bones.animations[k].animation_ticks;

            for (uint32_t i = 0; i < bone_animation_header->num_ticks; ++i)
            {
                for (uint32_t j = 0; j < format.num_bones; ++j)
                {
                    dest_anim_tick->transform = anim_tick->transform;
                    ++anim_tick;
                    ++dest_anim_tick;
                }
            }
            v3bones.animations[k].num_ticks = (uint32_t)bone_animation_header->num_ticks;
        }
        return true;
    }
    else
    {
        hassert(!"unimplemented");
    }

    uint8_t *base_offset = mesh_buffer;

    uint32_t memory_size = format.vertices_size + format.indices_size;
    memory_size += format.bone_ids_size + format.bone_weights_size;
    uint8_t *m = (uint8_t *)malloc(memory_size);

    uint32_t offset = 0;
    memcpy((void *)(m + offset), base_offset + format.vertices_offset, format.vertices_size);
    mesh->vertices = {
        (void *)m,
        format.vertices_size,
    };
    offset += format.vertices_size;

    memcpy((void *)(m + offset), base_offset + format.indices_offset, format.indices_size);
    mesh->indices = {
        (void *)(m + offset),
        format.indices_size,
    };
    offset += format.indices_size;

    return true;
}

void
load_program_failed(
    const char *program
)
{
    char msg[1024];
    sprintf(msg, "Could not load program %s\n", program);
    _platform->fail(msg);
    _platform->quit = true;
}

void
load_mesh_asset_failed(
    const char *filename
)
{
    char msg[1024];
    sprintf(msg, "Could not load mesh %s\n", filename);
    _platform->fail(msg);
    _platform->quit = true;
}

bool
populate_skybox(renderer_state *state, Skybox *skybox)
{
    hglGenBuffers(1, &skybox->vbo);
    hglGenBuffers(1, &skybox->ibo);

    if (state->crash_on_gl_errors) hglErrorAssert();
    par_shapes_mesh *mesh = par_shapes_create_parametric_sphere(32, 64);

    v3 vertices[harray_count(skybox->vertices)];
    hassert(mesh->npoints == harray_count(vertices));
    for (uint32_t i = 0; i < harray_count(vertices); ++i)
    {
        vertices[i] = {
            mesh->points[3*i],
            mesh->points[3*i+1],
            mesh->points[3*i+2],
        };
    }
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4127)
#endif
    hassert(sizeof(skybox->vertices) == sizeof(vertices));
    hassert(harray_count(skybox->vertices) == harray_count(vertices));
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    memcpy(skybox->vertices, vertices, sizeof(vertices));
    hglBindBuffer(GL_ARRAY_BUFFER, skybox->vbo);
    hglBufferData(GL_ARRAY_BUFFER,
            sizeof(skybox->vertices),
            skybox->vertices,
            GL_STATIC_DRAW);

    if (state->crash_on_gl_errors) hglErrorAssert();

    v3i faces[harray_count(skybox->faces)];
    assert(mesh->ntriangles == harray_count(faces));
    for (uint32_t i = 0; i < harray_count(faces); ++i)
    {
        faces[i] = {
            (int32_t)mesh->triangles[3*i],
            (int32_t)mesh->triangles[3*i+1],
            (int32_t)mesh->triangles[3*i+2],
        };
    }

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4127)
#endif
    hassert(sizeof(skybox->faces) == sizeof(faces));
    hassert(harray_count(skybox->faces) == harray_count(faces));
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    memcpy(skybox->faces, faces, sizeof(faces));
    hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox->ibo);
    if (state->crash_on_gl_errors) hglErrorAssert();
    hglBufferData(GL_ELEMENT_ARRAY_BUFFER,
            sizeof(skybox->faces),
            skybox->faces,
            GL_STATIC_DRAW);

    return true;
}

bool
program_init(renderer_state *state)
{
    hglEnable(GL_FRAMEBUFFER_SRGB);
    bool loaded = true;
    {
        imgui_program_struct *program = &state->imgui_program;
        imgui_program(program);

        hglGenBuffers(1, &state->vbo);
        hglGenBuffers(1, &state->ibo);
        hglGenBuffers(1, &state->QUADS_ibo);
        hglGenBuffers(1, &state->mesh_vertex_vbo);
        hglGenBuffers(1, &state->mesh_uvs_vbo);
        hglGenBuffers(1, &state->mesh_normals_vbo);
        hglGenBuffers(1, &state->mesh_bone_ids_vbo);
        hglGenBuffers(1, &state->mesh_bone_weights_vbo);

        hglGenVertexArrays(1, &state->vao);
        hglBindVertexArray(state->vao);
        hglBindBuffer(GL_ARRAY_BUFFER, state->vbo);
        hglEnableVertexAttribArray((GLuint)program->a_position_id);
        hglEnableVertexAttribArray((GLuint)program->a_uv_id);
        hglEnableVertexAttribArray((GLuint)program->a_color_id);

        hglVertexAttribPointer((GLuint)program->a_position_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, pos));
        hglVertexAttribPointer((GLuint)program->a_uv_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, uv));
        hglVertexAttribPointer((GLuint)program->a_color_id, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, col));

        ImGuiIO& io = ImGui::GetIO();
        uint8_t *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        auto &command_state = *state->command_state;

        {
            state->font_texture = new_texaddress(&command_state, width, height, TextureFormat::srgba);
            auto &ta = command_state.texture_addresses[state->font_texture];
            GLuint lod = 0;
            auto &tex = command_state.textures[ta.container_index];

            hglBindTexture(GL_TEXTURE_2D_ARRAY, tex);
            hglTexSubImage3D(
                GL_TEXTURE_2D_ARRAY,
                lod,
                0,
                0,
                ta.layer,
                width,
                height,
                1,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                pixels
            );
            hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
            hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
            io.Fonts->TexID = (void *)(intptr_t)state->font_texture;
        }

        state->imgui_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
        state->imgui_tex = hglGetUniformLocation(program->program, "tex");
        state->imgui_cb0 = hglGetUniformBlockIndex(program->program, "CB0");
        hglUniformBlockBinding(
            program->program,
            state->imgui_cb0,
            0);
    }

    {
        texarray_1_program_struct *program = &state->texarray_1_program;
        loaded &= texarray_1_program(program);
        if (!loaded)
        {
            return false;
        }
        state->texarray_1_cb0 = hglGetUniformBlockIndex(program->program, "CB0");
        hglUniformBlockBinding(
            state->texarray_1_program.program,
            state->texarray_1_cb0,
            0);
        state->texarray_1_cb1 = hglGetUniformBlockIndex(program->program, "CB1");
        hglUniformBlockBinding(
            state->texarray_1_program.program,
            state->texarray_1_cb1,
            1);
        state->texarray_1_cb2 = hglGetUniformBlockIndex(program->program, "CB2");
        hglUniformBlockBinding(
            state->texarray_1_program.program,
            state->texarray_1_cb2,
            2);
        state->texarray_1_cb3 = hglGetUniformBlockIndex(program->program, "CB3");
        hglUniformBlockBinding(
            state->texarray_1_program.program,
            state->texarray_1_cb3,
            3);
        state->texarray_1_cb4 = hglGetUniformBlockIndex(program->program, "CB4");
        hglUniformBlockBinding(
            state->texarray_1_program.program,
            state->texarray_1_cb4,
            4);
        state->texarray_1_shaderconfig = hglGetUniformBlockIndex(program->program, "SHADERCONFIG");
        hglUniformBlockBinding(
            state->texarray_1_program.program,
            state->texarray_1_shaderconfig,
            9);
        state->texarray_1_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
    }

    {
        texarray_1_vsm_program_struct *program = &state->texarray_1_vsm_program;
        loaded &= texarray_1_vsm_program(program);
        state->texarray_1_vsm_cb0 = hglGetUniformBlockIndex(program->program, "CB0");
        hglUniformBlockBinding(
            state->texarray_1_vsm_program.program,
            state->texarray_1_vsm_cb0,
            0);
        state->texarray_1_vsm_cb1 = hglGetUniformBlockIndex(program->program, "CB1");
        hglUniformBlockBinding(
            state->texarray_1_vsm_program.program,
            state->texarray_1_vsm_cb1,
            1);
        state->texarray_1_vsm_cb3 = hglGetUniformBlockIndex(program->program, "CB3");
        hglUniformBlockBinding(
            state->texarray_1_vsm_program.program,
            state->texarray_1_vsm_cb3,
            3);
        state->texarray_1_vsm_cb4 = hglGetUniformBlockIndex(program->program, "CB4");
        hglUniformBlockBinding(
            state->texarray_1_vsm_program.program,
            state->texarray_1_vsm_cb4,
            4);
        state->texarray_1_vsm_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
    }

    {
        texarray_1_water_program_struct *program = &state->texarray_1_water_program;
        loaded &= texarray_1_water_program(program);
        state->texarray_1_water_cb0 = hglGetUniformBlockIndex(program->program, "CB0");
        hglUniformBlockBinding(
            state->texarray_1_water_program.program,
            state->texarray_1_water_cb0,
            0);
        state->texarray_1_water_cb1 = hglGetUniformBlockIndex(program->program, "CB1");
        hglUniformBlockBinding(
            state->texarray_1_water_program.program,
            state->texarray_1_water_cb1,
            1);
        state->texarray_1_water_cb2 = hglGetUniformBlockIndex(program->program, "CB2");
        hglUniformBlockBinding(
            state->texarray_1_water_program.program,
            state->texarray_1_water_cb2,
            2);
        state->texarray_1_water_cb4 = hglGetUniformBlockIndex(program->program, "CB4");
        hglUniformBlockBinding(
            state->texarray_1_water_program.program,
            state->texarray_1_water_cb4,
            4);
        state->texarray_1_water_shaderconfig = hglGetUniformBlockIndex(program->program, "SHADERCONFIG");
        hglUniformBlockBinding(
            program->program,
            state->texarray_1_water_shaderconfig,
            9);
        state->texarray_1_water_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
    }

    {
        filter_gaussian_7x1_program_struct *program = &state->filter_gaussian_7x1_program;
        loaded &= filter_gaussian_7x1_program(program);
        state->filter_gaussian_7x1_cb0 = hglGetUniformBlockIndex(program->program, "CB0");
        hglUniformBlockBinding(
            program->program,
            state->filter_gaussian_7x1_cb0,
            0);
        state->filter_gaussian_7x1_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
    }

    {
        filter_sobel_program_struct *program = &state->filter_sobel_program;
        loaded &= filter_sobel_program(program);
        state->filter_sobel_cb0 = hglGetUniformBlockIndex(program->program, "CB0");
        hglUniformBlockBinding(
            program->program,
            state->filter_sobel_cb0,
            0);
        state->filter_sobel_shaderconfig = hglGetUniformBlockIndex(program->program, "SHADERCONFIG");
        hglUniformBlockBinding(
            program->program,
            state->filter_sobel_shaderconfig,
            9);
        state->filter_sobel_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
    }

    {
        sky_program_struct *program = &state->sky_program;
        loaded &= sky_program(program);
        populate_skybox(state, &state->skybox);

        state->sky_shaderconfig = hglGetUniformBlockIndex(program->program, "SHADERCONFIG");
        hglUniformBlockBinding(
            state->sky_program.program,
            state->sky_shaderconfig,
            9);
    }

    auto &command_state = *state->command_state;
    hglGenVertexArrays(1, &command_state.vao);
    hglGenBuffers(1, &command_state.texture_address_buffers[0].uniform_buffer.ubo);
    hglGenBuffers(1, &command_state.draw_data_buffer.ubo);
    hglGenBuffers(1, &command_state.bone_data_buffer.ubo);
    hglGenBuffers(1, &command_state.texcontainer_sampler_mapping_buffer.ubo);
    hglGenBuffers(1, &command_state.indirect_buffer.ubo);
    hglGenBuffers(1, &command_state.lights_buffer.ubo);
    hglGenBuffers(1, &command_state.shaderconfig_buffer.ubo);

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.texture_address_buffers[0].uniform_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(command_state.texture_addresses), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.draw_data_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(DrawDataList), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.lights_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(LightList), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.shaderconfig_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderConfig), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.bone_data_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(BoneDataList), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.texcontainer_sampler_mapping_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, 1024, (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.indirect_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(DrawElementsIndirectCommand) * 100, (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);

    return loaded;
}

int32_t
lookup_asset_file(renderer_state *state, const char *asset_file_path)
{
    int32_t result = -1;
    for (uint32_t i = 0; i < state->asset_file_count; ++i)
    {
        if (strcmp(asset_file_path, state->asset_files[i].asset_file_path) == 0)
        {
            result = (int32_t)i;
            break;
        }
    }
    return result;
}

int32_t
add_asset_file(renderer_state *state, const char *asset_file_path)
{
    int32_t result = lookup_asset_file(state, asset_file_path);
    hassert(state->asset_file_count < harray_count(state->asset_files));
    if (result < 0 && state->asset_file_count < harray_count(state->asset_files))
    {
        asset_file *f = state->asset_files + state->asset_file_count;
        result = (int32_t)state->asset_file_count;
        ++state->asset_file_count;
        strcpy(f->asset_file_path, asset_file_path);
    }
    return result;
}

int32_t
add_asset_file_texaddress_index(renderer_state *state, int32_t asset_file_id, TextureFormat format)
{
    int32_t result = lookup_asset_file_to_texaddress_index(state, asset_file_id);
    if (result < 0)
    {
        hassert(state->texaddress_count < harray_count(state->texaddresses));
        int32_t x, y;
        asset_file *asset_file0 = state->asset_files + asset_file_id;
        uint32_t texaddress_index;

        uint32_t actual_file_size = {};

        bool loaded = _platform->load_asset(
            asset_file0->asset_file_path,
            state->asset_scratch_size,
            state->asset_scratch,
            &actual_file_size
        );
        if (!loaded) {
            return false;
        }

        loaded = load_texture_array_asset(
            state,
            state->asset_scratch,
            actual_file_size,
            format,
            &x,
            &y,
            &texaddress_index);
        if (!loaded)
        {
            load_texture_asset_failed(asset_file0->asset_file_path);
            return false;
        }
        result = (int32_t)state->texaddress_count;
        ++state->texaddress_count;
        add_asset_file_texaddress_index_lookup(state, asset_file_id, (int32_t)texaddress_index);
    }
    return result;
}

int32_t
add_asset_file_mesh(renderer_state *state, int32_t asset_file_id)
{
    int32_t result = lookup_asset_file_to_mesh(state, asset_file_id);
    hassert(state->mesh_count < harray_count(state->meshes));
    if (result < 0)
    {
        asset_file *asset_file0 = state->asset_files + asset_file_id;
        bool loaded = load_mesh_asset(
            state,
            asset_file0->asset_file_path,
            state->asset_scratch,
            state->asset_scratch_size,
            state->meshes + state->mesh_count);
        if (!loaded)
        {
            load_mesh_asset_failed(asset_file0->asset_file_path);
            return false;
        }
        result = (int32_t)state->mesh_count;
        ++state->mesh_count;
        add_asset_file_mesh_lookup(state, asset_file_id, result);
    }
    return result;
}

int32_t
_add_asset(renderer_state *state, RendererAssetType type, const char *asset_name, int32_t asset_file_id)
{
    int32_t result = -1;
    hassert(state->asset_count < harray_count(state->assets));
    asset *asset0 = state->assets + state->asset_count;
    asset0->asset_id = 0;
    strcpy(asset0->asset_name, asset_name);
    asset0->asset_file_id = asset_file_id;
    asset0->format = TextureFormat::srgba;
    result = (int32_t)state->asset_count;
    ++state->asset_count;
    return result;
}

int32_t
add_asset(renderer_state *state, const char *asset_name, int32_t asset_file_id, v2 st0, v2 st1)
{
    int32_t result = _add_asset(state, RendererAssetType::texture, asset_name, asset_file_id);
    asset *asset0 = state->assets + result;
    asset0->st0 = st0;
    asset0->st1 = st1;
    return result;
}

int32_t
add_asset(renderer_state *state, const char *asset_name, const char *asset_file_name, v2 st0, v2 st1)
{
    int32_t result = -1;

    int32_t asset_file_id = add_asset_file(state, asset_file_name);
    if (asset_file_id >= 0)
    {
        result = add_asset(state, asset_name, asset_file_id, st0, st1);
    }
    return result;
}

int32_t
add_rgba8_asset(renderer_state *state, const char *asset_name, const char *asset_file_name, v2 st0, v2 st1)
{
    int32_t result = -1;

    int32_t asset_file_id = add_asset_file(state, asset_file_name);
    if (asset_file_id >= 0)
    {
        result = add_asset(state, asset_name, asset_file_id, st0, st1);
        if (result >= 0)
        {
            asset *asset0 = state->assets + result;
            asset0->format = TextureFormat::rgba8;
        }
    }
    return result;
}

int32_t
add_mesh_asset(renderer_state *state, const char *asset_name, int32_t asset_file_id)
{
    int32_t result = _add_asset(state, RendererAssetType::mesh, asset_name, asset_file_id);
    return result;
}

int32_t
add_mesh_asset(renderer_state *state, const char *asset_name, const char *asset_file_name)
{
    int32_t result = -1;

    int32_t asset_file_id = add_asset_file(state, asset_file_name);
    if (asset_file_id >= 0)
    {
        result = add_mesh_asset(state, asset_name, asset_file_id);
    }
    return result;
}

AssetPack
build_asset_pack(MemoryArena *arena)
{
    AssetPack result;
    const int MAX_ASSETS = 10;
    const int MAX_PIECES = 10;
    const int HASH_BITS = 6;
    result.assets = PushArray("temp_assets", arena, Asset, MAX_ASSETS);
    result.asset_pieces = PushArray("temp_asset_pieces", arena, AssetPiece, MAX_PIECES);
    result.asset_hash_size = 1 << HASH_BITS;
    result.asset_hash = PushArray("temp_asset_hash", arena, uint32_t, result.asset_hash_size);
    result.filenames = (char *)PushSize("temp_asset_filenames", arena, MAX_PIECES * 100);
    result.piece_storage_type = AssetPieceStorageType::Files;

    uint32_t next_filename = 0;
    uint32_t next_asset_id = 0;
    uint32_t next_piece_id = 0;

    {

        Asset &a = result.assets[next_asset_id];
        a.asset_type = AssetType::Texture;
        a.asset_sub_type.texture = TextureAssetSubType::Diffuse;
        a.last_modified = 0;
        a.num_asset_pieces = 1;
        a.asset_piece_id = next_piece_id;
        const char *asset_name = "testing/nobiax/modular_building/diffuse";
        uint32_t asset_id = fnv1a_32((uint8_t *)asset_name, (uint32_t)strlen(asset_name));
        a.asset_id = asset_id;
        result.asset_hash[asset_id % result.asset_hash_size] = next_asset_id + 1;
        next_asset_id++;

        AssetPiece &p = result.asset_pieces[next_piece_id];
        const char *filename = "testing/nobiax/modular_building/diffuse.tga";
        p.offset = next_filename;
        strcpy(result.filenames + next_filename, filename);
        next_filename += (uint32_t)strlen(filename) + 1;
        p.size = 50331692;
        p.metadata.texture.container_format = AssetContainerFormat::Plain;
        p.metadata.texture.texture_format = AssetTextureFormat::CompressedImage;
        p.metadata.texture.dimension = {4096, 4096, 0};
        p.metadata.texture.bpp = 32;
        p.metadata.texture.num_mipmaps = 1;
        next_piece_id++;
    }

    return result;
}

AssetPack
build_asset_pack_file(MemoryArena *arena)
{
    AssetPack result;

    uint32_t actual_file_size;
    bool loaded = _platform->load_asset(
        "assets.pack",
        _state->asset_scratch_size,
        _state->asset_scratch,
        &actual_file_size
    );
    hassert(loaded);

    union _header
    {
        uint32_t format;
        char format_string[4];
    } *header = (_header *)_state->asset_scratch;

    hassert(header->format_string[0] == 'H');

    uint32_t *version = (uint32_t *)(header + 1);

    AssetFile_0 *af = (AssetFile_0 *)(version + 1);


    result.assets = (Asset *)PushSize("pack_assets", arena, af->asset_size);
    result.asset_count = af->asset_count;
    result.asset_pieces = (AssetPiece *)PushSize("pack_asset_pieces", arena, af->piece_size);
    result.asset_hash_size = af->asset_hash_size;
    result.asset_hash = PushArray("pack_asset_hash", arena, uint32_t, af->asset_hash_size);
    result.filenames = (char *)PushSize("pack_asset_filenames", arena, af->filename_size);
    result.piece_storage_type = af->piece_storage_type;

    uint8_t *content = ((uint8_t *)af) + af->header_size;

    memcpy(result.assets, content + af->asset_offset, af->asset_size);
    memcpy(result.asset_pieces, content + af->piece_offset, af->piece_size);
    memcpy(result.asset_hash, content + af->hash_offset, af->hash_size);
    memcpy(result.filenames, content + af->filename_offset, af->filename_size);

    return result;
}

extern "C" RENDERER_SETUP(renderer_setup)
{
    static std::chrono::steady_clock::time_point last_frame_start_time = std::chrono::steady_clock::now();

    _platform = memory->platform_api;
    if (!memory->renderer_block)
    {
        memory->renderer_block = _platform->allocate_memory("renderer", 256 * 1024 * 1024);
        bootstrap_memory_arena(memory->renderer_block, renderer_state, arena);
        renderer_state *state = (renderer_state *)memory->renderer_block->base;
        _state = state;
        state->command_state = PushStruct("command_state", &state->arena, CommandState);
        for (uint32_t i = 0; i < harray_count(state->command_state->lists.command_lists); ++i)
        {
            state->command_state->lists.command_lists[i].generation = 1;
        }
        state->bitmap_scratch_size = 4096 * 4096 * 4;
        state->bitmap_scratch = (char *)PushSize("bitmap_scratch", &state->arena, state->bitmap_scratch_size);
        state->asset_scratch_size = 4096 * 4096 * 4;
        state->asset_scratch = (uint8_t *)PushSize("asset_scratch", &state->arena, state->bitmap_scratch_size);

        asset_management_state_init(&state->asset_state, &state->arena, 16, 32);
        state->asset_pack = build_asset_pack(&state->arena);
        state->asset_pack_file = build_asset_pack_file(&state->arena);
        add_pack_to_asset_management_state(&state->asset_state, &state->asset_pack_file);
    }
    GlobalDebugTable = memory->debug_table;
    TIMED_FUNCTION();

    /*
#if !defined(NEEDS_EGL) && !defined(__APPLE__)
    if (!glCreateProgram)
    {
    }
#endif
    */
    //_platform->render_lists_count = 0;
    renderer_state *state = (renderer_state *)memory->renderer_block->base;
    _state = state;

    if (state->initialized)
    {
        // OpenGL hooks for OBS, ..., can introduce errors between frames...
        if (state->crash_on_gl_errors) hglErrorAssert(true);
    }

    memory->imgui_state = ImGui::GetCurrentContext();
    if (!state->initialized)
    {
        glsetup_counters = &state->glsetup_counters;
        for (uint32_t i = 0; i < 6; ++i)
        {
            state->gl_debug_toggles[i] = 1;
        }
        state->glsetup_counters_history.first = -1;
        load_glfuncs(_platform->glgetprocaddress);
        if (state->crash_on_gl_errors) hglErrorAssert();
        state->gl_vendor = (const char *)hglGetString(GL_VENDOR);
        state->gl_renderer = (const char *)hglGetString(GL_RENDERER);
        state->gl_version = (const char *)hglGetString(GL_VERSION);
        hglGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &state->gl_uniform_buffer_offset_alignment);
        hglGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &state->gl_max_uniform_block_size);
        hglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &state->gl_max_texture_image_units);
        state->uniform_block_size = state->gl_max_uniform_block_size;
        int32_t num_extensions;
        hglGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

        for (uint32_t i = 0; i < (uint32_t)num_extensions; ++i)
        {
            hassert(i < harray_count(state->gl_extension_strings));
            const char *extension = state->gl_extension_strings[i] = (const char *)hglGetStringi(GL_EXTENSIONS, i);
            state->gl_extension_strings[i] = extension;
            if (strcmp(extension, "GL_ARB_multi_draw_indirect") == 0)
            {
                state->gl_extensions.gl_arb_multi_draw_indirect = true;
            }
            if (strcmp(extension, "GL_ARB_draw_indirect") == 0)
            {
                state->gl_extensions.gl_arb_draw_indirect = true;
            }
        }

        if (strstr(state->gl_renderer, "Intel HD Graphics 4"))
        {
            state->multisample_disabled = false;
            state->framebuffer_scale = 0.5f;
            memory->shadowmap_size = 256;
        }
        else
        {
            state->multisample_disabled = false;
            state->framebuffer_scale = 1.0f;
            memory->shadowmap_size = 512;
        }
        bool loaded = program_init(state);
        hassert(loaded);
        state->initialized = true;
        state->generation_id = 1;
        state->show_debug = 1;
        state->crash_on_gl_errors = 1;
        //state->flush_for_profiling = 1;
        add_asset(state, "mouse_cursor_old", "ui/slick_arrows/slick_arrow-delta.png", {0.0f, 0.0f}, {1.0f, 1.0f});
        add_asset(state, "mouse_cursor", "testing/kenney/cursorSword_silver.png", {0.0f, 0.0f}, {1.0f, 1.0f});
        add_mesh_asset(state, "tree_mesh", "testing/low_poly_tree/tree.hjm");
        add_mesh_asset(state, "cube_mesh", "testing/cube.hjm");

        add_mesh_asset(state, "kenney_blocky_advanced_mesh", "testing/kenney/blocky_advanced4.hjm");
        add_mesh_asset(state, "kenney_blocky_advanced_mesh2", "testing/kenney/blocky_advanced4.hjm");
        add_asset(state, "kenney_blocky_advanced_cowboy_texture", "testing/kenney/skin_exclusiveCowboy.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "blockfigureRigged6_mesh", "testing/human_low.hjm");
        add_asset(state, "blockfigureRigged6_texture", "testing/blockfigureRigged6.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_asset(state, "white_texture", "testing/white.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "dog2_mesh", "testing/dog2/dog3.hjm");
        add_asset(state, "dog2_texture", "testing/dog2/dog3C.png", {0.0f, 1.0f}, {1.0f, 0.0f});

        add_rgba8_asset(state, "water_normal", "testing/water_normal.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_rgba8_asset(state, "water_dudv", "testing/water_dudv.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_asset(state, "square_texture", "testing/square.png", {0.0f, 1.0f}, {1.0f, 0.0f});

        uint32_t scratch_pos = 0;
        for (uint32_t i = 0; i < harray_count(state->indices_scratch) / 6; ++i)
        {
            uint32_t start = i * 4;
            state->indices_scratch[scratch_pos++] = start + 0;
            state->indices_scratch[scratch_pos++] = start + 1;
            state->indices_scratch[scratch_pos++] = start + 2;
            state->indices_scratch[scratch_pos++] = start + 2;
            state->indices_scratch[scratch_pos++] = start + 3;
            state->indices_scratch[scratch_pos++] = start + 0;
        }
        hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->QUADS_ibo);
        hglBufferData(GL_ELEMENT_ARRAY_BUFFER,
            sizeof(state->indices_scratch),
            state->indices_scratch,
            GL_STATIC_DRAW);
        state->m4identity = m4identity();
        state->poisson_spread = 700;
        state->shadow_mode = 6;
        state->pcf_distance = 3;
        state->poisson_samples = 4;
        state->poisson_position_granularity = 1000.0f;
        state->vsm_minimum_variance = 0.0000002f;
        state->vsm_lightbleed_compensation = 0.001f;
        state->blur_scale_divisor = 4096.0f;
        state->shadowmap_bias = -0.0001f;
        hglGenQueries(
            2 * harray_count(state->timer_query_data.query_ids[0]),
            (uint32_t *)&state->timer_query_data.query_ids
        );
    }

    if (state->crash_on_gl_errors) hglErrorAssert();

    state->input = input;

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)input->window.width, (float)input->window.height);
    io.FontGlobalScale = 1.0f / state->framebuffer_scale;
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    std::chrono::steady_clock::time_point this_frame_start_time = std::chrono::steady_clock::now();
    auto diff = this_frame_start_time - last_frame_start_time;
    using microseconds = std::chrono::microseconds;
    int64_t duration_us = std::chrono::duration_cast<microseconds>(diff).count();
    io.DeltaTime = (float)duration_us / 1000 / 1000;
    last_frame_start_time = this_frame_start_time;

    io.MousePos = ImVec2((float)input->mouse.x, (float)input->mouse.y);
    io.MouseDown[0] = input->mouse.buttons.left.ended_down;
    io.MouseDown[1] = input->mouse.buttons.middle.ended_down;
    io.MouseDown[2] = input->mouse.buttons.right.ended_down;
    io.MouseWheel = input->mouse.vertical_wheel_delta;

    io.KeysDown[8] = false;
    io.KeysDown[9] = false;
    io.KeysDown[13] = false;
    io.KeysDown[27] = false;
    io.KeyMap[ImGuiKey_Backspace] = 8;
    io.KeyMap[ImGuiKey_Tab] = 9;
    io.KeyMap[ImGuiKey_Enter] = 13;
    io.KeyMap[ImGuiKey_Escape] = 27;

    if (memory->debug_keyboard)
    {
        keyboard_input *k = input->keyboard_inputs;
        for (uint32_t idx = 0;
                idx < harray_count(input->keyboard_inputs);
                ++idx)
        {
            keyboard_input *ki = k + idx;
            if (ki->type == keyboard_input_type::ASCII)
            {
                switch (ki->ascii)
                {
                    case 8:
                    case 9:
                    case 13:
                    case 27:
                    {
                        io.KeysDown[(uint32_t)ki->ascii] = true;
                    } break;
                    default:
                    {
                        io.AddInputCharacter((ImWchar)ki->ascii);
                    } break;
                }
            }
        }
    }

    state->original_mouse_input = input->mouse;
    bool wanted_capture_mouse = io.WantCaptureMouse;

    ImGui::NewFrame();

    if (io.WantCaptureKeyboard)
    {
        memory->debug_keyboard = true;
    } else
    {
        memory->debug_keyboard = false;
    }

    if (!io.WantCaptureMouse && wanted_capture_mouse)
    {
        input->mouse.buttons.left.repeat = true;
        state->original_mouse_input.buttons.left.repeat = true;
    }

    if (io.WantCaptureMouse)
    {
        input->mouse.buttons.left.repeat = !BUTTON_WENT_UP(input->mouse.buttons.left) || wanted_capture_mouse;
        input->mouse.buttons.left.ended_down = false;
        input->mouse.buttons.right.repeat = !BUTTON_WENT_UP(input->mouse.buttons.right) || wanted_capture_mouse;
        input->mouse.buttons.right.ended_down = false;
        input->mouse.buttons.middle.repeat = !BUTTON_WENT_UP(input->mouse.buttons.middle) || wanted_capture_mouse;
        input->mouse.buttons.middle.ended_down = false;
        input->mouse.vertical_wheel_delta = 0.0f;
    }

    io.MouseDrawCursor = io.WantCaptureMouse;

    if (state->flush_for_profiling) hglFlush();
    if (state->crash_on_gl_errors) hglErrorAssert();

    return true;
}

void render_draw_lists(ImDrawData* draw_data, renderer_state *state)
{
    TIMED_FUNCTION();

    hglEnable(GL_BLEND);
    hglBlendEquation(GL_FUNC_ADD);
    hglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    hglDisable(GL_CULL_FACE);
    hglDisable(GL_DEPTH_TEST);
    hglEnable(GL_SCISSOR_TEST);

    hglUseProgram(state->imgui_program.program);
    if (!state->rdl_vbo)
    {
        hglGenBuffers(1, &state->rdl_vbo);
        hglGenBuffers(1, &state->rdl_ibo);
        hglGenVertexArrays(1, &state->rdl_vao);

        hglBindVertexArray(state->rdl_vao);
        auto &program = state->imgui_program;
        hglEnableVertexAttribArray((GLuint)program.a_position_id);
        hglEnableVertexAttribArray((GLuint)program.a_uv_id);
        hglEnableVertexAttribArray((GLuint)program.a_color_id);
        hglBindBuffer(GL_ARRAY_BUFFER, state->rdl_vbo);
        hglBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(ImDrawVert), (GLvoid*)0, GL_STREAM_DRAW);
        hglVertexAttribPointer((GLuint)program.a_position_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, pos));
        hglVertexAttribPointer((GLuint)program.a_uv_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, uv));
        hglVertexAttribPointer((GLuint)program.a_color_id, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, col));
        hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->rdl_ibo);
        hglBufferData(GL_ELEMENT_ARRAY_BUFFER, 100000 * sizeof(ImDrawIdx), (GLvoid*)0, GL_STREAM_DRAW);
    }

    hglBindVertexArray(state->rdl_vao);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    auto &command_state = *state->command_state;
    hglBindBufferBase(
        GL_UNIFORM_BUFFER,
        0,
        command_state.texture_address_buffers[0].uniform_buffer.ubo);
    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.texture_address_buffers[0].uniform_buffer.ubo);
    if (command_state.texture_address_count != command_state.texture_address_last_buffer_count)
    {
        hglBufferSubData(
            GL_UNIFORM_BUFFER,
            0,
            sizeof(command_state.texture_addresses),
            command_state.texture_addresses);
        command_state.texture_address_last_buffer_count = command_state.texture_address_count;
    }

    static int32_t texcontainer_textures[harray_count(command_state.textures)] =
    {
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
    };

    hglUniform1iv(state->imgui_texcontainer, harray_count(texcontainer_textures), texcontainer_textures);
    if (state->crash_on_gl_errors) hglErrorAssert();
    /*
    for (uint32_t i = 0; i < harray_count(command_state.textures); ++i)
    {
        hglActiveTexture(GL_TEXTURE0 + texcontainer_textures[i]);
        hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[i]);
    }
    */

    // Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Setup viewport, orthographic projection matrix
    hglViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    const float ortho_projection[4][4] =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    hglUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, &ortho_projection[0][0]);
    hglUniformMatrix4fv(state->imgui_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    hglUniformMatrix4fv(state->imgui_program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    hglUniform1f(state->imgui_program.u_use_color_id, 1.0f);

    hglBindBuffer(GL_ARRAY_BUFFER, state->rdl_vbo);
    hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->rdl_ibo);

    hglUniform1i(state->imgui_program.u_texaddress_index_id, -1);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        hglBufferSubData(GL_ARRAY_BUFFER,
            0,
            (GLsizeiptr)(cmd_list->VtxBuffer.size() * sizeof(ImDrawVert)),
            (GLvoid*)&cmd_list->VtxBuffer.front());

        hglBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            0,
            (GLsizeiptr)(cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx)),
            (GLvoid*)&cmd_list->IdxBuffer.front());

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            int32_t imgui_texid = (int32_t)(intptr_t)pcmd->TextureId;
            hglUniform1i(state->imgui_program.u_texaddress_index_id, imgui_texid);
            if (imgui_texid >= 0)
            {
                hglActiveTexture(GL_TEXTURE0);
                TextureAddress texaddress = command_state.texture_addresses[imgui_texid];
                uint32_t container_id = texaddress.container_index;
                hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[container_id]);
            }
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                hglScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                hglDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }
    hglBindVertexArray(0);
}

static uint32_t generations_updated_this_frame = 0;

void
update_asset_descriptor_asset_id(renderer_state *state, asset_descriptor *descriptor)
{
    if (descriptor->generation_id != state->generation_id)
    {
        ++generations_updated_this_frame;
        int32_t result = -1;
        for (uint32_t i = 0; i < state->asset_count; ++i)
        {
            if (strcmp(descriptor->asset_name, state->assets[i].asset_name) == 0)
            {
                result = (int32_t)i;
                break;
            }
        }
        descriptor->asset_id = result;
        descriptor->generation_id = state->generation_id;
    }
}

void
draw_overdraw(renderer_state *state)
{
    hglDisable(GL_DEPTH_TEST);
    hglDisable(GL_BLEND);
    hglDepthFunc(GL_ALWAYS);
    hglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    m4 identity = m4identity();
    hglUseProgram(state->imgui_program.program);
    hglUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, (float *)&identity);
    hglUniformMatrix4fv(state->imgui_program.u_view_matrix_id, 1, GL_FALSE, (float *)&identity);
    hglUniformMatrix4fv(state->imgui_program.u_model_matrix_id, 1, GL_FALSE, (float *)&identity);
    hglUniform1f(state->imgui_program.u_use_color_id, 0.0f);

    uint32_t texture = state->white_texture;
    v2 st0 = {0, 0};
    v2 st1 = {1, 1};
    hglBindVertexArray(state->vao);

    struct
    {
        v2 pos;
        v2 uv;
        uint32_t col;
    } vertices[] = {
        {
            { -1, -1, },
            { st0.x, st0.y },
            0xffffffff,
        },
        {
            { 1, -1, },
            { st1.x, st0.y },
            0xffffffff,
        },
        {
            { 1, 1, },
            { st1.x, st1.y },
            0xffffffff,
        },
        {
            { -1, 1, },
            { st0.x, st1.y },
            0xffffffff,
        },
    };

    hglBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    hglVertexAttribPointer((GLuint)state->imgui_program.a_position_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, pos));
    hglVertexAttribPointer((GLuint)state->imgui_program.a_uv_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, uv));
    hglVertexAttribPointer((GLuint)state->imgui_program.a_color_id, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, col));
    hglBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    uint32_t indices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
    hglBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), (GLvoid*)indices, GL_STREAM_DRAW);

    hglBindTexture(GL_TEXTURE_2D, texture);

    hglColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);
    hglStencilFunc(GL_NOTEQUAL, 0, 0xf8 | 0x01);
    hglDrawElements(GL_TRIANGLES, (GLsizei)harray_count(indices), GL_UNSIGNED_INT, (void *)0);

    hglColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
    hglStencilFunc(GL_NOTEQUAL, 0, 0xf8 | 0x02);
    hglDrawElements(GL_TRIANGLES, (GLsizei)harray_count(indices), GL_UNSIGNED_INT, (void *)0);

    hglColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
    hglStencilFunc(GL_NOTEQUAL, 0, 0xf8 | 0x04);
    hglDrawElements(GL_TRIANGLES, (GLsizei)harray_count(indices), GL_UNSIGNED_INT, (void *)0);

    hglStencilFunc(GL_ALWAYS, 0, 0);
    hglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    hglBindVertexArray(0);

    hglEnable(GL_DEPTH_TEST);
    hglEnable(GL_BLEND);
    hglDepthFunc(GL_LESS);
}

bool
get_texaddress_index_from_asset_pack(
    renderer_state *state,
    int32_t asset_descriptor_id,
    int32_t *texaddress_index,
    v2 *st0,
    v2 *st1
)
{
    TIMED_FUNCTION();
    *st0 = {0, 0};
    *st1 = {1, 1};
    bool result = false;

    LoadedAsset *loaded_asset = load_asset(&state->asset_state, asset_descriptor_id);

    if (!loaded_asset)
    {
        return result;
    }

    if (!loaded_asset->texture.initialized)
    {
        loaded_asset->texture.initialized = true;
        hassert(loaded_asset->pack->piece_storage_type == AssetPieceStorageType::Files);

        const char *filename = loaded_asset->pack->filenames + loaded_asset->asset_piece->offset;

        uint32_t actual_file_size;
        bool loaded = _platform->load_asset(
            filename,
            loaded_asset->asset_piece->size,
            state->asset_scratch,
            &actual_file_size
        );

        if (!loaded) {
            return false;
        }

        TextureFormat format = {};
        switch(loaded_asset->asset->asset_sub_type.texture)
        {
            case TextureAssetSubType::Diffuse:
            {
                format = TextureFormat::srgba;
            } break;
            case TextureAssetSubType::Specular:
            case TextureAssetSubType::Normal:
            {
                format = TextureFormat::rgba8;
            } break;
            default:
            {
                hassert("!unreachable, hopefully");
            }
        }

        uint32_t _texaddress_index;
        int32_t x, y;
        loaded = load_texture_array_asset(
            state,
            state->asset_scratch,
            actual_file_size,
            format,
            &x,
            &y,
            &_texaddress_index);
        if (!loaded)
        {
            load_texture_asset_failed(filename);
            return false;
        }
        loaded_asset->texture.texaddress_index = (int32_t)_texaddress_index;
    }

    *texaddress_index = loaded_asset->texture.texaddress_index;
    return true;

}

void
get_texaddress_index_from_asset_descriptor(
    renderer_state *state,
    asset_descriptor *descriptors,
    int32_t asset_descriptor_id,
    // out
    int32_t *texaddress_index,
    v2 *st0,
    v2 *st1)
{
    TIMED_FUNCTION();
    *st0 = {0, 0};
    *st1 = {1, 1};

    if (asset_descriptor_id == -1)
    {
    }
    else if (get_texaddress_index_from_asset_pack(state, asset_descriptor_id, texaddress_index, st0, st1))
    {
    }
    else
    {
        asset_descriptor *descriptor = descriptors + asset_descriptor_id;
        switch (descriptor->source_type)
        {
            case asset_descriptor_source_type::name:
            {
                update_asset_descriptor_asset_id(state, descriptor);
                if (descriptor->asset_id >= 0)
                {
                    asset *descriptor_asset = state->assets + descriptor->asset_id;
                    *st0 = descriptor_asset->st0;
                    *st1 = descriptor_asset->st1;
                    int32_t asset_file_id = descriptor_asset->asset_file_id;
                    int32_t idx = lookup_asset_file_to_texaddress_index(state, asset_file_id);
                    if (idx < 0)
                    {
                        idx = add_asset_file_texaddress_index(state, asset_file_id, descriptor_asset->format);
                    }
                    if (idx >= 0)
                    {
                        *texaddress_index = idx;
                    }
                }
            } break;
            case asset_descriptor_source_type::framebuffer:
            {
                if (FramebufferInitialized(descriptor->framebuffer))
                {
                    *texaddress_index = (int32_t)descriptor->framebuffer->_texture;
                }
            } break;
            case asset_descriptor_source_type::framebuffer_depth:
            {
                if (FramebufferInitialized(descriptor->framebuffer))
                {
                    *texaddress_index = (int32_t)descriptor->framebuffer->_renderbuffer;
                }
            } break;
            case asset_descriptor_source_type::dynamic_mesh:
            {
                hassert(!"Not a texture asset type");
            } break;
            case asset_descriptor_source_type::dynamic_texture:
            {
                auto &command_state = *state->command_state;
                DynamicTextureDescriptor *d = descriptor->dynamic_texture_descriptor;
                if (!d->_loaded)
                {
                    d->_texture = new_texaddress(&command_state, d->size.x, d->size.y, TextureFormat::srgba);
                    d->reload = true;
                    d->_loaded = true;
                }
                if (d->reload)
                {
                    d->reload = false;
                    auto &ta = command_state.texture_addresses[d->_texture];
                    GLuint lod = 0;

                    auto &tex = command_state.textures[ta.container_index];

                    hglBindTexture(GL_TEXTURE_2D_ARRAY, tex);
                    hglTexSubImage3D(
                        GL_TEXTURE_2D_ARRAY,
                        lod,
                        0,
                        0,
                        ta.layer,
                        d->size.x,
                        d->size.y,
                        1,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        d->data
                    );
                    hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    hglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
                }
                *texaddress_index = (int32_t)d->_texture;
            } break;
        }
    }
}

LoadedAsset *
load_mesh_from_asset_pack(
    renderer_state *state,
    uint32_t asset_descriptor_id
)
{
    LoadedAsset *loaded_asset = load_asset(&state->asset_state, asset_descriptor_id);

    if (!loaded_asset)
    {
        return loaded_asset;
    }

    if (!loaded_asset->mesh.initialized)
    {
        hassert(loaded_asset->pack->piece_storage_type == AssetPieceStorageType::Files);
        const char *filename = loaded_asset->pack->filenames + loaded_asset->asset_piece->offset;

        loaded_asset->mesh.mesh = PushStruct("mesh_from_asset_pack", &state->arena, Mesh);

        bool loaded = load_mesh_asset(state, filename,
            state->asset_scratch,
            state->asset_scratch_size,
            loaded_asset->mesh.mesh
        );
        if (!loaded)
        {
            load_mesh_asset_failed(filename);
            return 0;
        }
        loaded_asset->mesh.initialized = 1;
    }

    return loaded_asset;
}

Mesh *
get_mesh_from_asset_descriptor(
    renderer_state *state,
    asset_descriptor *descriptors,
    int32_t asset_descriptor_id)
{
    TIMED_FUNCTION();
    Mesh *mesh = 0;
    bool result = false;
    if (asset_descriptor_id == -1)
    {
        return mesh;
    }

    LoadedAsset *loaded_asset = load_mesh_from_asset_pack(state, asset_descriptor_id);
    int32_t *load_state = 0;
    V3Bones **v3bones_target = 0;
    if (loaded_asset)
    {
        mesh = loaded_asset->mesh.mesh;
        load_state = &loaded_asset->mesh.load_state;
        v3bones_target = &loaded_asset->mesh.v3bones;
    }
    else
    {
        asset_descriptor *descriptor = descriptors + asset_descriptor_id;
        switch (descriptor->source_type)
        {
            case asset_descriptor_source_type::framebuffer:
            case asset_descriptor_source_type::framebuffer_depth:
            case asset_descriptor_source_type::dynamic_texture:
            {
                hassert(!"Not a mesh asset");

            } break;
            case asset_descriptor_source_type::name:
            {
                update_asset_descriptor_asset_id(state, descriptor);
                if (descriptor->asset_id >= 0)
                {
                    asset *descriptor_asset = state->assets + descriptor->asset_id;
                    int32_t asset_file_id = descriptor_asset->asset_file_id;

                    int32_t mesh_id = lookup_asset_file_to_mesh(state, asset_file_id);
                    if (mesh_id < 0)
                    {
                        mesh_id = add_asset_file_mesh(state, asset_file_id);
                    }
                    hassert(mesh_id >= 0);
                    if (mesh_id >= 0)
                    {
                        mesh = state->meshes + mesh_id;
                        result = true;
                    }
                }
            } break;
            case asset_descriptor_source_type::dynamic_mesh:
            {
                mesh = (Mesh *)descriptor->ptr;
                hassert(mesh->dynamic);
                result = true;
            } break;
        }
        load_state = &descriptor->load_state;
        v3bones_target = &descriptor->v3bones;
    }

    if (!mesh)
    {
        return mesh;
    }

    if (result && *load_state != 2)
    {
        *load_state = 2;
        switch (mesh->mesh_format)
        {
            case MeshFormat::v3_bones:
            {
                auto &v3bones = *mesh->v3.v3bones;
                *v3bones_target = mesh->v3.v3bones;

                int32_t first_bone = 0;
                for (uint32_t i = 0; i < mesh->num_bones; ++i)
                {
                    if (v3bones.bone_parents[i] == -1)
                    {
                        first_bone = (int32_t)i;
                        break;
                    }
                }

                int32_t stack_location = 0;

                int32_t stack[100];
                stack[0] = first_bone;
                struct
                {
                    int32_t bone_id;
                    m4 transform;
                } parent_list[100];
                int32_t parent_list_location = 0;

                while (stack_location >= 0)
                {
                    int32_t bone = stack[stack_location];

                    int32_t parent_bone = v3bones.bone_parents[bone];
                    --stack_location;
                    while (parent_list[parent_list_location].bone_id != parent_bone && parent_list_location >= 0)
                    {
                        --parent_list_location;
                    }
                    ++parent_list_location;

                    m4 parent_matrix = m4identity();
                    if (parent_list_location)
                    {
                        parent_matrix = parent_list[parent_list_location - 1].transform;
                    }

                    m4 scale = m4scale(v3bones.default_transforms[bone].scale);
                    m4 translate = m4translate(v3bones.default_transforms[bone].translate);
                    m4 rotate = m4rotation(v3bones.default_transforms[bone].q);
                    m4 local_matrix = m4mul(translate,m4mul(rotate, scale));


                    m4 global_transform = m4mul(parent_matrix, local_matrix);

                    parent_list[parent_list_location] = {
                        bone,
                        global_transform,
                    };

                    v3bones.default_bones[bone] = m4mul(global_transform, v3bones.bone_offsets[bone]);

                    for (uint32_t i = 0; i < mesh->num_bones; ++i)
                    {
                        if (v3bones.bone_parents[i] == bone)
                        {
                            stack_location++;
                            stack[stack_location] = (int32_t)i;
                        }
                    }
                }
            } break;

            case MeshFormat::v3_boneless:
            {

            } break;

            case MeshFormat::first:
            {
                hassert(!"deprecated");
            } break;

            case MeshFormat::just_vertices:
            {
                // Nothing to do.
            } break;
        }
    }
    return mesh;
}

void
draw_quads(renderer_state *state, m4 *matrices, asset_descriptor *descriptors, render_entry_type_QUADS *quads)
{
    //hglDisable(GL_BLEND);
    if (state->crash_on_gl_errors) hglErrorAssert();
    m4 projection = matrices[quads->matrix_id];

    v2 st0 = {};
    v2 st1 = {};
    int32_t texture = 0;
    get_texaddress_index_from_asset_descriptor(
        state,
        descriptors,
        quads->asset_descriptor_id,
        &texture,
        &st0,
        &st1);

    if (state->crash_on_gl_errors) hglErrorAssert();
    hglUseProgram(state->imgui_program.program);
    hglUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, (float *)&projection);
    hglUniformMatrix4fv(state->imgui_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    hglUniformMatrix4fv(state->imgui_program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    hglUniform1f(state->imgui_program.u_use_color_id, 1.0f);
    if (state->crash_on_gl_errors) hglErrorAssert();

    hglUniform1i(state->imgui_program.u_texaddress_index_id, texture);
    hglBindVertexArray(state->vao);

    hassert(harray_count(state->vertices_scratch) >= quads->entry_count * 4);

    uint32_t scratch_pos = 0;
    for (uint32_t i = 0; i < quads->entry_count; ++i)
    {
        auto &entry = quads->entries[i];
        uint32_t col = 0x00000000;
        col |= (uint32_t)(entry.color.w * 255.0f) << 24;
        col |= (uint32_t)(entry.color.z * 255.0f) << 16;
        col |= (uint32_t)(entry.color.y * 255.0f) << 8;
        col |= (uint32_t)(entry.color.x * 255.0f) << 0;
        state->vertices_scratch[scratch_pos++] = {
            { entry.position.x, entry.position.y, },
            { st0.x, st0.y },
            col,
        };
        state->vertices_scratch[scratch_pos++] = {
            { entry.position.x + entry.size.x, entry.position.y, },
            { st1.x, st0.y },
            col,
        };

        state->vertices_scratch[scratch_pos++] = {
            { entry.position.x + entry.size.x, entry.position.y + entry.size.y, },
            { st1.x, st1.y },
            col,
        };

        state->vertices_scratch[scratch_pos++] = {
            { entry.position.x, entry.position.y + entry.size.y, },
            { st0.x, st1.y },
            col,
        };
    }

    if (state->crash_on_gl_errors) hglErrorAssert();
    hglBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    hglVertexAttribPointer((GLuint)state->imgui_program.a_position_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, pos));
    hglVertexAttribPointer((GLuint)state->imgui_program.a_uv_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, uv));
    hglVertexAttribPointer((GLuint)state->imgui_program.a_color_id, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, col));
    hglBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(state->vertices_scratch[0])) * 4 * quads->entry_count, state->vertices_scratch, GL_STREAM_DRAW);
    if (state->crash_on_gl_errors) hglErrorAssert();
    hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->QUADS_ibo);
    if (state->crash_on_gl_errors) hglErrorAssert();

    auto &command_state = *state->command_state;
    hglBindBufferBase(
        GL_UNIFORM_BUFFER,
        0,
        command_state.texture_address_buffers[0].uniform_buffer.ubo);
    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.texture_address_buffers[0].uniform_buffer.ubo);

    if (command_state.texture_address_count != command_state.texture_address_last_buffer_count)
    {
        hglBufferSubData(
            GL_UNIFORM_BUFFER,
            0,
            sizeof(command_state.texture_addresses),
            command_state.texture_addresses);
        command_state.texture_address_last_buffer_count = command_state.texture_address_count;
    }
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);
    static int32_t texcontainer_textures[harray_count(command_state.textures)] =
    {
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
    };

    hglUniform1iv(state->imgui_texcontainer, harray_count(texcontainer_textures), texcontainer_textures);
    if (state->crash_on_gl_errors) hglErrorAssert();
    /*
    for (uint32_t i = 0; i < harray_count(command_state.textures); ++i)
    {
        hglActiveTexture(GL_TEXTURE0 + texcontainer_textures[i]);
        hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[i]);
    }
    */
    hglActiveTexture(GL_TEXTURE0);
    TextureAddress texaddress = command_state.texture_addresses[texture];
    uint32_t container_id = texaddress.container_index;
    hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[container_id]);
    if (state->crash_on_gl_errors) hglErrorAssert();
    hglActiveTexture(GL_TEXTURE0);
    if (state->crash_on_gl_errors) hglErrorAssert();
    hglDrawElements(GL_TRIANGLES, (GLsizei)quads->entry_count * 6, GL_UNSIGNED_INT, (void *)0);
    if (state->crash_on_gl_errors) hglErrorAssert();

    hglUniform1i(state->imgui_program.u_texaddress_index_id, -1);
    if (state->crash_on_gl_errors) hglErrorAssert();

    hglBindVertexArray(0);
    if (state->crash_on_gl_errors) hglErrorAssert();
}

v3
calculate_camera_position(m4 view)
{
    m4 view_transposed = m4transposed(view);

    v3 n1 = {
        view_transposed.cols[0].x,
        view_transposed.cols[0].y,
        view_transposed.cols[0].z,
    };
    v3 n2 = {
        view_transposed.cols[1].x,
        view_transposed.cols[1].y,
        view_transposed.cols[1].z,
    };
    v3 n3 = {
        view_transposed.cols[2].x,
        view_transposed.cols[2].y,
        view_transposed.cols[2].z,
    };

    float d1 = view_transposed.cols[0].w;
    float d2 = view_transposed.cols[1].w;
    float d3 = view_transposed.cols[2].w;

    v3 n1n2 = v3cross(n1, n2);
    v3 n2n3 = v3cross(n2, n3);
    v3 n3n1 = v3cross(n3, n1);

    v3 top = v3add(
        v3mul(n2n3, d1),
        v3add(
            v3mul(n3n1, d2),
            v3mul(n1n2, d3)
        )
    );
    float denom = -1.0f / v3dot(n1, n2n3);

    return v3mul(top, denom);
}

int32_t
choose_buffer_for_mesh(renderer_state *state, Mesh *mesh)
{
    auto &command_state = *state->command_state;
    BufferKey k = {mesh->vertexformat};
    int32_t buffer = -1;
    for (uint32_t i = 0; i < command_state.buffers_used; ++i)
    {
        if (command_state.buffer_keys[i] == k)
        {
            buffer = (int32_t)i;
        }
    }
    if (buffer < 0)
    {
        if (command_state.buffers_used < harray_count(command_state.buffer_keys))
        {
            buffer = (int32_t)command_state.buffers_used++;
            command_state.buffer_keys[buffer] = k;
        }
    }
    return buffer;
}

bool
load_mesh_to_buffers(
    renderer_state *state,
    Mesh *mesh)
{
    auto &command_state = *state->command_state;
    if (!mesh->loaded)
    {
        int32_t buffer_id = choose_buffer_for_mesh(state, mesh);
        if (buffer_id < 0)
        {
            return false;
        }
        auto &vertex_buffer = command_state.vertex_buffers[buffer_id];
        auto &index_buffer = command_state.index_buffers[buffer_id];

        mesh->v3.vertex_buffer = (uint32_t)buffer_id;
        mesh->v3.vertex_base = vertex_buffer.vertex_count;
        mesh->v3.index_buffer = (uint32_t)buffer_id;
        mesh->v3.index_offset = index_buffer.index_count;

        if (!vertex_buffer.max_vertices)
        {
            hglGenBuffers(1, &vertex_buffer.vbo);
            hglGenBuffers(1, &index_buffer.ibo);
            switch (mesh->vertexformat)
            {
                case VertexFormat::v1:
                {
                    vertex_buffer.vertex_size = sizeof(vertexformat_1);
                } break;
                case VertexFormat::v2:
                {
                    vertex_buffer.vertex_size = sizeof(vertexformat_2);
                } break;
                case VertexFormat::v3:
                {
                    vertex_buffer.vertex_size = sizeof(vertexformat_3);
                } break;
                case VertexFormat::just_vertices:
                {
                    vertex_buffer.vertex_size = sizeof(vertexformat_just_vertices);
                } break;
                case VertexFormat::v0:
                default:
                {
                    hassert(!"Unhandled");
                } break;
            }
            vertex_buffer.max_vertices = 10000000;
            hglBindBuffer(GL_ARRAY_BUFFER,
                vertex_buffer.vbo);
            hglBufferData(GL_ARRAY_BUFFER,
                vertex_buffer.vertex_size * vertex_buffer.max_vertices,
                (void *)0,
                GL_DYNAMIC_DRAW);
        }

        if (!index_buffer.max_indices)
        {
            hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                index_buffer.ibo);
            index_buffer.max_indices = 10000000;
            hglBufferData(GL_ELEMENT_ARRAY_BUFFER,
                3 * 4 * index_buffer.max_indices,
                (void *)0,
                GL_DYNAMIC_DRAW);
        }
        vertex_buffer.vertex_count += mesh->v3.vertex_count;
        index_buffer.index_count += mesh->num_indices;

        mesh->loaded = true;
        mesh->reload = true;
    }

    if (mesh->reload)
    {
        uint32_t vertex_buffer_id = mesh->v3.vertex_buffer;
        uint32_t vertex_base = mesh->v3.vertex_base;
        uint32_t index_buffer_id = mesh->v3.index_buffer;
        uint32_t index_offset = mesh->v3.index_offset;

        auto &vertex_buffer = command_state.vertex_buffers[vertex_buffer_id];
        auto &index_buffer = command_state.index_buffers[index_buffer_id];

        hglBindBuffer(GL_ARRAY_BUFFER,
            vertex_buffer.vbo);
        hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
            index_buffer.ibo);

        if (!mesh->dynamic_max_vertices)
        {
            hassert(vertex_buffer.vertex_size * mesh->v3.vertex_count == mesh->vertices.size);
        }
        else
        {
            hassert(vertex_buffer.vertex_size * mesh->dynamic_max_vertices == mesh->vertices.size);
        }

        hglBufferSubData(GL_ARRAY_BUFFER,
            vertex_buffer.vertex_size * vertex_base,
            vertex_buffer.vertex_size * mesh->v3.vertex_count,
            mesh->vertices.data);

        if (!mesh->dynamic_max_indices)
        {
            hassert(4 * mesh->num_indices == mesh->indices.size);
        }
        else
        {
            hassert(4 * mesh->dynamic_max_indices == mesh->indices.size);
        }

        hglBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
            4 * index_offset,
            4 * mesh->num_indices,
            mesh->indices.data);
        mesh->reload = false;
    }


    return true;
}

int32_t
command_list_for_key_draw_data(
    CommandState *command_state,
    CommandKey *key,
    DrawElementsIndirectCommand *cmd,
    RenderEntryListConfig *config,
    DrawData *draw_data,
    uint32_t num_bones,
    m4 *bones
)
{
    uint32_t num_containers = 0;
    uint32_t containers[10];

    int32_t indices[] = {
        draw_data->texture_texaddress_index,
        draw_data->shadowmap_texaddress_index,
        draw_data->shadowmap_color_texaddress_index,
        draw_data->normal_texaddress_index,
        draw_data->specular_texaddress_index,
        config->_reflection_texaddress_index,
        config->_refraction_texaddress_index,
        config->_refraction_depth_texaddress_index,
        config->_dudv_map_texaddress_index,
        config->_normal_map_texaddress_index,
    };

    for (uint32_t i = 0; i < harray_count(indices); ++i)
    {
        int32_t &index = indices[i];
        if (index < 0)
        {
            continue;
        }
        uint32_t &container_index = command_state->texture_addresses[index].container_index;
        bool found_container = false;
        for (uint32_t j = 0; j < num_containers; ++j)
        {
            if (container_index == containers[j])
            {
                found_container = true;
                break;
            }
        }
        if (!found_container)
        {
            containers[num_containers] = container_index;
            ++num_containers;
        }
    }

    enum struct
    _SearchResult
    {
        exhausted,
        found,
        notfound,
    };
    _SearchResult search_result = _SearchResult::notfound;
    int32_t key_index = -1;
    auto &command_lists = command_state->lists;
    for (uint32_t i = 0; i < command_lists.num_command_lists; ++i)
    {
        key_index = (int32_t)i;
        if (command_lists.keys[i] == *key)
        {
            auto &command_list = command_lists.command_lists[i];
            if (command_list.num_commands >= harray_count(command_list.commands))
            {
                continue;
            }
            uint32_t num_containers_needed = 0;
            uint32_t containers_needed[10];
            for (uint32_t j = 0; j < num_containers; ++j)
            {
                bool found = false;
                uint32_t &container = containers[j];
                for (uint32_t k = 0; k < command_list.num_texcontainer_indices; ++k)
                {
                    TexContainerSamplerMapping &map = command_list.texcontainer_sampler_mapping[container];
                    if (map.generation == command_list.generation)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    containers_needed[num_containers_needed] = container;
                    ++num_containers_needed;
                }
            }
            if (num_containers_needed)
            {
                if (num_containers_needed + command_list.num_texcontainer_indices < harray_count(command_list.texcontainer_index))
                {
                    for (uint32_t j = 0; j < num_containers_needed; ++j)
                    {
                        command_list.texcontainer_index[command_list.num_texcontainer_indices].container_index = containers_needed[j];
                        command_list.texcontainer_sampler_mapping[containers_needed[j]] =
                        {
                            command_list.generation,
                            command_list.num_texcontainer_indices,

                        };
                        ++command_list.num_texcontainer_indices;
                    }
                }
                else
                {
                    continue;
                }
            }
            search_result = _SearchResult::found;
            break;
        }
    }
    if (search_result != _SearchResult::found)
    {
        if (key_index < (int32_t)harray_count(command_lists.keys) - 1)
        {
            ++key_index;
            auto &command_list = command_lists.command_lists[command_lists.num_command_lists];
            ++command_lists.num_command_lists;
            command_lists.keys[key_index] = *key;
            search_result = _SearchResult::found;

            for (uint32_t j = 0; j < num_containers; ++j)
            {
                command_list.texcontainer_index[command_list.num_texcontainer_indices].container_index = containers[j];
                command_list.texcontainer_sampler_mapping[containers[j]] =
                {
                    command_list.generation,
                    command_list.num_texcontainer_indices,

                };
                ++command_list.num_texcontainer_indices;
            }
        }
        else
        {
            search_result = _SearchResult::exhausted;
            hassert(!"Not enough space for new command list");
        }
    }

    if (key_index < 0)
    {
        return key_index;
    }

    auto &command_list = command_lists.command_lists[key_index];

    if (num_bones)
    {
        draw_data->bone_offset = (int32_t)command_list.num_bones;

        auto bone_data = command_list.bone_data_list.data + command_list.num_bones;
        memcpy(
            (void *)bone_data,
            bones,
            sizeof(m4) * num_bones);

        command_list.num_bones += num_bones;
    }

    command_list.draw_data_list.data[command_list.num_commands] = *draw_data;
    command_list.commands[command_list.num_commands] = *cmd;

    ++command_list.num_commands;

    return key_index;
}


void
draw_mesh_from_asset_v3_bones(
    renderer_state *state,
    RenderEntryListConfig *config,
    m4 *matrices,
    asset_descriptor *descriptors,
    LightDescriptor *light_descriptors,
    ArmatureDescriptor *armature_descriptors,
    Material *materials,
    render_entry_type_mesh_from_asset *mesh_from_asset,
    Mesh *mesh
)
{
    TIMED_FUNCTION();
    if (state->crash_on_gl_errors) hglErrorAssert();

    bool debug = false;
    if (mesh_from_asset->mesh_asset_descriptor_id == 997)
    {
        debug = true;
    }

    load_mesh_to_buffers(state, mesh);

    DrawElementsIndirectCommand cmd = {};
    cmd.count = mesh->num_indices;
    cmd.primCount = 1; // number of instances
    cmd.firstIndex = mesh->v3.index_offset;
    cmd.baseVertex = (int32_t)mesh->v3.vertex_base;
    cmd.baseInstance = 0;

    //key.shadowmap_color_asset_descriptor_id = light.shadowmap_color_asset_descriptor_id;
    v2 st0 = {};
    v2 st1 = {};
    int32_t texture_texaddress_index = -1;
    int32_t normal_texaddress_index = -1;
    int32_t specular_texaddress_index = -1;
    if (debug)
    {
        hassert(debug);
    }
    struct
    {
        int32_t asset_descriptor;
        int32_t *texaddress_index;
    } load[] =
    {
        { config->reflection_asset_descriptor, &config->_reflection_texaddress_index },
        { config->refraction_asset_descriptor, &config->_refraction_texaddress_index },
        { config->refraction_depth_asset_descriptor, &config->_refraction_depth_texaddress_index },
        { config->dudv_map_asset_descriptor, &config->_dudv_map_texaddress_index },
        { config->normal_map_asset_descriptor, &config->_normal_map_texaddress_index },
    };
    for (uint32_t i = 0; i < harray_count(load); ++i)
    {
        get_texaddress_index_from_asset_descriptor(
            state,
            descriptors,
            load[i].asset_descriptor,
            load[i].texaddress_index,
            &st0,
            &st1);
    }
    switch (mesh_from_asset->flags.use_materials)
    {
        case 0:
        {
            get_texaddress_index_from_asset_descriptor(
                state,
                descriptors,
                mesh_from_asset->texture_asset_descriptor_id,
                &texture_texaddress_index,
                &st0,
                &st1);
        } break;
        case 1:
        {
            get_texaddress_index_from_asset_descriptor(
                state,
                descriptors,
                materials[mesh_from_asset->texture_asset_descriptor_id].diffuse_id,
                &texture_texaddress_index,
                &st0,
                &st1);
            get_texaddress_index_from_asset_descriptor(
                state,
                descriptors,
                materials[mesh_from_asset->texture_asset_descriptor_id].normal_id,
                &normal_texaddress_index,
                &st0,
                &st1);
            get_texaddress_index_from_asset_descriptor(
                state,
                descriptors,
                materials[mesh_from_asset->texture_asset_descriptor_id].specular_id,
                &specular_texaddress_index,
                &st0,
                &st1);
        } break;
    }
    int32_t shadowmap_texaddress_index = -1;
    int32_t shadowmap_color_texaddress_index = -1;
    int32_t light_index = 0;
    if (mesh_from_asset->flags.attach_shadowmap)
    {
        auto &light = light_descriptors[0];
        light_index = 0;
        shadowmap_texaddress_index = -1;
        shadowmap_color_texaddress_index = light.shadowmap_color_texaddress_asset_descriptor_id;
        /*
        get_texaddress_index_from_asset_descriptor(
            state,
            descriptors,
            light.shadowmap_asset_descriptor_id,
            &shadowmap_texaddress_index,
            &st0,
            &st1);
            */
        get_texaddress_index_from_asset_descriptor(
            state,
            descriptors,
            light.shadowmap_color_texaddress_asset_descriptor_id,
            &shadowmap_color_texaddress_index,
            &st0,
            &st1);
    }
    else
    {
        shadowmap_texaddress_index = -1;
        shadowmap_color_texaddress_index = -1;
    }

    m4 &projection = matrices[mesh_from_asset->projection_matrix_id];
    m4 view;
    if (mesh_from_asset->view_matrix_id >= 0)
    {
        view = matrices[mesh_from_asset->view_matrix_id];
    }
    else
    {
        view = m4identity();
    }
    m4 &model = mesh_from_asset->model_matrix;

    ArmatureDescriptor *armature = 0;
    if (mesh_from_asset->armature_descriptor_id >= 0)
    {
        armature = armature_descriptors + mesh_from_asset->armature_descriptor_id;
    }
    m4 *bones = mesh->v3.v3bones->default_bones;
    if (armature)
    {
        bones = armature->bones;
    }

    DrawData draw_data = {};
    draw_data.texture_texaddress_index = texture_texaddress_index;
    draw_data.normal_texaddress_index = normal_texaddress_index;
    draw_data.specular_texaddress_index = specular_texaddress_index;
    draw_data.shadowmap_texaddress_index = shadowmap_texaddress_index;
    draw_data.shadowmap_color_texaddress_index = shadowmap_color_texaddress_index;
    draw_data.projection = projection;
    draw_data.view = view;
    draw_data.model = model;
    draw_data.light_index = light_index;
    v3 camera_position = calculate_camera_position(view);
    draw_data.camera_position = camera_position;
    draw_data.object_identifier = mesh_from_asset->object_identifier;

    CommandKey key = {};
    key.shader_type = mesh_from_asset->shader_type;
    key.vertexformat = mesh->vertexformat;
    key.vertex_buffer = mesh->v3.vertex_buffer;
    key.index_buffer = mesh->v3.index_buffer;
    key.count = 0;
    key.primitive_type = GL_TRIANGLES;

    command_list_for_key_draw_data(
        state->command_state,
        &key,
        &cmd,
        config,
        &draw_data,
        mesh->num_bones,
        bones
    );
    if (state->crash_on_gl_errors) hglErrorAssert();
    return;
}

void
draw_mesh_from_asset_v3_boneless(
    renderer_state *state,
    RenderEntryListConfig *config,
    m4 *matrices,
    asset_descriptor *descriptors,
    LightDescriptor *light_descriptors,
    ArmatureDescriptor *armature_descriptors,
    Material *materials,
    render_entry_type_mesh_from_asset *mesh_from_asset,
    Mesh *mesh
)
{
    TIMED_FUNCTION();

    bool debug = false;
    if (mesh_from_asset->mesh_asset_descriptor_id == 97)
    {
        debug = true;
    }

    bool loaded = load_mesh_to_buffers(state, mesh);
    if (!loaded)
    {
        return;
    }

    DrawElementsIndirectCommand cmd = {};
    cmd.count = mesh->num_indices;
    cmd.primCount = 1; // number of instances
    cmd.firstIndex = mesh->v3.index_offset;
    cmd.baseVertex = (int32_t)mesh->v3.vertex_base;
    cmd.baseInstance = 0;

    //key.shadowmap_color_asset_descriptor_id = light.shadowmap_color_asset_descriptor_id;
    v2 st0 = {};
    v2 st1 = {};
    int32_t texture_texaddress_index = -1;
    int32_t normal_texaddress_index = -1;
    int32_t specular_texaddress_index = -1;
    if (debug)
    {
        hassert(debug);
    }
    struct
    {
        int32_t asset_descriptor;
        int32_t *texaddress_index;
    } load[] =
    {
        { config->reflection_asset_descriptor, &config->_reflection_texaddress_index },
        { config->refraction_asset_descriptor, &config->_refraction_texaddress_index },
        { config->refraction_depth_asset_descriptor, &config->_refraction_depth_texaddress_index },
        { config->dudv_map_asset_descriptor, &config->_dudv_map_texaddress_index },
        { config->normal_map_asset_descriptor, &config->_normal_map_texaddress_index },
    };
    for (uint32_t i = 0; i < harray_count(load); ++i)
    {
        get_texaddress_index_from_asset_descriptor(
            state,
            descriptors,
            load[i].asset_descriptor,
            load[i].texaddress_index,
            &st0,
            &st1);
    }
    switch (mesh_from_asset->flags.use_materials)
    {
        case 0:
        {
            get_texaddress_index_from_asset_descriptor(
                state,
                descriptors,
                mesh_from_asset->texture_asset_descriptor_id,
                &texture_texaddress_index,
                &st0,
                &st1);
        } break;
        case 1:
        {
            get_texaddress_index_from_asset_descriptor(
                state,
                descriptors,
                materials[mesh_from_asset->texture_asset_descriptor_id].diffuse_id,
                &texture_texaddress_index,
                &st0,
                &st1);
            get_texaddress_index_from_asset_descriptor(
                state,
                descriptors,
                materials[mesh_from_asset->texture_asset_descriptor_id].normal_id,
                &normal_texaddress_index,
                &st0,
                &st1);
            get_texaddress_index_from_asset_descriptor(
                state,
                descriptors,
                materials[mesh_from_asset->texture_asset_descriptor_id].specular_id,
                &specular_texaddress_index,
                &st0,
                &st1);

        } break;
    }
    int32_t shadowmap_texaddress_index = -1;
    int32_t shadowmap_color_texaddress_index = -1;
    int32_t light_index = 0;
    if (mesh_from_asset->flags.attach_shadowmap)
    {
        auto &light = light_descriptors[0];
        light_index = 0;
        shadowmap_texaddress_index = -1;
        shadowmap_color_texaddress_index = light.shadowmap_color_texaddress_asset_descriptor_id;
        /*
        get_texaddress_index_from_asset_descriptor(
            state,
            descriptors,
            light.shadowmap_asset_descriptor_id,
            &shadowmap_texaddress_index,
            &st0,
            &st1);
            */
        get_texaddress_index_from_asset_descriptor(
            state,
            descriptors,
            light.shadowmap_color_texaddress_asset_descriptor_id,
            &shadowmap_color_texaddress_index,
            &st0,
            &st1);
    }
    else
    {
        shadowmap_texaddress_index = -1;
        shadowmap_color_texaddress_index = -1;
    }

    m4 &projection = matrices[mesh_from_asset->projection_matrix_id];
    m4 view;
    if (mesh_from_asset->view_matrix_id >= 0)
    {
        view = matrices[mesh_from_asset->view_matrix_id];
    }
    else
    {
        view = m4identity();
    }
    m4 &model = mesh_from_asset->model_matrix;

    DrawData draw_data = {};
    draw_data.texture_texaddress_index = texture_texaddress_index;
    draw_data.normal_texaddress_index = normal_texaddress_index;
    draw_data.specular_texaddress_index = specular_texaddress_index;
    draw_data.shadowmap_texaddress_index = shadowmap_texaddress_index;
    draw_data.shadowmap_color_texaddress_index = shadowmap_color_texaddress_index;
    draw_data.projection = projection;
    draw_data.view = view;
    draw_data.model = model;
    draw_data.light_index = light_index;
    v3 camera_position = calculate_camera_position(view);
    draw_data.camera_position = camera_position;
    draw_data.bone_offset = -1;
    draw_data.object_identifier = mesh_from_asset->object_identifier;


    CommandKey key = {};
    key.shader_type = mesh_from_asset->shader_type;
    key.vertexformat = mesh->vertexformat;
    key.vertex_buffer = mesh->v3.vertex_buffer;
    key.index_buffer = mesh->v3.index_buffer;
    key.count = 0;
    key.primitive_type = GL_TRIANGLES;

    command_list_for_key_draw_data(
        state->command_state,
        &key,
        &cmd,
        config,
        &draw_data,
        0,
        0
    );
    return;
}

void
draw_mesh_from_asset_just_vertices(
    renderer_state *state,
    RenderEntryListConfig *config,
    m4 *matrices,
    asset_descriptor *descriptors,
    LightDescriptor *light_descriptors,
    ArmatureDescriptor *armature_descriptors,
    Material *materials,
    render_entry_type_mesh_from_asset *mesh_from_asset,
    Mesh *mesh
)
{
    TIMED_FUNCTION();

    bool loaded = load_mesh_to_buffers(state, mesh);
    if (!loaded)
    {
        return;
    }

    DrawElementsIndirectCommand cmd = {};
    cmd.count = mesh->num_indices;
    cmd.primCount = 1; // number of instances
    cmd.firstIndex = mesh->v3.index_offset;
    cmd.baseVertex = (int32_t)mesh->v3.vertex_base;
    cmd.baseInstance = 0;

    m4 &projection = matrices[mesh_from_asset->projection_matrix_id];
    m4 view;
    if (mesh_from_asset->view_matrix_id >= 0)
    {
        view = matrices[mesh_from_asset->view_matrix_id];
    }
    else
    {
        view = m4identity();
    }
    m4 &model = mesh_from_asset->model_matrix;

    DrawData draw_data = {};
    draw_data.texture_texaddress_index = -1;
    draw_data.normal_texaddress_index = -1;
    draw_data.specular_texaddress_index = -1;
    draw_data.shadowmap_texaddress_index = -1;
    draw_data.shadowmap_color_texaddress_index = -1;
    draw_data.projection = projection;
    draw_data.view = view;
    draw_data.model = model;
    draw_data.light_index = 0;
    v3 camera_position = calculate_camera_position(view);
    draw_data.camera_position = camera_position;
    draw_data.bone_offset = -1;
    draw_data.object_identifier = mesh_from_asset->object_identifier;

    CommandKey key = {};
    key.shader_type = mesh_from_asset->shader_type;
    key.vertexformat = mesh->vertexformat;
    key.vertex_buffer = mesh->v3.vertex_buffer;
    key.index_buffer = mesh->v3.index_buffer;
    key.count = 0;
    key.primitive_type = GL_LINES;

    command_list_for_key_draw_data(
        state->command_state,
        &key,
        &cmd,
        config,
        &draw_data,
        0,
        0
    );
    return;
}

void
draw_mesh_from_asset(
    renderer_state *state,
    RenderEntryListConfig *config,
    m4 *matrices,
    asset_descriptor *descriptors,
    LightDescriptor *light_descriptors,
    ArmatureDescriptor *armature_descriptors,
    Material *materials,
    render_entry_type_mesh_from_asset *mesh_from_asset
)
{
    Mesh *mesh = get_mesh_from_asset_descriptor(state, descriptors, mesh_from_asset->mesh_asset_descriptor_id);

    hassert(mesh->primitive_type != PrimitiveType::unset);
    switch(mesh->mesh_format)
    {
        case MeshFormat::v3_bones:
        {
            return draw_mesh_from_asset_v3_bones(
                state,
                config,
                matrices,
                descriptors,
                light_descriptors,
                armature_descriptors,
                materials,
                mesh_from_asset,
                mesh);
        } break;
        case MeshFormat::v3_boneless:
        {
            return draw_mesh_from_asset_v3_boneless(
                state,
                config,
                matrices,
                descriptors,
                light_descriptors,
                armature_descriptors,
                materials,
                mesh_from_asset,
                mesh);
        } break;
        case MeshFormat::just_vertices:
        {
            return draw_mesh_from_asset_just_vertices(
                state,
                config,
                matrices,
                descriptors,
                light_descriptors,
                armature_descriptors,
                materials,
                mesh_from_asset,
                mesh);
        }
        case MeshFormat::first:
        {
            hassert(!"Unreachable");
        } break;
    }
    hassert(!"Unreachable");
}

void
apply_filter(renderer_state *state, asset_descriptor *descriptors, render_entry_type_apply_filter *filter)
{
    hglDisable(GL_CULL_FACE);
    hglDisable(GL_DEPTH_TEST);
    auto &command_state = *state->command_state;
    int32_t a_position_id = 0;
    int32_t a_texcoord_id = 0;

    v2 st0 = {0, 0};
    v2 st1 = {1, 1};
    int32_t texture;
    get_texaddress_index_from_asset_descriptor(
        state, descriptors, filter->source_asset_descriptor_id,
        &texture, &st0, &st1);

    int32_t texcontainer = -1;

    void *shaderconfig = {};
    uint32_t shaderconfig_size = 0;

    FilterSobelShaderConfig fssc;

    hglBindVertexArray(state->vao);
    switch (filter->type)
    {
        case ApplyFilterType::none:
        {
            ImGui::Text("Filter::none");
            auto &program = state->filter_gaussian_7x1_program;
            texcontainer = state->filter_gaussian_7x1_texcontainer;
            hglUseProgram(program.program);
            hglUniformMatrix4fv(program.u_projection_id, 1, GL_FALSE, (float *)&state->m4identity);
            hglUniformMatrix4fv(program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
            hglUniformMatrix4fv(program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
            hglUniform1i(program.u_texaddress_index_id, texture);
        } break;
        case ApplyFilterType::gaussian_7x1_x:
        case ApplyFilterType::gaussian_7x1_y:
        {
            ImGui::Text("Filter::gaussian");
            auto &program = state->filter_gaussian_7x1_program;
            texcontainer = state->filter_gaussian_7x1_texcontainer;
            hglUseProgram(program.program);
            hglUniformMatrix4fv(program.u_projection_id, 1, GL_FALSE, (float *)&state->m4identity);
            hglUniformMatrix4fv(program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
            hglUniformMatrix4fv(program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
            a_position_id = program.a_position_id;
            a_texcoord_id = program.a_texcoord_id;
            float blur_scale_divisor = filter->args.gaussian.blur_scale_divisor;
            if (!blur_scale_divisor)
            {
                blur_scale_divisor = state->blur_scale_divisor;
            }
            v2 blur_scale = { 1.0f / blur_scale_divisor, 0 };
            if (filter->type == ApplyFilterType::gaussian_7x1_y)
            {
                blur_scale = { 0, 1.0f / blur_scale_divisor };
            }
            hglUniform2fv(program.u_blur_scale_id, 1, (float *)&blur_scale);
            hglUniform1i(program.u_texaddress_index_id, texture);
        } break;
        case ApplyFilterType::sobel:
        {
            ImGui::Text("Sobel");
            auto &program = state->filter_sobel_program;
            fssc.texaddress_index = texture;
            texcontainer = state->filter_sobel_texcontainer;
            hglUseProgram(program.program);
            shaderconfig = &fssc;
            shaderconfig_size = sizeof(fssc);
            a_position_id = program.a_position_id;
            a_texcoord_id = program.a_texcoord_id;
            fssc.texture_scale = {
                1.0f / 960.f,
                1.0f / 540.f,
            };
        } break;
    }

    if (state->crash_on_gl_errors) hglErrorAssert();
    hglBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    struct vertex_format
    {
        v3 pos;
        v2 uv;
    } vertices[] = {
        {
            {-1.0f,-1.0f },
            { 0.0f, 0.0f },
        },
        {
            { 1.0f,-1.0f },
            { 1.0f, 0.0f },
        },
        {
            { 1.0f, 1.0f },
            { 1.0f, 1.0f },
        },
        {
            {-1.0f, 1.0f },
            { 0.0f, 1.0f },
        },
    };
    hglBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    hglVertexAttribPointer(
            (GLuint)a_position_id, 3, GL_FLOAT, GL_FALSE,
            sizeof(vertex_format), (void *)offsetof(vertex_format, pos));
    hglVertexAttribPointer((GLuint)a_texcoord_id, 2, GL_FLOAT, GL_FALSE,
            sizeof(vertex_format), (void *)offsetof(vertex_format, uv));

    uint32_t indices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
    hglBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), (GLvoid*)indices, GL_STREAM_DRAW);

    hglBindBufferBase(
        GL_UNIFORM_BUFFER,
        0,
        command_state.texture_address_buffers[0].uniform_buffer.ubo);

    if (shaderconfig)
    {
        hglBindBufferBase(
            GL_UNIFORM_BUFFER,
            9,
            command_state.shaderconfig_buffer.ubo);
        hglBindBuffer(GL_UNIFORM_BUFFER, command_state.shaderconfig_buffer.ubo);
        hglBufferSubData(
            GL_UNIFORM_BUFFER,
            0,
            shaderconfig_size,
            shaderconfig);
    }
    static int32_t texcontainer_textures[harray_count(command_state.textures)] =
    {
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
    };
    hglUniform1iv(texcontainer, harray_count(texcontainer_textures), texcontainer_textures);
    for (uint32_t i = 0; i < harray_count(command_state.textures); ++i)
    {
        hglActiveTexture(GL_TEXTURE0 + i);
        hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[i]);
    }

    /*
    hglUniform1i(state->filter_gaussian_7x1_texaddress_id, 0);
    hglActiveTexture(GL_TEXTURE0);
    hglBindTexture(GL_TEXTURE_2D, texture);
    */

    hglDrawElements(GL_TRIANGLES, (GLsizei)harray_count(indices), GL_UNSIGNED_INT, (void *)0);
    hglBindTexture(GL_TEXTURE_2D, 0);
    if (state->crash_on_gl_errors) hglErrorAssert();

    hglBindVertexArray(0);
    hglDisable(GL_DEPTH_TEST);
    hglDepthFunc(GL_ALWAYS);
    hglEnable(GL_BLEND);
    if (state->crash_on_gl_errors) hglErrorAssert();

    return;
}

void
framebuffer_blit(renderer_state *state, asset_descriptor *descriptors, render_entry_type_framebuffer_blit *item, v2i size)
{

    asset_descriptor *descriptor = descriptors + item->fbo_asset_descriptor_id;
    FramebufferDescriptor *f = descriptor->framebuffer;
    uint32_t fbo = f->_fbo;

    hglBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    hglBlitFramebuffer(
        0,
        0,
        size.x,
        size.y,
        0,
        0,
        size.x,
        size.y,
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
        GL_NEAREST);
    if (state->crash_on_gl_errors) hglErrorAssert();
}

void
draw_sky(renderer_state *state, m4 *matrices, asset_descriptor *descriptors, render_entry_type_sky *sky)
{
    auto &program = state->sky_program;
    hglUseProgram(program.program);

    hglEnable(GL_DEPTH_TEST);
    hglDepthFunc(GL_LESS);
    hglDisable(GL_CULL_FACE);
    hglCullFace(GL_FRONT);
    hglDisable(GL_BLEND);

    hglBindVertexArray(state->vao);

    v2 st0 = {0, 0};
    v2 st1 = {1, 1};

    if (state->crash_on_gl_errors) hglErrorAssert();
    hglBindBuffer(GL_ARRAY_BUFFER, state->skybox.vbo);
    hglVertexAttribPointer((GLuint)program.a_position_id, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    hglEnableVertexAttribArray((GLuint)program.a_position_id);
    hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->skybox.ibo);

    m4 projection_matrix = matrices[sky->projection_matrix_id];
    m4 view_matrix = matrices[sky->view_matrix_id];
    v3 camera_position = calculate_camera_position(view_matrix);

    hglUniform3fv(program.u_camera_position_id, 1, (float *)&camera_position);
    hglUniform3fv(program.u_light_direction_id, 1, (float *)&sky->light_direction);
    m4 model_matrix = m4scale(1000.0f);

    hglUniformMatrix4fv(program.u_model_matrix_id, 1, GL_FALSE, (float *)&model_matrix);
    hglUniformMatrix4fv(program.u_view_matrix_id, 1, GL_FALSE, (float *)&view_matrix);
    hglUniformMatrix4fv(program.u_projection_matrix_id, 1, GL_FALSE, (float *)&projection_matrix);

    v3 v3InvWavelength = {
        1 / pow(0.650f, 4),
        1 / pow(0.570f, 4),
        1 / pow(0.475f, 4),
    };

    float kr = 0.0025f;
    float km = 0.0015f;
    float sun_brightness = 15.0f;
    float fKrESun = sun_brightness * kr;
    float fKmESun = sun_brightness * km;
    float pi = 3.14159265358979f;
    float fKr4PI = kr * 4 * pi;
    float fKm4PI = km * 4 * pi;
    float fInnerRadius = 6356752.0f;
    float fOuterRadius = fInnerRadius * 1.025f;
    float fScale = 1 / (fOuterRadius - fInnerRadius);
    float fScaleDepth = 0.25f;
    float fScaleOverScaleDepth = fScale / fScaleDepth;

    SkyShaderConfig shaderconfig = {
        v3InvWavelength,
        kr,
        km,
        sun_brightness,
        fKrESun,
        fKmESun,
        pi,
        fKr4PI,
        fKm4PI,
        fInnerRadius,
        fOuterRadius,
        fScale,
        fScaleDepth,
        fScaleOverScaleDepth,
    };

    auto &command_state = *state->command_state;
    hglBindBufferBase(
        GL_UNIFORM_BUFFER,
        9,
        command_state.shaderconfig_buffer.ubo);
    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.shaderconfig_buffer.ubo);
    hglBufferSubData(
        GL_UNIFORM_BUFFER,
        0,
        sizeof(ShaderConfig),
        &shaderconfig);

    hglDrawElements(GL_TRIANGLES, (GLsizei)harray_count(state->skybox.faces) * 3, GL_UNSIGNED_INT, (void *)0);


    hglBindVertexArray(0);
}

void
append_command(
    renderer_state *state,
    CommandKey *key,
    DrawElementsIndirectCommand cmd
)
{
    /*
    auto &lists = state->command_state.lists;
    for (uint32_t i = 0; i < lists.num_command_lists; ++i)
    {

    }
    target_shader_texture_vertexformat_commands cmds;
    uint32_t index = cmds.num_commands++;
    commands[index] = cmd;
    */
}

static bool multi_draw_enabled = true;
void
draw_indirect(
    renderer_state *state,
    render_entry_list *list,
    LightDescriptors light_descriptors,
    asset_descriptor *descriptors
    )
{
    if (state->crash_on_gl_errors) hglErrorAssert();
    TIMED_FUNCTION();
    auto &command_state = *state->command_state;
    auto &command_lists = command_state.lists;

    //hglDisable(GL_BLEND);
    if (!list->config.depth_disabled)
    {
        hglEnable(GL_DEPTH_TEST);
        hglDepthFunc(GL_LESS);
    }
    else
    {
        hglDisable(GL_DEPTH_TEST);
    }

    if (!list->config.cull_disabled)
    {
        hglEnable(GL_CULL_FACE);
        if (!list->config.cull_front)
        {
            hglCullFace(GL_BACK);
        }
        else
        {
            hglCullFace(GL_FRONT);
        }
    }
    else
    {
        hglDisable(GL_CULL_FACE);
    }

    bool found_commands = false;
    ShaderType shader_type = {};
    for (uint32_t i = 0; i < command_lists.num_command_lists; ++i)
    {
        CommandKey &command_key = command_lists.keys[i];
        auto &command_list = command_lists.command_lists[i];

        if (command_list.num_commands)
        {
            found_commands = true;
            shader_type = command_key.shader_type;
            break;
        }
    }
    if (!found_commands)
    {
        return;
    }
    ShaderConfig shaderconfig = {};
    uint32_t program_data_index = 0;
    struct
    {
        int32_t texcontainer_uniform;
        int32_t draw_data_index_uniform;
        int32_t a_position_id;
        int32_t a_texcoord_id;
        int32_t a_normal_id;
        int32_t a_bone_ids_id;
        int32_t a_bone_weights_id;
        int32_t a_tangent_id;
    } program_data[] =
    {
        {
            state->texarray_1_texcontainer,
            state->texarray_1_program.u_draw_data_index_id,
            state->texarray_1_program.a_position_id,
            state->texarray_1_program.a_texcoord_id,
            state->texarray_1_program.a_normal_id,
            state->texarray_1_program.a_bone_ids_id,
            state->texarray_1_program.a_bone_weights_id,
            state->texarray_1_program.a_tangent_id,
        },
        {
            state->texarray_1_vsm_texcontainer,
            state->texarray_1_vsm_program.u_draw_data_index_id,
            state->texarray_1_vsm_program.a_position_id,
            state->texarray_1_vsm_program.a_texcoord_id,
            -1,
            state->texarray_1_vsm_program.a_bone_ids_id,
            state->texarray_1_vsm_program.a_bone_weights_id,
            -1,
        },
        {
            state->texarray_1_water_texcontainer,
            state->texarray_1_water_program.u_draw_data_index_id,
            state->texarray_1_water_program.a_position_id,
            -1
            -1,
            -1,
            -1,
            -1,
        },
    };

    if (state->crash_on_gl_errors) hglErrorAssert();
    switch (shader_type)
    {
        case ShaderType::standard:
        {
            program_data_index = 0;
            auto &program = state->texarray_1_program;

            hglUseProgram(program.program);
            hglUniform1f(program.u_bias_id, state->shadowmap_bias);
            hglUniform1f(program.u_minimum_variance_id, state->vsm_minimum_variance);
            hglUniform1f(program.u_lightbleed_compensation_id, state->vsm_lightbleed_compensation);
            shaderconfig.texarray_1_shaderconfig = {
                list->config.clipping_plane,
                {}
            };
            shaderconfig.texarray_1_shaderconfig.flags.normal_map_disabled = list->config.normal_map_disabled;
            shaderconfig.texarray_1_shaderconfig.flags.show_normals = list->config.show_normals;
            shaderconfig.texarray_1_shaderconfig.flags.show_tangent = list->config.show_tangent;
            shaderconfig.texarray_1_shaderconfig.flags.show_normalmap = list->config.show_normalmap;

            shaderconfig.texarray_1_shaderconfig.flags.specular_map_disabled = list->config.specular_map_disabled;
            shaderconfig.texarray_1_shaderconfig.flags.show_specular = list->config.show_specular;
            shaderconfig.texarray_1_shaderconfig.flags.show_specularmap = list->config.show_specularmap;
            shaderconfig.texarray_1_shaderconfig.flags.show_object_identifier = list->config.show_object_identifier;
            shaderconfig.texarray_1_shaderconfig.flags.show_texcontainer_index = list->config.show_texcontainer_index;

            shaderconfig.texarray_1_shaderconfig.flags.use_color = list->config.use_color;
            shaderconfig.texarray_1_shaderconfig.color = list->config.color;
        } break;
        case ShaderType::variance_shadow_map:
        {
            program_data_index = 1;
            auto &program = state->texarray_1_vsm_program;
            hglUseProgram(program.program);
            hglCullFace(GL_FRONT);
        } break;
        case ShaderType::water:
        {
            program_data_index = 2;
            auto &program = state->texarray_1_water_program;

            hglUseProgram(program.program);
            hglEnable(GL_BLEND);
            hglBlendEquation(GL_FUNC_ADD);
            hglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            int32_t reflection_texaddress_index = - 1;
            int32_t refraction_texaddress_index = - 1;
            int32_t refraction_depth_texaddress_index = - 1;
            int32_t dudv_map_texaddress_index = - 1;
            int32_t normal_map_texaddress_index = - 1;
            v2 st0, st1;
            get_texaddress_index_from_asset_descriptor(
                state, descriptors,
                list->config.reflection_asset_descriptor,
                &reflection_texaddress_index, &st0, &st1);
            get_texaddress_index_from_asset_descriptor(
                state, descriptors,
                list->config.refraction_asset_descriptor,
                &refraction_texaddress_index, &st0, &st1);
            get_texaddress_index_from_asset_descriptor(
                state, descriptors,
                list->config.refraction_depth_asset_descriptor,
                &refraction_depth_texaddress_index, &st0, &st1);
            get_texaddress_index_from_asset_descriptor(
                state, descriptors,
                list->config.dudv_map_asset_descriptor,
                &dudv_map_texaddress_index, &st0, &st1);
            get_texaddress_index_from_asset_descriptor(
                state, descriptors,
                list->config.normal_map_asset_descriptor,
                &normal_map_texaddress_index, &st0, &st1);
            static float move_factor = 0.0f;
            move_factor += 0.0004f;
            shaderconfig.texarray_1_water_shaderconfig = {
                list->config.camera_position,
                reflection_texaddress_index,
                refraction_texaddress_index,
                refraction_depth_texaddress_index,
                dudv_map_texaddress_index,
                normal_map_texaddress_index,
                20.0f,
                0.02f,
                move_factor,
                state->vsm_minimum_variance,
                state->shadowmap_bias,
                state->vsm_lightbleed_compensation,
                list->config.near_,
                list->config.far_,
            };
        } break;
    }
    if (state->crash_on_gl_errors) hglErrorAssert();

    auto &pd = program_data[program_data_index];

    for (uint32_t i = 0; i < light_descriptors.count; ++i)
    {
        auto &light = light_descriptors.descriptors[i];
        v4 position_or_direction = {};
        switch (light.type)
        {
            case LightType::directional:
            {
                v3 direction = light.direction;
                direction = v3normalize(direction);

                position_or_direction = {
                    direction.x,
                    direction.y,
                    direction.z,
                    0.0f,
                };
            } break;
        }

        command_state.light_list.lights[i] = {
            position_or_direction,
            light.shadowmap_matrix,
            light.color,
            light.ambient_intensity,
            light.diffuse_intensity,
            light.attenuation_constant,
            light.attenuation_linear,
            light.attenuation_exponential,
        };
    }

    if (state->crash_on_gl_errors) hglErrorAssert();
    hglBindBufferBase(
        GL_UNIFORM_BUFFER,
        0,
        command_state.texture_address_buffers[0].uniform_buffer.ubo);
    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.texture_address_buffers[0].uniform_buffer.ubo);

    if (command_state.texture_address_count != command_state.texture_address_last_buffer_count)
    {
        hglBufferSubData(
            GL_UNIFORM_BUFFER,
            0,
            sizeof(command_state.texture_addresses),
            command_state.texture_addresses);
        command_state.texture_address_last_buffer_count = command_state.texture_address_count;
    }

    if (state->crash_on_gl_errors) hglErrorAssert();
    hglBindBufferBase(
        GL_UNIFORM_BUFFER,
        2,
        command_state.lights_buffer.ubo);
    hglBindBuffer(
        GL_UNIFORM_BUFFER,
        command_state.lights_buffer.ubo);
    hglBufferSubData(
        GL_UNIFORM_BUFFER,
        0,
        sizeof(LightList),
        command_state.light_list.lights);

    if (list->config.use_clipping_plane)
    {
        hglEnable(GL_CLIP_DISTANCE0);
    }
    else
    {
        hglDisable(GL_CLIP_DISTANCE0);
    }
    if (state->crash_on_gl_errors) hglErrorAssert();
    hglBindBufferBase(
        GL_UNIFORM_BUFFER,
        9,
        command_state.shaderconfig_buffer.ubo);
    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.shaderconfig_buffer.ubo);
    hglBufferSubData(
        GL_UNIFORM_BUFFER,
        0,
        sizeof(ShaderConfig),
        &shaderconfig);

    if (state->crash_on_gl_errors) hglErrorAssert();
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);

    static int32_t texcontainer_textures[harray_count(command_state.textures)] =
    {
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
    };
    if (state->crash_on_gl_errors) hglErrorAssert();
    hglUniform1iv(pd.texcontainer_uniform, harray_count(texcontainer_textures), texcontainer_textures);
    /*
    for (uint32_t i = 0; i < harray_count(command_state.textures); ++i)
    {
        hglActiveTexture(GL_TEXTURE0 + i);
        hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[i]);
    }
    hglActiveTexture(GL_TEXTURE0);
    */

    if (state->crash_on_gl_errors) hglErrorAssert();

    for (uint32_t i = 0; i < command_lists.num_command_lists; ++i)
    {
        CommandKey &command_key = command_lists.keys[i];
        auto &command_list = command_lists.command_lists[i];

        if (!command_list.num_commands)
        {
            continue;
        }

        switch (command_key.vertexformat)
        {
            case VertexFormat::v1:
            case VertexFormat::v2:
            case VertexFormat::v3:
            case VertexFormat::just_vertices:
            break;
            case VertexFormat::v0:
            default:
            {
                ++command_list.generation;
                command_list.num_commands = 0;
                command_list.num_bones = 0;
                command_list.num_texcontainer_indices = 0;
                continue;
            } break;
        }

        if (!command_list._vao_initialized)
        {
            hglGenVertexArrays(1, &command_list.vao);
            hglBindVertexArray(command_list.vao);
            hglBindBuffer(
                GL_ARRAY_BUFFER,
                command_state.vertex_buffers[command_key.vertex_buffer].vbo);
            switch (command_key.vertexformat)
            {
                case VertexFormat::v1:
                {
                    hglVertexAttribPointer((GLuint)pd.a_position_id, 3, GL_FLOAT, GL_FALSE, sizeof(vertexformat_1), (void *)offsetof(vertexformat_1, position));
                    hglEnableVertexAttribArray((GLuint)pd.a_position_id);
                    if (pd.a_texcoord_id >= 0)
                    {
                        hglVertexAttribPointer((GLuint)pd.a_texcoord_id, 2, GL_FLOAT, GL_FALSE, sizeof(vertexformat_1), (void *)offsetof(vertexformat_1, texcoords));
                        hglEnableVertexAttribArray((GLuint)pd.a_texcoord_id);
                    }
                    if (pd.a_normal_id >= 0)
                    {
                        hglVertexAttribPointer((GLuint)pd.a_normal_id, 3, GL_FLOAT, GL_FALSE, sizeof(vertexformat_1), (void *)offsetof(vertexformat_1, normal));
                        hglEnableVertexAttribArray((GLuint)pd.a_normal_id);
                    }
                } break;
                case VertexFormat::v2:
                {
                    hglVertexAttribPointer((GLuint)pd.a_position_id, 3, GL_FLOAT, GL_FALSE, sizeof(vertexformat_2), (void *)offsetof(vertexformat_2, position));
                    hglEnableVertexAttribArray((GLuint)pd.a_position_id);
                    if (pd.a_texcoord_id >= 0)
                    {
                        hglVertexAttribPointer((GLuint)pd.a_texcoord_id, 2, GL_FLOAT, GL_FALSE, sizeof(vertexformat_2), (void *)offsetof(vertexformat_2, texcoords));
                        hglEnableVertexAttribArray((GLuint)pd.a_texcoord_id);
                    }
                    if (pd.a_normal_id >= 0)
                    {
                        hglVertexAttribPointer((GLuint)pd.a_normal_id, 3, GL_FLOAT, GL_FALSE, sizeof(vertexformat_2), (void *)offsetof(vertexformat_2, normal));
                        hglEnableVertexAttribArray((GLuint)pd.a_normal_id);
                    }
                    if (pd.a_bone_ids_id >= 0)
                    {
                        hglVertexAttribIPointer(
                            (GLuint)pd.a_bone_ids_id,
                            4,
                            GL_INT,
                            sizeof(vertexformat_2),
                            (void *)offsetof(vertexformat_2, bone_ids));
                        hglEnableVertexAttribArray((GLuint)pd.a_bone_ids_id);
                    }
                    if (pd.a_bone_weights_id >= 0)
                    {
                        hglVertexAttribPointer(
                            (GLuint)pd.a_bone_weights_id,
                            4,
                            GL_FLOAT,
                            GL_FALSE,
                            sizeof(vertexformat_2),
                            (void *)offsetof(vertexformat_2, bone_weights));
                        hglEnableVertexAttribArray((GLuint)pd.a_bone_weights_id);
                    }
                } break;
                case VertexFormat::v3:
                {
                    hglVertexAttribPointer((GLuint)pd.a_position_id, 3, GL_FLOAT, GL_FALSE, sizeof(vertexformat_3), (void *)offsetof(vertexformat_3, position));
                    hglEnableVertexAttribArray((GLuint)pd.a_position_id);
                    if (pd.a_texcoord_id >= 0)
                    {
                        hglVertexAttribPointer((GLuint)pd.a_texcoord_id, 2, GL_FLOAT, GL_FALSE, sizeof(vertexformat_3), (void *)offsetof(vertexformat_3, texcoords));
                        hglEnableVertexAttribArray((GLuint)pd.a_texcoord_id);
                    }
                    if (pd.a_normal_id >= 0)
                    {
                        hglVertexAttribPointer((GLuint)pd.a_normal_id, 3, GL_FLOAT, GL_FALSE, sizeof(vertexformat_3), (void *)offsetof(vertexformat_3, normal));
                        hglEnableVertexAttribArray((GLuint)pd.a_normal_id);
                    }
                    if (pd.a_tangent_id >= 0)
                    {
                        hglVertexAttribPointer((GLuint)pd.a_tangent_id, 3, GL_FLOAT, GL_FALSE, sizeof(vertexformat_3), (void *)offsetof(vertexformat_3, tangent));
                        hglEnableVertexAttribArray((GLuint)pd.a_tangent_id);
                    }
                } break;

                case VertexFormat::just_vertices:
                {
                    hglVertexAttribPointer((GLuint)pd.a_position_id, 3, GL_FLOAT, GL_FALSE, sizeof(vertexformat_just_vertices), (void *)offsetof(vertexformat_just_vertices, position));
                    hglEnableVertexAttribArray((GLuint)pd.a_position_id);
                } break;

                case VertexFormat::v0:
                default:
                {
                    hassert(!"Unreachable");
                } break;
            }
            command_list._vao_initialized = true;
        }
        else
        {
            hglBindVertexArray(command_list.vao);
        }

        if (state->show_debug)
        {
            ImGui::Text(
                "Command list %d, size %d, bones %d, shader_type %s, vertexformat %d, vertex_buffer %d, index_buffer %d",
                i,
                command_list.num_commands,
                command_list.num_bones,
                shader_type_configs[(uint32_t)command_key.shader_type].name,
                command_key.vertexformat,
                command_key.vertex_buffer,
                command_key.index_buffer
                );
            char texture_list[1024] = {};
            uint32_t texture_list_len = 0;
            for (uint32_t j = 0; j < command_list.num_texcontainer_indices; ++j)
            {
                char comma[] = {',', ' ', 0};
                if (j == 0)
                {
                    comma[0] = ' ';
                }
                texture_list_len += sprintf(texture_list + texture_list_len, "%s%d", comma, command_list.texcontainer_index[j].container_index);
            }
            ImGui::Text("Textures: %s", texture_list);
        }
        hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
            command_state.index_buffers[command_key.index_buffer].ibo);

        hglBindBufferBase(
            GL_UNIFORM_BUFFER,
            1,
            command_state.draw_data_buffer.ubo);
        hglBindBuffer(
            GL_UNIFORM_BUFFER,
            command_state.draw_data_buffer.ubo);
        hglBufferSubData(
            GL_UNIFORM_BUFFER,
            0,
            sizeof(DrawDataList),
            command_list.draw_data_list.data);
        hglBindBufferBase(
            GL_UNIFORM_BUFFER,
            3,
            command_state.bone_data_buffer.ubo);
        hglBindBuffer(
            GL_UNIFORM_BUFFER,
            command_state.bone_data_buffer.ubo);
        hglBufferSubData(
            GL_UNIFORM_BUFFER,
            0,
            sizeof(BoneDataList),
            command_list.bone_data_list.data);

        if (state->crash_on_gl_errors) hglErrorAssert();

        hglBindBufferBase(
            GL_UNIFORM_BUFFER,
            4,
            command_state.texcontainer_sampler_mapping_buffer.ubo);

        if (state->crash_on_gl_errors) hglErrorAssert();

        hglBindBuffer(
            GL_UNIFORM_BUFFER,
            command_state.texcontainer_sampler_mapping_buffer.ubo);

        if (state->crash_on_gl_errors) hglErrorAssert();

        struct
        {
            TexContainerSamplerMapping texcontainer_sampler_mapping[16];
        } cb4_data;

        memcpy(
            cb4_data.texcontainer_sampler_mapping,
            command_list.texcontainer_sampler_mapping,
            sizeof(cb4_data.texcontainer_sampler_mapping)
        );

        for (uint32_t j = 0; j < harray_count(command_state.textures); ++j)
        {
            hglActiveTexture(GL_TEXTURE0 + j);
            hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[j]);
        }

        for (uint32_t j = 0; j < command_list.num_texcontainer_indices; ++j)
        {
            hglActiveTexture(GL_TEXTURE0 + j);
            uint32_t container_index = command_list.texcontainer_index[j].container_index;
            hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[container_index]);
        }
        hglActiveTexture(GL_TEXTURE0);

        /*
        for (uint32_t j = 0; j < NUM_TEXARRAY_TEXTURES; ++j)
        {
            TexContainerSamplerMapping tcsm = command_list.texcontainer_sampler_mapping[j];
            if (tcsm.generation == command_list.generation)
            {
                ImGui::Text("texcontainer_sampler_mapping[%d] = %d", j, tcsm.texcontainer_index);
            }
        }
        */
        if (state->crash_on_gl_errors) hglErrorAssert();

        hglBufferSubData(
            GL_UNIFORM_BUFFER,
            0,
            sizeof(cb4_data),
            (void *)&cb4_data);

        if (state->crash_on_gl_errors) hglErrorAssert();

        hglBindBuffer(GL_UNIFORM_BUFFER, 0);

        hglBindBuffer(GL_DRAW_INDIRECT_BUFFER, command_state.indirect_buffer.ubo);
        hglBufferSubData(
            GL_DRAW_INDIRECT_BUFFER,
            0,
            sizeof(command_list.commands),
            command_list.commands);
        // For now, no points
        hglLineWidth(1.0f);
        hassert(command_key.primitive_type);
        if (state->gl_extensions.gl_arb_multi_draw_indirect && multi_draw_enabled)
        {
            hglUniform1i(pd.draw_data_index_uniform, 0);
            hglMultiDrawElementsIndirect(
                command_key.primitive_type,
                GL_UNSIGNED_INT,
                (void *)0,
                command_list.num_commands,
                0);
        }
        else
        {
            for (uint32_t j = 0; j < command_list.num_commands; ++j)
            {
                //auto &command = command_list.commands[j];

                hglUniform1i(pd.draw_data_index_uniform, j);
                /*
                hglDrawElementsBaseVertex(
                    command_key.primitive_type,
                    command.count,
                    GL_UNSIGNED_INT,
                    (void *)(intptr_t)(command.firstIndex * 4),
                    command.baseVertex);
                */
                /*
                hglDrawElementsInstancedBaseVertexBaseInstance(
                    command_key.primitive_type,
                    command.count,
                    GL_UNSIGNED_INT,
                    (void *)(intptr_t)(command.firstIndex * 4),
                    command.primCount,
                    command.baseVertex,
                    0);
                */
                hglDrawElementsIndirect(
                    command_key.primitive_type,
                    GL_UNSIGNED_INT,
                    (void *)(j * sizeof(DrawElementsIndirectCommand)));
            }
        }

        ++command_list.generation;
        command_list.num_commands = 0;
        command_list.num_texcontainer_indices = 0;
        command_list.num_bones = 0;
    }
    if (state->crash_on_gl_errors) hglErrorAssert();
}

enum struct
FramebufferTextureTargets
{
    two_dee,
    two_dee_multisample,
    two_dee_array,
    two_dee_multisample_array,
};

struct
FramebufferTextureConfig
{
    uint32_t texture_target;
    bool array;
    bool multisample;
} framebuffer_texture_configs[] =
{
    { GL_TEXTURE_2D, false, false },
    { GL_TEXTURE_2D_MULTISAMPLE, false, true },
    { GL_TEXTURE_2D_ARRAY/**/, true, false },
    { GL_TEXTURE_2D_MULTISAMPLE_ARRAY/**/, true, true },
};

FramebufferTextureConfig
framebuffer_texture_config(FramebufferDescriptor *framebuffer, bool multisample_allowed)
{
    uint32_t is_multisample = (uint32_t)(framebuffer->_flags.use_multisample_buffer && multisample_allowed);
    uint32_t is_array = framebuffer->_flags.use_texarray;
    return framebuffer_texture_configs[2 * is_array + is_multisample];
}

void
framebuffer_initialize_color_buffer_texarray(
    renderer_state *state,
    FramebufferDescriptor *framebuffer,
    FramebufferTextureConfig texture_config,
    v2i framebuffer_size,
    uint32_t multisample_samples)
{
    TextureFormat format = TextureFormat::rgba16f;
    if (framebuffer->_flags.use_rg32f_buffer)
    {
        format = TextureFormat::rg32f;

    }
    framebuffer->_texture = new_texaddress(state->command_state, framebuffer_size.x, framebuffer_size.y, format);
}


void
framebuffer_initialize_depth_buffer_texarray(
    renderer_state *state,
    FramebufferDescriptor *framebuffer,
    FramebufferTextureConfig texture_config,
    v2i framebuffer_size,
    uint32_t multisample_samples)
{
    framebuffer->_renderbuffer = new_texaddress(state->command_state, framebuffer_size.x, framebuffer_size.y, TextureFormat::depth24stencil8);
}

void
framebuffer_initialize(
    renderer_state *state,
    FramebufferDescriptor *framebuffer,
    FramebufferTextureConfig texture_config,
    v2i framebuffer_size,
    uint32_t multisample_samples)
{
    hglGenFramebuffers(1, &framebuffer->_fbo);
    hglBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->_fbo);
    if (state->crash_on_gl_errors) hglErrorAssert();

    if (!framebuffer->_flags.no_color_buffer)
    {
        framebuffer_initialize_color_buffer_texarray(state, framebuffer, texture_config, framebuffer_size, multisample_samples);
    }

    if (state->crash_on_gl_errors) hglErrorAssert();

    if (!framebuffer->_flags.use_depth_texture)
    {
        hglGenRenderbuffers(1, &framebuffer->_renderbuffer);
        hglBindRenderbuffer(GL_RENDERBUFFER, framebuffer->_renderbuffer);
        if (texture_config.multisample)
        {
            hglRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample_samples, GL_DEPTH24_STENCIL8, framebuffer_size.x, framebuffer_size.y);
        }
        else
        {
            hglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framebuffer_size.x, framebuffer_size.y);
        }
        if (state->crash_on_gl_errors) hglErrorAssert();
    }
    else
    {
        framebuffer_initialize_depth_buffer_texarray(state, framebuffer, texture_config, framebuffer_size, multisample_samples);
    }
    if (state->crash_on_gl_errors) hglErrorAssert();

    FramebufferMakeInitialized(framebuffer);
}

void
framebuffer_texture_attach(
    renderer_state *state,
    FramebufferDescriptor *framebuffer,
    FramebufferTextureConfig texture_config)
{
    auto &command_state = *state->command_state;
    hassert(framebuffer->_fbo);
    if (!framebuffer->_flags.no_color_buffer)
    {
        auto &ta = command_state.texture_addresses[framebuffer->_texture];
        auto &texture_id = command_state.textures[ta.container_index];
        hglFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_id, 0, (int32_t)ta.layer);
        GLenum draw_buffers[] =
        {
            GL_COLOR_ATTACHMENT0,
        };
        hglDrawBuffers(harray_count(draw_buffers), draw_buffers);
    }
    else
    {
        hglDrawBuffer(GL_NONE);
    }
    if (state->crash_on_gl_errors) hglErrorAssert();

    if (!framebuffer->_flags.use_depth_texture)
    {
        hglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer->_renderbuffer);
        if (framebuffer->_flags.use_stencil_buffer)
        {
            hglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer->_renderbuffer);
        }
    }
    else
    {
        auto &ta = command_state.texture_addresses[framebuffer->_renderbuffer];
        auto &texture_id = command_state.textures[ta.container_index];
        hglFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_id, 0, (int32_t)ta.layer);
    }
    if (state->crash_on_gl_errors) hglErrorAssert();

}

void
framebuffer_setup(
    renderer_state *state,
    FramebufferDescriptor *framebuffer,
    bool *clear,
    v4 *clear_color,
    v2i *size)
{
    v2i framebuffer_size = {
        (int32_t)(framebuffer->size.x * state->framebuffer_scale),
        (int32_t)(framebuffer->size.y * state->framebuffer_scale),
    };
    TIMED_BLOCK("framebuffer setup");
    if (!framebuffer->_flags.no_clear_each_frame)
    {
        if (!framebuffer->_flags.cleared_this_frame)
        {
            *clear = true;
            *clear_color = framebuffer->clear_color;
            framebuffer->_flags.cleared_this_frame = true;
        }
    }

    FramebufferTextureConfig texture_config = framebuffer_texture_config(framebuffer, !state->multisample_disabled);
    uint32_t multisample_samples = 2;
    //bool framebuffer_is_multisample = framebuffer->_flags.use_multisample_buffer && !state->multisample_disabled;
    if (!FramebufferInitialized(framebuffer))
    {
        framebuffer_initialize(state, framebuffer, texture_config, framebuffer_size, multisample_samples);
        hglViewport(0, 0, framebuffer_size.x, framebuffer_size.y);
        framebuffer_texture_attach(state, framebuffer, texture_config);
        hglViewport(0, 0, framebuffer_size.x, framebuffer_size.y);
        GLenum framebuffer_status = hglCheckFramebufferStatus(GL_FRAMEBUFFER);
        hassert(framebuffer_status == GL_FRAMEBUFFER_COMPLETE);
    }
    hglBindFramebuffer(GL_FRAMEBUFFER, framebuffer->_fbo);

    *size = framebuffer_size;
    if (state->flush_for_profiling) hglFlush();
}

void
arena_debug(renderer_state *state, bool *show)
{
    if (ImGui::Begin("Renderer Memory Arena", show))
    {
        ImGui::Columns(3, "mycolumns");
        ImGui::Separator();
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("Size"); ImGui::NextColumn();
        ImGui::Text("Code"); ImGui::NextColumn();
        ImGui::Separator();
        MemoryBlockDebugList *list = state->arena.debug_list;
        while (list)
        {
            for (uint32_t i = 0; i < list->num_data; ++i)
            {
                MemoryBlockDebugData *data = list->data + i;
                ImGui::Text("%s", data->label);
                ImGui::NextColumn();
                ImGui::Text("% 12d", data->size);
                ImGui::NextColumn();
                ImGui::Text("%64s", data->guid);
                ImGui::NextColumn();
            }
            list = list->next;
        }
        ImGui::Columns(1);
        ImGui::Separator();
    }
    ImGui::End();
}

extern "C" RENDERER_RENDER(renderer_render)
{
#if 0
    return true;
#else
    renderer_state *state = (renderer_state *)memory->renderer_block->base;
    {
        TIMED_BLOCK("Wait for vsync");
        if (state->flush_for_profiling) hglFlush();

        if (state->crash_on_gl_errors) hglErrorAssert();
    }
    TIMED_FUNCTION();

    _platform_memory = memory;

    ImGuiIO& io = ImGui::GetIO();
    generations_updated_this_frame = 0;

    hassert(_platform->render_lists_count > 0);

    window_data *window = &state->input->window;
    if (state->crash_on_gl_errors) hglErrorAssert();

    uint32_t render_lists_to_process[20];
    uint32_t render_lists_to_process_count = 0;

    {
        TIMED_BLOCK("Render list sorting");
        while(render_lists_to_process_count < _platform->render_lists_count)
        {
            for (uint32_t i = 0; i < _platform->render_lists_count; ++i)
            {
                render_entry_list *render_list = _platform->render_lists[i];
                if (!render_list->depends_on_slots)
                {
                    bool handled = false;
                    for (uint32_t j = 0; j < render_lists_to_process_count; ++j)
                    {
                        if (render_lists_to_process[j] == i)
                        {
                            handled = true;
                            break;
                        }

                    }
                    if (handled)
                    {
                        continue;
                    }
                    hassert(render_lists_to_process_count < harray_count(render_lists_to_process));
                    render_lists_to_process[render_lists_to_process_count++] = i;
                    for (uint32_t j = 0; j < _platform->render_lists_count; ++j)
                    {
                        uint32_t inverse = (uint32_t)~(1 << i);
                        render_entry_list *render_list_to_undepend = _platform->render_lists[j];
                        render_list_to_undepend->depends_on_slots &= inverse;
                    }
                }
            }
        }
    }

    RecordTimerQuery(&state->timer_query_data, DEBUG_NAME("initial error check"));
    hglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    EndTimerQuery();

    if (state->show_debug)
    {
        ImGui::Begin("Renderer debug", &state->show_debug);
        ImGui::End();
    }

    if (state->show_debug)
    {
        ImGui::Begin("Renderer debug");
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("This frame: %.3f ms", 1000.0f * io.DeltaTime);
    }

    uint32_t quads_drawn = 0;
    for (uint32_t ii = 0; ii < render_lists_to_process_count; ++ii)
    {
        render_entry_list *render_list = _platform->render_lists[render_lists_to_process[ii]];
        if (state->show_debug)
        {
            ImGui::Text("Render list: %s", strrchr(render_list->name, '|') + 1);
        }
        {
            RecordDebugEvent(DebugType::BeginBlock, render_list->name);
        }

        RecordTimerQuery(&state->timer_query_data, render_list->name);

        v2i size = {window->width, window->height};
        hglEnable(GL_BLEND);
        hglDisable(GL_CULL_FACE);
        hglDisable(GL_DEPTH_TEST);
        hglDisable(GL_STENCIL_TEST);
        hglDepthFunc(GL_ALWAYS);
        hglBlendEquation(GL_FUNC_ADD);
        hglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        hglScissor(0, 0, window->width, window->height);
        hglUseProgram(state->imgui_program.program);
        hglUniform1i(state->imgui_tex, 0);

        hglUniformMatrix4fv(state->imgui_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
        hglUniformMatrix4fv(state->imgui_program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
        uint32_t offset = 0;
        hassert(render_list->element_count > 0);

        m4 *matrices = {};
        asset_descriptor *asset_descriptors = {};
        LightDescriptors lights = {};
        ArmatureDescriptors armatures = {};
        MaterialDescriptors materials = {};

        uint32_t matrix_count = 0;
        uint32_t asset_descriptor_count = 0;

        FramebufferDescriptor *framebuffer = render_list->framebuffer;
        bool clear = false;
        v4 clear_color = {};
        if (framebuffer)
        {
            framebuffer_setup(state, framebuffer, &clear, &clear_color, &size);
            if (framebuffer->_flags.use_stencil_buffer)
            {
                hglEnable(GL_STENCIL_TEST);
                hglStencilFunc(GL_ALWAYS, 0, 0);
                hglStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
            }
        }
        hglViewport(0, 0, (GLsizei)size.x, (GLsizei)size.y);
        hglScissor(0, 0, size.x, size.y);
        if (clear)
        {
            TIMED_BLOCK("clear");
            hglClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
            hglClearDepth(1.0f);
            hglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            if (state->flush_for_profiling) hglFlush();
        }

        for (uint32_t elements = 0; elements < render_list->element_count; ++elements)
        {
            render_entry_header *header = (render_entry_header *)(render_list->base + offset);
            uint32_t element_size = 0;

            switch (header->type)
            {
                case render_entry_type::clear:
                {
                    ExtractRenderElementWithSize(clear, item, header, element_size);
                    v4 *color = &item->color;
                    hglScissor(0, 0, size.x, size.y);
                    hglClearColor(color->r, color->g, color->b, color->a);
                    hglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::matrices:
                {
                    ExtractRenderElementWithSize(matrices, item, header, element_size);
                    matrices = item->matrices;
                    matrix_count = item->count;
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::asset_descriptors:
                {
                    ExtractRenderElementWithSize(asset_descriptors, item, header, element_size);
                    asset_descriptors = item->descriptors;
                    asset_descriptor_count = item->count;
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::descriptors:
                {
                    ExtractRenderElementWithSize(descriptors, item, header, element_size);
                    lights = item->lights;
                    armatures = item->armatures;
                    materials = item->materials;
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::QUADS:
                {
                    ExtractRenderElementWithSize(QUADS, item, header, element_size);
                    draw_quads(state, matrices, asset_descriptors, item);
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::QUADS_lookup:
                {
                    ExtractRenderElementSizeOnly(QUADS_lookup, element_size);
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::mesh_from_asset:
                {
                    ExtractRenderElementWithSize(mesh_from_asset, item, header, element_size);
                    draw_mesh_from_asset(
                        state,
                        &render_list->config,
                        matrices,
                        asset_descriptors,
                        lights.descriptors,
                        armatures.descriptors,
                        materials.materials,
                        item);
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::apply_filter:
                {
                    ExtractRenderElementWithSize(apply_filter, item, header, element_size);
                    apply_filter(state, asset_descriptors, item);
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::framebuffer_blit:
                {
                    ExtractRenderElementWithSize(framebuffer_blit, item, header, element_size);
                    framebuffer_blit(state, asset_descriptors, item, size);
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::sky:
                {
                    ExtractRenderElementWithSize(sky, item, header, element_size);
                    draw_sky(state, matrices, asset_descriptors, item);
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::debug_texture_load:
                {
                    ExtractRenderElementWithSize(debug_texture_load, item, header, element_size);
                    v2 st0 = {};
                    v2 st1 = {};
                    int32_t texture = 0;
                    get_texaddress_index_from_asset_descriptor(
                        state, asset_descriptors, item->asset_descriptor_id,
                        &texture, &st0, &st1);
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::readpixel:
                {
                    ExtractRenderElementWithSize(readpixel, item, header, element_size);
                    hglReadPixels(
                        item->pixel_location.x,
                        item->pixel_location.y,
                        1,
                        1,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        item->result->pixel);
                    item->result->pixel_returned = true;
                } break;
                default:
                {
                    hassert(!"Unhandled render entry type")
                };
            }
            if (state->crash_on_gl_errors) hglErrorAssert();
            hassert(element_size > 0);
            offset += element_size;
        }

        draw_indirect(state, render_list, lights, asset_descriptors);

        if (framebuffer)
        {
            if (framebuffer->_flags.use_stencil_buffer && state->overdraw)
            {
                draw_overdraw(state);
            }
            hglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        }
        if (state->flush_for_profiling) hglFlush();
        EndTimerQuery();
        END_BLOCK();
        if (state->crash_on_gl_errors) hglErrorAssert();
    }

    if (state->show_debug) {

        auto &command_state = *state->command_state;
        for (uint32_t i = 0; i < harray_count(command_state.texture_configs); ++i)
        {
            TextureConfig ts = command_state.texture_configs[i];
            TextureLayerInfo &tli = command_state.texture_layer_info[i];
            if (ts.width)
            {
                ImGui::Text(
                    "Texture container %d, size %dx%d, format %s, %d/%d used",
                    i,
                    ts.width,
                    ts.height,
                    texture_format_configs[(uint32_t)ts.format].name,
                    tli.num_layers_used,
                    tli.max_layers
                    );
            }
        }

        ImGui::Text("Quads drawn: %d", quads_drawn);
        ImGui::Text("Generations updated: %d", generations_updated_this_frame);
        const char *shadow_modes[] =
        {
            "none",
            "sampler2dshadow",
            "static poisson",
            "random poisson",
            "pcf",
            "vsm_color",
            "vsm",
            "bone_weights",
            "bone_ids",
        };
        ImGui::Checkbox("Overdraw", &state->overdraw);
        ImGui::ListBox("Shadow mode", &state->shadow_mode, shadow_modes, harray_count(shadow_modes));
        ImGui::DragInt("Poisson spread", &state->poisson_spread);
        ImGui::DragFloat("Shadowmap bias", &state->shadowmap_bias, 0.00001f, -0.1f, 0.1f, "%.9f");
        ImGui::DragInt("PCF distance", &state->pcf_distance);
        ImGui::DragInt("Poisson Samples", &state->poisson_samples);
        ImGui::DragFloat("Poisson Position Granularity", &state->poisson_position_granularity);
        ImGui::DragFloat("VSM Minimum Variance", &state->vsm_minimum_variance, 0.0000001f, 0.0f, 0.002f, "%.9f");
        ImGui::DragFloat("VSM Lightbleed Compensation", &state->vsm_lightbleed_compensation, 0.001f, 0.0f, 0.5f, "%.9f");
        ImGui::DragFloat("Blur Scale Divisor", &state->blur_scale_divisor, 1.0f, 1.0f, 8192.0f);
        ImGui::End();
    }

    if (state->show_gl_debug) {
        ImGui::Begin("OpenGL debug", &state->show_gl_debug);
        ImGui::Checkbox("Multi-draw", &multi_draw_enabled);

        if (ImGui::Button("Graphs..."))
            ImGui::OpenPopup("graphs");
        if (ImGui::BeginPopup("graphs"))
        {
            for (int i = 0; i < harray_count(counter_names_and_offsets); i++)
            {
                ImGui::MenuItem(counter_names_and_offsets[i].name, "", &state->gl_debug_toggles[i]);
            }
            ImGui::EndPopup();
        }

        auto history = &state->glsetup_counters_history;

        if (history->first >= 0)
        {
            int32_t values_count = 0;
            if (history->first == 0)
            {
                values_count = history->last + 1;
            }
            else
            {
                values_count = harray_count(history->entries);
            }
            for (uint32_t i = 0; i < harray_count(counter_names_and_offsets); ++i)
            {
                if (state->gl_debug_toggles[i])
                {
                    auto name_and_offset = counter_names_and_offsets[i];
                    float *values = (float *)((uint8_t *)history->entries + name_and_offset.offset);
                    int32_t _max = INT_MIN;
                    int32_t _min = INT_MAX;
                    int32_t _total = 0;
                    for (int32_t j = 0; j < values_count; ++j)
                    {
                        int32_t _value = (int32_t)(values[j * sizeof(GLSetupCounters) / sizeof(float)]);
                        hassert(_value >= 0);
                        _total += _value;
                        _max = max(_max, _value);
                        _min = min(_min, _value);
                    }
                    int32_t _avg = _total / values_count;
                    sprintf(name_and_offset.overlay, "max %d, min %d, avg %d",
                        _max, _min, _avg);
                    ImGui::PlotLines(
                        name_and_offset.name, // label
                        values,
                        values_count, // number of values
                        history->first, // offset of first value
                        name_and_offset.overlay, // overlay text
                        0.0, // scale_min
                        FLT_MAX, // scale_max
                        ImVec2(600,150), // size
                        sizeof(GLSetupCounters) // stride
                    );
                }
            }
        }
        ImGui::End();
    }

    if (state->show_arena) {
        arena_debug(state, &state->show_arena);
    }

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Renderer"))
        {
            ImGui::MenuItem("Debug rendering", "", &state->show_debug);
            ImGui::MenuItem("Mesh animation debug", "", &state->show_animation_debug);
            ImGui::MenuItem("OpenGL debug", "", &state->show_gl_debug);
            ImGui::MenuItem("Memory Arena", "", &state->show_arena);
            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();


    RecordTimerQuery(&state->timer_query_data, DEBUG_NAME("render_draw_lists"));
    hglBindVertexArray(0);
    render_draw_lists(draw_data, state);
    EndTimerQuery();

    if (state->crash_on_gl_errors) hglErrorAssert();

    {
        auto history = &state->glsetup_counters_history;
        if (history->first < 0)
        {
            history->entries[0] = *glsetup_counters;
            history->first = 0;
            history->last = 0;
        }
        else
        {
            history->last = (history->last + 1) % (int32_t)harray_count(history->entries);
            if (history->last == history->first)
            {
                history->first = (history->first + 1) % (int32_t)harray_count(history->entries);
            }
            history->entries[history->last] = *glsetup_counters;
        }
    }
    ResetCounters(glsetup_counters);

    CollectAndSwapTimers(&state->timer_query_data);

    input->mouse = state->original_mouse_input;

    hglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    hglErrorAssert();
    return true;
#endif
};
