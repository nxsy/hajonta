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

uniform vec2 u_offset;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_perspective;
uniform vec4 u_mvp_enabled;
uniform vec4 u_w_lightPosition;
uniform int u_model_mode;
uniform int u_shading_mode;

in vec4 a_pos;
in vec4 a_color;
in vec4 a_style;
in vec4 a_normal;

out vec4 v_color;
out vec4 v_style;
out vec4 v_normal;

out vec4 v_w_vertexPosition;
out vec4 v_c_vertexNormal;
out vec4 v_c_eyeDirection;
out vec4 v_c_lightDirection;

void main (void)
{
    v_color = a_color;
    v_style = a_style;
    v_normal = a_normal;

    if (u_mvp_enabled.x == 0)
    {
        gl_Position = a_pos + vec4(u_offset, 0.0, 0.0);
    }
    else
    {
        gl_Position = u_perspective * u_view * u_model * a_pos;
        v_w_vertexPosition = u_model * a_pos;
        vec4 c_vertexPosition = u_view * u_model * a_pos;

        v_c_eyeDirection = vec4(vec3(0,0,0) - c_vertexPosition.xyz, 1);
        vec4 c_lightPosition = u_view * u_w_lightPosition;

        v_c_lightDirection = c_lightPosition + v_c_eyeDirection;
        v_c_vertexNormal = u_view * inverse(transpose(u_model)) * a_normal;
    }
    if (u_mvp_enabled.w == 1)
    {
        v_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}
