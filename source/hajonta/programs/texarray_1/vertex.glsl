#version 410 core

#ifdef GL_ARB_shader_draw_parameters
#extension GL_ARB_shader_draw_parameters : enable
#endif

precision highp float;
precision highp int;
layout(std140, column_major) uniform;

in vec3 a_position;
in vec2 a_texcoord;
in vec3 a_normal;
in ivec4 a_bone_ids;
in vec4 a_bone_weights;

out vec3 v_w_position;
out vec2 v_texcoord;
out vec3 v_w_normal;
out vec4 v_l_position;
flat out uint v_draw_id;

uniform float u_minimum_variance;
uniform float u_lightbleed_compensation;
uniform float u_bias;

struct
Plane
{
    vec3 normal;
    float distance;
};

struct
ShaderConfig
{
    Plane clipping_plane;
    int use_clipping_plane;
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

layout(std140) uniform CB3
{
    mat4 bones[100];
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

    vec4 normal = vec4(a_normal, 1.0);
    vec4 position = vec4(a_position, 1.0);

    if (dd.bone_offset >= 0)
    {
        mat4 bone_transform = mat4(0);
        bone_transform += bones[dd.bone_offset + a_bone_ids[0]] * a_bone_weights[0];
        bone_transform += bones[dd.bone_offset + a_bone_ids[1]] * a_bone_weights[1];
        bone_transform += bones[dd.bone_offset + a_bone_ids[2]] * a_bone_weights[2];
        bone_transform += bones[dd.bone_offset + a_bone_ids[3]] * a_bone_weights[3];
        normal = bone_transform * normal;
        position = bone_transform * position;
    }

    v_texcoord = a_texcoord;
    v_w_normal = mat3(transpose(inverse(model_matrix))) * (normal.xyz / normal.w);
    vec4 w_position = model_matrix * position;
    v_w_position = w_position.xyz / w_position.w;
    gl_Position = projection_matrix * view_matrix * w_position;
    if (dd.light_index >= 0)
    {
        Light light = lights[dd.light_index];
        v_l_position = light.lightspace_matrix * w_position;
    }
    gl_ClipDistance[0] = dot(
        vec4(v_w_position, 1.0f),
        vec4(shader_config.clipping_plane.normal, shader_config.clipping_plane.distance)
    );
}
