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
in vec3 v3Direction;
in vec3 frontColor;
in vec3 secondaryFrontColor;

out vec4 o_color;

void main()
{
    float g = -0.95;
    float g2 = g * g;
    vec3 v3LightPos = -u_light_direction;
    v3LightPos /= length(v3LightPos);

    float fCos = dot(v3LightPos, normalize(v3Direction));
    float fRayleighPhase = 0.75 * (1.0 + fCos*fCos);
    float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos*fCos) / pow(1.0 + g2 - 2.0*g*fCos, 1.5);

    //vec3 color = fRayleighPhase * frontColor + fMiePhase * secondaryFrontColor;
    vec3 color = frontColor + fMiePhase * secondaryFrontColor;
    o_color = vec4(color, 1);

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
