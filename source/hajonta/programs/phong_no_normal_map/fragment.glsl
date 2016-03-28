#version 330

uniform sampler2D tex;

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

out vec4 o_color;

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

    o_color = vec4((ambient + diffuse + specular) * attenuation * material_color.rgb * light.color, 1.0f);
    o_color.a = material_color.a;
    if (o_color.a < 0.001)
    {
        discard;
    }
}
