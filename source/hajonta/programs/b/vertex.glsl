#version 150
uniform vec2 u_offset;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_perspective;
uniform vec4 u_mvp_enabled;

in vec4 a_pos;
in vec4 a_color;
in vec4 a_style;

out vec4 v_color;
out vec4 v_style;
void main (void)
{
    v_color = a_color;
    v_style = a_style;
    if (u_mvp_enabled.x == 0)
    {
        gl_Position = a_pos + vec4(u_offset, 0.0, 0.0);
    }
    else
    {
        gl_Position = u_perspective * u_view * u_model * a_pos;
    }
}
