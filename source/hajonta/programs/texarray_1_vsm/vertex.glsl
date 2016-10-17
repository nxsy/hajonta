#version 410 core

precision highp float;
precision highp int;
layout(std140, column_major) uniform;

in vec3 a_position;
in vec2 a_texcoord;

out vec2 v_texcoord;
out vec4 v_position;

struct DrawData
{
    mat4 projection;
    mat4 view;
    mat4 model;
    int texture_texaddress_index;
    int shadowmap_texaddress_index;
    int shadowmap_color_texaddress_index;
    int light_index;
    mat4 lightspace_matrix;
    vec3 camera_position;
};

layout(std140) uniform CB1
{
    DrawData draw_data[100];
};
uniform int u_draw_data_index;

void main()
{
    DrawData dd = draw_data[u_draw_data_index];
    mat4 model_matrix = dd.model;
    mat4 view_matrix = dd.view;
    mat4 projection_matrix = dd.projection;

    vec4 w_position = model_matrix * vec4(a_position, 1);
    v_texcoord = a_texcoord;
    v_position = projection_matrix * view_matrix * w_position;
    gl_Position = v_position;
}
