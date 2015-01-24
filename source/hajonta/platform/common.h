#include <stdint.h>

#ifdef HAJONTA_DEBUG
#define hassert(expression) if(!(expression)) {*(int *)0 = 0;}
#else
#define hassert(expression)
#endif

#define harray_count(array) (sizeof(array) / sizeof((array)[0]))

struct hajonta_thread_context
{
    void *opaque;
};

#define PLATFORM_FAIL(func_name) void func_name(hajonta_thread_context *ctx, char *failure_reason)
typedef PLATFORM_FAIL(platform_fail_func);

#define PLATFORM_GLGETPROCADDRESS(func_name) void* func_name(hajonta_thread_context *ctx, char *function_name)
typedef PLATFORM_GLGETPROCADDRESS(platform_glgetprocaddress_func);

struct platform_memory
{
    bool initialized;
    uint64_t size;
    void *memory;

    platform_fail_func *platform_fail;
    platform_glgetprocaddress_func *platform_glgetprocaddress;
};

struct game_input
{
    float delta_t;
};

#define GAME_UPDATE_AND_RENDER(func_name) void func_name(hajonta_thread_context *ctx, platform_memory *memory, game_input *input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render_func);

// FROM glext.h -

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31

#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;

typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef GLint (APIENTRYP PFNGLGETATTRIBLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr, const void *data, GLenum usage);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLUNIFORM2FVPROC) (GLint location, GLsizei count, const GLfloat *value);

PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM2FVPROC glUniform2fv;

inline void
load_glfuncs(hajonta_thread_context *ctx, platform_glgetprocaddress_func *get_proc_address)
{
    glCreateProgram =
        (PFNGLCREATEPROGRAMPROC)get_proc_address(ctx, "glCreateProgram");
    glCreateShader =
        (PFNGLCREATESHADERPROC)get_proc_address(ctx, "glCreateShader");
    glLinkProgram =
        (PFNGLLINKPROGRAMPROC)get_proc_address(ctx, "glLinkProgram");
    glShaderSource =
        (PFNGLSHADERSOURCEPROC)get_proc_address(ctx, "glShaderSource");
    glUseProgram =
        (PFNGLUSEPROGRAMPROC)get_proc_address(ctx, "glUseProgram");
    glGetShaderiv =
        (PFNGLGETSHADERIVPROC)get_proc_address(ctx, "glGetShaderiv");
    glGetShaderInfoLog =
        (PFNGLGETSHADERINFOLOGPROC)get_proc_address(ctx, "glGetShaderInfoLog");
    glAttachShader =
        (PFNGLATTACHSHADERPROC)get_proc_address(ctx, "glAttachShader");
    glGetProgramiv =
        (PFNGLGETPROGRAMIVPROC)get_proc_address(ctx, "glGetProgramiv");
    glGetProgramInfoLog =
        (PFNGLGETPROGRAMINFOLOGPROC)get_proc_address(ctx, "glGetProgramInfoLog");
    glGetAttribLocation =
        (PFNGLGETATTRIBLOCATIONPROC)get_proc_address(ctx, "glGetAttribLocation");
    glEnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)get_proc_address(ctx, "glEnableVertexAttribArray");
    glCompileShader =
        (PFNGLCOMPILESHADERPROC)get_proc_address(ctx, "glCompileShader");
    glVertexAttribPointer =
        (PFNGLVERTEXATTRIBPOINTERPROC)get_proc_address(ctx, "glVertexAttribPointer");
    glGenVertexArrays =
        (PFNGLGENVERTEXARRAYSPROC)get_proc_address(ctx, "glGenVertexArrays");
    glBindVertexArray =
        (PFNGLBINDVERTEXARRAYPROC)get_proc_address(ctx, "glBindVertexArray");
    glGenBuffers =
        (PFNGLGENBUFFERSPROC)get_proc_address(ctx, "glGenBuffers");
    glBindBuffer =
        (PFNGLBINDBUFFERPROC)get_proc_address(ctx, "glBindBuffer");
    glBufferData =
        (PFNGLBUFFERDATAPROC)get_proc_address(ctx, "glBufferData");
    glGetUniformLocation =
        (PFNGLGETUNIFORMLOCATIONPROC)get_proc_address(ctx, "glGetUniformLocation");
    glUniform2fv =
        (PFNGLUNIFORM2FVPROC)get_proc_address(ctx, "glUniform2fv");
}
