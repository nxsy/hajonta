#version 330
#
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

in vec3 a_position;
in vec2 a_texcoord;

out vec2 v_texcoord;

void main()
{
    v_texcoord = a_texcoord;
    vec4 v_position = vec4(a_position.xyz, 1);
    gl_Position = v_position;
}
