#version 150
uniform vec2 u_offset;

in vec4 a_pos;
in vec4 a_color;
in vec4 a_style;

out vec4 v_color;
out vec4 v_style;
void main (void)
{
    v_color = a_color;
    v_style = a_style;
    gl_Position = a_pos + vec4(u_offset, 0.0, 0.0);
}
