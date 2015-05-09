#version 150

in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_tex_coord;

in vec4 v_w_vertexPosition;
in vec4 v_c_vertexNormal;
in vec4 v_c_vertexTangent;
in vec4 v_c_eyeDirection;
in vec4 v_c_lightDirection;
in vec4 v_c_lightPosition;

out vec4 o_color;

uniform sampler2D tex;
uniform sampler2D normal_texture;
uniform sampler2D ao_texture;
uniform sampler2D emit_texture;
uniform float specular_intensity = 0.01;
uniform float specular_power = 2.0;

uniform int u_shader_mode;
uniform int u_shader_config_flags;
uniform vec4 u_w_lightPosition;
uniform mat4 u_model;
uniform mat4 u_view;

uniform bool DONOTRUN = false;

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
    float ambient_intensity;
    float diffuse_intensity;
};

struct Attenuation
{
    float constant;
    float linear;
    float exponential;
};

struct PointLight
{
    vec3 position;
    vec3 color;
    float ambient_intensity;
    float diffuse_intensity;
    Attenuation attenuation;
};

uniform DirectionalLight u_directional_light;
uniform PointLight u_point_lights[8];
uniform int u_num_point_lights = 0;

struct ShaderConfig
{
    int mode;
    int config;
};

bool ignore_normal_texture(ShaderConfig config)
{
    return (config.config & 1) == 1;
}

bool ignore_ao_texture(ShaderConfig config)
{
    return (config.config & 2) == 2;
}

bool ignore_emit_texture(ShaderConfig config)
{
    return (config.config & 4) == 4;
}

bool enabled(in sampler2D texture)
{
    ivec2 size = textureSize(texture, 0);
    return (size.x > 1);
}

vec3 get_normal(ShaderConfig config)
{
    vec3 normal = normalize(v_c_vertexNormal.xyz);
    if (!ignore_normal_texture(config) && enabled(normal_texture))
    {
        vec4 bump_normal_raw = texture(normal_texture, v_tex_coord);
        vec3 bump_normal = bump_normal_raw.xyz * 2.0 - 1.0;
        vec3 n = normal;
        vec3 t = normalize(v_c_vertexTangent.xyz);
        t = normalize(t - dot(t, n) * n);
        vec3 b = cross(t, n);
        mat3 tbn_m = mat3(t, b, n);
        normal = normalize(tbn_m * bump_normal);
    }
    return normal;
}

vec4 light_contribution(ShaderConfig config, DirectionalLight l, vec3 n)
{
    float cosTheta = clamp(dot(n, l.direction), 0, 1);

    vec4 ambient_color = vec4(l.color, 1) * l.ambient_intensity;
    vec4 diffuse_color = vec4(0);
    vec4 specular_color = vec4(0);

    if (cosTheta > 0)
    {
        diffuse_color = vec4(l.color, 1) * l.diffuse_intensity * cosTheta;

        vec3 E = normalize(v_c_eyeDirection.xyz);
        vec3 R = reflect(-l.direction, n);
        float cosAlpha = clamp(dot(E, R), 0, 1);

        if (cosAlpha > 0)
        {
            specular_color = vec4(l.color, 1) * specular_intensity * pow(cosAlpha, specular_power);
        }
    }

    if (!ignore_ao_texture(config) && enabled(ao_texture))
    {
        ambient_color *= max(0, texture(ao_texture, v_tex_coord).r);
    }

    return ambient_color + diffuse_color + specular_color;
}

vec4 apply_attenuation(vec4 light, Attenuation a, float distance)
{
    return light / (
        a.constant +
        a.linear * distance +
        a.exponential * distance * distance
        );
}

vec4 point_light_contribution(ShaderConfig config, PointLight p, vec3 n, float distance)
{
    vec3 lightDirection = p.position + v_c_eyeDirection.xyz;

    DirectionalLight dl;
    dl.direction = (u_view * vec4(lightDirection, 1.0)).xyz;
    dl.color = p.color;
    dl.ambient_intensity = p.ambient_intensity;
    dl.diffuse_intensity = p.diffuse_intensity;
    vec4 light = light_contribution(config, dl, n);

    return apply_attenuation(light, p.attenuation, distance);
}

vec4 blinn_phong_shading(ShaderConfig config)
{
    vec4 o_color = texture(tex, v_tex_coord);

    vec3 normal = get_normal(config);

    float distance = length(u_w_lightPosition - v_w_vertexPosition);

    vec4 light = vec4(0);
    light += light_contribution(config, u_directional_light, normal);

    for (int i = 0; i < u_num_point_lights; ++i)
    {
        light += point_light_contribution(config, u_point_lights[i], normal, distance);
    }

    light.w = 1.0;
    o_color *= light;

    if (!ignore_emit_texture(config) && enabled(emit_texture))
    {
        vec4 emit_color = texture(emit_texture, v_tex_coord);
        o_color.r = max(o_color.r, emit_color.r);
        o_color.g = max(o_color.g, emit_color.g);
        o_color.b = max(o_color.b, emit_color.b);
    }
    return o_color;
}

vec4 standard_shading(ShaderConfig config)
{
    return blinn_phong_shading(config);
}

void main(void)
{
    ShaderConfig config = { u_shader_mode, u_shader_config_flags };
    switch (config.mode)
    {
        case 0:
        {
            o_color = standard_shading(config);
        } break;
        case 1:
        {
            o_color = texture(tex, v_tex_coord);
        } break;
        case 2:
        {
            o_color = texture(normal_texture, v_tex_coord);
        } break;
        case 3:
        {
            o_color = texture(ao_texture, v_tex_coord);
        } break;
        case 4:
        {
            o_color = texture(emit_texture, v_tex_coord);
        } break;
        case 5:
        {
            o_color = blinn_phong_shading(config);
        } break;
    }
}
