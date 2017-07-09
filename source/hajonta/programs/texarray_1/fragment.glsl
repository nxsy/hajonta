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
const int show_texcontainer_index_flag = 1 << 8;
const int show_newtexcontainer_index_flag = 1 << 9;

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

struct TexContainerSamplerMapping
{
    uint generation;
    uint texcontainer_index;
};

layout(std140) uniform CB4
{
    TexContainerSamplerMapping texcontainer_sampler_mapping[32];
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

vec4 texcontainer_fetch(int texaddress_index, vec2 texcoord2)
{
    Tex2DAddress addr = texAddress[texaddress_index];
    vec3 texCoord = vec3(texcoord2, addr.Page);
    TexContainerSamplerMapping tcsm = texcontainer_sampler_mapping[addr.Container];
    //uint newContainer = texcontainer_index[tcsm.texcontainer_index].container_index;
    uint newContainer = tcsm.texcontainer_index;
    return texture(TexContainer[newContainer], texCoord);
}

float linstep(float low, float high, float v)
{
    return clamp((v-low)/(high-low), 0.0, 1.0);
}

float shadow_visibility_vsm(vec4 lightspace_position, vec3 normal, vec3 light_dir)
{
    DrawData dd = draw_data[v_draw_id];
    vec3 lightspace_coords = lightspace_position.xyz / lightspace_position.w;
    lightspace_coords = (1.0f + lightspace_coords) * 0.5f;


    vec2 moments = texcontainer_fetch(dd.shadowmap_color_texaddress_index, lightspace_coords.xy).xy;

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

    vec3 tex_normal = texcontainer_fetch(dd.normal_texaddress_index, v_texcoord.xy).xyz;

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
    if ((shader_config.flags & show_object_identifier_flag) == show_object_identifier_flag)
    {
    /*
        o_color = vec4(
            (dd.object_identifier & 1) == 1 ? 1.0f : 0.0f,
            (dd.object_identifier & 2) == 2 ? 1.0f : 0.0f,
            (dd.object_identifier & 4) == 4 ? 1.0f : 0.0f,
            1.0f
        );
        */
        o_color = vec4(
            (dd.object_identifier & 0x0000ff) / 255.0f,
            ((dd.object_identifier & 0x00ff00) >> 8) / 255.0f,
            ((dd.object_identifier & 0xff0000) >> 16) / 255.0f,
            1.0f
        );
    } else
    if ((shader_config.flags & show_texcontainer_index_flag) == show_texcontainer_index_flag)
    {
        Tex2DAddress addr = texAddress[dd.texture_texaddress_index];
        o_color = vec4(
            (addr.Container & 1) == 1 ? 0.25f : 0.0f +
            (addr.Container & 8) == 8 ? 0.5f : 0.0f,
            (addr.Container & 2) == 2 ? 0.25f : 0.0f +
            (addr.Container & 16) == 16 ? 0.5f : 0.0f,
            (addr.Container & 4) == 4 ? 0.25f : 0.0f +
            (addr.Container & 32) == 32 ? 0.5f : 0.0f,
            1.0f
        );
    } else
    if ((shader_config.flags & show_newtexcontainer_index_flag) == show_newtexcontainer_index_flag)
    {
        Tex2DAddress addr = texAddress[dd.texture_texaddress_index];
        TexContainerSamplerMapping tcsm = texcontainer_sampler_mapping[addr.Container];
        uint newContainer = tcsm.texcontainer_index;
        o_color = vec4(
            (newContainer & 1) == 1 ? 0.25f : 0.0f +
            (newContainer & 8) == 8 ? 0.5f : 0.0f,
            (newContainer & 2) == 2 ? 0.25f : 0.0f +
            (newContainer & 16) == 16 ? 0.5f : 0.0f,
            (newContainer & 4) == 4 ? 0.25f : 0.0f +
            (newContainer & 32) == 32 ? 0.5f : 0.0f,
            1.0f
        );
        if (skip)
        {
            o_color = vec4(0.25, 0.5, 0.75, 1.0);
            o_color = texcontainer_fetch(dd.texture_texaddress_index, v_texcoord.xy);
        }
    } else
    if (dd.texture_texaddress_index >= 0)
    {
        vec4 diffuse_color = texcontainer_fetch(dd.texture_texaddress_index, v_texcoord.xy);

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
                    float specular_strength = texcontainer_fetch(dd.specular_texaddress_index, v_texcoord.xy).r;
                    specular *= specular_strength;
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
    }
    else
    {
        o_color = vec4(1,0,1,1);
    }
    if ((shader_config.flags & show_normalmap_flag) == show_normalmap_flag && dd.normal_texaddress_index >= 0)
    {
        o_color = texcontainer_fetch(dd.normal_texaddress_index, v_texcoord.xy);
    }

    if ((shader_config.flags & show_specularmap_flag) == show_specularmap_flag && dd.specular_texaddress_index >= 0)
    {
        o_color = texcontainer_fetch(dd.specular_texaddress_index, v_texcoord.xy);
    }
}
