#version 150
//
// Prefixes:
// u_ - uniforms
// a_ - vertex attributes ("in")
// v_ - varying fragment attributes ("out", old: "varying")
//
// Secondary prefixes:
// w_ - world-space position/direction
// c_ - camera-space position/direction
//

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec4 u_w_lightPosition;
uniform int u_shader_mode;
uniform int u_shader_config_flags;

in vec4 a_pos;
in vec4 a_normal;
in vec4 a_tangent;
in vec2 a_tex_coord;

out vec4 v_normal;
out vec4 v_tangent;
out vec2 v_tex_coord;

out vec4 v_w_vertexPosition;
out vec4 v_c_vertexNormal;
out vec4 v_c_vertexTangent;
out vec4 v_c_eyeDirection;

void main (void)
{
    v_normal = a_normal;
    v_tex_coord = a_tex_coord;

    gl_Position = u_projection * u_view * u_model * a_pos;
    v_tangent = u_model * a_tangent;

    v_w_vertexPosition = u_model * a_pos;
    vec4 c_vertexPosition = u_view * u_model * a_pos;

    v_c_eyeDirection = vec4(vec3(0,0,0) - c_vertexPosition.xyz, 1);
    v_c_vertexNormal = u_view * inverse(transpose(u_model)) * a_normal;
    v_c_vertexTangent = u_view * inverse(transpose(u_model)) * a_tangent;
}
