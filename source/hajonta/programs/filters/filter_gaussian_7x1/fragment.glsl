#version 330

uniform vec2 u_blur_scale;
uniform sampler2D tex;

in vec2 v_texcoord;

out vec4 o_color;

void main()
{
    vec4 color = vec4(0.0);

    color += texture(tex, v_texcoord + (vec2(-3.0) * u_blur_scale.xy)) * (1.0/64.0);
    color += texture(tex, v_texcoord + (vec2(-2.0) * u_blur_scale.xy)) * (6.0/64.0);
    color += texture(tex, v_texcoord + (vec2(-1.0) * u_blur_scale.xy)) * (15.0/64.0);
    color += texture(tex, v_texcoord + (vec2(0.0) * u_blur_scale.xy))  * (20.0/64.0);
    color += texture(tex, v_texcoord + (vec2(1.0) * u_blur_scale.xy))  * (15.0/64.0);
    color += texture(tex, v_texcoord + (vec2(2.0) * u_blur_scale.xy))  * (6.0/64.0);
    color += texture(tex, v_texcoord + (vec2(3.0) * u_blur_scale.xy))  * (1.0/64.0);

    o_color = color;
}
