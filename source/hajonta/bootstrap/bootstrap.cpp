#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "hajonta/platform/neutral.h"
#define hstrvalue(x) #x
#define hquoted(x) hstrvalue(x)

int
mkdir_recursively(char *path)
{
    int64_t path_offset = 0;
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
    char glsetup[MAX_PATH] = {};
    _snprintf(glsetup, sizeof(glsetup), "generated%shajonta%srenderer%sopengl_setup.h", SLASH, SLASH, SLASH);
    mkdir_recursively(glsetup);
    char glsetuptemp[MAX_PATH] = {};
    _snprintf(glsetuptemp, sizeof(glsetuptemp), "%s.tmp", glsetup);
    FILE *p = fopen(glsetuptemp, "w");
    if (!p)
    {
        printf("Failed to open %s\n\n", glsetuptemp);
        return 1;
    }

    char buffer[1024];

    sprintf(buffer, "#pragma once\n\n");
    fwrite(buffer, 1, strlen(buffer), p);

    sprintf(buffer, "struct GLSetupCounters {\n");
    fwrite(buffer, 1, strlen(buffer), p);
#define HGLD(b,a,...)  { sprintf(buffer, "    float %s;\n", hquoted(b)); fwrite(buffer, 1, strlen(buffer), p); }
#include "hajonta/platform/glextlist.txt"
#undef HGLD
    sprintf(buffer, R"EOF(};

static GLSetupCounters *glsetup_counters = 0;

void
ResetCounters(GLSetupCounters *counters)
{
    *counters = {};
};

struct GLCounterNamesAndOffsets {
    const char *name;
    uint32_t offset;
    char overlay[100];
} counter_names_and_offsets[] =
{
)EOF");
    fwrite(buffer, 1, strlen(buffer), p);

#define HGLD(b,a,...)  { sprintf(buffer, R"EOF(    { "%s", offsetof(GLSetupCounters, %s) }, )EOF" "\n", hquoted(b), hquoted(b)); fwrite(buffer, 1, strlen(buffer), p); }
#include "hajonta/platform/glextlist.txt"
#undef HGLD

    sprintf(buffer, "};\n\n");
    fwrite(buffer, 1, strlen(buffer), p);

#define HGLD(b,a,...) \
    { \
        sprintf(buffer, "%s %s;\n", \
            hquoted(PFNGL##a##PROC), \
            hquoted(_gl##b)); \
        fwrite(buffer, 1, strlen(buffer), p); \
    }
#include "hajonta/platform/glextlist.txt"
#undef HGLD

    sprintf(buffer, R"EOF(
inline void
load_glfuncs(platform_glgetprocaddress_func *get_proc_address)
{
)EOF");
    fwrite(buffer, 1, strlen(buffer), p);
#define HGLD(b,a,...)  { sprintf(buffer, R"EOF(    %s = (%s)get_proc_address((char *)"%s");)EOF" "\n", hquoted(_gl##b), hquoted(PFNGL##a##PROC), hquoted(gl##b)); fwrite(buffer, 1, strlen(buffer), p); }
#include "hajonta/platform/glextlist.txt"
#undef HGLD
    sprintf(buffer, "};\n");
    fwrite(buffer, 1, strlen(buffer), p);


#define HGLD(b,a,...)  { sprintf(buffer, R"EOF(
template<typename... Args>
inline auto %s(Args... args)
{
    if (glsetup_counters) {
        glsetup_counters->%s++;
    };
    return %s(args...);
}
)EOF", hquoted(hgl##b), hquoted(b), hquoted(_gl##b)); fwrite(buffer, 1, strlen(buffer), p); }
#include "hajonta/platform/glextlist.txt"
#undef HGLD

    fclose(p);

#if defined(_WIN32)
    CopyFile(glsetuptemp, glsetup, 0);
#else
    FILE *temp = fopen(glsetuptemp, "r");
    FILE *final = fopen(glsetup, "w");

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
    fclose(final);
#endif
    return 0;
}
