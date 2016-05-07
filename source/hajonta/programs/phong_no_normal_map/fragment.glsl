#version 330

uniform sampler2D tex;
uniform sampler2DShadow shadowmap_tex;
uniform sampler2D shadowmap_color_tex;

uniform int u_lightspace_available;
uniform int u_shadow_mode;
uniform int u_poisson_spread;
uniform float u_bias;
uniform int u_pcf_distance;
uniform int u_poisson_samples;
uniform float u_poisson_position_granularity;
uniform float u_minimum_variance;
uniform float u_lightbleed_compensation;
uniform vec3 u_camera_position;

struct Light {
    vec4 position_or_direction;
    vec3 color;

    float ambient_intensity;
    float diffuse_intensity;

    float attenuation_constant;
    float attenuation_linear;
    float attenuation_exponential;
};

uniform Light light;

in vec3 v_w_position;
in vec2 v_texcoord;
in vec3 v_w_normal;
in vec4 v_l_position;

flat in ivec4 v_bone_ids;
in vec4 v_bone_weights;

out vec4 o_color;

vec2 poissonDisk[16] = vec2[](
   vec2(-0.94201624,-0.39906216),
   vec2( 0.94558609,-0.76890725),
   vec2(-0.09418410,-0.92938870),
   vec2( 0.34495938, 0.29387760),
   vec2(-0.91588581, 0.45771432),
   vec2(-0.81544232,-0.87912464),
   vec2(-0.38277543, 0.27676845),
   vec2( 0.97484398, 0.75648379),
   vec2( 0.44323325,-0.97511554),
   vec2( 0.53742981,-0.47373420),
   vec2(-0.26496911,-0.41893023),
   vec2( 0.79197514, 0.19090188),
   vec2(-0.24188840, 0.99706507),
   vec2(-0.81409955, 0.91437590),
   vec2( 0.19984126, 0.78641367),
   vec2( 0.14383161,-0.14100790)
);

float linstep(float low, float high, float v)
{
    return clamp((v-low)/(high-low), 0.0, 1.0);
}

float shadow_visibility_vsm(vec4 lightspace_position, vec3 normal, vec3 light_dir)
{
    vec3 lightspace_coords = lightspace_position.xyz / lightspace_position.w;
    lightspace_coords = (1.0f + lightspace_coords) * 0.5f;
    vec2 moments = texture(shadowmap_color_tex, lightspace_coords.xy).xy;
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

float shadow_visibility(vec4 lightspace_position, vec3 normal, vec3 light_dir)
{
    vec3 lightspace_coords = lightspace_position.xyz / lightspace_position.w;
    lightspace_coords = (1.0f + lightspace_coords) * 0.5f;
    float fragment_depth = lightspace_coords.z;
    return texture(shadowmap_tex, vec3(lightspace_coords.xy, fragment_depth + u_bias));
}

float shadow_visibility_pcf(vec4 lightspace_position, vec3 normal, vec3 light_dir)
{
    vec3 lightspace_coords = lightspace_position.xyz / lightspace_position.w;
    lightspace_coords = (1.0f + lightspace_coords) * 0.5f;

    float x_offset = 1.0 / 4096.0f;
    float y_offset = 1.0 / 4096.0f;

    float accumulator = 0.0;
    float weight_accumulator = 0.0f;

    for (int y = -u_pcf_distance ; y <= u_pcf_distance ; y++) {
        for (int x = -u_pcf_distance ; x <= u_pcf_distance ; x++) {
            float weight = 1.0f;
            vec3 lookup_offset = vec3(x * x_offset, y * y_offset, u_bias);
            vec3 lookup_coord = lightspace_coords + lookup_offset;
            accumulator += weight * texture(shadowmap_tex, lookup_coord);
            weight_accumulator += weight;
        }
    }

    return (accumulator / weight_accumulator);
}
float rand(vec2 n)
{
  return 0.5 + 0.5 * fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

float random(vec3 seed, int i) {
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float shadow_visibility_poisson(vec4 lightspace_position, vec3 normal, vec3 light_dir)
{
    vec3 lightspace_coords = lightspace_position.xyz / lightspace_position.w;
    lightspace_coords = (1.0f + lightspace_coords) * 0.5f;
    float fragment_depth = lightspace_coords.z;

    float visibility = 1.0;
    float weight = 1.0f / u_poisson_samples;
    for (int i =  0; i < u_poisson_samples; i++){
        int index = i;
        visibility -= weight * (1.0 - texture(shadowmap_tex, vec3(lightspace_coords.xy + poissonDisk[index] / u_poisson_spread, lightspace_coords.z + u_bias)));
    }
    return visibility;
}

float shadow_visibility_poisson_random(vec4 lightspace_position, vec3 normal, vec3 light_dir)
{
    vec3 lightspace_coords = lightspace_position.xyz / lightspace_position.w;
    lightspace_coords = (1.0f + lightspace_coords) * 0.5f;
    float fragment_depth = lightspace_coords.z;

    float visibility = 1.0;
    float weight = 1.0f / u_poisson_samples;
    for (int i =  0; i < u_poisson_samples; i++){
        int index = int(16.0 * random(floor(v_w_position.xyz * u_poisson_position_granularity), i)) % 16;
        visibility -= weight * (1.0 - texture(shadowmap_tex, vec3(lightspace_coords.xy + poissonDisk[index] / u_poisson_spread, lightspace_coords.z + u_bias)));
    }
    return visibility;
}

void main()
{
    vec4 material_color = texture(tex, v_texcoord);

    float ambient = light.ambient_intensity;

    vec3 w_normal = normalize(v_w_normal);
    vec3 w_surface_to_light_direction = -light.position_or_direction.xyz;
    float direction_similarity = max(dot(w_normal, w_surface_to_light_direction), 0.0);
    float diffuse = light.diffuse_intensity * direction_similarity;

    vec3 w_surface_to_eye = normalize(-u_camera_position - v_w_position);
    vec3 w_reflection_direction = reflect(-w_surface_to_light_direction.xyz, w_normal);
    float specular = pow(max(dot(w_surface_to_eye, w_reflection_direction), 0.0), 10.0f);

    float attenuation = 1.0f;
    if (light.position_or_direction.w > 0.0f)
    {
        float distance = length(light.position_or_direction.xyz - v_w_position);
        attenuation = 1.0f / (light.attenuation_constant + light.attenuation_linear * distance + light.attenuation_exponential * (distance * distance));
    }

    float visibility = 1.0f;
    if (u_lightspace_available == 1)
    {
        switch (u_shadow_mode)
        {
            case 0:
            {
                o_color = vec4((ambient + visibility * (diffuse + specular)) * attenuation * material_color.rgb * light.color, 1.0f);
            } break;
            case 1:
            {
                visibility = shadow_visibility(v_l_position, w_normal, w_surface_to_light_direction);
                o_color = vec4((ambient + visibility * (diffuse + specular)) * attenuation * material_color.rgb * light.color, 1.0f);
            } break;
            case 2:
            {
                visibility = shadow_visibility_poisson(v_l_position, w_normal, w_surface_to_light_direction);
                o_color = vec4((ambient + visibility * (diffuse + specular)) * attenuation * material_color.rgb * light.color, 1.0f);
            } break;
            case 3:
            {
                visibility = shadow_visibility_poisson_random(v_l_position, w_normal, w_surface_to_light_direction);
                o_color = vec4((ambient + visibility * (diffuse + specular)) * attenuation * material_color.rgb * light.color, 1.0f);
            } break;
            case 4:
            {
                visibility = shadow_visibility_pcf(v_l_position, w_normal, w_surface_to_light_direction);
                o_color = vec4((ambient + visibility * (diffuse + specular)) * attenuation * material_color.rgb * light.color, 1.0f);
            } break;
            case 5:
            {
                vec3 lightspace_coords = v_l_position.xyz / v_l_position.w;
                lightspace_coords = (1.0f + lightspace_coords) * 0.5f;
                o_color = texture(shadowmap_color_tex, lightspace_coords.xy);
            } break;
            case 6:
            {
                visibility = shadow_visibility_vsm(v_l_position, w_normal, w_surface_to_light_direction);
                o_color = vec4((ambient + visibility * (diffuse + specular)) * attenuation * material_color.rgb * light.color, 1.0f);
            } break;
            case 7:
            {
                if (v_bone_weights.x > 0)
                {
                    o_color = vec4(v_bone_weights.xyz, 1);
                }
                else
                {
                    o_color = vec4(1,0,1,1);
                }
            } break;
            case 8:
            {
                if (v_bone_weights.x > 0)
                {
                    o_color = vec4(v_bone_ids.xyz, 1);
                }
                else
                {
                    o_color = vec4(1,0,1,1);
                }
            } break;
        }
    }
    o_color.a = material_color.a;
    if (o_color.a < 0.001)
    {
        discard;
    }
}
