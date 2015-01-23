// FROM wglext.h - https://www.opengl.org/registry/api/GL/wglext.h

#ifndef WGL_ARB_create_context
#define WGL_ARB_create_context 1
#define WGL_CONTEXT_DEBUG_BIT_ARB         0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB       0x2093
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define ERROR_INVALID_VERSION_ARB         0x2095
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);
#ifdef WGL_WGLEXT_PROTOTYPES
HGLRC WINAPI wglCreateContextAttribsARB (HDC hDC, HGLRC hShareContext, const int *attribList);
#endif
#endif /* WGL_ARB_create_context */

#ifndef WGL_ARB_create_context_profile
#define WGL_ARB_create_context_profile 1
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_PROFILE_ARB         0x2096
#endif /* WGL_ARB_create_context_profile */

#ifndef WGL_EXT_swap_control
#define WGL_EXT_swap_control 1
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
typedef int (WINAPI * PFNWGLGETSWAPINTERVALEXTPROC) (void);
#ifdef WGL_WGLEXT_PROTOTYPES
BOOL WINAPI wglSwapIntervalEXT (int interval);
int WINAPI wglGetSwapIntervalEXT (void);
#endif
#endif /* WGL_EXT_swap_control */

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

PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

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
load_glfuncs()
{
    glCreateProgram =
        (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glCreateShader =
        (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glLinkProgram =
        (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glShaderSource =
        (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glUseProgram =
        (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glGetShaderiv =
        (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog =
        (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glAttachShader =
        (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glGetProgramiv =
        (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog =
        (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glGetAttribLocation =
        (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
    glEnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
    glCompileShader =
        (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glVertexAttribPointer =
        (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
    glGenVertexArrays =
        (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
    glBindVertexArray =
        (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
    glGenBuffers =
        (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    glBindBuffer =
        (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glBufferData =
        (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    glGetUniformLocation =
        (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniform2fv =
        (PFNGLUNIFORM2FVPROC)wglGetProcAddress("glUniform2fv");

    wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
}
