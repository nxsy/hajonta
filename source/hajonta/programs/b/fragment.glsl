#version 150

in vec4 v_color;
in vec4 v_style;
in vec4 v_normal;

in vec4 v_w_vertexPosition;
in vec4 v_c_vertexNormal;
in vec4 v_c_eyeDirection;
in vec4 v_c_lightDirection;

out vec4 o_color;
uniform sampler2D tex;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform vec4 u_mvp_enabled;
uniform vec4 u_w_lightPosition;
uniform int u_model_mode;
uniform int u_shading_mode;

void main(void)
{
    o_color = v_color;

    if (u_mvp_enabled.w == 1)
    {

    }
    else
    {
        /*
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
        */
        if (u_model_mode == 1)
        {
            discard;
        }
        else if (u_model_mode == 2)
        {
            vec4 normal_clamped = v_normal / 2 + 0.5;
            o_color = vec4(normal_clamped.xyz, 1);
        }
        else if (u_model_mode == 3)
        {
            vec4 lightDirection_clamped = normalize(v_c_lightDirection) / 2 + 0.5;
            o_color = vec4(lightDirection_clamped.xyz, 1);
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
                    if (u_shading_mode == 1)
                    {
                        vec3 light_color = vec3(1.0f, 1.0f, 0.9f);
                        float light_power = 70.0f;

                        vec3 material_diffuse_color = o_color.rgb;
                        vec3 material_ambient_color = material_diffuse_color * 0.2;
                        vec3 material_specular_color = vec3(0.3, 0.3, 0.3);
                        float distance = length(u_w_lightPosition - v_w_vertexPosition);
                        vec3 n = normalize(v_c_vertexNormal.xyz);
                        vec3 l = normalize(v_c_lightDirection.xyz);
                        float cosTheta = clamp(dot(n, l), 0, 1);
                        vec3 E = normalize(v_c_eyeDirection.xyz);
                        vec3 R = reflect(-l, n);
                        float cosAlpha = clamp(dot(E, R), 0, 1);
                        o_color = vec4(
                            // Ambient : simulates indirect lighting
                            material_ambient_color +
                            // Diffuse : "color" of the object
                            material_diffuse_color * light_color * light_power * cosTheta / (distance*distance) +
                            // Specular : reflective highlight, like a mirror
                            material_specular_color * light_color * light_power * pow(cosAlpha,5) / (distance*distance),
                            1);
                    }
                }
            }
        }
    }
}
