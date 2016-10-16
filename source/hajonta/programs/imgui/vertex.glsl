#version 330
uniform mat4 u_projection;
uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;

uniform float u_use_color;
uniform int u_texaddress_index;

in vec3 a_position;
in vec2 a_uv;
in vec4 a_color;

out vec2 v_uv;
out vec4 v_color;

void main()
{
    v_uv = a_uv;
    if (u_use_color > 0.0f)
    {
        v_color = a_color;
    }
    else
    {
        v_color = vec4(1, 1, 1, 1);
    }
    gl_Position = u_projection * u_view_matrix * u_model_matrix * vec4(a_position.xyz, 1);
}
