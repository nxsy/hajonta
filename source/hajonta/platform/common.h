#pragma once

#include <stddef.h>
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

#define PLATFORM_DEBUG_MESSAGE(func_name) void func_name(hajonta_thread_context *ctx, char *message)
typedef PLATFORM_DEBUG_MESSAGE(platform_debug_message_func);

#define PLATFORM_GLGETPROCADDRESS(func_name) void* func_name(hajonta_thread_context *ctx, char *function_name)
typedef PLATFORM_GLGETPROCADDRESS(platform_glgetprocaddress_func);

struct platform_memory
{
    bool initialized;
    uint64_t size;
    void *memory;

    platform_fail_func *platform_fail;
    platform_glgetprocaddress_func *platform_glgetprocaddress;
    platform_debug_message_func *platform_debug_message;
};

struct game_button_state
{
    bool ended_down;
};

struct game_buttons
{
    game_button_state move_up;
    game_button_state move_down;
    game_button_state move_left;
    game_button_state move_right;
};

struct game_controller_state
{
    bool is_active;

    float stick_x;
    float stick_y;

    union
    {
        game_button_state _buttons[sizeof(game_buttons)];
        game_buttons buttons;
    };
};

#define NUM_CONTROLLERS ((uint32_t)4)
struct game_input
{
    float delta_t;

    game_controller_state controllers[NUM_CONTROLLERS];
};

inline game_controller_state *get_controller(game_input *input, uint32_t index)
{
    hassert(index < harray_count(input->controllers));
    return &input->controllers[index];
}

struct game_sound_output
{
    int32_t samples_per_second;
    int32_t channels;
    int32_t number_of_samples;
    void *samples;
};

#define GAME_UPDATE_AND_RENDER(func_name) void func_name(hajonta_thread_context *ctx, platform_memory *memory, game_input *input, game_sound_output *sound_output)
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
#if !defined(NEEDS_EGL)
typedef ptrdiff_t GLsizeiptr;
#endif

typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
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

#if !defined(NEEDS_EGL) && !defined(__APPLE__)
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
        (PFNGLCREATEPROGRAMPROC)get_proc_address(ctx, (char *)"glCreateProgram");
    glCreateShader =
        (PFNGLCREATESHADERPROC)get_proc_address(ctx, (char *)"glCreateShader");
    glLinkProgram =
        (PFNGLLINKPROGRAMPROC)get_proc_address(ctx, (char *)"glLinkProgram");
    glShaderSource =
        (PFNGLSHADERSOURCEPROC)get_proc_address(ctx, (char *)"glShaderSource");
    glUseProgram =
        (PFNGLUSEPROGRAMPROC)get_proc_address(ctx, (char *)"glUseProgram");
    glGetShaderiv =
        (PFNGLGETSHADERIVPROC)get_proc_address(ctx, (char *)"glGetShaderiv");
    glGetShaderInfoLog =
        (PFNGLGETSHADERINFOLOGPROC)get_proc_address(ctx, (char *)"glGetShaderInfoLog");
    glAttachShader =
        (PFNGLATTACHSHADERPROC)get_proc_address(ctx, (char *)"glAttachShader");
    glGetProgramiv =
        (PFNGLGETPROGRAMIVPROC)get_proc_address(ctx, (char *)"glGetProgramiv");
    glGetProgramInfoLog =
        (PFNGLGETPROGRAMINFOLOGPROC)get_proc_address(ctx, (char *)"glGetProgramInfoLog");
    glGetAttribLocation =
        (PFNGLGETATTRIBLOCATIONPROC)get_proc_address(ctx, (char *)"glGetAttribLocation");
    glEnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)get_proc_address(ctx, (char *)"glEnableVertexAttribArray");
    glCompileShader =
        (PFNGLCOMPILESHADERPROC)get_proc_address(ctx, (char *)"glCompileShader");
    glVertexAttribPointer =
        (PFNGLVERTEXATTRIBPOINTERPROC)get_proc_address(ctx, (char *)"glVertexAttribPointer");
    glGenVertexArrays =
        (PFNGLGENVERTEXARRAYSPROC)get_proc_address(ctx, (char *)"glGenVertexArrays");
    glBindVertexArray =
        (PFNGLBINDVERTEXARRAYPROC)get_proc_address(ctx, (char *)"glBindVertexArray");
    glGenBuffers =
        (PFNGLGENBUFFERSPROC)get_proc_address(ctx, (char *)"glGenBuffers");
    glBindBuffer =
        (PFNGLBINDBUFFERPROC)get_proc_address(ctx, (char *)"glBindBuffer");
    glBufferData =
        (PFNGLBUFFERDATAPROC)get_proc_address(ctx, (char *)"glBufferData");
    glGetUniformLocation =
        (PFNGLGETUNIFORMLOCATIONPROC)get_proc_address(ctx, (char *)"glGetUniformLocation");
    glUniform2fv =
        (PFNGLUNIFORM2FVPROC)get_proc_address(ctx, (char *)"glUniform2fv");
}
#endif
