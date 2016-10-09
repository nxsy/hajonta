#version 410 core

precision highp float;
precision highp int;
layout(std140, column_major) uniform;

layout(std140) struct Tex2DAddress
{
    uint Container;
    float Page;
};

layout(std140) uniform CB0
{
    Tex2DAddress texAddress[100];
};

uniform int u_draw_data_index;

layout(std140) struct DrawData
{
    mat4 projection;
    mat4 view;
    mat4 model;
    int texture_texaddress_index;
    int shadowmap_texaddress_index;
    int shadowmap_color_texaddress_index;
};

layout(std140) uniform CB1
{
    DrawData draw_data[100];
};

uniform sampler2DArray TexContainer[10];

uniform bool do_not_skip = true;
 uniform bool skip = false;

in vec3 v_w_position;
in vec2 v_texcoord;
in vec3 v_w_normal;

out vec4 o_color;

void main()
{
    DrawData dd = draw_data[u_draw_data_index];
    if (dd.texture_texaddress_index >= 0)
    {
        Tex2DAddress addr = texAddress[dd.texture_texaddress_index];
        vec3 texCoord = vec3(v_texcoord.xy, addr.Page);
        vec4 t = texture(TexContainer[addr.Container], texCoord);
        o_color = vec4(t.xyz, 1);
    }
    else
    {
        o_color = vec4(1,0,1,1);
    }
}
