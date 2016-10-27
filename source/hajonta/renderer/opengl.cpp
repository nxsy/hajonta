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
#include "hajonta/programs/ui2d.h"
#include "hajonta/programs/filters/filter_gaussian_7x1.h"
#include "hajonta/programs/sky.h"
#include "hajonta/programs/texarray_1.h"
#include "hajonta/programs/texarray_1_vsm.h"
#include "hajonta/programs/texarray_1_water.h"

#include "stb_truetype.h"
#include "hajonta/ui/ui2d.cpp"
#include "hajonta/image.cpp"
#include "hajonta/math.cpp"

static platform_memory *_platform_memory;

static float h_pi = 3.14159265358979f;
static float h_halfpi = h_pi / 2.0f;

DebugTable *GlobalDebugTable;

struct
vertexformat_1
{
    v3 position;
    v3 normal;
    v2 texcoords;
};

struct
vertexformat_2
{
    v3 position;
    v3 normal;
    v2 texcoords;
    v4i bone_ids;
    v4 bone_weights;
};

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
    uint32_t vertexformat;
    uint32_t vertex_buffer;
    uint32_t index_buffer;

    bool operator==(const CommandKey &other) const
    {
        return
            (count == other.count) &&
            (shader_type == other.shader_type) &&
            (vertexformat == other.vertexformat) &&
            (vertex_buffer == other.vertex_buffer) &&
            (index_buffer == other.index_buffer);
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

struct
CommandList
{
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
#if 0
    return;
#else
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

struct ui2d_state
{
    ui2d_program_struct ui2d_program;
    uint32_t vbo;
    uint32_t ibo;
    uint32_t vao;
    uint32_t mouse_texture;
};

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
AssetType
{
    texture,
    mesh,
};

struct asset
{
    AssetType type;
    uint32_t asset_id;
    char asset_name[100];
    int32_t asset_file_id;
    v2 st0;
    v2 st1;
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
    uint32_t query_ids[2][20];
    const char *guid[2][20];
    uint32_t buffer;
};

inline void
RecordTimerQuery(TimerQueryData *timer_query_data, const char *guid)
{
    auto &buffer = timer_query_data->buffer;
    auto &query_count = timer_query_data->query_count[buffer];
    auto &query_id = timer_query_data->query_ids[buffer][query_count];
    timer_query_data->guid[buffer][query_count] = guid;
    hglBeginQuery(GL_TIME_ELAPSED, query_id);
    ++query_count;
}

inline void
CollectAndSwapTimers(TimerQueryData *timer_query_data)
{
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
TexArray1ShaderConfig
{
    Plane clipping_plane;
    int32_t use_clipping_plane;
};

struct
TexArray1WaterShaderConfig
{
    v3 camera_position;
    int reflection_texaddress_index;
    int refraction_texaddress_index;
    int dudv_map_texaddress_index;
    int normal_map_texaddress_index;
    float tiling;

    float wave_strength;
    float move_factor;
    float minimum_variance;
    float bias;
    float lightbleed_compensation;
};

union
ShaderConfig
{
    TexArray1ShaderConfig texarray_1_shaderconfig;
    TexArray1WaterShaderConfig texarray_1_water_shaderconfig;
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

#define TEXTURE_FORMATS \
    X(rgba8, GL_RGBA8, GL_RGBA) \
    X(srgba, GL_SRGB8_ALPHA8, GL_RGBA) \
    X(rg32f, GL_RG32F, GL_RG) \
    X(rgba16f, GL_RGBA16F, GL_RGBA)

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

#define NUM_TEXARRAY_TEXTURES 10
struct
CommandState
{
    VertexBuffer vertex_buffers[10];
    IndexBuffer index_buffers[10];

    TextureAddressBuffer texture_address_buffers[10];
    // new, draining?

    DrawDataBuffer draw_data_buffer;
    BoneDataBuffer bone_data_buffer;
    IndirectBuffer indirect_buffer;
    ShaderConfigBuffer shaderconfig_buffer;
    LightList light_list;
    LightsBuffer lights_buffer;
    CommandLists lists;

    uint32_t textures[NUM_TEXARRAY_TEXTURES];
    TextureConfig texture_configs[NUM_TEXARRAY_TEXTURES];
    uint32_t texture_layer_count[NUM_TEXARRAY_TEXTURES];

    uint32_t texture_address_count;
    TextureAddress texture_addresses[100];
    uint32_t vao;
};

struct renderer_state
{
    bool initialized;
    CommandState command_state;

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
    uint32_t texarray_1_shaderconfig;
    int32_t texarray_1_texcontainer;
    int32_t texarray_1_shadowmap_tex_id;
    int32_t texarray_1_shadowmap_color_tex_id;

    texarray_1_vsm_program_struct texarray_1_vsm_program;
    uint32_t texarray_1_vsm_cb0;
    uint32_t texarray_1_vsm_cb1;
    uint32_t texarray_1_vsm_cb3;
    int32_t texarray_1_vsm_texcontainer;

    texarray_1_water_program_struct texarray_1_water_program;
    uint32_t texarray_1_water_cb0;
    uint32_t texarray_1_water_cb1;
    uint32_t texarray_1_water_cb2;
    uint32_t texarray_1_water_shaderconfig;
    int32_t texarray_1_water_texcontainer;

    filter_gaussian_7x1_program_struct filter_gaussian_7x1_program;
    uint32_t filter_gaussian_7x1_cb0;
    int32_t filter_gaussian_7x1_texcontainer;

    sky_program_struct sky_program;
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

    ui2d_state ui_state;

    char bitmap_scratch[4096 * 4096 * 4];
    uint8_t asset_scratch[4096 * 4096 * 4];
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

    asset_file asset_files[128];
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
lookup_asset_file_to_texture(renderer_state *state, int32_t asset_file_id)
{
    int32_t result = -1;
    for (uint32_t i = 0; i < state->texture_lookup_count; ++i)
    {
        asset_file_to_texture *lookup = state->texture_lookup + i;
        if (lookup->asset_file_id == asset_file_id)
        {
            result = lookup->texture_offset;
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

static renderer_state _GlobalRendererState;

void
draw_ui2d(renderer_state *state, ui2d_push_context *pushctx)
{
    ui2d_state *ui_state = &state->ui_state;
    ui2d_program_struct *ui2d_program = &ui_state->ui2d_program;

    hglDisable(GL_DEPTH_TEST);
    hglDepthFunc(GL_ALWAYS);
    hglUseProgram(ui2d_program->program);
    hglBindVertexArray(ui_state->vao);

    float screen_pixel_size[] =
    {
        (float)state->input->window.width,
        (float)state->input->window.height,
    };
    hglUniform2fv(ui2d_program->screen_pixel_size_id, 1, (float *)&screen_pixel_size);

    hglBindBuffer(GL_ARRAY_BUFFER, ui_state->vbo);
    hglBufferData(GL_ARRAY_BUFFER,
            (GLsizei)(pushctx->num_vertices * sizeof(pushctx->vertices[0])),
            pushctx->vertices,
            GL_DYNAMIC_DRAW);

    hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_state->ibo);
    hglBufferData(GL_ELEMENT_ARRAY_BUFFER,
            (GLsizei)(pushctx->num_elements * sizeof(pushctx->elements[0])),
            pushctx->elements,
            GL_DYNAMIC_DRAW);

    hglEnableVertexAttribArray((GLuint)ui2d_program->a_pos_id);
    hglEnableVertexAttribArray((GLuint)ui2d_program->a_tex_coord_id);
    hglEnableVertexAttribArray((GLuint)ui2d_program->a_texid_id);
    hglEnableVertexAttribArray((GLuint)ui2d_program->a_options_id);
    hglEnableVertexAttribArray((GLuint)ui2d_program->a_channel_color_id);
    hglVertexAttribPointer((GLuint)ui2d_program->a_pos_id, 2, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)0);
    hglVertexAttribPointer((GLuint)ui2d_program->a_tex_coord_id, 2, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, tex_coord));
    hglVertexAttribPointer((GLuint)ui2d_program->a_texid_id, 1, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, texid));
    hglVertexAttribPointer((GLuint)ui2d_program->a_options_id, 1, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, options));
    hglVertexAttribPointer((GLuint)ui2d_program->a_channel_color_id, 3, GL_FLOAT, GL_FALSE, sizeof(ui2d_vertex_format), (void *)offsetof(ui2d_vertex_format, channel_color));

    GLint uniform_locations[10] = {};
    char msg[] = "tex[xx]";
    for (int idx = 0; idx < harray_count(uniform_locations); ++idx)
    {
        sprintf(msg, "tex[%d]", idx);
        uniform_locations[idx] = hglGetUniformLocation(ui2d_program->program, msg);

    }

    for (uint32_t i = 0; i < pushctx->num_textures; ++i)
    {
        hglActiveTexture(GL_TEXTURE0 + i);
        hglBindTexture(GL_TEXTURE_2D, pushctx->textures[i]);
        hglUniform1i(
            uniform_locations[i],
            (GLint)i);
    }

    // TODO(nbm): Get textures working properly.
    hglActiveTexture(GL_TEXTURE0);
    hglBindTexture(GL_TEXTURE_2D, ui_state->mouse_texture);
    hglUniform1i(uniform_locations[0], 0);

    hglDrawElements(GL_TRIANGLES, (GLsizei)pushctx->num_elements, GL_UNSIGNED_INT, (void *)0);
    if (state->crash_on_gl_errors) hglErrorAssert();
}

bool
load_texture_asset(
    renderer_state *state,
    const char *filename,
    uint8_t *image_buffer,
    uint32_t image_size,
    int32_t *x,
    int32_t *y,
    GLuint *tex,
    GLenum target
)
{
    if (!_platform->load_asset(filename, image_size, image_buffer)) {
        return false;
    }

    uint32_t actual_size;
    load_image(image_buffer, image_size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch),
            x, y, &actual_size);

    if (tex)
    {
        if (!*tex)
        {
            hglGenTextures(1, tex);
        }
        hglBindTexture(GL_TEXTURE_2D, *tex);
    }

    hglTexImage2D(target, 0, GL_SRGB8_ALPHA8,
        *x, *y, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, state->bitmap_scratch);
    hglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    hglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    hglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    hglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return true;
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
    auto &layer = command_state->texture_layer_count[container_index];

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
    const char *filename,
    uint8_t *image_buffer,
    uint32_t image_size,
    int32_t *x,
    int32_t *y,
    uint32_t *texaddress_index
)
{
    if (!_platform->load_asset(filename, image_size, image_buffer)) {
        return false;
    }

    uint32_t actual_size;
    load_image(image_buffer, image_size, (uint8_t *)state->bitmap_scratch, sizeof(state->bitmap_scratch),
            x, y, &actual_size);

    auto &command_state = state->command_state;
    TextureFormat format = TextureFormat::srgba;
    uint32_t container_index = container_index_for_size(&command_state, *x, *y, format);
    auto &tex = command_state.textures[container_index];
    GLuint lod = 0;

    auto &layer = command_state.texture_layer_count[container_index];
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
    char *filename
)
{
    char msg[1024];
    sprintf(msg, "Could not load %s\n", filename);
    _platform->fail(msg);
    _platform->quit = true;
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
    if (!_platform->load_asset(filename, mesh_buffer_size, mesh_buffer)) {
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
        uint32_t normals_offset;
        uint32_t normals_size;
        uint32_t texcoords_offset;
        uint32_t texcoords_size;
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

    if (version->version == 1)
    {
        struct binary_format_v1
        {
            uint32_t vertices_offset;
            uint32_t vertices_size;
            uint32_t normals_offset;
            uint32_t normals_size;
            uint32_t texcoords_offset;
            uint32_t texcoords_size;
            uint32_t indices_offset;
            uint32_t indices_size;
            uint32_t num_triangles;
        } *v1_format = (binary_format_v1 *)mesh_buffer;
        format.vertices_offset = v1_format->vertices_offset;
        format.vertices_size = v1_format->vertices_size;
        format.normals_offset = v1_format->normals_offset;
        format.normals_size = v1_format->normals_size;
        format.texcoords_offset = v1_format->texcoords_offset;
        format.texcoords_size = v1_format->texcoords_size;
        format.indices_offset = v1_format->indices_offset;
        format.indices_size = v1_format->indices_size;
        format.num_triangles = v1_format->num_triangles;
        mesh_buffer += sizeof(binary_format_v1);
        mesh->num_triangles = format.num_triangles;
        mesh->num_bones = format.num_bones;
    }
    else if (version->version == 2)
    {
        struct binary_format_v2
        {
            uint32_t header_size;
            uint32_t vertices_offset;
            uint32_t vertices_size;
            uint32_t normals_offset;
            uint32_t normals_size;
            uint32_t texcoords_offset;
            uint32_t texcoords_size;
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
        } *v2_format = (binary_format_v2 *)mesh_buffer;
        format.vertices_offset = v2_format->vertices_offset;
        format.vertices_size = v2_format->vertices_size;
        format.normals_offset = v2_format->normals_offset;
        format.normals_size = v2_format->normals_size;
        format.texcoords_offset = v2_format->texcoords_offset;
        format.texcoords_size = v2_format->texcoords_size;
        format.indices_offset = v2_format->indices_offset;
        format.indices_size = v2_format->indices_size;
        format.num_triangles = v2_format->num_triangles;
        format.num_bones = v2_format->num_bones;
        format.bone_names_offset = v2_format->bone_names_offset;
        format.bone_names_size = v2_format->bone_names_size;
        format.bone_ids_offset = v2_format->bone_ids_offset;
        format.bone_ids_size = v2_format->bone_ids_size;
        format.bone_weights_offset = v2_format->bone_weights_offset;
        format.bone_weights_size = v2_format->bone_weights_size;
        format.bone_parent_offset = v2_format->bone_parent_offset;
        format.bone_parent_size = v2_format->bone_parent_size;
        format.bone_offsets_offset = v2_format->bone_offsets_offset;
        format.bone_offsets_size = v2_format->bone_offsets_size;
        format.bone_default_transform_offset = v2_format->bone_default_transform_offset;
        format.bone_default_transform_size = v2_format->bone_default_transform_size;
        format.animation_offset = v2_format->animation_offset;
        format.animation_size = v2_format->animation_size;
        mesh_buffer += v2_format->header_size;
        mesh->num_triangles = format.num_triangles;
        mesh->num_bones = format.num_bones;
    }
    else if (version->version == 3)
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
        mesh->vertexformat = v3_format->vertexformat;
        mesh->v3_boneless.vertex_count = v3_format->num_vertices;
        mesh_buffer += v3_format->header_size;
        vertexformat_1 *vf = (vertexformat_1 *)mesh_buffer;
        vertexformat_1 vf0 = vf[0];
        vertexformat_1 vf1 = vf[1];
        vertexformat_1 vf2 = vf[2];
        vertexformat_1 vf3 = vf[3];
        vertexformat_1 vf4 = vf[4];
        vertexformat_1 vf5 = vf[5];
        vertexformat_1 vf6 = vf[6];
        mesh->num_triangles = format.num_triangles;
        mesh->num_bones = format.num_bones;
    }
    else
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
            uint32_t animation_offset;
            uint32_t animation_size;
        } *v3_format = (binary_format_v3_bones *)mesh_buffer;
        format.vertices_offset = v3_format->vertices_offset;
        format.vertices_size = v3_format->vertices_size;
        format.indices_offset = v3_format->indices_offset;
        format.indices_size = v3_format->indices_size;
        format.num_triangles = v3_format->num_triangles;
        mesh->mesh_format = MeshFormat::v3_bones;
        mesh->vertexformat = v3_format->vertexformat;
        mesh->v3_bones.vertex_count = v3_format->num_vertices;

        format.num_bones = v3_format->num_bones;
        format.bone_names_offset = v3_format->bone_names_offset;
        format.bone_names_size = v3_format->bone_names_size;
        format.bone_parent_offset = v3_format->bone_parent_offset;
        format.bone_parent_size = v3_format->bone_parent_size;
        format.bone_offsets_offset = v3_format->bone_offsets_offset;
        format.bone_offsets_size = v3_format->bone_offsets_size;
        format.bone_default_transform_offset = v3_format->bone_default_transform_offset;
        format.bone_default_transform_size = v3_format->bone_default_transform_size;
        format.animation_offset = v3_format->animation_offset;
        format.animation_size = v3_format->animation_size;

        mesh_buffer += v3_format->header_size;
        vertexformat_2 *vf = (vertexformat_2 *)mesh_buffer;
        vertexformat_2 vf0 = vf[0];
        vertexformat_2 vf1 = vf[1];
        vertexformat_2 vf2 = vf[2];
        vertexformat_2 vf3 = vf[3];
        vertexformat_2 vf4 = vf[4];
        vertexformat_2 vf5 = vf[5];
        vertexformat_2 vf6 = vf[6];
        mesh->num_triangles = format.num_triangles;
        mesh->num_bones = format.num_bones;

        uint8_t *base_offset = mesh_buffer;

        uint32_t memory_size = format.vertices_size + format.normals_size + format.texcoords_size + format.indices_size;
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

        int32_t *bone_parents = (int32_t *)(base_offset + format.bone_parent_offset);
        m4 *bone_offsets = (m4 *)(base_offset + format.bone_offsets_offset);
        MeshBoneDescriptor *default_transforms = (MeshBoneDescriptor *)(base_offset + format.bone_default_transform_offset);
        uint8_t *bone_name = base_offset + format.bone_names_offset;
        for (uint32_t i = 0; i < format.num_bones; ++i)
        {
            mesh->v3_bones.bone_parents[i] = bone_parents[i];
            mesh->v3_bones.bone_offsets[i] = bone_offsets[i];
            mesh->v3_bones.default_transforms[i] = default_transforms[i];
            uint8_t bone_name_length = *bone_name;
            snprintf(mesh->v3_bones.bone_names[i], bone_name_length, "%*s", bone_name_length, bone_name + 1);

            bone_name += bone_name_length + 1;
        }

        mesh->v3_bones.num_ticks = 0;
        if (format.animation_size)
        {
            BoneAnimationHeader *bone_animation_header = (BoneAnimationHeader *)(base_offset + format.animation_offset);
            AnimTick *anim_tick = (AnimTick *)(bone_animation_header + 1);
            for (uint32_t i = 0; i < bone_animation_header->num_ticks; ++i)
            {
                for (uint32_t j = 0; j < format.num_bones; ++j)
                {
                    mesh->v3_bones.animation_ticks[i][j].transform = anim_tick->transform;
                    ++anim_tick;
                }
            }
            mesh->v3_bones.num_ticks = (uint32_t)bone_animation_header->num_ticks;
        }
        return true;
    }

    uint8_t *base_offset = mesh_buffer;

    uint32_t memory_size = format.vertices_size + format.normals_size + format.texcoords_size + format.indices_size;
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

    if (format.texcoords_size)
    {
        memcpy((void *)(m + offset), base_offset + format.texcoords_offset, format.texcoords_size);
        mesh->legacy.uvs = {
            (void *)(m + offset),
            format.texcoords_size,
        };
        offset += format.texcoords_size;
    }

    if (format.normals_size)
    {
        memcpy((void *)(m + offset), base_offset + format.normals_offset, format.normals_size);
        mesh->legacy.normals = {
            (void *)(m + offset),
            format.normals_size,
        };
        offset += format.normals_size;
    }

    if (format.bone_ids_size)
    {
        memcpy((void *)(m + offset), base_offset + format.bone_ids_offset, format.bone_ids_size);
        mesh->legacy.bone_ids = {
            (void *)(m + offset),
            format.bone_ids_size,
        };
        offset += format.bone_ids_size;
    }

    if (format.bone_weights_size)
    {
        memcpy((void *)(m + offset), base_offset + format.bone_weights_offset, format.bone_weights_size);
        mesh->legacy.bone_weights = {
            (void *)(m + offset),
            format.bone_weights_size,
        };
        offset += format.bone_weights_size;
    }

    if (format.num_bones)
    {
        int32_t *bone_parents = (int32_t *)(base_offset + format.bone_parent_offset);
        m4 *bone_offsets = (m4 *)(base_offset + format.bone_offsets_offset);
        MeshBoneDescriptor *default_transforms = (MeshBoneDescriptor *)(base_offset + format.bone_default_transform_offset);
        uint8_t *bone_name = base_offset + format.bone_names_offset;
        for (uint32_t i = 0; i < format.num_bones; ++i)
        {
            mesh->legacy.bone_parents[i] = bone_parents[i];
            mesh->legacy.bone_offsets[i] = bone_offsets[i];
            mesh->legacy.default_transforms[i] = default_transforms[i];
            uint8_t bone_name_length = *bone_name;
            snprintf(mesh->legacy.bone_names[i], bone_name_length, "%*s", bone_name_length, bone_name + 1);

            bone_name += bone_name_length + 1;
        }

        mesh->legacy.num_ticks = 0;
        if (format.animation_size)
        {
            BoneAnimationHeader *bone_animation_header = (BoneAnimationHeader *)(base_offset + format.animation_offset);
            AnimTick *anim_tick = (AnimTick *)(bone_animation_header + 1);
            for (uint32_t i = 0; i < bone_animation_header->num_ticks; ++i)
            {
                for (uint32_t j = 0; j < format.num_bones; ++j)
                {
                    mesh->legacy.animation_ticks[i][j].transform = anim_tick->transform;
                    ++anim_tick;
                }
            }
            mesh->legacy.num_ticks = (uint32_t)bone_animation_header->num_ticks;
        }
    }
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
    char *filename
)
{
    char msg[1024];
    sprintf(msg, "Could not load mesh %s\n", filename);
    _platform->fail(msg);
    _platform->quit = true;
}

bool
ui2d_program_init(renderer_state *state)
{
    ui2d_state *ui_state = &state->ui_state;
    ui2d_program_struct *program = &ui_state->ui2d_program;
    bool loaded = ui2d_program(program);
    if (!loaded)
    {
        load_program_failed("ui2d");
        return false;
    }

    hglGenBuffers(1, &ui_state->vbo);
    hglGenBuffers(1, &ui_state->ibo);

    hglGenVertexArrays(1, &ui_state->vao);
    hglBindVertexArray(ui_state->vao);

    int32_t x, y;
    char filename[] = "ui/slick_arrows/slick_arrow-delta.png";
    loaded = load_texture_asset(state, filename, state->asset_scratch, sizeof(state->asset_scratch), &x, &y, &state->ui_state.mouse_texture, GL_TEXTURE_2D);
    if (!loaded)
    {
        load_texture_asset_failed(filename);
        return false;
    }
    return true;
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

        hglGenTextures(1, &state->font_texture);
        hglBindTexture(GL_TEXTURE_2D, state->font_texture);
        hglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        hglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        hglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        io.Fonts->TexID = (void *)(intptr_t)state->font_texture;

        hglGenTextures(1, &state->white_texture);
        hglBindTexture(GL_TEXTURE_2D, state->white_texture);
        uint32_t color32 = 0xffffffff;
        hglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color32);
        state->imgui_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
        state->imgui_tex = hglGetUniformLocation(program->program, "tex");
        state->imgui_cb0 = hglGetUniformBlockIndex(program->program, "CB0");
        hglUniformBlockBinding(
            program->program,
            state->imgui_cb0,
            0);
    }
    hglFlush();
    hglErrorAssert();

    hglFlush();
    hglErrorAssert();

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
        state->texarray_1_shaderconfig = hglGetUniformBlockIndex(program->program, "SHADERCONFIG");
        hglUniformBlockBinding(
            state->texarray_1_program.program,
            state->texarray_1_shaderconfig,
            9);
        state->texarray_1_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
    }

    hglFlush();
    hglErrorAssert();

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
        state->texarray_1_vsm_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
    }

    hglFlush();
    hglErrorAssert();

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
        state->texarray_1_water_shaderconfig = hglGetUniformBlockIndex(program->program, "SHADERCONFIG");
        hglUniformBlockBinding(
            program->program,
            state->texarray_1_water_shaderconfig,
            9);
        state->texarray_1_water_texcontainer = hglGetUniformLocation(program->program, "TexContainer");
    }
    hglFlush();
    hglErrorAssert();

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
    hglFlush();
    hglErrorAssert();

    {
        sky_program_struct *program = &state->sky_program;
        loaded &= sky_program(program);
        populate_skybox(state, &state->skybox);
    }
    hglFlush();
    hglErrorAssert();

    auto &command_state = state->command_state;
    hglGenVertexArrays(1, &command_state.vao);
    hglGenBuffers(1, &command_state.vertex_buffers[0].vbo);
    hglGenBuffers(1, &command_state.index_buffers[0].ibo);
    hglGenBuffers(1, &command_state.vertex_buffers[1].vbo);
    hglGenBuffers(1, &command_state.index_buffers[1].ibo);
    hglGenBuffers(1, &command_state.texture_address_buffers[0].uniform_buffer.ubo);
    hglGenBuffers(1, &command_state.draw_data_buffer.ubo);
    hglGenBuffers(1, &command_state.bone_data_buffer.ubo);
    hglGenBuffers(1, &command_state.indirect_buffer.ubo);
    hglGenBuffers(1, &command_state.lights_buffer.ubo);
    hglGenBuffers(1, &command_state.shaderconfig_buffer.ubo);
    hglFlush();
    hglErrorAssert();

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.texture_address_buffers[0].uniform_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(command_state.texture_addresses), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);
    hglFlush();
    hglErrorAssert();

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.draw_data_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(DrawDataList), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);
    hglFlush();
    hglErrorAssert();

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.lights_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(LightList), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);
    hglFlush();
    hglErrorAssert();

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.shaderconfig_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderConfig), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);
    hglFlush();
    hglErrorAssert();

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.bone_data_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(BoneDataList), (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);

    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.indirect_buffer.ubo);
    hglBufferData(GL_UNIFORM_BUFFER, sizeof(DrawElementsIndirectCommand) * 100, (void *)0, GL_DYNAMIC_DRAW);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);
    hglFlush();
    hglErrorAssert();

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
add_asset_file_texture(renderer_state *state, int32_t asset_file_id)
{
    int32_t result = lookup_asset_file_to_texture(state, asset_file_id);
    hassert(state->texture_count < harray_count(state->textures));
    if (result < 0)
    {
        int32_t x, y;
        asset_file *asset_file0 = state->asset_files + asset_file_id;
        bool loaded = load_texture_asset(state, asset_file0->asset_file_path, state->asset_scratch, sizeof(state->asset_scratch), &x, &y, state->textures + state->texture_count, GL_TEXTURE_2D);
        if (!loaded)
        {
            load_texture_asset_failed(asset_file0->asset_file_path);
            return false;
        }
        result = (int32_t)state->texture_count;
        ++state->texture_count;
        add_asset_file_texture_lookup(state, asset_file_id, (int32_t)state->textures[result]);
    }
    return result;
}

int32_t
add_asset_file_texaddress_index(renderer_state *state, int32_t asset_file_id)
{
    int32_t result = lookup_asset_file_to_texaddress_index(state, asset_file_id);
    if (result < 0)
    {
        hassert(state->texaddress_count < harray_count(state->texaddresses));
        int32_t x, y;
        asset_file *asset_file0 = state->asset_files + asset_file_id;
        uint32_t texaddress_index;
        bool loaded = load_texture_array_asset(
            state,
            asset_file0->asset_file_path,
            state->asset_scratch,
            sizeof(state->asset_scratch),
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
            sizeof(state->asset_scratch),
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
_add_asset(renderer_state *state, AssetType type, const char *asset_name, int32_t asset_file_id)
{
    int32_t result = -1;
    hassert(state->asset_count < harray_count(state->assets));
    asset *asset0 = state->assets + state->asset_count;
    asset0->asset_id = 0;
    strcpy(asset0->asset_name, asset_name);
    asset0->asset_file_id = asset_file_id;
    result = (int32_t)state->asset_count;
    ++state->asset_count;
    return result;
}

int32_t
add_asset(renderer_state *state, const char *asset_name, int32_t asset_file_id, v2 st0, v2 st1)
{
    int32_t result = _add_asset(state, AssetType::texture, asset_name, asset_file_id);
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
add_mesh_asset(renderer_state *state, const char *asset_name, int32_t asset_file_id)
{
    int32_t result = _add_asset(state, AssetType::mesh, asset_name, asset_file_id);
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

int32_t
add_tilemap_asset(renderer_state *state, const char *asset_name, const char *asset_file_name, uint32_t width, uint32_t height, uint32_t tile_width, uint32_t tile_height, uint32_t spacing, uint32_t tile_id)
{
    uint32_t tiles_wide = (width + spacing) / (tile_width + spacing);
    uint32_t tile_x_position = tile_id % tiles_wide;
    uint32_t tile_y_position = tile_id / tiles_wide;
    float tile_x_base = tile_x_position * (tile_width + spacing) / (float)width;
    float tile_y_base = tile_y_position * (tile_height + spacing) / (float)height;
    float tile_x_offset = (float)(tile_width - 1) / width;
    float tile_y_offset = (float)(tile_height - 1) / height;
    v2 st0 = { tile_x_base + 0.001f, tile_y_base + tile_y_offset };
    v2 st1 = { tile_x_base + tile_x_offset, tile_y_base + 0.001f};

    return add_asset(state, asset_name, asset_file_name, st0, st1);
}

extern "C" RENDERER_SETUP(renderer_setup)
{
    static std::chrono::steady_clock::time_point last_frame_start_time = std::chrono::steady_clock::now();

    _platform = memory->platform_api;
    GlobalDebugTable = memory->debug_table;
    TIMED_FUNCTION();

    /*
#if !defined(NEEDS_EGL) && !defined(__APPLE__)
    if (!glCreateProgram)
    {
    }
#endif
    */
    memory->render_lists_count = 0;
    renderer_state *state = &_GlobalRendererState;

    if (_GlobalRendererState.initialized)
    {
        // OpenGL hooks for OBS, ..., can introduce errors between frames...
        if (state->crash_on_gl_errors) hglErrorAssert(true);
    }

    memory->imgui_state = ImGui::GetCurrentContext();
    if (!_GlobalRendererState.initialized)
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
            memory->shadowmap_size = 4096;
        }
        hglFlush();
        hglErrorAssert();
        bool loaded = program_init(&_GlobalRendererState);
        hglFlush();
        hglErrorAssert();
        hassert(loaded);
        hassert(ui2d_program_init(&_GlobalRendererState));
        hglFlush();
        hglErrorAssert();
        _GlobalRendererState.initialized = true;
        state->generation_id = 1;
        state->crash_on_gl_errors = 1;
        add_asset(state, "mouse_cursor_old", "ui/slick_arrows/slick_arrow-delta.png", {0.0f, 0.0f}, {1.0f, 1.0f});
        add_asset(state, "mouse_cursor", "testing/kenney/cursorSword_silver.png", {0.0f, 0.0f}, {1.0f, 1.0f});
        add_tilemap_asset(state, "sea_0", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 31);
        add_tilemap_asset(state, "ground_0", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 21);
        add_tilemap_asset(state, "sea_ground_br", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 13);
        add_tilemap_asset(state, "sea_ground_bl", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 14);
        add_tilemap_asset(state, "sea_ground_tr", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 33);
        add_tilemap_asset(state, "sea_ground_tl", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 34);
        add_tilemap_asset(state, "sea_ground_r", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 32);
        add_tilemap_asset(state, "sea_ground_l", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 30);
        add_tilemap_asset(state, "sea_ground_t", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 11);
        add_tilemap_asset(state, "sea_ground_b", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 51);
        add_tilemap_asset(state, "sea_ground_t_l", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 10);
        add_tilemap_asset(state, "sea_ground_t_r", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 12);
        add_tilemap_asset(state, "sea_ground_b_l", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 50);
        add_tilemap_asset(state, "sea_ground_b_r", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 52);
        add_tilemap_asset(state, "bottom_wall", "testing/kenney/RPGpack_sheet_2X.png", 2560, 1664, 128, 128, 0, 69);
        add_asset(state, "player", "testing/kenney/alienPink_stand.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_asset(state, "familiar_ship", "testing/kenney/shipBlue.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_asset(state, "familiar", "testing/kenney/alienBlue_stand.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "plane_mesh", "testing/plane.hjm");
        add_mesh_asset(state, "tree_mesh", "testing/low_poly_tree/tree.hjm");
        add_asset(state, "tree_texture", "testing/low_poly_tree/bake.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "horse_mesh", "testing/rigged_horse/horse.hjm");
        add_asset(state, "horse_texture", "testing/rigged_horse/bake.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "chest_mesh", "testing/chest/chest.hjm");
        add_asset(state, "chest_texture", "testing/chest/diffuse.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "konserian_mesh", "testing/konserian_swamptree/konserian.hjm");
        add_asset(state, "konserian_texture", "testing/konserian_swamptree/BAKE.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "cactus_mesh", "testing/cactus/cactus.hjm");
        add_asset(state, "cactus_texture", "testing/cactus/diffuse.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "kitchen_mesh", "testing/kitchen/kitchen.hjm");
        add_asset(state, "kitchen_texture", "testing/kitchen/BAKE.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "nature_pack_tree_mesh", "testing/kenney/Nature_Pack_3D/tree.hjm");
        add_asset(state, "nature_pack_tree_texture", "testing/kenney/Nature_Pack_3D/tree.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_asset(state, "another_ground_0", "testing/kenney/rpgTile039.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "cube_mesh", "testing/cube.hjm");
        add_asset(state, "cube_texture", "testing/wood-tex2a.png", {0.0f, 1.0f}, {1.0f, 0.0f});

        add_asset(state, "knp_palette", "testing/kenney/Nature_Pack_3D/palettised/palette.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "knp_Brown_Cliff_01", "testing/kenney/Nature_Pack_3D/Brown_Cliff_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_Bottom_01", "testing/kenney/Nature_Pack_3D/Brown_Cliff_Bottom_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_Bottom_Corner_01", "testing/kenney/Nature_Pack_3D/Brown_Cliff_Bottom_Corner_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_Bottom_Corner_Green_Top_01", "testing/kenney/Nature_Pack_3D/Brown_Cliff_Bottom_Corner_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_Bottom_Green_Top_01", "testing/kenney/Nature_Pack_3D/Brown_Cliff_Bottom_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_Corner_01", "testing/kenney/Nature_Pack_3D/Brown_Cliff_Corner_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_Corner_Green_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Brown_Cliff_Corner_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_End_01", "testing/kenney/Nature_Pack_3D/palettised/Brown_Cliff_End_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_End_Green_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Brown_Cliff_End_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_Green_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Brown_Cliff_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_Top_01", "testing/kenney/Nature_Pack_3D/Brown_Cliff_Top_01.hjm");
        add_mesh_asset(state, "knp_Brown_Cliff_Top_Corner_01", "testing/kenney/Nature_Pack_3D/Brown_Cliff_Top_Corner_01.hjm");
        add_mesh_asset(state, "knp_Brown_Waterfall_01", "testing/kenney/Nature_Pack_3D/palettised/Brown_Waterfall_01.hjm");
        add_mesh_asset(state, "knp_Brown_Waterfall_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Brown_Waterfall_Top_01.hjm");
        add_mesh_asset(state, "knp_Campfire_01", "testing/kenney/Nature_Pack_3D/palettised/Campfire_01.hjm");
        add_mesh_asset(state, "knp_Fallen_Trunk_01", "testing/kenney/Nature_Pack_3D/palettised/Fallen_Trunk_01.hjm");
        add_mesh_asset(state, "knp_Flower_Red_01", "testing/kenney/Nature_Pack_3D/palettised/Flower_Red_01.hjm");
        add_mesh_asset(state, "knp_Flower_Tall_Red_01", "testing/kenney/Nature_Pack_3D/palettised/Flower_Tall_Red_01.hjm");
        add_mesh_asset(state, "knp_Flower_Tall_Yellow_01", "testing/kenney/Nature_Pack_3D/palettised/Flower_Tall_Yellow_01.hjm");
        add_mesh_asset(state, "knp_Flower_Tall_Yellow_02", "testing/kenney/Nature_Pack_3D/palettised/Flower_Tall_Yellow_02.hjm");
        add_mesh_asset(state, "knp_Flower_Yellow_01", "testing/kenney/Nature_Pack_3D/palettised/Flower_Yellow_01.hjm");
        add_mesh_asset(state, "knp_Flower_Yellow_02", "testing/kenney/Nature_Pack_3D/palettised/Flower_Yellow_02.hjm");
        add_mesh_asset(state, "knp_Grass_01", "testing/kenney/Nature_Pack_3D/palettised/Grass_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_Bottom_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_Bottom_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_Bottom_Corner_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_Bottom_Corner_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_Bottom_Corner_Green_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_Bottom_Corner_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_Bottom_Green_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_Bottom_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_Corner_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_Corner_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_Corner_Green_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_Corner_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_End_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_End_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_End_Green_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_End_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_Green_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_Green_Top_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_Top_01.hjm");
        add_mesh_asset(state, "knp_Grey_Cliff_Top_Corner_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Cliff_Top_Corner_01.hjm");
        add_mesh_asset(state, "knp_Grey_Waterfall_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Waterfall_01.hjm");
        add_mesh_asset(state, "knp_Grey_Waterfall_Top_01", "testing/kenney/Nature_Pack_3D/palettised/Grey_Waterfall_Top_01.hjm");
        add_mesh_asset(state, "knp_Hanging_Moss_01", "testing/kenney/Nature_Pack_3D/palettised/Hanging_Moss_01.hjm");
        add_mesh_asset(state, "knp_Large_Oak_Dark_01", "testing/kenney/Nature_Pack_3D/Large_Oak_Dark_01.hjm");
        add_mesh_asset(state, "knp_Large_Oak_Fall_01", "testing/kenney/Nature_Pack_3D/Large_Oak_Fall_01.hjm");
        add_mesh_asset(state, "knp_Large_Oak_Green_01", "testing/kenney/Nature_Pack_3D/Large_Oak_Green_01.hjm");
        add_mesh_asset(state, "knp_Mushroom_Brown_01", "testing/kenney/Nature_Pack_3D/palettised/Mushroom_Brown_01.hjm");
        add_mesh_asset(state, "knp_Mushroom_Red_01", "testing/kenney/Nature_Pack_3D/palettised/Mushroom_Red_01.hjm");
        add_mesh_asset(state, "knp_Mushroom_Tall_01", "testing/kenney/Nature_Pack_3D/palettised/Mushroom_Tall_01.hjm");
        add_mesh_asset(state, "knp_Oak_Dark_01", "testing/kenney/Nature_Pack_3D/palettised/Oak_Dark_01.hjm");
        add_mesh_asset(state, "knp_Oak_Fall_01", "testing/kenney/Nature_Pack_3D/palettised/Oak_Fall_01.hjm");
        add_mesh_asset(state, "knp_Oak_Green_01", "testing/kenney/Nature_Pack_3D/palettised/Oak_Green_01.hjm");
        add_mesh_asset(state, "knp_Plant_1_01", "testing/kenney/Nature_Pack_3D/palettised/Plant_1_01.hjm");
        add_mesh_asset(state, "knp_Plant_2_01", "testing/kenney/Nature_Pack_3D/palettised/Plant_2_01.hjm");
        add_mesh_asset(state, "knp_Plant_3_01", "testing/kenney/Nature_Pack_3D/palettised/Plant_3_01.hjm");
        add_mesh_asset(state, "knp_Plate_Grass_01", "testing/kenney/Nature_Pack_3D/Plate_Grass_01.hjm");
        add_mesh_asset(state, "knp_Plate_Grass_Dirt_01", "testing/kenney/Nature_Pack_3D/palettised/Plate_Grass_Dirt_01.hjm");
        add_mesh_asset(state, "knp_Plate_River_01", "testing/kenney/Nature_Pack_3D/palettised/Plate_River_01.hjm");
        add_mesh_asset(state, "knp_Plate_River_Corner_01", "testing/kenney/Nature_Pack_3D/palettised/Plate_River_Corner_01.hjm");
        add_mesh_asset(state, "knp_Plate_River_Corner_Dirt_01", "testing/kenney/Nature_Pack_3D/palettised/Plate_River_Corner_Dirt_01.hjm");
        add_mesh_asset(state, "knp_Plate_River_Dirt_01", "testing/kenney/Nature_Pack_3D/palettised/Plate_River_Dirt_01.hjm");
        add_mesh_asset(state, "knp_Rock_1_01", "testing/kenney/Nature_Pack_3D/palettised/Rock_1_01.hjm");
        add_mesh_asset(state, "knp_Rock_2_01", "testing/kenney/Nature_Pack_3D/palettised/Rock_2_01.hjm");
        add_mesh_asset(state, "knp_Rock_3_01", "testing/kenney/Nature_Pack_3D/palettised/Rock_3_01.hjm");
        add_mesh_asset(state, "knp_Rock_4_01", "testing/kenney/Nature_Pack_3D/palettised/Rock_4_01.hjm");
        add_mesh_asset(state, "knp_Rock_5_01", "testing/kenney/Nature_Pack_3D/palettised/Rock_5_01.hjm");
        add_mesh_asset(state, "knp_Rock_6_01", "testing/kenney/Nature_Pack_3D/palettised/Rock_6_01.hjm");
        add_mesh_asset(state, "knp_Tall_Rock_1_01", "testing/kenney/Nature_Pack_3D/palettised/Tall_Rock_1_01.hjm");
        add_mesh_asset(state, "knp_Tall_Rock_2_01", "testing/kenney/Nature_Pack_3D/palettised/Tall_Rock_2_01.hjm");
        add_mesh_asset(state, "knp_Tall_Rock_3_01", "testing/kenney/Nature_Pack_3D/palettised/Tall_Rock_3_01.hjm");
        add_mesh_asset(state, "knp_Tent_01", "testing/kenney/Nature_Pack_3D/palettised/Tent_01.hjm");
        add_mesh_asset(state, "knp_Tent_Poles_01", "testing/kenney/Nature_Pack_3D/palettised/Tent_Poles_01.hjm");
        add_mesh_asset(state, "knp_Tree_01", "testing/kenney/Nature_Pack_3D/palettised/Tree_01.hjm");
        add_mesh_asset(state, "knp_Tree_02", "testing/kenney/Nature_Pack_3D/palettised/Tree_02.hjm");
        add_mesh_asset(state, "knp_Trunk_01", "testing/kenney/Nature_Pack_3D/palettised/Trunk_01.hjm");
        add_mesh_asset(state, "knp_Trunk_02", "testing/kenney/Nature_Pack_3D/palettised/Trunk_02.hjm");
        add_mesh_asset(state, "knp_Trunk_Alt_01", "testing/kenney/Nature_Pack_3D/palettised/Trunk_Alt_01.hjm");
        add_mesh_asset(state, "knp_Trunk_Alt_02", "testing/kenney/Nature_Pack_3D/palettised/Trunk_Alt_02.hjm");
        add_mesh_asset(state, "knp_Water_lily_01", "testing/kenney/Nature_Pack_3D/palettised/Water_lily_01.hjm");
        add_mesh_asset(state, "knp_Wood_Fence_01", "testing/kenney/Nature_Pack_3D/palettised/Wood_Fence_01.hjm");
        add_mesh_asset(state, "knp_Wood_Fence_Broken_01", "testing/kenney/Nature_Pack_3D/palettised/Wood_Fence_Broken_01.hjm");
        add_mesh_asset(state, "knp_Wood_Fence_Gate_01", "testing/kenney/Nature_Pack_3D/palettised/Wood_Fence_Gate_01.hjm");

        add_mesh_asset(state, "kenney_blocky_advanced_mesh", "testing/kenney/blocky_advanced3.hjm");
        add_mesh_asset(state, "kenney_blocky_advanced_mesh2", "testing/kenney/blocky_advanced3.hjm");
        add_asset(state, "kenney_blocky_advanced_cowboy_texture", "testing/kenney/skin_exclusiveCowboy.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "blockfigureRigged6_mesh", "testing/human_low.hjm");
        add_asset(state, "blockfigureRigged6_texture", "testing/blockfigureRigged6.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "cube_bounds_mesh", "testing/cube_bounds.hjm");
        add_asset(state, "white_texture", "testing/white.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_mesh_asset(state, "dog2_mesh", "testing/dog2/dog2.hjm");
        add_asset(state, "dog2_texture", "testing/dog2/dog2Color.png", {0.0f, 1.0f}, {1.0f, 0.0f});

        add_asset(state, "water_normal", "testing/water_normal.png", {0.0f, 1.0f}, {1.0f, 0.0f});
        add_asset(state, "water_dudv", "testing/water_dudv.png", {0.0f, 1.0f}, {1.0f, 0.0f});

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

    _GlobalRendererState.input = input;

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
    io.KeyMap[ImGuiKey_Backspace] = 8;

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
                if (ki->ascii == 8)
                {
                     io.KeysDown[(uint32_t)ki->ascii] = true;
                }
                else
                {
                    io.AddInputCharacter((ImWchar)ki->ascii);
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
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    hglEnable(GL_BLEND);
    hglBlendEquation(GL_FUNC_ADD);
    hglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    hglDisable(GL_CULL_FACE);
    hglDisable(GL_DEPTH_TEST);
    hglEnable(GL_SCISSOR_TEST);
    hglActiveTexture(GL_TEXTURE0);
    hglBindTexture(GL_TEXTURE_2D, 0);
    hglUseProgram(state->imgui_program.program);

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
    hglBindVertexArray(state->vao);

    hglBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    hglVertexAttribPointer((GLuint)state->imgui_program.a_position_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, pos));
    hglVertexAttribPointer((GLuint)state->imgui_program.a_uv_id, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, uv));
    hglVertexAttribPointer((GLuint)state->imgui_program.a_color_id, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void *)offsetof(ImDrawVert, col));

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        hglBindBuffer(GL_ARRAY_BUFFER, state->vbo);
        hglBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(cmd_list->VtxBuffer.size() * sizeof(ImDrawVert)), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
        hglBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx)), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                hglBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
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


void
draw_quad(renderer_state *state, m4 *matrices, asset_descriptor *descriptors, render_entry_type_quad *quad)
{
    m4 projection = matrices[quad->matrix_id];

    uint32_t texture = state->white_texture;
    v2 st0 = {0, 0};
    v2 st1 = {1, 1};
    if (quad->asset_descriptor_id != -1)
    {
        asset_descriptor *descriptor = descriptors + quad->asset_descriptor_id;
        update_asset_descriptor_asset_id(state, descriptor);
        if (descriptor->asset_id >= 0)
        {
            asset *descriptor_asset = state->assets + descriptor->asset_id;
            st0 = descriptor_asset->st0;
            st1 = descriptor_asset->st1;
            int32_t asset_file_id = descriptor_asset->asset_file_id;
            int32_t texture_lookup = lookup_asset_file_to_texture(state, asset_file_id);
            if (texture_lookup >= 0)
            {
                texture = (uint32_t)texture_lookup;
            }
            else
            {
                int32_t texture_id = add_asset_file_texture(state, asset_file_id);
                if (texture_id >= 0)
                {
                    texture = (uint32_t)texture_id;
                }
            }
        }
    }

    hglUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, (float *)&projection);
    hglUniform1f(state->imgui_program.u_use_color_id, 1.0f);
    hglBindVertexArray(state->vao);

    uint32_t col = 0x00000000;
    col |= (uint32_t)(quad->color.w * 255.0f) << 24;
    col |= (uint32_t)(quad->color.z * 255.0f) << 16;
    col |= (uint32_t)(quad->color.y * 255.0f) << 8;
    col |= (uint32_t)(quad->color.x * 255.0f) << 0;
    hglBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    struct
    {
        v2 pos;
        v2 uv;
        uint32_t col;
    } vertices[] = {
        {
            { quad->position.x, quad->position.y, },
            { st0.x, st0.y },
            col,
        },
        {
            { quad->position.x + quad->size.x, quad->position.y, },
            { st1.x, st0.y },
            col,
        },
        {
            { quad->position.x + quad->size.x, quad->position.y + quad->size.y, },
            { st1.x, st1.y },
            col,
        },
        {
            { quad->position.x, quad->position.y + quad->size.y, },
            { st0.x, st1.y },
            col,
        },
    };
    hglBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    uint32_t indices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
    hglBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), (GLvoid*)indices, GL_STREAM_DRAW);

    hglBindTexture(GL_TEXTURE_2D, texture);
    hglDrawElements(GL_TRIANGLES, (GLsizei)harray_count(indices), GL_UNSIGNED_INT, (void *)0);

    hglBindVertexArray(0);
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

    if (asset_descriptor_id != -1)
    {
        asset_descriptor *descriptor = descriptors + asset_descriptor_id;
        switch (descriptor->type)
        {
            case asset_descriptor_type::name:
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
                        idx = add_asset_file_texaddress_index(state, asset_file_id);
                    }
                    if (idx >= 0)
                    {
                        *texaddress_index = idx;
                    }
                }
            } break;
            case asset_descriptor_type::framebuffer:
            {
                if (FramebufferInitialized(descriptor->framebuffer))
                {
                    *texaddress_index = (int32_t)descriptor->framebuffer->_texture;
                }
            } break;
            case asset_descriptor_type::framebuffer_depth:
            {
                //if (FramebufferInitialized(descriptor->framebuffer))
                //{
                //    *texture = descriptor->framebuffer->_renderbuffer;
                //}
                hassert(!"Not implemented");
            } break;
            case asset_descriptor_type::dynamic_mesh:
            {
                hassert(!"Not a texture asset type");
            } break;
            case asset_descriptor_type::dynamic_texture:
            {
                auto &command_state = state->command_state;
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

void
get_texture_id_from_asset_descriptor(
    renderer_state *state,
    asset_descriptor *descriptors,
    int32_t asset_descriptor_id,
    // out
    uint32_t *texture,
    v2 *st0,
    v2 *st1)
{
    TIMED_FUNCTION();
    *texture = state->white_texture;
    *st0 = {0, 0};
    *st1 = {1, 1};

    if (asset_descriptor_id != -1)
    {
        asset_descriptor *descriptor = descriptors + asset_descriptor_id;
        switch (descriptor->type)
        {
            case asset_descriptor_type::name:
            {
                update_asset_descriptor_asset_id(state, descriptor);
                if (descriptor->asset_id >= 0)
                {
                    asset *descriptor_asset = state->assets + descriptor->asset_id;
                    *st0 = descriptor_asset->st0;
                    *st1 = descriptor_asset->st1;
                    int32_t asset_file_id = descriptor_asset->asset_file_id;
                    int32_t texture_lookup = lookup_asset_file_to_texture(state, asset_file_id);
                    if (texture_lookup >= 0)
                    {
                        *texture = (uint32_t)texture_lookup;
                    }
                    else
                    {
                        int32_t texture_id = add_asset_file_texture(state, asset_file_id);
                        if (texture_id >= 0)
                        {
                            *texture = (uint32_t)texture_id;
                        }
                    }
                }
            } break;
            case asset_descriptor_type::framebuffer:
            {
                if (FramebufferInitialized(descriptor->framebuffer))
                {
                    *texture = descriptor->framebuffer->_texture;
                }
            } break;
            case asset_descriptor_type::framebuffer_depth:
            {
                if (FramebufferInitialized(descriptor->framebuffer))
                {
                    *texture = descriptor->framebuffer->_renderbuffer;
                }
            } break;
            case asset_descriptor_type::dynamic_mesh:
            {
                hassert(!"Not a texture asset type");
            } break;
            case asset_descriptor_type::dynamic_texture:
            {
                hassert(!"Not implemented");
            } break;
        }

    }
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
    if (asset_descriptor_id != -1)
    {
        asset_descriptor *descriptor = descriptors + asset_descriptor_id;
        switch (descriptor->type)
        {
            case asset_descriptor_type::framebuffer:
            case asset_descriptor_type::framebuffer_depth:
            case asset_descriptor_type::dynamic_texture:
            {
                hassert(!"Not a mesh asset");

            } break;
            case asset_descriptor_type::name:
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
            case asset_descriptor_type::dynamic_mesh:
            {
                mesh = (Mesh *)descriptor->ptr;
                hassert(mesh->dynamic);
                result = true;
            } break;
        }

        if (result)
        {
            descriptor->load_state = 2;
            descriptor->mesh_data.num_bones = mesh->num_bones;
            switch (mesh->mesh_format)
            {
                case MeshFormat::v3_bones:
                {
                    descriptor->mesh_data.num_ticks = mesh->v3_bones.num_ticks;
                    descriptor->mesh_data.bone_offsets = mesh->v3_bones.bone_offsets;
                    descriptor->mesh_data.bone_parents = mesh->v3_bones.bone_parents;
                    descriptor->mesh_data.default_transforms = mesh->v3_bones.default_transforms;
                    descriptor->mesh_data.animation_ticks = (AnimTick *)mesh->v3_bones.animation_ticks;
                    descriptor->mesh_data.num_animtick_ticks = harray_count(mesh->v3_bones.animation_ticks);
                    descriptor->mesh_data.num_animtick_bones = harray_count(mesh->v3_bones.animation_ticks[0]);

                    descriptor->mesh_data.bone_names = (char *)mesh->v3_bones.bone_names;
                    descriptor->mesh_data.num_bonename_bones = harray_count(mesh->v3_bones.bone_names);
                    descriptor->mesh_data.num_bonename_chars = harray_count(mesh->v3_bones.bone_names[0]);

                    int32_t first_bone = 0;
                    for (uint32_t i = 0; i < mesh->num_bones; ++i)
                    {
                        if (mesh->v3_bones.bone_parents[i] == -1)
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

                        int32_t parent_bone = mesh->v3_bones.bone_parents[bone];
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

                        m4 scale = m4scale(mesh->v3_bones.default_transforms[bone].scale);
                        m4 translate = m4translate(mesh->v3_bones.default_transforms[bone].translate);
                        m4 rotate = m4rotation(mesh->v3_bones.default_transforms[bone].q);
                        m4 local_matrix = m4mul(translate,m4mul(rotate, scale));


                        m4 global_transform = m4mul(parent_matrix, local_matrix);

                        parent_list[parent_list_location] = {
                            bone,
                            global_transform,
                        };

                        mesh->v3_bones.default_bones[bone] = m4mul(global_transform, mesh->v3_bones.bone_offsets[bone]);

                        for (uint32_t i = 0; i < mesh->num_bones; ++i)
                        {
                            if (mesh->v3_bones.bone_parents[i] == bone)
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
                    descriptor->mesh_data.num_ticks = mesh->legacy.num_ticks;
                    descriptor->mesh_data.bone_offsets = mesh->legacy.bone_offsets;
                    descriptor->mesh_data.bone_parents = mesh->legacy.bone_parents;
                    descriptor->mesh_data.default_transforms = mesh->legacy.default_transforms;
                    descriptor->mesh_data.animation_ticks = (AnimTick *)mesh->legacy.animation_ticks;
                    descriptor->mesh_data.num_animtick_ticks = harray_count(mesh->legacy.animation_ticks);
                    descriptor->mesh_data.num_animtick_bones = harray_count(mesh->legacy.animation_ticks[0]);

                    descriptor->mesh_data.bone_names = (char *)mesh->legacy.bone_names;
                    descriptor->mesh_data.num_bonename_bones = harray_count(mesh->legacy.bone_names);
                    descriptor->mesh_data.num_bonename_chars = harray_count(mesh->legacy.bone_names[0]);

                    int32_t first_bone = 0;
                    for (uint32_t i = 0; i < mesh->num_bones; ++i)
                    {
                        if (mesh->legacy.bone_parents[i] == -1)
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

                        int32_t parent_bone = mesh->legacy.bone_parents[bone];
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

                        m4 scale = m4scale(mesh->legacy.default_transforms[bone].scale);
                        m4 translate = m4translate(mesh->legacy.default_transforms[bone].translate);
                        m4 rotate = m4rotation(mesh->legacy.default_transforms[bone].q);
                        m4 local_matrix = m4mul(translate,m4mul(rotate, scale));


                        m4 global_transform = m4mul(parent_matrix, local_matrix);

                        parent_list[parent_list_location] = {
                            bone,
                            global_transform,
                        };

                        mesh->legacy.default_bones[bone] = m4mul(global_transform, mesh->legacy.bone_offsets[bone]);

                        for (uint32_t i = 0; i < mesh->num_bones; ++i)
                        {
                            if (mesh->legacy.bone_parents[i] == bone)
                            {
                                stack_location++;
                                stack[stack_location] = (int32_t)i;
                            }
                        }
                    }

                } break;
            }
        }
    }
    return mesh;
}

void
draw_quads(renderer_state *state, m4 *matrices, asset_descriptor *descriptors, render_entry_type_QUADS *quads)
{
    if (state->crash_on_gl_errors) hglErrorAssert();
    m4 projection = matrices[quads->matrix_id];

    v2 st0 = {};
    v2 st1 = {};
    int32_t texture = 0;
    bool use_texaddress = false;
    if (quads->asset_descriptor_id >= 0)
    {
        auto &descriptor = descriptors[quads->asset_descriptor_id];
        if (descriptor.type == asset_descriptor_type::framebuffer)
        {
            use_texaddress = true;
        }
    }

    if (use_texaddress)
    {
        get_texaddress_index_from_asset_descriptor(
            state,
            descriptors,
            quads->asset_descriptor_id,
            &texture,
            &st0,
            &st1);
    }
    else
    {
        get_texture_id_from_asset_descriptor(
            state, descriptors, quads->asset_descriptor_id,
            (uint32_t *)&texture, &st0, &st1);
    }

    if (state->crash_on_gl_errors) hglErrorAssert();
    hglUseProgram(state->imgui_program.program);
    hglUniformMatrix4fv(state->imgui_program.u_projection_id, 1, GL_FALSE, (float *)&projection);
    hglUniformMatrix4fv(state->imgui_program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    hglUniformMatrix4fv(state->imgui_program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
    hglUniform1f(state->imgui_program.u_use_color_id, 1.0f);
    if (state->crash_on_gl_errors) hglErrorAssert();
    if (use_texaddress)
    {
        hglUniform1i(state->imgui_program.u_texaddress_index_id, texture);
    }
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

    auto &command_state = state->command_state;
    hglBindBufferBase(
        GL_UNIFORM_BUFFER,
        0,
        command_state.texture_address_buffers[0].uniform_buffer.ubo);
    hglBindBuffer(GL_UNIFORM_BUFFER, command_state.texture_address_buffers[0].uniform_buffer.ubo);
    GLvoid* p = hglMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    memcpy(p, command_state.texture_addresses, sizeof(command_state.texture_addresses));
    hglUnmapBuffer(GL_UNIFORM_BUFFER);
    hglBindBuffer(GL_UNIFORM_BUFFER, 0);
    if (state->crash_on_gl_errors) hglErrorAssert();

    if (state->crash_on_gl_errors) hglErrorAssert();
    hglUniform1i(state->imgui_tex, 0);
    hglActiveTexture(GL_TEXTURE0);

    if (!use_texaddress)
    {
        hglBindTexture(GL_TEXTURE_2D, texture);
    }
    if (state->crash_on_gl_errors) hglErrorAssert();
    static int32_t texcontainer_textures[harray_count(command_state.textures)] =
    {
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
    };

    hglUniform1iv(state->imgui_texcontainer, harray_count(texcontainer_textures), texcontainer_textures);
    if (state->crash_on_gl_errors) hglErrorAssert();
    for (uint32_t i = 0; i < harray_count(command_state.textures); ++i)
    {
        hglActiveTexture(GL_TEXTURE0 + texcontainer_textures[i]);
        hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[i]);
    }
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
    float denom = 1.0f / v3dot(n1, n2n3);

    return v3mul(top, denom);
}

void
draw_mesh_from_asset_v3_bones(
    renderer_state *state,
    m4 *matrices,
    asset_descriptor *descriptors,
    LightDescriptor *light_descriptors,
    ArmatureDescriptor *armature_descriptors,
    render_entry_type_mesh_from_asset *mesh_from_asset,
    Mesh *mesh
)
{
    TIMED_FUNCTION();
    if (state->crash_on_gl_errors) hglErrorAssert();
    auto &command_state = state->command_state;

    bool debug = false;
    if (mesh_from_asset->mesh_asset_descriptor_id == 997)
    {
        debug = true;
    }

    if (!mesh->loaded)
    {
        uint32_t vertex_buffer_id = 1;
        uint32_t index_buffer_id = 1;
        auto &vertex_buffer = command_state.vertex_buffers[vertex_buffer_id];
        auto &index_buffer = command_state.index_buffers[index_buffer_id];
        uint32_t vertex_base = vertex_buffer.vertex_count;
        uint32_t index_offset = index_buffer.index_count;
        mesh->v3_bones.vertex_buffer = vertex_buffer_id;
        mesh->v3_bones.vertex_base = vertex_base;
        mesh->v3_bones.index_buffer = index_buffer_id;
        mesh->v3_bones.index_offset = index_offset;
        hglBindBuffer(GL_ARRAY_BUFFER,
            vertex_buffer.vbo);
        if (!vertex_buffer.max_vertices)
        {
            vertex_buffer.vertex_size = sizeof(vertexformat_2);
            vertex_buffer.max_vertices = 100000;
            hglBufferData(GL_ARRAY_BUFFER,
                vertex_buffer.vertex_size * vertex_buffer.max_vertices,
                (void *)0,
                GL_DYNAMIC_DRAW);
        }
        hassert(vertex_buffer.vertex_size * mesh->v3_bones.vertex_count == mesh->vertices.size);
        hglBufferSubData(GL_ARRAY_BUFFER,
            vertex_buffer.vertex_size * vertex_buffer.vertex_count,
            vertex_buffer.vertex_size * mesh->v3_bones.vertex_count,
            mesh->vertices.data);
        vertex_buffer.vertex_count += mesh->v3_bones.vertex_count;
        hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
            index_buffer.ibo);
        if (!index_buffer.max_indices)
        {
            index_buffer.max_indices = 100000;
            hglBufferData(GL_ELEMENT_ARRAY_BUFFER,
                3 * 4 * index_buffer.max_indices,
                (void *)0,
                GL_DYNAMIC_DRAW);
        }
        hassert(4 * mesh->num_triangles * 3 == mesh->indices.size);
        hglBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
            4 * index_buffer.index_count,
            4 * mesh->num_triangles * 3,
            mesh->indices.data);
        index_buffer.index_count += mesh->num_triangles * 3;
        mesh->loaded = true;
    }

    CommandKey key = {};
    key.shader_type = mesh_from_asset->shader_type;
    key.vertexformat = mesh->vertexformat;
    key.vertex_buffer = mesh->v3_bones.vertex_buffer;
    key.index_buffer = mesh->v3_bones.index_buffer;
    key.count = 0;

    enum struct
    _SearchResult
    {
        exhausted,
        found,
        notfound,
    };
    _SearchResult search_result = _SearchResult::notfound;
    int32_t key_index = -1;
    auto &command_lists = state->command_state.lists;
    for (uint32_t i = 0; i < command_lists.num_command_lists; ++i)
    {
        key_index = (int32_t)i;
        if (command_lists.keys[i] == key)
        {
            auto &command_list = command_lists.command_lists[i];
            if (command_list.num_commands < harray_count(command_list.commands))
            {
                search_result = _SearchResult::found;
                break;
            }
        }
    }
    if (search_result != _SearchResult::found)
    {
        if (key_index < (int32_t)harray_count(command_lists.keys) - 1)
        {
            ++key_index;
            ++command_lists.num_command_lists;
            command_lists.keys[key_index] = key;
            search_result = _SearchResult::found;
        }
        else
        {
            search_result = _SearchResult::exhausted;
            hassert(!"Not enough space for new command list");
        }
    }
    auto &command_list = command_lists.command_lists[key_index];

    auto &cmd = command_list.commands[command_list.num_commands];
    cmd.count = 3 * mesh->num_triangles; // number of triangles * 3
    cmd.primCount = 1; // number of instances
    cmd.firstIndex = mesh->v3_bones.index_offset;
    cmd.baseVertex = (int32_t)mesh->v3_bones.vertex_base;
    cmd.baseInstance = 0;

    //key.shadowmap_color_asset_descriptor_id = light.shadowmap_color_asset_descriptor_id;
    v2 st0 = {};
    v2 st1 = {};
    int32_t texture_texaddress_index = -1;
    if (debug)
    {
        hassert(debug);
    }
    get_texaddress_index_from_asset_descriptor(
        state,
        descriptors,
        mesh_from_asset->texture_asset_descriptor_id,
        &texture_texaddress_index,
        &st0,
        &st1);
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
    m4 *bones = mesh->v3_bones.default_bones;
    if (armature)
    {
        bones = armature->bones;
    }

    uint32_t bone_offset = command_list.num_bones;
    command_list.num_bones += mesh->num_bones;
    auto bone_data = command_list.bone_data_list.data + bone_offset;
    memcpy(
        (void *)bone_data,
        bones,
        sizeof(m4) * mesh->num_bones);

    auto &draw_data = command_list.draw_data_list.data[command_list.num_commands];
    draw_data.texture_texaddress_index = texture_texaddress_index;
    draw_data.shadowmap_texaddress_index = shadowmap_texaddress_index;
    draw_data.shadowmap_color_texaddress_index = shadowmap_color_texaddress_index;
    draw_data.projection = projection;
    draw_data.view = view;
    draw_data.model = model;
    draw_data.light_index = light_index;
    v3 camera_position = calculate_camera_position(view);
    draw_data.camera_position = camera_position;
    draw_data.bone_offset = (int32_t)bone_offset;

    ++command_list.num_commands;
    if (state->crash_on_gl_errors) hglErrorAssert();

    return;
}
void
draw_mesh_from_asset_v3_boneless(
    renderer_state *state,
    m4 *matrices,
    asset_descriptor *descriptors,
    LightDescriptor *light_descriptors,
    ArmatureDescriptor *armature_descriptors,
    render_entry_type_mesh_from_asset *mesh_from_asset,
    Mesh *mesh
)
{
    TIMED_FUNCTION();
    auto &command_state = state->command_state;

    bool debug = false;
    if (mesh_from_asset->mesh_asset_descriptor_id == 97)
    {
        debug = true;
    }

    if (!mesh->loaded)
    {
        uint32_t vertex_buffer_id = 0;
        uint32_t index_buffer_id = 0;
        auto &vertex_buffer = command_state.vertex_buffers[vertex_buffer_id];
        auto &index_buffer = command_state.index_buffers[index_buffer_id];
        uint32_t vertex_base = vertex_buffer.vertex_count;
        uint32_t index_offset = index_buffer.index_count;

        mesh->v3_boneless.vertex_buffer = vertex_buffer_id;
        mesh->v3_boneless.vertex_base = vertex_base;
        mesh->v3_boneless.index_buffer = index_buffer_id;
        mesh->v3_boneless.index_offset = index_offset;
        hglBindBuffer(GL_ARRAY_BUFFER,
            vertex_buffer.vbo);
        hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
            index_buffer.ibo);

        if (!vertex_buffer.max_vertices)
        {
            vertex_buffer.vertex_size = sizeof(vertexformat_1);
            vertex_buffer.max_vertices = 10000000;
            hglBufferData(GL_ARRAY_BUFFER,
                vertex_buffer.vertex_size * vertex_buffer.max_vertices,
                (void *)0,
                GL_DYNAMIC_DRAW);
        }

        if (!index_buffer.max_indices)
        {
            index_buffer.max_indices = 10000000;
            hglBufferData(GL_ELEMENT_ARRAY_BUFFER,
                3 * 4 * index_buffer.max_indices,
                (void *)0,
                GL_DYNAMIC_DRAW);
        }
        mesh->loaded = true;
        mesh->reload = true;

        if (!mesh->dynamic_max_vertices)
        {
            vertex_buffer.vertex_count += mesh->v3_boneless.vertex_count;
        }
        else
        {
            vertex_buffer.vertex_count += mesh->dynamic_max_vertices;
        }
        if (!mesh->dynamic_max_triangles)
        {
            index_buffer.index_count += mesh->num_triangles * 3;
        }
        else
        {
            index_buffer.index_count += mesh->dynamic_max_triangles * 3;
        }
    }

    if (mesh->reload)
    {
        if (mesh->dynamic)
        {
            bool f = mesh->dynamic;
            (void)f;
        }
        uint32_t vertex_buffer_id = mesh->v3_boneless.vertex_buffer;
        uint32_t vertex_base = mesh->v3_boneless.vertex_base;
        uint32_t index_buffer_id = mesh->v3_boneless.index_buffer;
        uint32_t index_offset = mesh->v3_boneless.index_offset;

        auto &vertex_buffer = command_state.vertex_buffers[vertex_buffer_id];
        auto &index_buffer = command_state.index_buffers[index_buffer_id];

        hglBindBuffer(GL_ARRAY_BUFFER,
            vertex_buffer.vbo);
        hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
            index_buffer.ibo);

        if (!mesh->dynamic_max_vertices)
        {
            hassert(vertex_buffer.vertex_size * mesh->v3_boneless.vertex_count == mesh->vertices.size);
        }
        else
        {
            hassert(vertex_buffer.vertex_size * mesh->dynamic_max_vertices == mesh->vertices.size);
        }

        hglBufferSubData(GL_ARRAY_BUFFER,
            vertex_buffer.vertex_size * vertex_base,
            vertex_buffer.vertex_size * mesh->v3_boneless.vertex_count,
            mesh->vertices.data);

        if (!mesh->dynamic_max_triangles)
        {
            hassert(4 * mesh->num_triangles * 3 == mesh->indices.size);
        }
        else
        {
            hassert(4 * mesh->dynamic_max_triangles * 3 == mesh->indices.size);
        }

        hglBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
            4 * index_offset,
            4 * mesh->num_triangles * 3,
            mesh->indices.data);
        mesh->reload = false;
    }


    CommandKey key = {};
    key.shader_type = mesh_from_asset->shader_type;
    key.vertexformat = mesh->vertexformat;
    key.vertex_buffer = mesh->v3_boneless.vertex_buffer;
    key.index_buffer = mesh->v3_boneless.index_buffer;
    key.count = 0;

    enum struct
    _SearchResult
    {
        exhausted,
        found,
        notfound,
    };
    _SearchResult search_result = _SearchResult::notfound;
    int32_t key_index = -1;
    auto &command_lists = state->command_state.lists;
    for (uint32_t i = 0; i < command_lists.num_command_lists; ++i)
    {
        key_index = (int32_t)i;
        if (command_lists.keys[i] == key)
        {
            auto &command_list = command_lists.command_lists[i];
            if (command_list.num_commands < harray_count(command_list.commands))
            {
                search_result = _SearchResult::found;
                break;
            }
        }
    }
    if (search_result != _SearchResult::found)
    {
        if (key_index < (int32_t)harray_count(command_lists.keys) - 1)
        {
            ++key_index;
            ++command_lists.num_command_lists;
            command_lists.keys[key_index] = key;
            search_result = _SearchResult::found;
        }
        else
        {
            search_result = _SearchResult::exhausted;
            hassert(!"Not enough space for new command list");
        }
    }
    auto &command_list = command_lists.command_lists[key_index];

    auto &cmd = command_list.commands[command_list.num_commands];
    cmd.count = 3 * mesh->num_triangles; // number of triangles * 3
    cmd.primCount = 1; // number of instances
    cmd.firstIndex = mesh->v3_boneless.index_offset;
    cmd.baseVertex = (int32_t)mesh->v3_boneless.vertex_base;
    cmd.baseInstance = 0;

    //key.shadowmap_color_asset_descriptor_id = light.shadowmap_color_asset_descriptor_id;
    v2 st0 = {};
    v2 st1 = {};
    int32_t texture_texaddress_index = -1;
    if (debug)
    {
        hassert(debug);
    }
    get_texaddress_index_from_asset_descriptor(
        state,
        descriptors,
        mesh_from_asset->texture_asset_descriptor_id,
        &texture_texaddress_index,
        &st0,
        &st1);
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

    auto &draw_data = command_list.draw_data_list.data[command_list.num_commands];
    draw_data.texture_texaddress_index = texture_texaddress_index;
    draw_data.shadowmap_texaddress_index = shadowmap_texaddress_index;
    draw_data.shadowmap_color_texaddress_index = shadowmap_color_texaddress_index;
    draw_data.projection = projection;
    draw_data.view = view;
    draw_data.model = model;
    draw_data.light_index = light_index;
    v3 camera_position = calculate_camera_position(view);
    draw_data.camera_position = camera_position;
    draw_data.bone_offset = -1;

    ++command_list.num_commands;

    return;
}

void
draw_mesh_from_asset(
    renderer_state *state,
    m4 *matrices,
    asset_descriptor *descriptors,
    LightDescriptor *light_descriptors,
    ArmatureDescriptor *armature_descriptors,
    render_entry_type_mesh_from_asset *mesh_from_asset
)
{
    Mesh *mesh = get_mesh_from_asset_descriptor(state, descriptors, mesh_from_asset->mesh_asset_descriptor_id);
    switch(mesh->mesh_format)
    {
        case MeshFormat::v3_bones:
        {
            return draw_mesh_from_asset_v3_bones(
                state,
                matrices,
                descriptors,
                light_descriptors,
                armature_descriptors,
                mesh_from_asset,
                mesh);
        } break;
        case MeshFormat::v3_boneless:
        {
            return draw_mesh_from_asset_v3_boneless(
                state,
                matrices,
                descriptors,
                light_descriptors,
                armature_descriptors,
                mesh_from_asset,
                mesh);
        } break;
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
    auto &command_state = state->command_state;
    int32_t a_position_id = 0;
    int32_t a_texcoord_id = 0;

    v2 st0 = {0, 0};
    v2 st1 = {1, 1};
    int32_t texture;
    get_texaddress_index_from_asset_descriptor(
        state, descriptors, filter->source_asset_descriptor_id,
        &texture, &st0, &st1);

    int32_t texcontainer = -1;

    hglBindVertexArray(state->vao);
    switch (filter->type)
    {
        case ApplyFilterType::none:
        {
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
            auto &program = state->filter_gaussian_7x1_program;
            texcontainer = state->filter_gaussian_7x1_texcontainer;
            hglUseProgram(program.program);
            hglUniformMatrix4fv(program.u_projection_id, 1, GL_FALSE, (float *)&state->m4identity);
            hglUniformMatrix4fv(program.u_view_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
            hglUniformMatrix4fv(program.u_model_matrix_id, 1, GL_FALSE, (float *)&state->m4identity);
            a_position_id = program.a_position_id;
            a_texcoord_id = program.a_texcoord_id;
            v2 blur_scale = { 1.0f / state->blur_scale_divisor, 0 };
            if (filter->type == ApplyFilterType::gaussian_7x1_y)
            {
                blur_scale = { 0, 1.0f / state->blur_scale_divisor };
            }
            hglUniform2fv(program.u_blur_scale_id, 1, (float *)&blur_scale);
            hglUniform1i(program.u_texaddress_index_id, texture);
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
    auto &command_state = state->command_state;
    auto &command_lists = command_state.lists;

    hglDisable(GL_BLEND);
    hglEnable(GL_DEPTH_TEST);
    hglDepthFunc(GL_LESS);
    hglEnable(GL_CULL_FACE);

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
        },
        {
            state->texarray_1_vsm_texcontainer,
            state->texarray_1_vsm_program.u_draw_data_index_id,
            state->texarray_1_vsm_program.a_position_id,
            state->texarray_1_vsm_program.a_texcoord_id,
            -1,
            state->texarray_1_vsm_program.a_bone_ids_id,
            state->texarray_1_vsm_program.a_bone_weights_id,
        },
        {
            state->texarray_1_water_texcontainer,
            state->texarray_1_water_program.u_draw_data_index_id,
            state->texarray_1_water_program.a_position_id,
            -1
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
            hglCullFace(GL_BACK);
            shaderconfig.texarray_1_shaderconfig = {
                list->clipping_plane,
                (int32_t)list->flags.use_clipping_plane,
            };
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
            hglCullFace(GL_BACK);

            int32_t reflection_texaddress_index = - 1;
            int32_t refraction_texaddress_index = - 1;
            int32_t dudv_map_texaddress_index = - 1;
            int32_t normal_map_texaddress_index = - 1;
            v2 st0, st1;
            get_texaddress_index_from_asset_descriptor(
                state, descriptors,
                list->reflection_asset_descriptor,
                &reflection_texaddress_index, &st0, &st1);
            get_texaddress_index_from_asset_descriptor(
                state, descriptors,
                list->refraction_asset_descriptor,
                &refraction_texaddress_index, &st0, &st1);
            get_texaddress_index_from_asset_descriptor(
                state, descriptors,
                list->dudv_map_asset_descriptor,
                &dudv_map_texaddress_index, &st0, &st1);
            get_texaddress_index_from_asset_descriptor(
                state, descriptors,
                list->normal_map_asset_descriptor,
                &normal_map_texaddress_index, &st0, &st1);
            static float move_factor = 0.0f;
            move_factor += 0.0004f;
            shaderconfig.texarray_1_water_shaderconfig = {
                list->camera_position,
                reflection_texaddress_index,
                refraction_texaddress_index,
                dudv_map_texaddress_index,
                normal_map_texaddress_index,
                10.0f,
                0.02f,
                move_factor,
                state->vsm_minimum_variance,
                state->shadowmap_bias,
                state->vsm_lightbleed_compensation,
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
    hglBufferSubData(
        GL_UNIFORM_BUFFER,
        0,
        sizeof(command_state.texture_addresses),
        command_state.texture_addresses);

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

    if (list->flags.use_clipping_plane)
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
    };
    if (state->crash_on_gl_errors) hglErrorAssert();
    hglUniform1iv(pd.texcontainer_uniform, harray_count(texcontainer_textures), texcontainer_textures);
    for (uint32_t i = 0; i < harray_count(command_state.textures); ++i)
    {
        hglActiveTexture(GL_TEXTURE0 + i);
        hglBindTexture(GL_TEXTURE_2D_ARRAY, command_state.textures[i]);
    }

    hglActiveTexture(GL_TEXTURE0);
    if (state->crash_on_gl_errors) hglErrorAssert();

    for (uint32_t i = 0; i < command_lists.num_command_lists; ++i)
    {
        CommandKey &command_key = command_lists.keys[i];
        auto &command_list = command_lists.command_lists[i];

        if (!command_list.num_commands)
        {
            continue;
        }

        if (command_key.vertexformat > 2)
        {
            command_list.num_commands = 0;
            command_list.num_bones = 0;
            continue;
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
                case 1:
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
                case 2:
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

            }
            command_list._vao_initialized = true;
        }
        else
        {
            hglBindVertexArray(command_list.vao);
        }

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

        hglBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
            command_state.index_buffers[command_key.index_buffer].ibo);

        if (command_key.shader_type == ShaderType::water)
        {
            auto &dd = command_list.draw_data_list.data[0];
            v3 &camera_position = dd.camera_position;
            //camera_position.y *= -1.0f;
            ImGui::Text("Water camera position: %f %f %f",
                camera_position.x, camera_position.y, camera_position.z);

            ImGui::Text("light_index: %d", dd.light_index);
        }

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

        hglBindBuffer(GL_UNIFORM_BUFFER, 0);

        hglBindBuffer(GL_DRAW_INDIRECT_BUFFER, command_state.indirect_buffer.ubo);
        hglBufferSubData(
            GL_DRAW_INDIRECT_BUFFER,
            0,
            sizeof(command_list.commands),
            command_list.commands);
        if (state->gl_extensions.gl_arb_multi_draw_indirect && multi_draw_enabled)
        {
            hglUniform1i(pd.draw_data_index_uniform, 0);
            hglMultiDrawElementsIndirect(
                GL_TRIANGLES,
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
                    GL_TRIANGLES,
                    command.count,
                    GL_UNSIGNED_INT,
                    (void *)(intptr_t)(command.firstIndex * 4),
                    command.baseVertex);
                */
                /*
                hglDrawElementsInstancedBaseVertexBaseInstance(
                    GL_TRIANGLES,
                    command.count,
                    GL_UNSIGNED_INT,
                    (void *)(intptr_t)(command.firstIndex * 4),
                    command.primCount,
                    command.baseVertex,
                    0);
                */
                hglDrawElementsIndirect(
                    GL_TRIANGLES,
                    GL_UNSIGNED_INT,
                    (void *)(j * sizeof(DrawElementsIndirectCommand)));
            }
        }

        command_list.num_commands = 0;
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
    framebuffer->_texture = new_texaddress(&state->command_state, framebuffer_size.x, framebuffer_size.y, format);
}


void
framebuffer_initialize_depth_buffer_texarray(
    renderer_state *state,
    FramebufferDescriptor *framebuffer,
    FramebufferTextureConfig texture_config,
    v2i framebuffer_size,
    uint32_t multisample_samples)
{
    framebuffer->_renderbuffer = new_texaddress(&state->command_state, framebuffer_size.x, framebuffer_size.y, TextureFormat::rgba16f);
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
        hassert(!"bitrotted");
#if 0
        hglGenTextures(1, &framebuffer->_renderbuffer);
        if (texture_config.multisample)
        {
            hglBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebuffer->_renderbuffer);
            hglTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, multisample_samples, GL_DEPTH_COMPONENT16, framebuffer_size.x, framebuffer_size.y, true);
        }
        else
        {
            hglBindTexture(GL_TEXTURE_2D, framebuffer->_renderbuffer);
            hglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, framebuffer_size.x, framebuffer_size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            hglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            hglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            hglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            hglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
#endif
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
    auto &command_state = state->command_state;
    hglBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->_fbo);
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
        hassert(!"bitrotted");
#if 0
        if (texture_config.multisample)
        {
            hglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, framebuffer->_renderbuffer, 0);
            //hglFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, framebuffer->_renderbuffer, 0);
        }
        else
        {
            hglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebuffer->_renderbuffer, 0);
            //hglFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, framebuffer->_renderbuffer, 0);
        }
#endif
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
    }

    framebuffer_texture_attach(state, framebuffer, texture_config);

    hglViewport(0, 0, framebuffer_size.x, framebuffer_size.y);
    GLenum framebuffer_status = hglCheckFramebufferStatus(GL_FRAMEBUFFER);
    hassert(framebuffer_status == GL_FRAMEBUFFER_COMPLETE);
    *size = framebuffer_size;
    if (state->flush_for_profiling) hglFlush();
}

extern "C" RENDERER_RENDER(renderer_render)
{
    renderer_state *state = &_GlobalRendererState;
    {
        TIMED_BLOCK("Wait for vsync");
        if (state->flush_for_profiling) hglFlush();
        if (state->crash_on_gl_errors) hglErrorAssert();
    }
    TIMED_FUNCTION();

    _platform_memory = memory;

    ImGuiIO& io = ImGui::GetIO();
    generations_updated_this_frame = 0;

    hassert(memory->render_lists_count > 0);

    window_data *window = &state->input->window;
    if (state->crash_on_gl_errors) hglErrorAssert();

    uint32_t render_lists_to_process[20];
    uint32_t render_lists_to_process_count = 0;

    {
        TIMED_BLOCK("Render list sorting");
        while(render_lists_to_process_count < memory->render_lists_count)
        {
            for (uint32_t i = 0; i < memory->render_lists_count; ++i)
            {
                render_entry_list *render_list = memory->render_lists[i];
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
                    for (uint32_t j = 0; j < memory->render_lists_count; ++j)
                    {
                        uint32_t inverse = (uint32_t)~(1 << i);
                        render_entry_list *render_list_to_undepend = memory->render_lists[j];
                        render_list_to_undepend->depends_on_slots &= inverse;
                    }
                }
            }
        }
    }

    uint32_t quads_drawn = 0;
    uint32_t meshes_drawn[1000] = {};
    for (uint32_t ii = 0; ii < render_lists_to_process_count; ++ii)
    {
        render_entry_list *render_list = memory->render_lists[render_lists_to_process[ii]];
        {
            RecordDebugEvent(DebugType::BeginBlock, render_list->name);
        }

        RecordTimerQuery(&state->timer_query_data, render_list->name);

        v2i size = {window->width, window->height};
        hglActiveTexture(GL_TEXTURE0);
        hglBindTexture(GL_TEXTURE_2D, 0);
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
                case render_entry_type::ui2d:
                {
                    ExtractRenderElementWithSize(ui2d, item, header, element_size);
                    draw_ui2d(state, item->pushctx);
                    if (state->crash_on_gl_errors) hglErrorAssert();
                } break;
                case render_entry_type::quad:
                {
                    ExtractRenderElementWithSize(quad, item, header, element_size);
                    hassert(matrix_count > 0 || item->matrix_id == -1);
                    hassert(asset_descriptor_count > 0 || item->asset_descriptor_id == -1);
                    draw_quad(state, matrices, asset_descriptors, item);
                    ++quads_drawn;
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
                    ++meshes_drawn[item->mesh_asset_descriptor_id];
                    draw_mesh_from_asset(state, matrices, asset_descriptors, lights.descriptors, armatures.descriptors, item);
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
        hglEndQuery(GL_TIME_ELAPSED);
        END_BLOCK();
        if (state->crash_on_gl_errors) hglErrorAssert();
    }

    if (state->show_debug) {
        ImGui::Begin("Renderer debug", &state->show_debug);
        ImGui::Text("Quads drawn: %d", quads_drawn);
        ImGui::Text("Generations updated: %d", generations_updated_this_frame);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("This frame: %.3f ms", 1000.0f * io.DeltaTime);

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

    auto &command_state = state->command_state;
    for (uint32_t i = 0; i < harray_count(command_state.texture_configs); ++i)
    {
        TextureConfig ts = command_state.texture_configs[i];
        if (ts.width)
        {
            ImGui::Text(
                "Texture container %d, size %dx%d, format %s",
                i,
                ts.width,
                ts.height,
                texture_format_configs[(uint32_t)ts.format].name
                );
        }
    }

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Renderer"))
        {
            ImGui::MenuItem("Debug rendering", "", &state->show_debug);
            ImGui::MenuItem("Mesh animation debug", "", &state->show_animation_debug);
            ImGui::MenuItem("OpenGL debug", "", &state->show_gl_debug);
            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();


    RecordTimerQuery(&state->timer_query_data, DEBUG_NAME("render_draw_lists"));
    hglBindVertexArray(0);
    render_draw_lists(draw_data, state);
    hglEndQuery(GL_TIME_ELAPSED);

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
    (void)meshes_drawn;

    hglErrorAssert();
    return true;
};
