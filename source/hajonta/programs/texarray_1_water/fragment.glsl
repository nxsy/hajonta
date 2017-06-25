#version 410 core

precision highp float;
precision highp int;
layout(std140, column_major) uniform;

struct
ShaderConfig
{
    vec3 camera_position;
    int reflection_texaddress_index;
    int refraction_texaddress_index;
    int refraction_depth_texaddress_index;
    int dudv_map_texaddress_index;
    int normal_map_texaddress_index;
    float tiling;

    float wave_strength;
    float move_factor;
    float minimum_variance;
    float bias;
    float lightbleed_compensation;
    float near;
    float far;
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

in vec3 v_w_position;
in vec4 v_l_position;
in vec4 v_c_position;
in vec2 v_texcoord;
flat in uint v_draw_id;

out vec4 o_color;

vec2
distortion1()
{
    Tex2DAddress addr = texAddress[shader_config.dudv_map_texaddress_index];
    vec3 texCoord = vec3(v_texcoord.x + shader_config.move_factor, v_texcoord.y, addr.Page);
    vec2 distortion1 = texture(TexContainer[addr.Container], texCoord).rg;

    texCoord = vec3(-v_texcoord.x + shader_config.move_factor, v_texcoord.y + shader_config.move_factor, addr.Page);
    vec2 distortion2 = texture(TexContainer[addr.Container], texCoord).rg;

    return distortion1 + distortion2;
}

struct
Distortion
{
    vec2 distortion;
    vec3 normal;
};

Distortion
distortion2()
{
    Distortion d;
    Tex2DAddress addr = texAddress[shader_config.dudv_map_texaddress_index];

    vec2 texcoord2 = vec2(v_texcoord.x + shader_config.move_factor, v_texcoord.y);
    vec3 texCoord = vec3(texcoord2, addr.Page);

    vec2 distorted_texcoord = texture(TexContainer[addr.Container], texCoord).rg * 0.1f;
    distorted_texcoord = v_texcoord + vec2(distorted_texcoord.x, distorted_texcoord.y+shader_config.move_factor);

    texCoord = vec3(distorted_texcoord, addr.Page);
    d.distortion = texture(TexContainer[addr.Container], texCoord).rg * 2 - 1;

    addr = texAddress[shader_config.normal_map_texaddress_index];
    texCoord = vec3(distorted_texcoord, addr.Page);
    d.normal = texture(TexContainer[addr.Container], texCoord).rgb;
    d.normal = vec3(
        d.normal.r * 2 - 1,
        d.normal.b * 2 - 1,
        d.normal.g * 2 - 1
    );
    d.normal = normalize(d.normal);

    return d;
}

float linstep(float low, float high, float v)
{
    return clamp((v-low)/(high-low), 0.0, 1.0);
}

float shadow_visibility_vsm(vec4 lightspace_position, vec3 light_dir)
{
    DrawData dd = draw_data[v_draw_id];
    vec3 lightspace_coords = lightspace_position.xyz / lightspace_position.w;
    lightspace_coords = (1.0f + lightspace_coords) * 0.5f;


    Tex2DAddress addr = texAddress[dd.shadowmap_color_texaddress_index];
    vec3 texCoord = vec3(lightspace_coords.xy, addr.Page);
    vec2 moments = texture(TexContainer[addr.Container], texCoord).xy;

    float moment1 = moments.r;
    float moment2 = moments.g;

    float variance = max(moment2 - moment1 * moment1, shader_config.minimum_variance);
    float fragment_depth = lightspace_coords.z + shader_config.bias;
    float diff = fragment_depth - moment1;
    if(diff > 0.0 && lightspace_coords.z > moment1)
    {
        float visibility = variance / (variance + diff * diff);
        visibility = linstep(shader_config.lightbleed_compensation, 1.0f, visibility);
        visibility = clamp(visibility, 0.0f, 1.0f);
        return visibility;
    }
    else
    {
        return 1.0;
    }
}


void main()
{
    DrawData dd = draw_data[v_draw_id];
    if (shader_config.refraction_texaddress_index >= 0)
    {
        vec2 ndc = (v_c_position.xy / v_c_position.w + 1.0f) / 2.0f;
        vec2 refraction = ndc;
        vec2 reflection = vec2(ndc.x, -ndc.y);

        Tex2DAddress addr;
        vec3 texCoord;

        addr = texAddress[shader_config.refraction_depth_texaddress_index];
        texCoord = vec3(ndc, addr.Page);
        float depth = 2 * texture(TexContainer[addr.Container], texCoord).r - 1;
        float near = shader_config.near;
        float far = shader_config.far;

        float floor_distance = 
            (2.0 * near * far) /
            (far + near - depth * (far - near));

        float water_depth = 2 * gl_FragCoord.z - 1;
        float water_distance = 
            (2.0 * near * far) /
            (far + near - water_depth * (far - near));

        float water_floor_distance = floor_distance - water_distance;

        Distortion d = distortion2();

        d.distortion *= pow(clamp(water_floor_distance / 2, 0, 1), 2.0f);
        float distance_normal_step = pow(water_distance / 75, 2.0);

        d.normal.y *= mix(3, 12, distance_normal_step);
        d.distortion /= mix(1, 4, distance_normal_step);
        d.normal = normalize(d.normal);

        refraction += d.distortion * shader_config.wave_strength;
        refraction = clamp(refraction, 0.001, 0.999);
        reflection += d.distortion * shader_config.wave_strength;
        reflection.x = clamp(reflection.x, 0.001, 0.999);
        reflection.y = clamp(reflection.y, -0.999, -0.001);

        addr = texAddress[shader_config.refraction_texaddress_index];
        texCoord = vec3(refraction, addr.Page);
        vec3 refraction_color = texture(TexContainer[addr.Container], texCoord).rgb;

        addr = texAddress[shader_config.reflection_texaddress_index];
        texCoord = vec3(reflection, addr.Page);
        vec3 reflection_color = texture(TexContainer[addr.Container], texCoord).rgb;

        vec3 to_camera = normalize(shader_config.camera_position - v_w_position);
        float refractiveness = dot(to_camera, d.normal);
        refractiveness = pow(refractiveness, 0.75f);
        refractiveness = clamp(refractiveness, 0, 1);

        Light light = lights[dd.light_index];
        vec3 from_light = normalize(light.position_or_direction.xyz);

        vec3 w_surface_to_light_direction = -light.position_or_direction.xyz;
        float direction_similarity = max(
            min(
                dot(d.normal, w_surface_to_light_direction),
                dot(vec3(0,1,0), w_surface_to_light_direction)
            ),
            0.0);

        float horizon_similarity = pow(max(dot(vec3(0,1,0), w_surface_to_light_direction), 0.0), 0.5);

        float ambient = light.ambient_intensity * clamp(horizon_similarity, 0.1, 0.5) * 2.0;
        float diffuse = light.diffuse_intensity * direction_similarity;

        float specular = 0;
        if (direction_similarity > 0)
        {
            vec3 reflected_light = reflect(from_light, d.normal);
            specular = max(dot(reflected_light, to_camera), 0);
            specular = pow(specular, 20);
            //specular *= direction_similarity;
        }

        float visibility = shadow_visibility_vsm(v_l_position, w_surface_to_light_direction);
        visibility = clamp(visibility, 0.05, 1.0);
        diffuse = visibility * diffuse;
        float sh_clamp = pow(clamp(0.2 + water_floor_distance * 4, 0, 1), 2.5f);
        float specular_highlights = visibility * specular * 0.8f * sh_clamp;
        //specular_highlights = 0;


        float refraction_clamp = pow(clamp(water_floor_distance / 5, 0, 1), 1);
        refraction_color = mix(refraction_color, vec3(0.0, 0.1, 0.2), refraction_clamp);

        o_color.a = 1.0f;
        o_color.rgb = mix(reflection_color, refraction_color, refractiveness);
        //o_color.rgb = mix(o_color.rgb, vec3(0.0, 0.15, 0.25), 0.2f);
        o_color.rgb *= (ambient + diffuse);
        o_color.rgb += specular_highlights;
        o_color.rgb *= light.color;

        float transparency = mix(0.0f, 1.0f, pow(clamp(water_floor_distance * 20, 0, 1), 1.0));
        o_color.a = transparency;

        if (!skip)
        {
            //o_color.rgb = vec3(distance_normal_step);
            //o_color = vec4(transparency, transparency, transparency, 1.0f);
            //o_color = vec4(specular_highlights, specular_highlights, specular_highlights, 1.0f);
            //o_color = vec4(sh_clamp, sh_clamp, sh_clamp, 1.0f);
            //o_color *= transparency;
        /*
            o_color = vec4(
                v_w_position.xyz / 100 + 0.5,
                1);
            o_color = vec4(
                to_camera.xyz / 10 + 0.5,
                1);
            o_color = vec4(
                visibility,
                visibility,
                visibility,
                1);
            o_color = vec4(
                shader_config.minimum_variance * 100,
                shader_config.minimum_variance * 100,
                shader_config.minimum_variance * 100,
                1);
            o_color = vec4(
                abs(floor_distance / 4),
                abs(floor_distance / 8),
                abs(floor_distance / 16),
                1.0f);

            o_color = vec4(
                abs(water_distance / 4),
                abs(water_distance / 8),
                abs(water_distance / 16),
                1.0f);

            o_color = vec4(
                abs(water_floor_distance / 4),
                abs(water_floor_distance / 8),
                abs(water_floor_distance / 16),
                1.0f);
                */

            /*
            o_color = vec4(
                abs(depth) * 0,
                abs(water_depth) / 4,
                abs(water_depth) / 8,
                1.0f);
            */
            //o_color = vec4(0,0,0,0);
        }
    }
    else
    {
        o_color = vec4(1,0,1,1);
    }
}
