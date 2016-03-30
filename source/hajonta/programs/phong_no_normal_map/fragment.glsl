#version 330

uniform sampler2D tex;
uniform sampler2D shadowmap_tex;

uniform int u_lightspace_available;

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

out vec4 o_color;

float in_shadow(vec4 lightspace_position, vec3 normal, vec3 light_dir)
{
    vec3 lightspace_coords = lightspace_position.xyz / lightspace_position.w;
    lightspace_coords = (1.0f + lightspace_coords) * 0.5f;
    float light_depth = texture(shadowmap_tex, lightspace_coords.xy).r;
    float fragment_depth = lightspace_coords.z;
    float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);
    return fragment_depth - bias > light_depth ? 1.0 : 0.0;
}

void main()
{
    vec4 material_color = texture(tex, v_texcoord);

    float ambient = light.ambient_intensity;

    vec3 w_normal = normalize(v_w_normal);
    vec3 w_surface_to_light_direction = -light.position_or_direction.xyz;
    float direction_similarity = max(dot(w_normal, w_surface_to_light_direction), 0.0);
    float diffuse = light.diffuse_intensity * direction_similarity;

    vec3 w_surface_to_eye = normalize(-v_w_position);
    vec3 w_reflection_direction = reflect(-w_surface_to_light_direction.xyz, w_normal);
    float specular = pow(max(dot(w_surface_to_eye, w_reflection_direction), 0.0), 10.0f);

    float attenuation = 1.0f;
    if (light.position_or_direction.w > 0.0f)
    {
        float distance = length(light.position_or_direction.xyz - v_w_position);
        attenuation = 1.0f / (light.attenuation_constant + light.attenuation_linear * distance + light.attenuation_exponential * (distance * distance));
    }

    vec3 light_color = light.color;
    if (u_lightspace_available == 1)
    {
        light_color *= (1.0f - in_shadow(v_l_position, w_normal, w_surface_to_light_direction));
    }
    o_color = vec4((ambient + diffuse + specular) * attenuation * material_color.rgb * light_color, 1.0f);
    o_color.a = material_color.a;
    if (o_color.a < 0.001)
    {
        discard;
    }
}
