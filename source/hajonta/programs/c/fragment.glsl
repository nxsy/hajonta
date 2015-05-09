#version 150

in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_tex_coord;

in vec4 v_w_vertexPosition;
in vec4 v_c_vertexNormal;
in vec4 v_c_vertexTangent;
in vec4 v_c_eyeDirection;
in vec4 v_c_lightDirection;

out vec4 o_color;

uniform sampler2D tex;
uniform sampler2D normal_texture;
uniform sampler2D ao_texture;
uniform sampler2D emit_texture;

uniform int u_shader_mode;
uniform int u_shader_config_flags;
uniform vec4 u_w_lightPosition;
uniform mat4 u_model;
uniform mat4 u_view;

uniform bool DONOTRUN = false;

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

vec4 blinn_phong_shading(ShaderConfig config)
{
    vec4 o_color = texture(tex, v_tex_coord);

    vec4 lightDirection = v_c_lightDirection;

    vec3 light_color = vec3(1.0f, 1.0f, 0.9f);
    float light_power = 70.0f;

    vec3 normal = get_normal(config);

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

    if (!ignore_ao_texture(config) && enabled(ao_texture))
    {
        ambient_component *= max(0, texture(ao_texture, v_tex_coord).r);
    }

    o_color = vec4(
        ambient_component + diffuse_component + specular_component,
        1);

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
