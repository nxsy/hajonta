#version 150

in vec4 v_color;
in vec4 v_style;
out vec4 o_color;
uniform sampler2D tex;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform vec4 u_mvp_enabled;
uniform int u_model_mode;

void main(void)
{
    o_color = v_color;

    if (u_mvp_enabled.w == 1)
    {

    }
    else
    {
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
        if (u_model_mode == 1)
        {
            discard;
        }
        else
        {
            if (v_style.x > 1.1)
            {
                vec2 tex_coord = v_color.xy;
                if (v_style.x < 2.1)
                {
                    o_color =
                        vec4(texture(tex, tex_coord));
                }
                else
                {
                    if (v_style.y < 0.5)
                    {
                        o_color =
                            vec4(texture(tex, tex_coord));
                    }
                    else if (v_style.y < 1.5)
                    {
                        o_color =
                            vec4(texture(tex1, tex_coord));
                    }
                    else if (v_style.y < 2.5)
                    {
                        o_color =
                            vec4(texture(tex2, tex_coord));
                    }
                    else if (v_style.y < 3.5)
                    {
                        o_color =
                            vec4(texture(tex3, tex_coord));
                    }
                }
            }
        }
    }
}
