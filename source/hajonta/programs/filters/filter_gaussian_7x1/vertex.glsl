#version 330
#
uniform mat4 u_projection;
uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;
uniform vec2 u_blur_scale;

in vec3 a_position;
in vec2 a_texcoord;

out vec2 v_texcoord;

void main()
{
    v_texcoord = a_texcoord;
    vec4 v_position = u_projection * u_view_matrix * u_model_matrix * vec4(a_position.xyz, 1);
    gl_Position = v_position;
}
