#version 330

uniform sampler2DArray TexContainer[10];

in vec2 v_texcoord;
in vec4 v_position;

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
    mat4 lightspace_matrix;
    vec3 camera_position;
    int bone_offset;
};

layout(std140) uniform CB1
{
    DrawData draw_data[100];
};
uniform int u_draw_data_index;

void main()
{
    DrawData dd = draw_data[u_draw_data_index];
    Tex2DAddress addr = texAddress[dd.texture_texaddress_index];
    vec3 texCoord = vec3(v_texcoord.xy, addr.Page);
    float transparency = texture(TexContainer[addr.Container], texCoord).a;
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
