#version 410 core

#ifdef GL_ARB_shader_draw_parameters
#extension GL_ARB_shader_draw_parameters : enable
#endif

precision highp float;
precision highp int;
layout(std140, column_major) uniform;

in vec3 a_position;
in vec2 a_texcoord;
in ivec4 a_bone_ids;
in vec4 a_bone_weights;

out vec2 v_texcoord;
out vec4 v_position;
flat out uint v_draw_id;

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

layout(std140) uniform CB3
{
    mat4 bones[100];
};

uniform int u_draw_data_index;
 uniform bool skip = false;

void main()
{
    uint draw_data_index = u_draw_data_index;
#ifdef GL_ARB_shader_draw_parameters
    draw_data_index += gl_DrawIDARB;
#endif
    v_draw_id = draw_data_index;
    DrawData dd = draw_data[draw_data_index];
    mat4 model_matrix = dd.model;
    mat4 view_matrix = dd.view;
    mat4 projection_matrix = dd.projection;

    vec4 position = vec4(a_position, 1.0);

    if (dd.bone_offset >= 0)
    {
        mat4 bone_transform = mat4(0);
        bone_transform += bones[dd.bone_offset + a_bone_ids[0]] * a_bone_weights[0];
        bone_transform += bones[dd.bone_offset + a_bone_ids[1]] * a_bone_weights[1];
        bone_transform += bones[dd.bone_offset + a_bone_ids[2]] * a_bone_weights[2];
        bone_transform += bones[dd.bone_offset + a_bone_ids[3]] * a_bone_weights[3];
        position = bone_transform * position;
    }

    vec4 w_position = model_matrix * position;
    v_texcoord = a_texcoord;
    v_position = projection_matrix * view_matrix * w_position;
    gl_Position = v_position;
}
