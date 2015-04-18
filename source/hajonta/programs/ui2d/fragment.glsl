#version 150

in vec2 v_tex_coord;
in uint v_options;
in vec3 v_channel_color;
out vec4 o_color;

uniform sampler2D tex;

void main(void)
{
    o_color = vec4(texture(tex, v_tex_coord));

    if (v_options & 1)
    {
        if (o_color.r == 0)
        {
            discard;
        }
        o_color = vec4(v_channel_color, o_color.r);
    }
}
