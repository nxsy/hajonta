#version 330

uniform sampler2D tex;

in vec2 v_texcoord;
in vec4 v_position;

out vec4 o_color;

void main()
{
    float transparency = texture(tex, v_texcoord.st).a;
    if (transparency < 0.001)
    {
        discard;
    }
    float depth = 0.5f + 0.5f * (v_position.z / v_position.w);
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    float moment2 = (depth * depth) + 0.25*(dx*dx+dy*dy);
    o_color = vec4(depth, moment2, 0.0f, 1);
}
