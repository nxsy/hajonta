#version 410 core

uniform sampler2DArray TexContainer[14];

in vec2 v_texcoord;
in vec4 v_position;
flat in uint v_draw_id;

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
    int normal_texaddress_index;
    int specular_texaddress_index;
};

layout(std140) uniform CB1
{
    DrawData draw_data[100];
};

struct TexContainerSamplerMapping
{
    uint generation;
    uint texcontainer_index;
};

struct TexContainerIndexItem
{
    uint container_index;
    // 3 bytes wasted due to alignment!
};

layout(std140) uniform CB4
{
    TexContainerIndexItem texcontainer_index[8];
    TexContainerSamplerMapping texcontainer_sampler_mapping[16];
};

vec4 texcontainer_fetch(int texaddress_index, vec2 texcoord2)
{
    Tex2DAddress addr = texAddress[texaddress_index];
    vec3 texCoord = vec3(texcoord2, addr.Page);
    TexContainerSamplerMapping tcsm = texcontainer_sampler_mapping[addr.Container];
    //uint newContainer = texcontainer_index[tcsm.texcontainer_index].container_index;
    uint newContainer = tcsm.texcontainer_index;
    return texture(TexContainer[newContainer], texCoord);
}

void main()
{
    DrawData dd = draw_data[v_draw_id];

    float transparency = texcontainer_fetch(dd.texture_texaddress_index, v_texcoord.xy).a;
    if (transparency < 0.001)
    {
        discard;
    }
    float depth = 0.5f + 0.5f * (v_position.z / v_position.w);
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    float moment2 = (depth * depth) + 0.25*(dx*dx+dy*dy);
    o_color = vec4(depth, moment2, 0.0f, 1);
}
