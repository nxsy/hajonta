#include <stdio.h>
#include <sys/stat.h>

#include <windows.h>

int
mkdir_recursively(char *path)
{
    int path_offset = 0;
    while(char *next_slash = strstr(path + path_offset, "\\"))
    {
        path_offset = next_slash - path + 1;
        char directory[MAX_PATH];
        _snprintf(directory, MAX_PATH, "%.*s", next_slash - path, path);
        CreateDirectory(directory, 0);
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
    _snprintf(vertexshader, sizeof(vertexshader), "%s\\%s\\%s\\vertex.glsl", argv[1], argv[2], argv[3]);
    char fragshader[MAX_PATH] = {};
    _snprintf(fragshader, sizeof(fragshader), "%s\\%s\\%s\\fragment.glsl", argv[1], argv[2], argv[3]);

    FILE *v = fopen(vertexshader, "r");
    FILE *f = fopen(fragshader, "r");
    if (!v)
    {
        printf("Failed to open %s\n\n", vertexshader);
        return 1;
    }

    if (!f)
    {
        printf("Failed to open %s\n\n", fragshader);
        return 1;
    }

    char outputprogram[MAX_PATH] = {};
    _snprintf(outputprogram, sizeof(outputprogram), "generated\\%s\\%s.h", argv[2], argv[3]);
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
    strcpy(buffer, "struct a_program_struct\n{ int32_t program;\n};\n\n");
    fwrite(buffer, 1, strlen(buffer), p);

    strcpy(buffer, "bool a_program(a_program_struct *state, hajonta_thread_context *ctx, platform_memory *memory)\n{\n");
    fwrite(buffer, 1, strlen(buffer), p);
    strcpy(buffer, "    state->program = glCreateProgram();");
    fwrite(buffer, 1, strlen(buffer), p);
    strcpy(buffer, "    char *vertex_shader_source = R\"EOF(\n");
    fwrite(buffer, 1, strlen(buffer), p);

    while (feof(v) == 0)
    {
        int size_read = fread(buffer, 1, sizeof(buffer), v);
        fwrite("        ", 1, 8, p);
        int size_write = fwrite(buffer, 1, size_read, p);
        if (size_read != size_write)
        {
            printf("size_read (%d) != size_write(%d)\n", size_read, size_write);
            return 1;
        }
    }

    strcpy(buffer, "\n)EOF\";\n");
    fwrite(buffer, 1, strlen(buffer), p);

    strcpy(buffer, "char *fragment_shader_source = R\"EOF(\n");
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

    char midbuffer[] = R"EOF(
    uint32_t vertex_shader_id;
    uint32_t fragment_shader_id;

    {
        uint32_t shader = vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        int compiled;
        glShaderSource(shader, 1, &vertex_shader_source, 0);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char info_log[1024];
            glGetShaderInfoLog(shader, sizeof(info_log), 0, info_log);
            memory->platform_fail(ctx, info_log);
            return false;
        }
    }
    {
        uint32_t shader = fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
        int compiled;
        glShaderSource(shader, 1, &fragment_shader_source, 0);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char info_log[1024];
            glGetShaderInfoLog(shader, sizeof(info_log), 0, info_log);
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
        char info_log[1024];
        glGetProgramInfoLog(state->program, sizeof(info_log), 0, info_log);
        memory->platform_fail(ctx, info_log);
        return false;
    }

    glUseProgram(state->program);
    return true;
    )EOF";

    fwrite(midbuffer, 1, strlen(midbuffer), p);

    strcpy(buffer, "}\n");
    fwrite(buffer, 1, strlen(buffer), p);
    fclose(p);

    CopyFile(outputprogramtemp, outputprogram, 0);
}
