#version 330

uniform mat4 u_projection;
uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;
uniform mat4 u_lightspace_matrix;

uniform int u_lightspace_available;

in vec3 a_position;
in vec2 a_texcoord;
in vec3 a_normal;

out vec3 v_w_position;
out vec2 v_texcoord;
out vec3 v_w_normal;
out vec4 v_l_position;

void main()
{
    v_texcoord = a_texcoord;
    v_w_normal = mat3(transpose(inverse(u_model_matrix))) * a_normal;
    v_w_position = vec3(u_model_matrix * vec4(a_position, 1));
    if (u_lightspace_available == 1)
    {
        v_l_position = u_lightspace_matrix * vec4(v_w_position.xyz, 1);
    }
    gl_Position = u_projection * u_view_matrix * vec4(v_w_position.xyz, 1);
}
