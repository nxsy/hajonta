#version 330

uniform vec2 u_blur_scale;
uniform sampler2DArray TexContainer[10];
uniform int u_texaddress_index;

struct Tex2DAddress
{
    uint Container;
    float Page;
};

layout(std140) uniform CB0
{
    Tex2DAddress texAddress[100];
};

in vec2 v_texcoord;

out vec4 o_color;

vec4
add_contribution(in sampler2DArray s, float Page, float offset, float contribution, float total)
{
    vec2 texcoord2 = v_texcoord + (vec2(offset) * u_blur_scale.xy);
    vec3 texcoord3 = vec3(texcoord2, Page);
    return texture(s, texcoord3) * (contribution/total);
}

void main()
{
    vec4 color = vec4(0.0);

    Tex2DAddress addr = texAddress[u_texaddress_index];

    color  = add_contribution(TexContainer[addr.Container], addr.Page, -3.0, 1.0, 64.0);
    color += add_contribution(TexContainer[addr.Container], addr.Page, -2.0, 6.0, 64.0);
    color += add_contribution(TexContainer[addr.Container], addr.Page, -1.0, 15.0, 64.0);
    color += add_contribution(TexContainer[addr.Container], addr.Page,  0.0, 20.0, 64.0);
    color += add_contribution(TexContainer[addr.Container], addr.Page,  1.0, 15.0, 64.0);
    color += add_contribution(TexContainer[addr.Container], addr.Page,  2.0, 6.0, 64.0);
    color += add_contribution(TexContainer[addr.Container], addr.Page,  3.0, 1.0, 64.0);

    o_color = color;
}
