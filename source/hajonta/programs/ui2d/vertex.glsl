#version 150
uniform vec2 screen_pixel_size;

in vec2 a_pos;
in vec2 a_tex_coord;
in uint a_tex_id;

out vec2 v_tex_coord;
out uint v_tex_id;

void main (void)
{
    gl_Position = a_pos + vec4(u_offset, 0.0, 0.0);
}
