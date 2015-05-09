#version 150

in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_tex_coord;

in vec4 v_w_vertexPosition;
in vec4 v_c_vertexNormal;
in vec4 v_c_eyeDirection;
in vec4 v_c_lightDirection;

out vec4 o_color;

uniform sampler2D tex;
uniform sampler2D normal_texture;

uniform vec4 u_w_lightPosition;
uniform mat4 u_model;
uniform mat4 u_view;

uniform bool DONOTRUN = false;

struct ShaderConfig
{
    int config;
};

uniform ShaderConfig config;

bool use_normal_map(ShaderConfig config)
{
    return (config.config & 1) == 0;
}

bool use_ao_texture(ShaderConfig config)
{
    return (config.config & 2) == 0;
}

bool use_emit_texture(ShaderConfig config)
{
    return (config.config & 4) == 0;
}

bool enabled(in sampler2D texture)
{
    ivec2 size = textureSize(texture, 0);
    return (size.x > 1);
}

void main(void)
{
    o_color = vec4(1.0, 0, 1.0, 1.0);
    o_color = vec4(v_tex_coord, 0.0, 1.0);
    o_color = texture(tex, v_tex_coord);
    vec4 normal = texture(normal_texture, v_tex_coord);

    if (enabled(normal_texture))
    {
        o_color = normal;
    }
    else if (DONOTRUN)
    {
        o_color = u_w_lightPosition;
    }

    return;


    /*
    vec3 normal = normalize(v_c_vertexNormal.xyz);
    vec4 lightDirection = v_c_lightDirection;

    if (use_normal_map(config) && material.normal_texture > 0)
    {
        vec4 bump_normal_raw = texture_load(material.normal_texture, v_tex_coord);
        vec3 bump_normal = bump_normal_raw.xyz * 2.0 - 1.0;
        vec3 n = normalize(v_normal.xyz);
        vec3 t = normalize(v_tangent.xyz);
        t = normalize(t - dot(t, n) * n);
        vec3 b = cross(t, n);
        mat3 tbn_m = mat3(t, b, n);
        normal = normalize(tbn_m * bump_normal);
        normal = normalize(u_view * inverse(transpose(u_model)) * vec4(normal, 1)).xyz;
    }

    vec3 light_color = vec3(1.0f, 1.0f, 0.9f);
    float light_power = 70.0f;

    vec3 material_diffuse_color = o_color.rgb;
    vec3 material_ambient_color = material_diffuse_color * 0.3;
    vec3 material_specular_color = vec3(0.01, 0.01, 0.01);
    float distance = length(u_w_lightPosition - v_w_vertexPosition);
    vec3 n = normalize(normal);
    vec3 l = normalize(lightDirection.xyz);
    float cosTheta = clamp(dot(n, l), 0, 1);
    vec3 E = normalize(v_c_eyeDirection.xyz);
    vec3 R = reflect(-l, n);
    float cosAlpha = clamp(dot(E, R), 0, 1);

    vec3 ambient_component = material_ambient_color;
    vec3 diffuse_component = material_diffuse_color *
        light_color * light_power * cosTheta /
        (distance*distance);
    vec3 specular_component = material_specular_color *
        light_color * light_power * pow(cosAlpha,5) /
        (distance*distance);

    if (use_ao_texture(config) && material.ao_texture > 0)
    {
        ambient_component *= max(0, texture_load(material.ao_texture, v_tex_coord).r);
    }

    o_color = vec4(
        ambient_component + diffuse_component + specular_component,
        1);

    if (use_emit_texture(config) && material.emit_texture > 0)
    {
        vec4 emit_color = texture_load(material.emit_texture, v_tex_coord);
        o_color.r = max(o_color.r, emit_color.r);
        o_color.g = max(o_color.g, emit_color.g);
        o_color.b = max(o_color.b, emit_color.b);
    }
    */
}
