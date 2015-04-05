#version 150

in int8 v_tex_id;
in uint v_tex_coord;
out vec4 o_color;

uniform sampler2D tex[16];

void main(void)
{
    o_color = vec4(0, 0, 0, 0);
}
