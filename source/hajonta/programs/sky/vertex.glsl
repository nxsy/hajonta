#version 330

uniform mat4 u_model_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;

uniform vec3 u_camera_position;
uniform vec3 u_light_direction;

in vec3 a_position;

out vec3 v_w_position;

void main()
{
    vec4 w_position = u_model_matrix * vec4(a_position, 1.0f);
    v_w_position = w_position.xyz / w_position.w;
    gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vec4(a_position, 1.0f);
}
