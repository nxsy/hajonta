#version 330

uniform sampler2DArray TexContainer[10];

struct
ShaderConfig
{
    int texaddress_index;
    vec2 texture_scale;
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

in vec2 v_texcoord;

out vec4 o_color;

uniform bool skip = false;

void main()
{

    Tex2DAddress addr = texAddress[shader_config.texaddress_index];

    ivec3 texture_size = textureSize(TexContainer[addr.Container], 0);

    ivec3 texcoord = ivec3(
        texture_size.x * v_texcoord.x,
        texture_size.y * v_texcoord.y,
        addr.Page
    );

    ivec3 offset;
    offset = ivec3(-1, 1, 0);
    float tl = length(texelFetch(TexContainer[addr.Container], texcoord + offset, 0).xyz);

    offset.y = 0;
    float l =  length(texelFetch(TexContainer[addr.Container], texcoord + offset, 0).xyz);

    offset.y = -1;
    float bl =  length(texelFetch(TexContainer[addr.Container], texcoord + offset, 0).xyz);

    offset.x = 0;
    offset.y = 1;
    float t =  length(texelFetch(TexContainer[addr.Container], texcoord + offset, 0).xyz);

    offset.y = -1;
    float b =  length(texelFetch(TexContainer[addr.Container], texcoord + offset, 0).xyz);

    offset.x = 1;
    offset.y = 1;
    float tr = length(texelFetch(TexContainer[addr.Container], texcoord + offset, 0).xyz);

    offset.y = 0;
    float r =  length(texelFetch(TexContainer[addr.Container], texcoord + offset, 0).xyz);

    offset.y = -1;
    float br = length(texelFetch(TexContainer[addr.Container], texcoord + offset, 0).xyz);

    vec2 edges = vec2(
         tl + 2.0f * l + bl - tr - 2.0f * r - br,
        -tl - 2.0f * t - tr + bl + 2.0f * b + br 
    );

    float intensity = length(edges);
    o_color = vec4(1.0f, 1.0f, 1.0f, intensity);
}
