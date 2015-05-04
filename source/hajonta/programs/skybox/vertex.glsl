#version 150
#
in vec3 a_pos;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_tex_coord;

void main (void)
{
    vec4 c_pos = u_projection * u_view * u_model * vec4(a_pos, 1.0);
    gl_Position = c_pos;
    v_tex_coord = a_pos;
}
