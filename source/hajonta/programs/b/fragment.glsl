#version 150

in vec4 v_color;
in vec4 v_style;
out vec4 o_color;
uniform sampler2D tex;

void main(void)
{
    o_color = v_color;

    if (v_style.x > 0 && v_style.x < 1.9)
    {
        if (v_style.w != 0)
        {
            if (mod(v_style.w, v_style.x) > v_style.y)
            {
                discard;
            }
        }
    }
    if (v_style.x > 1.1)
    {
        vec2 tex_coord = v_color.xy;
        o_color =
            vec4(texture(tex, tex_coord));
    }
}
