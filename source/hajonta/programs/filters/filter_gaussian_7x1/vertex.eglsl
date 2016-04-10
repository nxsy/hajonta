#version 330
uniform mat4 u_projection;

in vec2 a_position;
in vec2 a_uv;
in vec4 a_color;

out vec2 v_uv;
out vec4 v_color;

void main()
{
    v_uv = a_uv;
    v_color = a_color;
    gl_Position = u_projection * vec4(a_position.xy, 0, 1);
}
