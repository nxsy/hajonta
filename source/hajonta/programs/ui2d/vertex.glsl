#version 150
uniform vec2 screen_pixel_size;

in vec2 a_pos;
in vec2 a_tex_coord;

out vec2 v_tex_coord;

void main (void)
{
    vec2 half_screen_pixel_size = screen_pixel_size / 2;
    gl_Position = vec4((a_pos - half_screen_pixel_size) / half_screen_pixel_size, 0, 1);
    v_tex_coord = a_tex_coord;
}
