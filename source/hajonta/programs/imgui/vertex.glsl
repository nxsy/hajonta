#version 330
uniform mat4 u_projection;
uniform float u_use_color;

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
    gl_Position = u_projection * vec4(a_position.xyz, 1);
}
