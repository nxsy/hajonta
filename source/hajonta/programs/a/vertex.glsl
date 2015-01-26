#version 150
uniform vec2 u_offset;
in vec4 a_pos;
in vec4 a_color;
out vec4 v_color;
void main (void)
{
    v_color = a_color;
    gl_Position = a_pos + vec4(u_offset, 0.0, 0.0);
}
