#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hajonta/platform/neutral.h"

#include <atomic>

#define UniqueFileCounterString__(A, B, C, D) A "|" #B "|" #C "|" D
#define UniqueFileCounterString_(A, B, C, D) UniqueFileCounterString__(A, B, C, D)
#define DEBUG_NAME(Name) UniqueFileCounterString_(__FILE__, __LINE__, __COUNTER__, Name)

#ifdef HAJONTA_DEBUG
#define hassert(expression) if(!(expression)) { _platform->debug_message(DEBUG_NAME(__FUNCTION__)); *(volatile int *)0 = 0; }
#else
#define hassert(expression)
#endif

#define hstrvalue(x) #x
#define hquoted(x) hstrvalue(x)

#define harray_count(array) (sizeof(array) / sizeof((array)[0]))

struct loaded_file
{
    char *contents;
    uint32_t size;

    char file_path[MAX_PATH]; // platform path for load_nearby
};

struct
MemoryBlockDebugData
{
    char guid[256];
    char label[64];
    uint64_t used_before;
    uint64_t alignment;
    uint64_t location;
    uint64_t size;
    uint64_t used_after;
};

struct
MemoryBlockDebugList
{
    uint32_t num_data;
    MemoryBlockDebugData data[20];

    MemoryBlockDebugList *next;
};

struct
MemoryBlock
{
    uint64_t size;
    uint8_t *base;
    uint64_t used;
#ifdef HAJONTA_DEBUG
    char label[64];
#endif
};

struct
MemoryArena
{
    MemoryBlock *block;
    uint32_t minimum_block_size;

    // flags?
#ifdef HAJONTA_DEBUG
    MemoryBlockDebugList debug_list_builtin;
    MemoryBlockDebugList *debug_list;
#endif
};

#define PLATFORM_FAIL(func_name) void func_name(const char *failure_reason)
typedef PLATFORM_FAIL(platform_fail_func);

#define PLATFORM_DEBUG_MESSAGE(func_name) void func_name(char *message)
typedef PLATFORM_DEBUG_MESSAGE(platform_debug_message_func);

#define PLATFORM_GLGETPROCADDRESS(func_name) void* func_name(char *function_name)
typedef PLATFORM_GLGETPROCADDRESS(platform_glgetprocaddress_func);

#define PLATFORM_LOAD_ASSET(func_name) bool func_name(const char *asset_path, uint32_t size, uint8_t *dest, uint32_t *actual_size)
typedef PLATFORM_LOAD_ASSET(platform_load_asset_func);

#define PLATFORM_EDITOR_LOAD_FILE(func_name) bool func_name(loaded_file *target)
typedef PLATFORM_EDITOR_LOAD_FILE(platform_editor_load_file_func);

#define PLATFORM_EDITOR_LOAD_NEARBY_FILE(func_name) bool func_name(loaded_file *target, loaded_file existing_file, char *name)
typedef PLATFORM_EDITOR_LOAD_NEARBY_FILE(platform_editor_load_nearby_file_func);

#define PLATFORM_GET_THREAD_ID(func_name) uint32_t func_name()
typedef PLATFORM_GET_THREAD_ID(platform_get_thread_id_func);

#define PLATFORM_ALLOCATE_MEMORY(func_name) MemoryBlock *func_name(const char *comment, uint64_t size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory_func);

struct render_entry_list;
struct
PlatformApi
{
    platform_fail_func *fail;
    platform_glgetprocaddress_func *glgetprocaddress;
    platform_debug_message_func *debug_message;
    platform_load_asset_func *load_asset;
    platform_editor_load_file_func *editor_load_file;
    platform_editor_load_nearby_file_func *editor_load_nearby_file;
    platform_get_thread_id_func *get_thread_id;
    platform_allocate_memory_func *allocate_memory;

    bool stopping;
    char *stop_reason;
    bool quit;

    render_entry_list *render_lists[20];
    uint32_t render_lists_count;
};

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
    MemoryBlock *renderer_block;
    MemoryBlock *game_block;
    bool quit;
    bool debug_keyboard;

    void *imgui_state;
    DebugTable *debug_table;

    platform_cursor_settings cursor_settings;
    PlatformApi *platform_api;

    uint32_t shadowmap_size;
};
static PlatformApi *_platform;

#define BUTTON_ENDED_DOWN(x) (x.ended_down)
#define BUTTON_ENDED_UP(x) (!x.ended_down)
#define BUTTON_WENT_DOWN(x) (x.ended_down && !x.repeat)
#define BUTTON_WENT_UP(x) (!x.ended_down && !x.repeat)
#define BUTTON_STAYED_DOWN(x) (x.ended_down && x.repeat)
#define BUTTON_STAYED_UP(x) (!x.ended_down && x.repeat)

struct GameButtonState
{
    bool ended_down;
    bool repeat;
};

inline bool
BUTTON_DOWN_REPETITIVELY(GameButtonState button, uint32_t *repetition_var, uint32_t repetition_max)
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
    GameButtonState move_up;
    GameButtonState move_down;
    GameButtonState move_left;
    GameButtonState move_right;
    GameButtonState back;
    GameButtonState start;
};

struct game_controller_state
{
    bool is_active;

    float stick_x;
    float stick_y;

    union
    {
        GameButtonState _buttons[sizeof(game_buttons) / sizeof(GameButtonState)];
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

struct MouseButtons
{
    GameButtonState left;
    GameButtonState middle;
    GameButtonState right;
};

struct MouseInput
{
    bool is_active;
    int32_t x;
    int32_t y;

    float vertical_wheel_delta;

    union
    {
        GameButtonState _buttons[sizeof(MouseButtons) / sizeof(GameButtonState)];
        MouseButtons buttons;
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
    MouseInput mouse;
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

#define GAME_UPDATE_AND_RENDER(func_name) void func_name(platform_memory *memory, game_input *input, game_sound_output *sound_output)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render_func);

#define GAME_DEBUG_FRAME_END(func_name) void func_name(platform_memory *memory, game_input *input, game_sound_output *sound_output)
typedef GAME_DEBUG_FRAME_END(game_debug_frame_end_func);

#define RENDERER_SETUP(func_name) bool func_name(platform_memory *memory, game_input *input)
typedef RENDERER_SETUP(renderer_setup_func);

#define RENDERER_RENDER(func_name) bool func_name(platform_memory *memory, game_input *input)
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
load_glfuncs(platform_glgetprocaddress_func *get_proc_address)
{
#define HGLD(b,a) gl##b = (PFNGL##a##PROC)get_proc_address((char *)"gl"#b);
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

/*
 * These debug macros largely from Casey Muratori's Handmade Hero
 */
#define RecordDebugEvent(event_type, _guid)  \
uint32_t _h_event_index_count = (uint32_t)std::atomic_fetch_add(&GlobalDebugTable->event_index_count, 1);  \
        uint32_t _h_event_index = _h_event_index_count & 0x7FFFFFFF;  \
        hassert(_h_event_index < harray_count(GlobalDebugTable->events[0])); \
        DebugEvent *_h_event = GlobalDebugTable->events[_h_event_index_count >> 31] + _h_event_index;  \
        _h_event->tsc_cycles = __rdtsc(); \
        _h_event->type = event_type; \
        _h_event->thread_id = _platform->get_thread_id(); \
        _h_event->guid = _guid;

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

/*
 * These arena macros/functions largely from Casey Muratori's Handmade Hero
 */
#ifdef HAJONTA_DEBUG
#define PushStruct_(_guid, Comment, Arena, type, ...) (type *)_PushSize_DEBUG(_guid, Comment, Arena, sizeof(type), ## __VA_ARGS__)
#define PushStruct(Comment, Arena, type, ...) PushStruct_(DEBUG_NAME(__FUNCTION__), Comment, Arena, type, ## __VA_ARGS__)

#define PushArray_(_guid, Comment, Arena, type, count, ...) (type *)_PushSize_DEBUG(_guid, Comment, Arena, (count)*sizeof(type), ## __VA_ARGS__)
#define PushArray(Comment, Arena, type, count, ...) PushArray_(DEBUG_NAME(__FUNCTION__), Comment, Arena, type, count, ## __VA_ARGS__)

#define PushSize_(_guid, Comment, Arena, Size, ...) _PushSize_DEBUG(_guid, Comment, Arena, Size, ## __VA_ARGS__)
#define PushSize(Comment, Arena, Size, ...) PushSize_(DEBUG_NAME(__FUNCTION__), Comment, Arena, Size, ## __VA_ARGS__)
#else
#define PushStruct(Comment, Arena, type, ...) (type *)_PushSize(Arena, sizeof(type), ## __VA_ARGS__)
#define PushArray(Comment, Arena, type, count, ...) (type *)_PushSize(Arena, (count)*sizeof(type), ## __VA_ARGS__)
#define PushSize(Comment, Arena, Size, ...) _PushSize(Arena, Size, ## __VA_ARGS__)
#endif

struct
ArenaPushParams
{
    uint32_t alignment;
    struct
    {
        unsigned int clear:1;
    };
};

inline
ArenaPushParams
default_arena_params()
{
    ArenaPushParams result = {};
    result.alignment = 4;
    result.clear = false;
    return result;
}

uint64_t
get_alignment_offset(MemoryArena *arena, uint64_t alignment)
{
    uint64_t result = 0;

    uint64_t next_pointer = (uint64_t)arena->block->base + arena->block->used;
    uint64_t mask = alignment - 1;
    if(next_pointer & mask)
    {
        result = alignment - (next_pointer & mask);
    }
    return result;
}

inline void *
_PushSize(MemoryArena *arena, uint64_t size, MemoryBlockDebugData *debug_data = 0, ArenaPushParams params = default_arena_params())
{
    void *result = {};

    hassert(size > 0);
    hassert((arena->block->used + size) <= arena->block->size);

    uint64_t alignment = get_alignment_offset(arena, params.alignment);
    if (debug_data)
    {
        debug_data->used_before = arena->block->used;
        debug_data->alignment = alignment;
        debug_data->size = size;
    }
    arena->block->used += alignment;
    uint64_t location = arena->block->used;
    result = arena->block->base + location;
    arena->block->used += size;

    if (debug_data)
    {
        debug_data->location = location;
        debug_data->used_after = arena->block->used;
    }

    if(params.clear)
    {
        memset(result, 0, size);
    }

    return result;
}
/*
 * End of arena macros/functions from Handmade Hero
 */

inline
MemoryBlockDebugData *
get_next_block_debug_data(MemoryArena *arena)
{
    auto &debug_list = arena->debug_list;
    if (debug_list->num_data == harray_count(debug_list->data) - 1)
    {
        auto &data = debug_list->data[debug_list->num_data];
        strcpy(data.label, "memory_block_debug_data");
        MemoryBlockDebugList *new_debug_list = (MemoryBlockDebugList *)_PushSize(arena, sizeof(MemoryBlockDebugList), &data);
        new_debug_list->next = arena->debug_list;
        arena->debug_list = new_debug_list;
    }

    return debug_list->data + debug_list->num_data++;
}

inline void *
_PushSize_DEBUG(const char *guid, const char *comment, MemoryArena *arena, uint64_t size, ArenaPushParams params = default_arena_params())
{
    MemoryBlockDebugData *d = get_next_block_debug_data(arena);
    strncpy(d->label, comment, 63);
    d->label[63] = 0;
    strncpy(d->guid, guid, 255);
    d->guid[255] = 0;
    return _PushSize(arena, size, d, params);
}

#define bootstrap_memory_arena(block, typename, member, ...) (typename *)bootstrap_memory_arena_(block, sizeof(typename), offsetof(typename, member), ## __VA_ARGS__)
void *
bootstrap_memory_arena_(
    MemoryBlock *block,
    uint32_t size,
    uint32_t offset_to_arena)
{

    uint32_t header_size = size;
    header_size += (64 - header_size) % 64;
    MemoryArena bootstrap = {};
    bootstrap.block = block;
#ifdef HAJONTA_DEBUG
    bootstrap.debug_list = &bootstrap.debug_list_builtin;
#endif
    void *base = PushSize("Bootstrap", &bootstrap, header_size);
    MemoryArena *arena = (MemoryArena *)((uint8_t *)base + offset_to_arena);
    *arena = bootstrap;
#ifdef HAJONTA_DEBUG
    arena->debug_list = &arena->debug_list_builtin;
#endif
    return base;
}

#define create_sub_arena_(_guid, Comment, ParentArena, Size, BaseSize, OffsetToArena, ...) _create_sub_arena(_guid, Comment, ParentArena, Size, BaseSize, OffsetToArena, ## __VA_ARGS__)
#define create_sub_arena(Comment, ParentArena, Size, typename, member, ...) (typename *)create_sub_arena_(DEBUG_NAME(__FUNCTION__), Comment, ParentArena, Size, sizeof(typename), offsetof(typename, member), ## __VA_ARGS__)
void *
_create_sub_arena(
    const char *guid,
    const char *comment,
    MemoryArena *parent_arena,
    uint32_t size,
    uint32_t base_size,
    uint32_t offset_to_arena)
{
    uint32_t header_size = sizeof(MemoryBlock);
    header_size += (64 - header_size) % 64;

    uint32_t size_with_headers = header_size + size;
    MemoryBlock *block = (MemoryBlock *)PushSize_(guid, comment, parent_arena, size_with_headers);
    block->base = (uint8_t *)block + header_size;
    block->size = size;
    block->used = base_size;

    MemoryArena *arena = (MemoryArena *)(block->base + offset_to_arena);
    arena->block = block;
    arena->debug_list = &arena->debug_list_builtin;

    return arena;
}

#define PushString(Comment, Arena, Source, ...) _PushString(DEBUG_NAME(__FUNCTION__), Comment, Arena, Source)
#define PushStringWithLength(Comment, Arena, Length, Source, ...) _PushString(DEBUG_NAME(__FUNCTION__), Comment, Arena, Length, Source)
char *
_PushString(
    const char *guid,
    const char *comment,
    MemoryArena *parent_arena,
    uint32_t length,
    char *source)
{
    char *result = (char *)PushSize_(guid, comment, parent_arena, length + 1); // noclear
    memcpy(result, source, length);
    result[length] = 0;
    return result;
}

char *
_PushString(
    const char *guid,
    const char *comment,
    MemoryArena *parent_arena,
    char *source)
{
    return _PushString(guid, comment, parent_arena, (uint32_t)strlen(source), source);
}

