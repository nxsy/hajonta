#version 150

in vec2 v_tex_coord;
out vec4 o_color;

uniform sampler2D tex;

void main(void)
{
    o_color =
        vec4(texture(tex, v_tex_coord));
}
