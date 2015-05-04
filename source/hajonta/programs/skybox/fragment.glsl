#version 150

in vec3 v_tex_coord;

uniform samplerCube u_cube_tex;

out vec4 o_color;

void main(void)
{
    o_color = texture(u_cube_tex, v_tex_coord);
}
