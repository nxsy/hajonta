#version 410 core

uniform sampler2D tex;
uniform sampler2DArray TexContainer[14];
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
        o_color = v_color;
    }
    else
    {
        Tex2DAddress addr = texAddress[u_texaddress_index];
        vec3 texCoord = vec3(v_uv.st, addr.Page);
        // addr.Container is always 0, since there's only one
        // texture per draw in imgui, and there's no need
        // to maintain the whole container->sampler indirection
        vec4 diffuse_color = texture(TexContainer[0], texCoord);
        o_color = v_color * diffuse_color;
    }
    if (o_color.a < 0.001)
    {
        discard;
    }
}
