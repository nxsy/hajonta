#version 150

in vec4 a_pos;
in vec4 a_tex_coord;
out vec2 v_tex_coord;

void main()
{
    gl_Position = vec4(a_pos.xy, 0, 1);
    v_tex_coord = a_tex_coord.xy;
}
