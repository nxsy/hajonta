#version 150

in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_tex_coord;

in vec4 v_w_vertexPosition;
in vec4 v_c_vertexNormal;
in vec4 v_c_vertexTangent;
in vec4 v_c_eyeDirection;

out vec4 o_color;

uniform sampler2D tex;
uniform sampler2D normal_texture;
uniform sampler2D ao_texture;
uniform sampler2D emit_texture;
uniform sampler2D specular_exponent_texture;
uniform float u_specular_exponent = 32;

uniform int u_pass = 0;
uniform int u_shader_mode = 0;
uniform int u_shader_config_flags = 0;
uniform int u_ambient_mode = 0;
uniform int u_diffuse_mode = 0;
uniform int u_specular_mode = 0;
uniform int u_tonemap_mode = 0;
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
    int diffuse_mode;
    int specular_mode;
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

bool ignore_specular_exponent_texture(ShaderConfig config)
{
    return (config.config & 8) == 8;
}

bool ignore_gamma_correct(ShaderConfig config)
{
    return (config.config & 16) == 16;
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

float blinn_phong_specular_component(vec3 light_direction, vec3 eye_direction, vec3 normal, float shininess)
{
  vec3 half_dir = normalize(eye_direction + light_direction);

  return pow(max(0.0, dot(normal, half_dir)), shininess);
}

float gaussian_specular_component(vec3 light_direction, vec3 eye_direction, vec3 normal, float shininess)
{
  vec3 half_dir = normalize(light_direction + eye_direction);
  float theta = acos(dot(half_dir, normal));
  float w = theta / shininess;
  return exp(-w*w);
}

float lambert_diffuse_component(vec3 light_direction, vec3 normal)
{
    return max(0.0, dot(light_direction, normal));
}

vec4 light_contribution(ShaderConfig config, DirectionalLight l, vec3 n)
{
    vec4 ambient_color = vec4(0);
    vec4 diffuse_color = vec4(0);
    vec4 specular_color = vec4(0);

    switch (u_ambient_mode)
    {
        case 0:
        {
            ambient_color = vec4(l.color, 1) * l.ambient_intensity;
        } break;
        case 1:
        {
        } break;
    }

    float diffuse_component;
    switch (u_diffuse_mode)
    {
        case 0:
        {
            diffuse_component = lambert_diffuse_component(l.direction, n);
        } break;
        case 1:
        {
            diffuse_component = 0.0001f;
        } break;
        case 2:
        {
            diffuse_component = lambert_diffuse_component(l.direction, n);
        } break;
        default:
        {
            diffuse_component = lambert_diffuse_component(l.direction, n);
        } break;
    }

    if (diffuse_component > 0)
    {
        diffuse_color = vec4(l.color, 1) * l.diffuse_intensity * diffuse_component;

        float shininess = u_specular_exponent;
        if (!ignore_specular_exponent_texture(config) && enabled(specular_exponent_texture))
        {
            shininess = texture(specular_exponent_texture, v_tex_coord).r * 255.0;
        }
        if (shininess >= 1.0f)
        {
            float specular_component;
            switch (u_specular_mode)
            {
                case 0:
                {
                    specular_component = blinn_phong_specular_component(l.direction.xyz, v_c_eyeDirection.xyz, n, shininess);
                } break;
                case 1:
                {
                    specular_component = 0.0f;
                } break;
                case 2:
                {
                    specular_component = blinn_phong_specular_component(l.direction.xyz, v_c_eyeDirection.xyz, n, shininess);
                } break;
                case 3:
                {
                    specular_component = gaussian_specular_component(l.direction.xyz, v_c_eyeDirection.xyz, n, shininess / 255.0);
                } break;
                default:
                {
                    specular_component = blinn_phong_specular_component(l.direction.xyz, v_c_eyeDirection.xyz, n, shininess);
                } break;
            }
            specular_color = vec4(vec3(1) * specular_component, 1);
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

vec4 point_light_contribution(ShaderConfig config, PointLight p, vec3 n)
{
    vec3 lightDirection = p.position + v_c_eyeDirection.xyz;

    float distance = length(p.position - v_w_vertexPosition.xyz);

    DirectionalLight dl;
    dl.direction = (u_view * vec4(lightDirection, 1.0)).xyz;
    dl.color = p.color;
    dl.ambient_intensity = p.ambient_intensity;
    dl.diffuse_intensity = p.diffuse_intensity;
    vec4 light = light_contribution(config, dl, n);

    return apply_attenuation(light, p.attenuation, distance);
}

vec4 linearize_gamma(vec4 color)
{
    vec4 ret = vec4(
        pow(color.x, 2.2),
        pow(color.y, 2.2),
        pow(color.z, 2.2),
        color.w
    );

    return ret;
}

vec4 delinearize_gamma(vec4 color)
{
    vec4 ret = vec4(
        pow(color.x, 1/2.2),
        pow(color.y, 1/2.2),
        pow(color.z, 1/2.2),
        color.w
    );

    return ret;
}

vec4 reinhard(vec4 o_color)
{
    vec4 ret = o_color;
    ret = ret/(1+ret);
    ret.w = o_color.w;
    return ret;
}

vec4 reinhard_l(vec4 o_color)
{
    float luminance = 0.2126 * o_color.r + 0.7152 * o_color.g + 0.0722 * o_color.b;
    float nL = luminance / (1 + luminance);
    float scale = nL / luminance;
    o_color.r *= scale;
    o_color.g *= scale;
    o_color.b *= scale;
    return o_color;
}

vec4 hejl(vec4 o_color)
{
    vec3 x = o_color.rgb - 0.004;
    x.r = max(0, x.r);
    x.g = max(0, x.g);
    x.b = max(0, x.b);
    vec3 ret = (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06);
    return vec4(ret,1);
}

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;
float W = 11.2;

vec3 filmic_tonemap(vec3 color)
{
    vec3 x = color.rgb;
    color.rgb = ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
    return color;
}

vec4 filmic(vec4 o_color)
{
   float exposure_bias = 2.0f;
   vec3 curr = filmic_tonemap(exposure_bias * o_color.rgb);

   vec3 white_scale = 1.0f / filmic_tonemap(vec3(W));
   vec3 color = curr * white_scale;

   return vec4(color, o_color.w);
}

vec4 blinn_phong_shading(ShaderConfig config)
{
    vec4 o_color = texture(tex, v_tex_coord);
    if (!ignore_gamma_correct(config))
    {
        o_color = linearize_gamma(o_color);
    }

    vec3 normal = get_normal(config);

    vec4 light = vec4(0);
    light += light_contribution(config, u_directional_light, normal);

    for (int i = 0; i < u_num_point_lights; ++i)
    {
        light += point_light_contribution(config, u_point_lights[i], normal);
    }

    o_color *= light;

    if (!ignore_emit_texture(config) && enabled(emit_texture))
    {
        vec4 emit_color = texture(emit_texture, v_tex_coord);
        o_color += emit_color;
    }
    o_color.w = 1;

    bool no_delinearize = false;
    switch (u_tonemap_mode)
    {
        case 0:
        {
        } break;
        case 1:
        {
            o_color = reinhard(o_color);
        } break;
        case 2:
        {
            o_color = reinhard_l(o_color);
        } break;
        case 3:
        {
            no_delinearize = true;
            o_color = hejl(o_color);
        } break;
        case 4:
        {
            o_color = filmic(o_color);
        } break;
    }
    if (!ignore_gamma_correct(config) && !no_delinearize)
    {
        o_color = delinearize_gamma(o_color);
    }
    return o_color;
}

vec4 standard_shading(ShaderConfig config)
{
    return blinn_phong_shading(config);
}

void main(void)
{
    if (u_pass > 0)
    {
        o_color = texture(tex, v_tex_coord);
        if (o_color.a == 0)
        {
            discard;
        }
        return;
    }

    ShaderConfig config;
    config.mode = u_shader_mode;
    config.config = u_shader_config_flags;
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
        case 6:
        {
            o_color = texture(specular_exponent_texture, v_tex_coord);
        } break;
    }
}
