#version 410 core

uniform sampler2D tex;
uniform sampler2DArray TexContainer[10];
uniform int u_texaddress_index = -1;

in vec2 v_uv;
in vec4 v_color;

out vec4 o_color;

struct Tex2DAddress
{
    uint Container;
    float Page;
};
layout(std140) uniform CB0
{
    Tex2DAddress texAddress[100];
};

void main()
{
    if (u_texaddress_index < 0)
    {
        o_color = v_color * texture(tex, v_uv.st);
    }
    else
    {
        Tex2DAddress addr = texAddress[u_texaddress_index];
        vec3 texCoord = vec3(v_uv.st, addr.Page);
        vec4 diffuse_color = texture(TexContainer[addr.Container], texCoord);
        o_color = vec4(0.5, 0.5, 0.5, 1.0) * diffuse_color;
    }
    if (o_color.a < 0.001)
    {
        discard;
    }
}
