#version 410 core

#ifdef GL_ARB_shader_draw_parameters
#extension GL_ARB_shader_draw_parameters : enable
#endif

precision highp float;
precision highp int;
layout(std140, column_major) uniform;

in vec3 a_position;

out vec3 v_w_position;
out vec4 v_l_position;
out vec4 v_c_position;
out vec2 v_texcoord;
flat out uint v_draw_id;

struct
ShaderConfig
{
    vec3 camera_position;

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

layout(std140) uniform SHADERCONFIG
{
    ShaderConfig shader_config;
};

struct Tex2DAddress
{
    uint Container;
    float Page;
};

layout(std140) uniform CB0
{
    Tex2DAddress texAddress[100];
};

struct DrawData
{
    mat4 projection;
    mat4 view;
    mat4 model;
    int texture_texaddress_index;
    int shadowmap_texaddress_index;
    int shadowmap_color_texaddress_index;
    int light_index;
    vec3 camera_position;
    int bone_offset;
};

layout(std140) uniform CB1
{
    DrawData draw_data[100];
};

struct Light {
    vec4 position_or_direction;
    mat4 lightspace_matrix;
    vec3 color;

    float ambient_intensity;
    float diffuse_intensity;

    float attenuation_constant;
    float attenuation_linear;
    float attenuation_exponential;
};

layout(std140) uniform CB2
{
    Light lights[32];
};

uniform int u_draw_data_index;
 uniform bool do_not_skip = true;
 uniform bool skip = false;

void main()
{
    uint draw_data_index = u_draw_data_index;
#ifdef GL_ARB_shader_draw_parameters
    draw_data_index += gl_DrawIDARB;
#endif
    v_draw_id = draw_data_index;

    DrawData dd = draw_data[draw_data_index];
    mat4 model_matrix = dd.model;
    mat4 view_matrix = dd.view;
    mat4 projection_matrix = dd.projection;

    vec4 position = vec4(a_position, 1.0);

    vec4 w_position = model_matrix * position;
    v_w_position = w_position.xyz / w_position.w;
    v_c_position = projection_matrix * view_matrix * w_position;
    v_texcoord = vec2(a_position.x/2.0f + 0.5f, a_position.z / 2.0f + 0.5f) * shader_config.tiling;
    gl_Position = v_c_position;
    if (dd.light_index >= 0)
    {
        Light light = lights[dd.light_index];
        v_l_position = light.lightspace_matrix * w_position;
    }
}
