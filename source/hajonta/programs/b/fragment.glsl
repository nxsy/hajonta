#version 150

in vec4 v_color;
in vec4 v_style;
in vec4 v_normal;

in vec4 v_w_vertexPosition;
in vec4 v_c_vertexNormal;
in vec4 v_c_eyeDirection;
in vec4 v_c_lightDirection;

in mat3 v_tbn_matrix;

out vec4 o_color;
uniform sampler2D tex;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform vec4 u_mvp_enabled;
uniform vec4 u_w_lightPosition;
uniform int u_model_mode;
uniform int u_shading_mode;

vec4 tex_crazy(float t, vec2 tex_coord)
{
    vec4 o_color;
    if (t < -0.5)
    {
        o_color = vec4(0);
    }
    else if (t < 0.5)
    {
        o_color =
            vec4(texture(tex, tex_coord));
    }
    else if (t < 1.5)
    {
        o_color =
            vec4(texture(tex1, tex_coord));
    }
    else if (t < 2.5)
    {
        o_color =
            vec4(texture(tex2, tex_coord));
    }
    else if (t < 3.5)
    {
        o_color =
            vec4(texture(tex3, tex_coord));
    }
    else if (t < 4.5)
    {
        o_color =
            vec4(texture(tex4, tex_coord));
    }
    else if (t < 5.5)
    {
        o_color =
            vec4(texture(tex5, tex_coord));
    }

    return o_color;
}

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
            vec3 n = normalize(v_c_vertexNormal.xyz);
            vec3 l = normalize(v_c_lightDirection.xyz);
            float cosTheta = clamp(dot(n, l), 0, 1);
            o_color = vec4(cosTheta, cosTheta, cosTheta, 1.0);
        }
        else if (u_model_mode == 4)
        {
            vec2 tex_coord = v_color.xy;
            o_color = tex_crazy(v_style.z, tex_coord);
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
                    o_color = tex_crazy(v_style.y, tex_coord);
                    vec3 normal = normalize(v_c_vertexNormal.xyz);
                    if (u_shading_mode >= 2 && v_style.z >= 0)
                    {
                        vec3 bump_normal_raw = tex_crazy(v_style.z, tex_coord).xyz;
                        vec3 bump_normal = 2.0 * bump_normal_raw - vec3(1.0, 1.0, 1.0);
                        normal = v_tbn_matrix * bump_normal;
                    }
                    if (u_shading_mode >= 1)
                    {
                        vec3 light_color = vec3(1.0f, 1.0f, 0.9f);
                        float light_power = 70.0f;

                        vec3 material_diffuse_color = o_color.rgb;
                        vec3 material_ambient_color = material_diffuse_color * 0.2;
                        vec3 material_specular_color = vec3(0.3, 0.3, 0.3);
                        float distance = length(u_w_lightPosition - v_w_vertexPosition);
                        vec3 n = normalize(normal);
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
