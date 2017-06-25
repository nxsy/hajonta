#version 410 core

#ifdef GL_ARB_shader_draw_parameters
#extension GL_ARB_shader_draw_parameters : enable
#endif

precision highp float;
precision highp int;
layout(std140, column_major) uniform;

struct
Plane
{
    vec3 normal;
    float distance;
};

const int normal_map_disabled_flag = 1 << 0;
const int show_normals_flag = 1 << 1;
const int show_normalmap_flag = 1 << 2;
const int specular_map_disabled_flag = 1 << 3;
const int show_specular_flag = 1 << 4;
const int show_specularmap_flag = 1 << 5;
const int show_tangent_flag = 1 << 6;
const int show_object_identifier_flag = 1 << 7;

struct
ShaderConfig
{
    Plane clipping_plane;
    int flags;
};

layout(std140) uniform SHADERCONFIG
{
    ShaderConfig shader_config;
};

struct Tex2DAddress
{
    uint Container;
    float Page;
};

layout(std140) uniform CB0
{
    Tex2DAddress texAddress[100];
};

uniform int u_draw_data_index;

struct DrawData
{
    mat4 projection;
    mat4 view;
    mat4 model;
    int texture_texaddress_index;
    int shadowmap_texaddress_index;
    int shadowmap_color_texaddress_index;
    int light_index;
    vec3 camera_position;
    int bone_offset;
    int normal_texaddress_index;
    int specular_texaddress_index;
    int object_identifier;
};

layout(std140) uniform CB1
{
    DrawData draw_data[100];
};

struct Light {
    vec4 position_or_direction;
    mat4 lightspace_matrix;
    vec3 color;

    float ambient_intensity;
    float diffuse_intensity;

    float attenuation_constant;
    float attenuation_linear;
    float attenuation_exponential;
};

layout(std140) uniform CB2
{
    Light lights[32];
};

uniform sampler2DArray TexContainer[14];

uniform bool do_not_skip = true;
 uniform bool skip = false;

uniform float u_minimum_variance;
uniform float u_lightbleed_compensation;
uniform float u_bias;

in vec3 v_w_position;
in vec2 v_texcoord;
in vec3 v_w_normal;
in vec3 v_w_tangent;
in vec4 v_l_position;
flat in uint v_draw_id;

out vec4 o_color;

float linstep(float low, float high, float v)
{
    return clamp((v-low)/(high-low), 0.0, 1.0);
}

float shadow_visibility_vsm(vec4 lightspace_position, vec3 normal, vec3 light_dir)
{
    DrawData dd = draw_data[v_draw_id];
    vec3 lightspace_coords = lightspace_position.xyz / lightspace_position.w;
    lightspace_coords = (1.0f + lightspace_coords) * 0.5f;


    Tex2DAddress addr = texAddress[dd.shadowmap_color_texaddress_index];
    vec3 texCoord = vec3(lightspace_coords.xy, addr.Page);
    vec2 moments = texture(TexContainer[addr.Container], texCoord).xy;

    float moment1 = moments.r;
    float moment2 = moments.g;

    float variance = max(moment2 - moment1 * moment1, u_minimum_variance);
    float fragment_depth = lightspace_coords.z + u_bias;
    float diff = fragment_depth - moment1;
    if(diff > 0.0 && lightspace_coords.z > moment1)
    {
        float visibility = variance / (variance + diff * diff);
        visibility = linstep(u_lightbleed_compensation, 1.0f, visibility);
        visibility = clamp(visibility, 0.0f, 1.0f);
        return visibility;
    }
    else
    {
        return 1.0;
    }
}

vec3
calculate_normal(DrawData dd)
{
    vec3 w_normal = normalize(v_w_normal);
    if ((shader_config.flags & normal_map_disabled_flag) == normal_map_disabled_flag)
    {
        return w_normal;
    }
    if (dd.normal_texaddress_index < 0)
    {
        return w_normal;
    }

    Tex2DAddress addr = texAddress[dd.normal_texaddress_index];
    vec3 texCoord = vec3(v_texcoord.xy, addr.Page);

    vec3 tex_normal = texture(TexContainer[addr.Container], texCoord).xyz;
    tex_normal = 2 * tex_normal - 1;

    vec3 n = w_normal;
    vec3 t = normalize(v_w_tangent);

    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(t, n);

    mat3 tbn = mat3(t, b, n);
    w_normal = normalize(tbn * tex_normal);
    return w_normal;
}

void main()
{
    DrawData dd = draw_data[v_draw_id];
    if (dd.texture_texaddress_index >= 0)
    {
        Tex2DAddress addr = texAddress[dd.texture_texaddress_index];
        vec3 texCoord = vec3(v_texcoord.xy, addr.Page);
        vec4 diffuse_color = texture(TexContainer[addr.Container], texCoord);
        vec3 w_normal = calculate_normal(dd);

        Light light = lights[dd.light_index];
        vec3 from_light = normalize(light.position_or_direction.xyz);
        vec3 w_surface_to_light_direction = -light.position_or_direction.xyz;

        vec3 to_camera = normalize(dd.camera_position - v_w_position);

        //float direction_similarity = max(dot(w_normal, w_surface_to_light_direction), 0.0);
        float direction_similarity = max(
            min(
                dot(w_normal, w_surface_to_light_direction),
                dot(vec3(0,1,0), w_surface_to_light_direction)
            ),
            0.0);

        float ambient = light.ambient_intensity;

        float diffuse = light.diffuse_intensity * direction_similarity;
        vec3 w_surface_to_eye = normalize(dd.camera_position - v_w_position);
        vec3 w_reflection_direction = reflect(-w_surface_to_light_direction.xyz, w_normal);
        float specular = 0;
        if (direction_similarity > 0)
        {
            vec3 reflected_light = reflect(from_light, w_normal);
            specular = max(dot(reflected_light, to_camera), 0);
            specular = pow(specular, 5);

            if ((shader_config.flags & specular_map_disabled_flag) != specular_map_disabled_flag)
            {
                if (dd.specular_texaddress_index >= 0)
                {
                    addr = texAddress[dd.specular_texaddress_index];
                    texCoord = vec3(v_texcoord.xy, addr.Page);
                    specular *= texture(TexContainer[addr.Container], texCoord).r;
                }
            }
        }
        float attenuation = 1.0f;
        float visibility = 1.0f;
        o_color = vec4(diffuse_color.xyz, 1);
        visibility = shadow_visibility_vsm(v_l_position, w_normal, w_surface_to_light_direction);
        o_color = vec4((ambient + visibility * (diffuse + specular)) * attenuation * diffuse_color.rgb * light.color, 1.0f);
        if (skip)
        {
            o_color = vec4((ambient + visibility * (diffuse + specular)) * attenuation * diffuse_color.rgb * light.color, 1.0f);
        }
        if (skip)
        {
            o_color = vec4(
                light.ambient_intensity + v_w_position.y / 5,
                light.ambient_intensity + v_w_position.y / 5,
                light.ambient_intensity + v_w_position.y / 5,
                1);
            o_color = vec4(
                visibility,
                visibility,
                visibility,
                1);
        }
        o_color.a = diffuse_color.a;
        if (o_color.a < 0.001)
        {
            discard;
        }
        if (skip)
        {
            o_color = vec4(vec3(specular), 1.0f);
        }
        if ((shader_config.flags & show_normals_flag) == show_normals_flag)
        {
            o_color = vec4(w_normal, 1.0f);
        }
        if ((shader_config.flags & show_tangent_flag) == show_tangent_flag)
        {
            o_color = vec4(v_w_tangent, 1.0f);
        }
        if ((shader_config.flags & show_specular_flag) == show_specular_flag)
        {
            o_color = vec4(vec3(specular), 1.0f);
        }
        if ((shader_config.flags & show_object_identifier_flag) == show_object_identifier_flag)
        {
            o_color = vec4(
                (dd.object_identifier & 1) == 1 ? 1.0f : 0.0f,
                (dd.object_identifier & 2) == 2 ? 1.0f : 0.0f,
                (dd.object_identifier & 4) == 4 ? 1.0f : 0.0f,
                1.0f
            );
        }
    }
    else
    {
        o_color = vec4(1,0,1,1);
    }
    if ((shader_config.flags & show_normalmap_flag) == show_normalmap_flag && dd.normal_texaddress_index >= 0)
    {
        Tex2DAddress addr = texAddress[dd.normal_texaddress_index];
        vec3 texCoord = vec3(v_texcoord.xy, addr.Page);
        o_color = texture(TexContainer[addr.Container], texCoord);
    }

    if ((shader_config.flags & show_specularmap_flag) == show_specularmap_flag && dd.specular_texaddress_index >= 0)
    {
        Tex2DAddress addr = texAddress[dd.specular_texaddress_index];
        vec3 texCoord = vec3(v_texcoord.xy, addr.Page);
        o_color = texture(TexContainer[addr.Container], texCoord);
    }
}
