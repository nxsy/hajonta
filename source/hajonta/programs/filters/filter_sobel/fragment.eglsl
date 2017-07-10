#version 330

uniform sampler2D tex;

in vec2 v_uv;
in vec4 v_color;

out vec4 o_color;

void main()
{
    o_color = v_color * texture(tex, v_uv.st);
}
