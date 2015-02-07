#version 150

in vec4 v_color;
in vec4 v_style;
out vec4 o_color;

void main(void)
{
    if (v_style.x)
    {
        if (v_style.w != 0)
        {
            if (mod(v_style.w, v_style.x) > v_style.y)
            {
                discard;
            }
        }
    }
    o_color = v_color;
}
