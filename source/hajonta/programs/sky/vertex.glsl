#version 330

uniform mat4 u_model_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;

uniform vec3 u_camera_position;
uniform vec3 u_light_direction;

in vec3 a_position;

out vec3 v_w_position;
out vec3 v3Direction;
out vec3 frontColor;
out vec3 secondaryFrontColor;

float scale(float fCos)
{
    float fScaleDepth = 0.25;
    float x = 1.0 - fCos;
    return fScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

const int nSamples = 3;
const float fSamples = 3.0;

int quadratic(float a, float b, float c, out float t0, out float t1) 
{
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
    {
        return 0;
    }

    if (discriminant == 0)
    {
        t0 = -0.5 * b / a;
        return 1;
    }

    float q;
    float discriminant_sqrt = sqrt(discriminant);
    if (b > 0)
    {
        q = -0.5 * (b + discriminant_sqrt);
    }
    else
    {
        q = -0.5 * (b - discriminant_sqrt);
    }

    float x0 = q / a;
    float x1 = c / q;
    if (x0 > x1)
    {
        t0 = x1;
        t1 = x0;
    }
    else
    {
        t0 = x0;
        t1 = x1;
    }

    return 2;
}

float distance_to_edge_of_atmosphere(float height, float earth_radius, float atmosphere_radius, vec3 direction)
{
    float earth_height = earth_radius + height;

    vec3 L = vec3(0, earth_height, 0);
    float radius2 = atmosphere_radius * atmosphere_radius;
    float a = dot(direction, direction);
    float b = 2 * dot(direction, L);
    float c = dot(L, L) - radius2;

    float t0;
    float t1;
    if (quadratic(a, b, c, t0, t1) == 0)
    {
        return 0;
    }

    return t1;
}

void main()
{
    vec4 w_position = u_model_matrix * vec4(a_position, 1.0f);
    v_w_position = w_position.xyz / w_position.w;
    gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vec4(a_position, 1.0f);

    float kr = 0.0025;
    float km = 0.0015;
    float sun_brightness = 15.0f;
    float fKrESun = sun_brightness * kr;
    float fKmESun = sun_brightness * km;
    float pi = 3.14159265358979f;
    float fKr4PI = kr * 4 * pi;
    float fKm4PI = km * 4 * pi;
    float fInnerRadius = 6356752.0f;
    float fOuterRadius = fInnerRadius * 1.025;
    float fScale = 1 / (fOuterRadius - fInnerRadius);
    float fScaleDepth = 0.25;
    float fScaleOverScaleDepth = fScale / fScaleDepth;

    // Get the ray from the camera to the vertex, and its length (which is the far point of the ray passing through the atmosphere)
    vec3 ray = v_w_position.xyz - u_camera_position;
    float ray_length = length(ray);
    ray /= ray_length;

    // Recalculate ray length for real-world scale
    ray_length = distance_to_edge_of_atmosphere(u_camera_position.y, fInnerRadius, fOuterRadius, ray);

    vec3 wavelength = vec3(0.650, 0.570, 0.475);
    vec3 wavelength_to_4 = vec3(
        pow(wavelength.r, 4),
        pow(wavelength.g, 4),
        pow(wavelength.b, 4)
    );
    vec3 v3InvWavelength = 1.0f / wavelength_to_4;

    vec3 start = u_camera_position + vec3(0, fInnerRadius + 10.0f, 0);
    float height = length(start);

    // ---------------------------------------------------------------
    // Beginning of Sean O'Neil's atmospheric scattering from GPU Gems
    // ---------------------------------------------------------------

    float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - height));
    float fStartAngle = dot(ray, start) / height;
    float fStartOffset = fDepth * scale(fStartAngle);

    // Initialize the scattering loop variables
    float fSampleLength = ray_length / fSamples;
    float fScaledLength = fSampleLength * fScale;
    vec3 v3SampleRay = ray * fSampleLength;
    vec3 v3SamplePoint = start + v3SampleRay * 0.5;

    // Now loop through the sample rays
    vec3 v3FrontColor = vec3(0.0, 0.0, 0.0);
    vec3 v3LightPos = -u_light_direction;
    v3LightPos /= length(v3LightPos);
    for(int i=0; i<nSamples; i++)
    {
        float fHeight = length(v3SamplePoint);
        float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
        float fLightAngle = dot(v3LightPos, v3SamplePoint) / fHeight;
        float fCameraAngle = dot(ray, v3SamplePoint) / fHeight * 0.99;
        float fScatter = (fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle)));
        vec3 v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
        v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
        v3SamplePoint += v3SampleRay;
    }
    //v3FrontColor = clamp(v3FrontColor, 0, 2);

    // Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader
    secondaryFrontColor = v3FrontColor * fKmESun;
    frontColor = v3FrontColor * (v3InvWavelength * fKrESun);
    v3Direction = u_camera_position - v_w_position.xyz;
}
