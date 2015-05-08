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

in vec4 a_pos;
in int a_material_id;
in vec4 a_normal;
in vec4 a_tangent;

flat out int v_material_id;
out vec4 v_normal;
out vec4 v_tangent;

out vec4 v_w_vertexPosition;
out vec4 v_c_vertexNormal;
out vec4 v_c_eyeDirection;
out vec4 v_c_lightDirection;

void main (void)
{
    v_material_id = a_material_id;
    v_normal = a_normal;

    gl_Position = u_projection * u_view * u_model * a_pos;
    v_tangent = u_model * a_tangent;

    v_w_vertexPosition = u_model * a_pos;
    vec4 c_vertexPosition = u_view * u_model * a_pos;

    v_c_eyeDirection = vec4(vec3(0,0,0) - c_vertexPosition.xyz, 1);
    vec4 c_lightPosition = u_view * u_w_lightPosition;

    v_c_lightDirection = c_lightPosition + v_c_eyeDirection;
    v_c_vertexNormal = normalize(u_view * inverse(transpose(u_model)) * a_normal);
}
