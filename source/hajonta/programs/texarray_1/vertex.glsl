#version 410 core

precision highp float;
precision highp int;
layout(std140, column_major) uniform;

in vec3 a_position;
in vec2 a_texcoord;
in vec3 a_normal;

out vec3 v_w_position;
out vec2 v_texcoord;
out vec3 v_w_normal;
out vec4 v_l_position;

uniform float u_minimum_variance;
uniform float u_lightbleed_compensation;
uniform float u_bias;

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
    mat4 lightspace_matrix;
    vec3 camera_position;
};

layout(std140) uniform CB1
{
    DrawData draw_data[100];
};

uniform int u_draw_data_index;
 uniform bool do_not_skip = true;
 uniform bool skip = false;

void main()
{
    DrawData dd = draw_data[u_draw_data_index];
    mat4 model_matrix = dd.model;
    mat4 view_matrix = dd.view;
    mat4 projection_matrix = dd.projection;

    v_texcoord = a_texcoord;
    vec4 w_position;
    v_w_normal = mat3(transpose(inverse(model_matrix))) * a_normal;
    w_position = model_matrix * vec4(a_position, 1.0);
    gl_Position = projection_matrix * view_matrix * w_position;
    v_w_position = w_position.xyz / w_position.w;
    if (dd.light_index >= 0)
    {
        v_l_position = dd.lightspace_matrix * w_position;
    }
}
