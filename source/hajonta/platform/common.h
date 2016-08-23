#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hajonta/platform/neutral.h"

#include <atomic>

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

#define PLATFORM_FAIL(func_name) void func_name(hajonta_thread_context *ctx, const char *failure_reason)
typedef PLATFORM_FAIL(platform_fail_func);

#define PLATFORM_QUIT(func_name) void func_name(hajonta_thread_context *ctx)
typedef PLATFORM_QUIT(platform_quit_func);

#define PLATFORM_DEBUG_MESSAGE(func_name) void func_name(hajonta_thread_context *ctx, char *message)
typedef PLATFORM_DEBUG_MESSAGE(platform_debug_message_func);

#define PLATFORM_GLGETPROCADDRESS(func_name) void* func_name(hajonta_thread_context *ctx, char *function_name)
typedef PLATFORM_GLGETPROCADDRESS(platform_glgetprocaddress_func);

#define PLATFORM_LOAD_ASSET(func_name) bool func_name(hajonta_thread_context *ctx, const char *asset_path, uint32_t size, uint8_t *dest)
typedef PLATFORM_LOAD_ASSET(platform_load_asset_func);

#define PLATFORM_EDITOR_LOAD_FILE(func_name) bool func_name(hajonta_thread_context *ctx, loaded_file *target)
typedef PLATFORM_EDITOR_LOAD_FILE(platform_editor_load_file_func);

#define PLATFORM_EDITOR_LOAD_NEARBY_FILE(func_name) bool func_name(hajonta_thread_context *ctx, loaded_file *target, loaded_file existing_file, char *name)
typedef PLATFORM_EDITOR_LOAD_NEARBY_FILE(platform_editor_load_nearby_file_func);

#define PLATFORM_GET_THREAD_ID(func_name) uint32_t func_name()
typedef PLATFORM_GET_THREAD_ID(platform_get_thread_id_func);

// Normal means the OS is keeping track of the mouse cursor location.
// The program still needs to render the cursor, though.
//
// Unlimited means that the cursor location is given as delta from previous
// location.
enum struct platform_cursor_mode
{
    normal,
    unlimited,
    COUNT,
};

struct platform_cursor_settings
{
    bool supported_modes[(int)platform_cursor_mode::COUNT];
    platform_cursor_mode mode;
    bool position_set;
    int32_t mouse_x;
    int32_t mouse_y;
};

struct render_entry_list;

struct DebugTable;
struct platform_memory
{
    bool initialized;
    uint64_t size;
    void *memory;
    bool quit;
    bool debug_keyboard;

    void *imgui_state;
    DebugTable *debug_table;

    render_entry_list *render_lists[10];
    uint32_t render_lists_count;

    platform_cursor_settings cursor_settings;

    platform_fail_func *platform_fail;
    platform_glgetprocaddress_func *platform_glgetprocaddress;
    platform_debug_message_func *platform_debug_message;
    platform_load_asset_func *platform_load_asset;
    platform_editor_load_file_func *platform_editor_load_file;
    platform_editor_load_nearby_file_func *platform_editor_load_nearby_file;

    platform_get_thread_id_func *platform_get_thread_id;

    uint32_t shadowmap_size;
};

#define BUTTON_ENDED_DOWN(x) (x.ended_down)
#define BUTTON_ENDED_UP(x) (!x.ended_down)
#define BUTTON_WENT_DOWN(x) (x.ended_down && !x.repeat)
#define BUTTON_WENT_UP(x) (!x.ended_down && !x.repeat)
#define BUTTON_STAYED_DOWN(x) (x.ended_down && x.repeat)
#define BUTTON_STAYED_UP(x) (!x.ended_down && x.repeat)

struct game_button_state
{
    bool ended_down;
    bool repeat;
};

inline bool
BUTTON_DOWN_REPETITIVELY(game_button_state button, uint32_t *repetition_var, uint32_t repetition_max)
{
    bool result = false;
    if (button.ended_down)
    {
        if (!button.repeat)
        {
            result = true;
            *repetition_var = repetition_max;
        }
        else if (--*repetition_var == 0)
        {
            result = true;
            *repetition_var = repetition_max;
        }
    }
    return result;
}

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
    int32_t x;
    int32_t y;

    int32_t vertical_wheel_delta;

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

#define GAME_DEBUG_FRAME_END(func_name) void func_name(hajonta_thread_context *ctx, platform_memory *memory, game_input *input, game_sound_output *sound_output)
typedef GAME_DEBUG_FRAME_END(game_debug_frame_end_func);

#define RENDERER_SETUP(func_name) bool func_name(hajonta_thread_context *ctx, platform_memory *memory, game_input *input)
typedef RENDERER_SETUP(renderer_setup_func);

#define RENDERER_RENDER(func_name) bool func_name(hajonta_thread_context *ctx, platform_memory *memory, game_input *input)
typedef RENDERER_RENDER(renderer_render_func);


#ifdef HJ_DIRECTGL
#include "hajonta/thirdparty/glext.h"
#define HGLD(b,a) extern PFNGL##a##PROC gl##b;
extern "C" {
#include "hajonta/platform/glextlist.txt"
}
#undef HGLD

#define HGLD(b,a)  PFNGL##a##PROC gl##b;
#include "hajonta/platform/glextlist.txt"
#undef HGLD

inline void
load_glfuncs(hajonta_thread_context *ctx, platform_glgetprocaddress_func *get_proc_address)
{
#define HGLD(b,a) gl##b = (PFNGL##a##PROC)get_proc_address(ctx, (char *)"gl"#b);
#include "hajonta/platform/glextlist.txt"
#undef HGLD
}

inline void
glErrorAssert()
{
    GLenum error = glGetError();
    switch(error)
    {
        case GL_NO_ERROR:
        {
            return;
        } break;
        case GL_INVALID_ENUM:
        {
            hassert(!"Invalid enum");
        } break;
        case GL_INVALID_VALUE:
        {
            hassert(!"Invalid value");
        } break;
        case GL_INVALID_OPERATION:
        {
            hassert(!"Invalid operation");
        } break;
#if !defined(_WIN32)
        case GL_INVALID_FRAMEBUFFER_OPERATION:
        {
            hassert(!"Invalid framebuffer operation");
        } break;
#endif
        case GL_OUT_OF_MEMORY:
        {
            hassert(!"Out of memory");
        } break;
#if !defined(__APPLE__) && !defined(NEEDS_EGL)
        case GL_STACK_UNDERFLOW:
        {
            hassert(!"Stack underflow");
        } break;
        case GL_STACK_OVERFLOW:
        {
            hassert(!"Stack overflow");
        } break;
#endif
        default:
        {
            hassert(!"Unknown error");
        } break;
    }
}
#endif



// Debug

enum struct
DebugType
{
    FrameMarker,
    BeginBlock,
    EndBlock,
    OpenGLTimerResult,
};

struct
DebugEventData_FrameMarker
{
    float seconds;
};

struct
DebugEventData_BeginBlock
{
};

struct
DebugEventData_EndBlock
{
};

struct
DebugEventData_OpenGLTimerResult
{
    uint32_t result;
};

struct
DebugEvent
{
    DebugType type;

    uint64_t tsc_cycles;
    const char *guid;
    uint32_t thread_id;

    union
    {
        DebugEventData_FrameMarker framemarker;
        DebugEventData_BeginBlock begin_block;
        DebugEventData_EndBlock end_block;
        DebugEventData_OpenGLTimerResult opengl_timer_result;
    };
};

struct
DebugTable
{
    std::atomic<int32_t> event_index_count;
    DebugEvent events[2][65536];
};

extern DebugTable *GlobalDebugTable;
platform_get_thread_id_func *_platform_get_thread_id;

/*
 * These debug macros from Casey Muratori's Handmade Hero
 */
#define RecordDebugEvent(event_type, _guid)  \
uint32_t _h_event_index_count = std::atomic_fetch_add(&GlobalDebugTable->event_index_count, 1);  \
        uint32_t _h_event_index = _h_event_index_count & 0x7FFFFFFF;  \
        hassert(_h_event_index < harray_count(GlobalDebugTable->events[0])); \
        DebugEvent *_h_event = GlobalDebugTable->events[_h_event_index_count >> 31] + _h_event_index;  \
        _h_event->tsc_cycles = __rdtsc(); \
        _h_event->type = event_type; \
        _h_event->thread_id = _platform_get_thread_id(); \
        _h_event->guid = _guid;

#define UniqueFileCounterString__(A, B, C, D) A "|" #B "|" #C "|" D
#define UniqueFileCounterString_(A, B, C, D) UniqueFileCounterString__(A, B, C, D)
#define DEBUG_NAME(Name) UniqueFileCounterString_(__FILE__, __LINE__, __COUNTER__, Name)

#define FRAME_MARKER(seconds_elapsed) \
{RecordDebugEvent(DebugType::FrameMarker, DEBUG_NAME("Frame Marker")); \
    _h_event->framemarker.seconds = seconds_elapsed;}

#define OPENGL_TIMER_RESULT(_guid, _result) \
{RecordDebugEvent(DebugType::OpenGLTimerResult, _guid); \
    _h_event->opengl_timer_result.result = _result;}

#define TIMED_BLOCK__(_guid, _counter, ...) timed_block TimedBlock_##_counter(_guid, ## __VA_ARGS__)
#define TIMED_BLOCK_(_guid, _counter, ...) TIMED_BLOCK__(_guid, _counter, ## __VA_ARGS__)
#define TIMED_BLOCK(Name, ...) TIMED_BLOCK_(DEBUG_NAME(Name), __COUNTER__, ## __VA_ARGS__)

#ifdef _MSC_VER
#define TIMED_FUNCTION(...) TIMED_BLOCK_(DEBUG_NAME(__FUNCTION__), ## __VA_ARGS__)
#else
#define TIMED_FUNCTION_(_func, _counter, ...) static char _h_guid_##_counter[255]; static bool _h_initialized_##_counter = false; if (!_h_initialized_##_counter) { snprintf(_h_guid_##_counter, 255, "%s|%d|%d|%s", __FILE__, __LINE__, _counter, __func__); _h_initialized_##_counter =true; } TIMED_BLOCK_(_h_guid_##_counter, _counter)
#define TIMED_FUNCTION(...) TIMED_FUNCTION_(__func__, __COUNTER__)
#endif

#define BEGIN_BLOCK_(_guid) {RecordDebugEvent(DebugType::BeginBlock, _guid);}
#define END_BLOCK_(_guid) {RecordDebugEvent(DebugType::EndBlock, _guid);}

#define BEGIN_BLOCK(Name) BEGIN_BLOCK_(DEBUG_NAME(Name))
#define END_BLOCK() END_BLOCK_(DEBUG_NAME("END_BLOCK_"))

struct timed_block
{
    timed_block(const char *guid)
    {
        BEGIN_BLOCK_(guid);
    }

    ~timed_block()
    {
        END_BLOCK();
    }
};
/*
 * End of debug macros from Handmade Hero
 */
