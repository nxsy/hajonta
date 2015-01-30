#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <windows.h>
#define SLASH "\\"
#elif defined(__linux__)
#include <linux/limits.h>
#define MAX_PATH PATH_MAX
#define _snprintf snprintf
#define SLASH "/"
#endif

int
mkdir_recursively(char *path)
{
    int path_offset = 0;
    while(char *next_slash = strstr(path + path_offset, SLASH))
    {
        path_offset = next_slash - path + 1;
        char directory[MAX_PATH];
        _snprintf(directory, MAX_PATH, "%.*s", (int32_t)(next_slash - path), path);
#if defined(_WIN32)
        CreateDirectory(directory, 0);
#else
        mkdir(directory, 0777);
#endif
    }
    return 0;
}

int
main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Need three arguments:\n"
                "1. base source directory\n"
                "1. base program directory\n"
                "2. name of program directory containing vertex and fragment shader.\n\n");
        return 1;
    }

    char vertexshader[MAX_PATH] = {};
    _snprintf(vertexshader, sizeof(vertexshader), "%s%s%s%s%s%svertex.glsl", argv[1], SLASH, argv[2], SLASH, argv[3], SLASH);
    char eglvertexshader[MAX_PATH] = {};
    _snprintf(eglvertexshader, sizeof(eglvertexshader), "%s%s%s%s%s%svertex.eglsl", argv[1], SLASH, argv[2], SLASH, argv[3], SLASH);
    char fragshader[MAX_PATH] = {};
    _snprintf(fragshader, sizeof(fragshader), "%s%s%s%s%s%sfragment.glsl", argv[1], SLASH, argv[2], SLASH, argv[3], SLASH);
    char eglfragshader[MAX_PATH] = {};
    _snprintf(eglfragshader, sizeof(eglfragshader), "%s%s%s%s%s%sfragment.eglsl", argv[1], SLASH, argv[2], SLASH, argv[3], SLASH);

    FILE *v = fopen(vertexshader, "r");
    FILE *eglv = fopen(eglvertexshader, "r");
    FILE *f = fopen(fragshader, "r");
    FILE *eglf = fopen(eglfragshader, "r");
    if (!v)
    {
        printf("Failed to open %s\n\n", vertexshader);
        return 1;
    }

    if (!eglv)
    {
        printf("Failed to open %s\n\n", vertexshader);
        return 1;
    }

    if (!f)
    {
        printf("Failed to open %s\n\n", fragshader);
        return 1;
    }

    if (!eglf)
    {
        printf("Failed to open %s\n\n", fragshader);
        return 1;
    }

    char outputprogram[MAX_PATH] = {};
    _snprintf(outputprogram, sizeof(outputprogram), "generated%s%s%s%s.h", SLASH, argv[2], SLASH, argv[3]);
    mkdir_recursively(outputprogram);
    char outputprogramtemp[MAX_PATH] = {};
    _snprintf(outputprogramtemp, sizeof(outputprogramtemp), "%s.tmp", outputprogram);
    FILE *p = fopen(outputprogramtemp, "w");
    if (!p)
    {
        printf("Failed to open %s\n\n", outputprogramtemp);
        return 1;
    }

    char buffer[1024];

    strcpy(buffer, "struct a_program_struct\n{\n    uint32_t program;\n");
    fwrite(buffer, 1, strlen(buffer), p);

    char structbuffer[1024] = {};
    char *start_of_next_write = structbuffer;
    char uniforms[16][128] = {};
    char attribs[16][128] = {};
    uint32_t num_uniforms = sizeof(uniforms) / sizeof(uniforms[0]);
    uint32_t num_attribs = sizeof(uniforms) / sizeof(uniforms[0]);

    uint32_t uniform_index = 0;
    uint32_t attribute_index = 0;

    while (feof(v) == 0)
    {
        int size_read = fread(start_of_next_write, 1, structbuffer + sizeof(structbuffer) - start_of_next_write - 1, v);
        char *line = structbuffer;
        do {
            char *next_line_ending = strchr(line, '\n');
            int in_uniform = 0;
#define IN_LENGTH 3
#define UNIFORM_LENGTH 8
            if (strncmp(line, "in ", IN_LENGTH) == 0)
            {
                in_uniform = IN_LENGTH;
            }
            else if (strncmp(line, "uniform ", UNIFORM_LENGTH) == 0)
            {
                in_uniform = UNIFORM_LENGTH;
            }
            if (in_uniform)
            {
                char *next_space = strchr(line + in_uniform, ' ');
                if (next_space)
                {
                    ++next_space;
                }
                char *next_semicolon = strchr(line, ';');
                if (next_space && next_space < next_line_ending && next_semicolon && next_semicolon < next_line_ending)
                {
                    fwrite("    int32_t ", 1, 12, p);
                    strncpy(buffer, next_space, next_semicolon - next_space);
                    if (in_uniform == IN_LENGTH)
                    {
                        strncpy(attribs[attribute_index++], next_space, next_semicolon - next_space);
                        if (attribute_index == num_attribs)
                        {
                            printf("Only support %d attributes\n", num_attribs);
                            return 1;
                        }
                    }
                    else if (in_uniform == UNIFORM_LENGTH)
                    {
                        strncpy(uniforms[uniform_index++], next_space, next_semicolon - next_space);
                        if (uniform_index == num_uniforms)
                        {
                            printf("Only support %d uniforms\n", num_uniforms);
                            return 1;
                        }
                    }
                    fwrite(buffer, 1, next_semicolon - next_space, p);
                    fwrite("_id;\n", 1, 5, p);
                }
            }
            if (next_line_ending)
            {
                line = next_line_ending + 1;
            }
        } while (strchr(line, '\n'));

        //start_of_next_write = (char *)structbuffer + sizeof(structbuffer) - line;
        //memmove(structbuffer, line, sizeof(structbuffer) - line);
    }

    fseek(v, 0, SEEK_SET);

    strcpy(buffer, "};\n\n");
    fwrite(buffer, 1, strlen(buffer), p);

    strcpy(buffer, "bool a_program(a_program_struct *state, hajonta_thread_context *ctx, platform_memory *memory)\n{\n");
    fwrite(buffer, 1, strlen(buffer), p);
    strcpy(buffer, "    state->program = glCreateProgram();\n");
    fwrite(buffer, 1, strlen(buffer), p);
    strcpy(buffer, "    char *vertex_shader_source = R\"EOF(\n");
    fwrite(buffer, 1, strlen(buffer), p);

    while (feof(v) == 0)
    {
        int size_read = fread(buffer, 1, sizeof(buffer), v);
        int size_write = fwrite(buffer, 1, size_read, p);
        if (size_read != size_write)
        {
            printf("size_read (%d) != size_write(%d)\n", size_read, size_write);
            return 1;
        }
    }

    strcpy(buffer, "\n)EOF\";\n");
    fwrite(buffer, 1, strlen(buffer), p);

    strcpy(buffer, "    char *fragment_shader_source = R\"EOF(\n");
    fwrite(buffer, 1, strlen(buffer), p);

    while (feof(f) == 0)
    {
        int size_read = fread(buffer, 1, sizeof(buffer), f);
        int size_write = fwrite(buffer, 1, size_read, p);
        if (size_read != size_write)
        {
            printf("size_read (%d) != size_write(%d)\n", size_read, size_write);
            return 1;
        }
    }
    strcpy(buffer, "\n)EOF\";\n");
    fwrite(buffer, 1, strlen(buffer), p);

    strcpy(buffer, "    char *egl_vertex_shader_source = R\"EOF(\n");
    fwrite(buffer, 1, strlen(buffer), p);

    while (feof(eglv) == 0)
    {
        int size_read = fread(buffer, 1, sizeof(buffer), eglv);
        int size_write = fwrite(buffer, 1, size_read, p);
        if (size_read != size_write)
        {
            printf("size_read (%d) != size_write(%d)\n", size_read, size_write);
            return 1;
        }
    }

    strcpy(buffer, "\n)EOF\";\n");
    fwrite(buffer, 1, strlen(buffer), p);

    strcpy(buffer, "    char *egl_fragment_shader_source = R\"EOF(\n");
    fwrite(buffer, 1, strlen(buffer), p);

    while (feof(eglf) == 0)
    {
        int size_read = fread(buffer, 1, sizeof(buffer), eglf);
        int size_write = fwrite(buffer, 1, size_read, p);
        if (size_read != size_write)
        {
            printf("size_read (%d) != size_write(%d)\n", size_read, size_write);
            return 1;
        }
    }
    strcpy(buffer, "\n)EOF\";\n");
    fwrite(buffer, 1, strlen(buffer), p);

    char midbuffer[] = R"EOF(
    uint32_t vertex_shader_id;
    uint32_t fragment_shader_id;

    {
        uint32_t shader = vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        if (!shader)
        {
            memory->platform_fail(ctx, "Failed to allocate shader");
            return false;
        }
        int compiled;
#if defined(NEEDS_EGL)
        glShaderSource(shader, 1, (const char **)&egl_vertex_shader_source, 0);
#else
        glShaderSource(shader, 1, (const char **)&vertex_shader_source, 0);
#endif
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char info_log[1024] = {};
            strcpy(info_log, "vertex: ");
            glGetShaderInfoLog(shader, sizeof(info_log) - strlen(info_log), 0, info_log + strlen(info_log));
            memory->platform_fail(ctx, info_log);
            return false;
        }
    }
    {
        uint32_t shader = fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
        int compiled;
#if defined(NEEDS_EGL)
        glShaderSource(shader, 1, (const char**)&egl_fragment_shader_source, 0);
#else
        glShaderSource(shader, 1, (const char**)&fragment_shader_source, 0);
#endif
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char info_log[1024] = {};
            strcpy(info_log, "fragment: ");
            glGetShaderInfoLog(shader, sizeof(info_log) - strlen(info_log), 0, info_log + strlen(info_log));
            memory->platform_fail(ctx, info_log);
            return false;
        }
    }
    glAttachShader(state->program, vertex_shader_id);
    glAttachShader(state->program, fragment_shader_id);
    glLinkProgram(state->program);

    int linked;
    glGetProgramiv(state->program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char info_log[1024] = {};
        glGetProgramInfoLog(state->program, sizeof(info_log), 0, info_log);
        memory->platform_fail(ctx, info_log);
        return false;
    }

    glUseProgram(state->program);
)EOF";

    fwrite(midbuffer, 1, strlen(midbuffer), p);

    for (uint32_t i = 0;
            i < uniform_index;
            ++i)
    {
        char uniform_location_string[1024];
        sprintf(uniform_location_string, R"EOF(
    state->%s_id = glGetUniformLocation(state->program, "%s");
    if (state->%s_id < 0) {
        char info_log[] = "Could not locate %s uniform";
        memory->platform_fail(ctx, info_log);
        return false;
    }
)EOF", uniforms[i], uniforms[i], uniforms[i], uniforms[i]);
        fwrite(uniform_location_string, 1, strlen(uniform_location_string), p);
    }

    for (uint32_t i = 0;
            i < attribute_index;
            ++i)
    {
        char attrib_location_string[1024];
        sprintf(attrib_location_string, R"EOF(
    state->%s_id = glGetAttribLocation(state->program, "%s");
    if (state->%s_id < 0) {
        char info_log[] = "Could not locate %s attribute";
        memory->platform_fail(ctx, info_log);
        return false;
    }
)EOF", attribs[i], attribs[i], attribs[i], attribs[i]);
        fwrite(attrib_location_string, 1, strlen(attrib_location_string), p);
    }

    char midbuffer2[] = R"EOF(
    return true;
}
)EOF";
    fwrite(midbuffer2, 1, strlen(midbuffer2), p);
    fclose(p);

#if defined(_WIN32)
    CopyFile(outputprogramtemp, outputprogram, 0);
#else
    FILE *temp = fopen(outputprogramtemp, "r");
    FILE *final = fopen(outputprogram, "w");

    while (feof(temp) == 0)
    {
        int size_read = fread(buffer, 1, sizeof(buffer), temp);
        int size_write = fwrite(buffer, 1, size_read, final);
        if (size_read != size_write)
        {
            printf("size_read (%d) != size_write(%d)\n", size_read, size_write);
            return 1;
        }
    }
    fclose(f);
#endif
}
