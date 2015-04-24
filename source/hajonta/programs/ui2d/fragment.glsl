#version 150

in vec2 v_tex_coord;
flat in int v_texid;
flat in int v_options;
in vec3 v_channel_color;
out vec4 o_color;

uniform sampler2D tex[10];

void main(void)
{
    if (v_texid == 1)
    {
        o_color = vec4(texture(tex[0], v_tex_coord));
    }
    else if (v_texid == 2)
    {
        o_color = vec4(texture(tex[1], v_tex_coord));
    }
    else if (v_texid == 3)
    {
        o_color = vec4(texture(tex[2], v_tex_coord));
    }
    else
    {
        // Something hideously obviously broken
        o_color = vec4(1.0, 0, 1.0, 1.0);
    }

    if ((v_options & 1) == 1)
    {
        if (o_color.r == 0)
        {
            discard;
        }
        o_color = vec4(v_channel_color, o_color.r);
    }
}
