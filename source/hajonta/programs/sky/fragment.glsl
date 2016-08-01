#version 330

// For portions between indicator lines:
//
// For licensing information, see http://http.developer.nvidia.com/GPUGems/gpugems_app01.html:
//
// NVIDIA Statement on the Software
//
// The source code provided is freely distributable, so long as the NVIDIA header remains unaltered and user modifications are
// detailed.
//
// No Warranty
//
// THE SOFTWARE AND ANY OTHER MATERIALS PROVIDED BY NVIDIA ON THE ENCLOSED CD-ROM ARE PROVIDED "AS IS." NVIDIA DISCLAIMS ALL
// WARRANTIES, EXPRESS, IMPLIED OR STATUTORY, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//
// Limitation of Liability
//
// NVIDIA SHALL NOT BE LIABLE TO ANY USER, DEVELOPER, DEVELOPER'S CUSTOMERS, OR ANY OTHER PERSON OR ENTITY CLAIMING THROUGH OR
// UNDER DEVELOPER FOR ANY LOSS OF PROFITS, INCOME, SAVINGS, OR ANY OTHER CONSEQUENTIAL, INCIDENTAL, SPECIAL, PUNITIVE, DIRECT
// OR INDIRECT DAMAGES (WHETHER IN AN ACTION IN CONTRACT, TORT OR BASED ON A WARRANTY), EVEN IF NVIDIA HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF THE ESSENTIAL PURPOSE OF ANY
// LIMITED REMEDY. IN NO EVENT SHALL NVIDIA'S AGGREGATE LIABILITY TO DEVELOPER OR ANY OTHER PERSON OR ENTITY CLAIMING THROUGH
// OR UNDER DEVELOPER EXCEED THE AMOUNT OF MONEY ACTUALLY PAID BY DEVELOPER TO NVIDIA FOR THE SOFTWARE OR ANY OTHER MATERIALS.
//

//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

uniform vec3 u_camera_position;
uniform vec3 u_light_direction;

in vec3 v_w_position;

out vec4 o_color;

float scale(float fCos)
{
    float fScaleDepth = 0.25;
    float x = 1.0 - fCos;
    return fScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

const int nSamples = 2;
const float fSamples = 2.0;

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
    float g = -0.95;
    float g2 = g * g;

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

    // Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader
    vec3 secondaryFrontColor = v3FrontColor * fKmESun;
    vec3 frontColor = v3FrontColor * (v3InvWavelength * fKrESun);
    vec3 v3Direction = u_camera_position - v_w_position.xyz;

    float fCos = dot(v3LightPos, v3Direction) / length(v3Direction);
    float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos*fCos) / pow(1.0 + g2 - 2.0*g*fCos, 1.5);
    o_color = vec4(frontColor.rgb + fMiePhase * secondaryFrontColor.rgb, 1);

    // ---------------------------------------------------------
    // End of Sean O'Neil's atmospheric scattering from GPU Gems
    // ---------------------------------------------------------

    o_color = vec4(
        pow(o_color.r, 1/2.2),
        pow(o_color.g, 1/2.2),
        pow(o_color.b, 1/2.2),
        1
    );
}
