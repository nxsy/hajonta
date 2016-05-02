#version 330
uniform mat4 u_projection;
uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;

uniform mat4 u_bones[100];
uniform int u_bones_enabled;

in vec3 a_position;
in vec2 a_texcoord;

in ivec4 a_bone_ids;
in vec4 a_bone_weights;

out vec2 v_texcoord;
out vec4 v_position;

void main()
{
    mat4 bone_transform = mat4(0);
    if (u_bones_enabled > 0)
    {
        bone_transform += u_bones[a_bone_ids[0]] * a_bone_weights[0];
        bone_transform += u_bones[a_bone_ids[1]] * a_bone_weights[1];
        bone_transform += u_bones[a_bone_ids[2]] * a_bone_weights[2];
        bone_transform += u_bones[a_bone_ids[3]] * a_bone_weights[3];
    }
    else
    {
        bone_transform = mat4(1);
    }

    vec4 post_bone_position = bone_transform * vec4(a_position, 1.0);
    vec4 w_position = u_model_matrix * post_bone_position;
    v_texcoord = a_texcoord;
    v_position = u_projection * u_view_matrix * w_position;
    gl_Position = v_position;
}
