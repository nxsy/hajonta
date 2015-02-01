#version 150

in vec2 v_tex_coord;
uniform sampler2D tex;
out vec4 o_color;

void main(void)
{
    vec4 texture_color = vec4(texture( tex, v_tex_coord ).bgra);
    o_color = texture_color;
}
