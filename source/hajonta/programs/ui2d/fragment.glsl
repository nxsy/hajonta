#version 150

flat in int v_tex_id;
flat in uint v_tex_coord;
out vec4 o_color;

uniform sampler2D tex;

void main(void)
{
    o_color = vec4(0, 0, 0, 0);
}
