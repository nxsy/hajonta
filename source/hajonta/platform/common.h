#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hajonta/platform/neutral.h"

#ifdef HAJONTA_DEBUG
#define hassert(expression) if(!(expression)) {*(volatile int *)0 = 0;}
#else
#define hassert(expression)
#endif

#define hstrvalue(x) #x
#define hquoted(x) hstrvalue(x)

#define harray_count(array) (sizeof(array) / sizeof((array)[0]))

struct hajonta_thread_context
{
    void *opaque;
};

struct loaded_file
{
    char *contents;
    uint32_t size;

    char file_path[MAX_PATH]; // platform path for load_nearby
};

#define PLATFORM_FAIL(func_name) void func_name(hajonta_thread_context *ctx, char *failure_reason)
typedef PLATFORM_FAIL(platform_fail_func);

#define PLATFORM_QUIT(func_name) void func_name(hajonta_thread_context *ctx)
typedef PLATFORM_QUIT(platform_quit_func);

#define PLATFORM_DEBUG_MESSAGE(func_name) void func_name(hajonta_thread_context *ctx, char *message)
typedef PLATFORM_DEBUG_MESSAGE(platform_debug_message_func);

#define PLATFORM_GLGETPROCADDRESS(func_name) void* func_name(hajonta_thread_context *ctx, char *function_name)
typedef PLATFORM_GLGETPROCADDRESS(platform_glgetprocaddress_func);

#define PLATFORM_LOAD_ASSET(func_name) bool func_name(hajonta_thread_context *ctx, char *asset_path, uint32_t size, uint8_t *dest)
typedef PLATFORM_LOAD_ASSET(platform_load_asset_func);

#define PLATFORM_EDITOR_LOAD_FILE(func_name) bool func_name(hajonta_thread_context *ctx, loaded_file *target)
typedef PLATFORM_EDITOR_LOAD_FILE(platform_editor_load_file_func);

#define PLATFORM_EDITOR_LOAD_NEARBY_FILE(func_name) bool func_name(hajonta_thread_context *ctx, loaded_file *target, loaded_file existing_file, char *name)
typedef PLATFORM_EDITOR_LOAD_NEARBY_FILE(platform_editor_load_nearby_file_func);

struct platform_memory
{
    bool initialized;
    uint64_t size;
    void *memory;
    bool quit;
    bool debug_keyboard;

    platform_fail_func *platform_fail;
    platform_glgetprocaddress_func *platform_glgetprocaddress;
    platform_debug_message_func *platform_debug_message;
    platform_load_asset_func *platform_load_asset;
    platform_editor_load_file_func *platform_editor_load_file;
    platform_editor_load_nearby_file_func *platform_editor_load_nearby_file;
};

struct game_button_state
{
    bool ended_down;
    bool repeat;
};

struct game_buttons
{
    game_button_state move_up;
    game_button_state move_down;
    game_button_state move_left;
    game_button_state move_right;
    game_button_state back;
    game_button_state start;
};

struct game_controller_state
{
    bool is_active;

    float stick_x;
    float stick_y;

    union
    {
        game_button_state _buttons[sizeof(game_buttons) / sizeof(game_button_state)];
        game_buttons buttons;
    };
};

enum struct keyboard_input_type
{
    NONE,
    ASCII,
};

enum struct keyboard_input_modifiers
{
    LEFT_SHIFT = (1<<0),
    RIGHT_SHIFT = (1<<1),
    LEFT_CTRL = (1<<2),
    RIGHT_CTRL = (1<<3),
    LEFT_ALT = (1<<4),
    RIGHT_ALT = (1<<5),
    LEFT_CMD = (1<<6),
    RIGHT_CMD = (1<<7),
};

struct keyboard_input
{
    keyboard_input_type type;
    keyboard_input_modifiers modifiers;

    union {
        char ascii;
    };
};

struct mouse_buttons
{
    game_button_state left;
    game_button_state middle;
    game_button_state right;
};

struct mouse_input
{
    bool is_active;
    int32_t x, y;
    union
    {
        game_button_state _buttons[sizeof(mouse_buttons) / sizeof(game_button_state)];
        mouse_buttons buttons;
    };
};

struct window_data
{
    int width;
    int height;
};

#define MAX_KEYBOARD_INPUTS 10
#define NUM_CONTROLLERS ((uint32_t)4)
struct game_input
{
    float delta_t;

    window_data window;

    game_controller_state controllers[NUM_CONTROLLERS];
    keyboard_input keyboard_inputs[MAX_KEYBOARD_INPUTS];
    mouse_input mouse;
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
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31

#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82

#define GL_PRIMITIVE_RESTART              0x8F9D
#define GL_PRIMITIVE_RESTART_INDEX        0x8F9E

typedef char GLchar;
#if !defined(NEEDS_EGL)
typedef ptrdiff_t GLsizeiptr;
#endif

#if !defined(__APPLE__)
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
typedef void (APIENTRYP PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRYP PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRYP PFNGLUNIFORM3FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRYP PFNGLUNIFORM4FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRYP PFNGLUNIFORM2IPROC) (GLint location, GLint v0, GLint v1);
typedef void (APIENTRYP PFNGLUNIFORM3IPROC) (GLint location, GLint v0, GLint v1, GLint v2);
typedef void (APIENTRYP PFNGLUNIFORM4IPROC) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void (APIENTRYP PFNGLUNIFORM1FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM2FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM3FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM4FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM1IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM2IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM3IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM4IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX2FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
typedef void (APIENTRYP PFNGLPRIMITIVERESTARTINDEXPROC) (GLuint index);
#endif

#ifndef GL_TEXTURE0
#define GL_TEXTURE0                       0x84C0
#endif

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

PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM3FPROC glUniform3f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM2IPROC glUniform2i;
PFNGLUNIFORM3IPROC glUniform3i;
PFNGLUNIFORM4IPROC glUniform4i;
PFNGLUNIFORM1FVPROC glUniform1fv;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLUNIFORM1IVPROC glUniform1iv;
PFNGLUNIFORM2IVPROC glUniform2iv;
PFNGLUNIFORM3IVPROC glUniform3iv;
PFNGLUNIFORM4IVPROC glUniform4iv;
PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;

PFNGLPRIMITIVERESTARTINDEXPROC glPrimitiveRestartIndex;

#ifndef GL_VERSION_1_3
PFNGLACTIVETEXTUREPROC glActiveTexture;
#endif

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
    glUniform1f = (PFNGLUNIFORM1FPROC)get_proc_address(ctx, (char *)"glUniform1f");
    glUniform2f = (PFNGLUNIFORM2FPROC)get_proc_address(ctx, (char *)"glUniform2f");
    glUniform3f = (PFNGLUNIFORM3FPROC)get_proc_address(ctx, (char *)"glUniform3f");
    glUniform4f = (PFNGLUNIFORM4FPROC)get_proc_address(ctx, (char *)"glUniform4f");
    glUniform1i = (PFNGLUNIFORM1IPROC)get_proc_address(ctx, (char *)"glUniform1i");
    glUniform2i = (PFNGLUNIFORM2IPROC)get_proc_address(ctx, (char *)"glUniform2i");
    glUniform3i = (PFNGLUNIFORM3IPROC)get_proc_address(ctx, (char *)"glUniform3i");
    glUniform4i = (PFNGLUNIFORM4IPROC)get_proc_address(ctx, (char *)"glUniform4i");
    glUniform1fv = (PFNGLUNIFORM1FVPROC)get_proc_address(ctx, (char *)"glUniform1fv");
    glUniform2fv = (PFNGLUNIFORM2FVPROC)get_proc_address(ctx, (char *)"glUniform2fv");
    glUniform3fv = (PFNGLUNIFORM3FVPROC)get_proc_address(ctx, (char *)"glUniform3fv");
    glUniform4fv = (PFNGLUNIFORM4FVPROC)get_proc_address(ctx, (char *)"glUniform4fv");
    glUniform1iv = (PFNGLUNIFORM1IVPROC)get_proc_address(ctx, (char *)"glUniform1iv");
    glUniform2iv = (PFNGLUNIFORM2IVPROC)get_proc_address(ctx, (char *)"glUniform2iv");
    glUniform3iv = (PFNGLUNIFORM3IVPROC)get_proc_address(ctx, (char *)"glUniform3iv");
    glUniform4iv = (PFNGLUNIFORM4IVPROC)get_proc_address(ctx, (char *)"glUniform4iv");
    glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)get_proc_address(ctx, (char *)"glUniformMatrix2fv");
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)get_proc_address(ctx, (char *)"glUniformMatrix3fv");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)get_proc_address(ctx, (char *)"glUniformMatrix4fv");

    glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)get_proc_address(ctx, (char*)"glPrimitiveRestartIndex");

#ifndef GL_VERSION_1_3
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)get_proc_address(ctx, (char *)"glActiveTexture");
#endif
}
#endif
