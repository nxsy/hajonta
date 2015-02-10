/*
 * Copyright (c) 2014, Neil Blakey-Milner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/mman.h> // mmap, PROT_*, MAP_*
#include <sys/stat.h> // stat, fstat

#include <dirent.h>   // DIR *, opendir
#include <dlfcn.h>    // dlopen, dlsym, dlclose
#include <errno.h>    // errno
#include <fcntl.h>    // open, O_RDONLY
#include <stddef.h>    // printf
#include <stdio.h>    // printf
#include <stdlib.h>   // malloc, calloc, free
#include <string.h>   // strlen (I'm so lazy)
#include <time.h>     // clock_gettime, CLOCK_MONOTONIC
#include <unistd.h>   // readlink

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_keysyms.h>

#include <X11/keysym.h>
#include <X11/XF86keysym.h>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <linux/joystick.h>
#include <linux/limits.h>

#include <GL/gl.h>
#include <GL/glx.h>
#define glXGetProcAddress(x) (*glXGetProcAddressARB)((const GLubyte*)x)

#include "hajonta/platform/common.h"
#include "hajonta/platform/xcb.h"

xcb_format_t *xcb_find_format(xcb_state *state, uint32_t pad, uint32_t depth, uint32_t bpp)
{
    xcb_format_t *fmt = xcb_setup_pixmap_formats(state->setup);
    xcb_format_t *fmtend = fmt + xcb_setup_pixmap_formats_length(state->setup);
    while (fmt++ != fmtend)
    {
        if (fmt->scanline_pad == pad && fmt->depth == depth && fmt->bits_per_pixel == bpp)
        {
            return fmt;
        }
    }
    return (xcb_format_t *)0;
}

void
load_atoms(xcb_state *state)
{
    xcb_intern_atom_cookie_t wm_delete_window_cookie =
        xcb_intern_atom(state->connection, 0, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie =
        xcb_intern_atom(state->connection, 0, strlen("WM_PROTOCOLS"),
                "WM_PROTOCOLS");

    xcb_flush(state->connection);
    xcb_intern_atom_reply_t* wm_delete_window_cookie_reply =
        xcb_intern_atom_reply(state->connection, wm_delete_window_cookie, 0);
    xcb_intern_atom_reply_t* wm_protocols_cookie_reply =
        xcb_intern_atom_reply(state->connection, wm_protocols_cookie, 0);

    state->wm_protocols = wm_protocols_cookie_reply->atom;
    state->wm_delete_window = wm_delete_window_cookie_reply->atom;
}

void
get_binary_name(xcb_state *state)
{
    // NOTE(nbm): There are probably pathological cases where this won't work - for
    // example if the underlying file has been removed/moved.
    readlink("/proc/self/exe", state->binary_name, PATH_MAX);
    for (char *c = state->binary_name; *c; ++c)
    {
        if (*c == '/')
        {
            state->last_slash = c + 1;
        }
    }
}

void
cat_strings(
    size_t src_a_count, char *src_a,
    size_t src_b_count, char *src_b,
    size_t dest_count, char *dest
    )
{
    size_t counter = 0;
    for (
            size_t i = 0;
            i < src_a_count && counter++ < dest_count;
            ++i)
    {
        *dest++ = *src_a++;
    }
    for (
            size_t i = 0;
            i < src_b_count && counter++ < dest_count;
            ++i)
    {
        *dest++ = *src_b++;
    }

    *dest++ = 0;
}

void
build_full_filename(xcb_state *state, char *filename, int dest_count, char *dest)
{
    cat_strings(state->last_slash -
            state->binary_name, state->binary_name, strlen(filename), filename,
            dest_count, dest);
}

void
load_game(xcb_game_code *game_code, char *path)
{
    struct stat statbuf = {};
    uint32_t stat_result = stat(path, &statbuf);
    if (stat_result != 0)
    {
        printf("Failed to stat game code at %s", path);
        return;
    }
    game_code->last_updated = statbuf.st_mtime;

    game_code->library_handle = dlopen(path, RTLD_LAZY);
    if (game_code->library_handle == 0)
    {
        char *error = dlerror();
        printf("Unable to load library at path %s: %s\n", path, error);
        return;
    }
    game_code->game_update_and_render =
        (game_update_and_render_func *)dlsym(game_code->library_handle, "game_update_and_render");
    if (game_code->game_update_and_render == 0)
    {
        char *error = dlerror();
        printf("Unable to load symbol GameUpdateAndRender: %s\n", error);
        free(error);
        _exit(1);
    }
}

void
unload_game(xcb_game_code *game_code)
{
    if (game_code->library_handle)
    {
        dlclose(game_code->library_handle);
        game_code->library_handle = 0;
    }
    game_code->game_update_and_render = 0;
}

static void
process_keyboard_message(game_button_state *new_state, bool is_down)
{
    if (new_state->ended_down != is_down)
    {
        new_state->ended_down = is_down;
        new_state->repeat = false;
    }
}

void
xcb_process_events(xcb_state *state)
{
    game_controller_state *old_keyboard_controller = get_controller(state->old_input, 0);
    game_controller_state *new_keyboard_controller = get_controller(state->new_input, 0);
    *new_keyboard_controller = {};
    new_keyboard_controller->is_active = true;
    for (
            uint button_index = 0;
            button_index < harray_count(new_keyboard_controller->_buttons);
            ++button_index)
    {
        new_keyboard_controller->_buttons[button_index].ended_down =
            old_keyboard_controller->_buttons[button_index].ended_down;
        new_keyboard_controller->_buttons[button_index].repeat = true;
    }

    xcb_generic_event_t *event;
    while ((event = xcb_poll_for_event(state->connection)))
    {
        // NOTE(nbm): The high-order bit of response_type is whether the event
        // is synthetic.  I'm not sure I care, but let's grab it in case.
        bool synthetic_event = (event->response_type & 0x80) != 0;
        uint8_t response_type = event->response_type & ~0x80;
        switch(response_type)
        {
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE:
            {
                xcb_key_press_event_t *e = (xcb_key_press_event_t *)event;
                bool is_down = (response_type == XCB_KEY_PRESS);
                xcb_keysym_t keysym = xcb_key_symbols_get_keysym(state->key_symbols, e->detail, 0);

                switch(keysym)
                {
                    case XK_Escape:
                    {
                        process_keyboard_message(&new_keyboard_controller->buttons.back, is_down);
                    } break;
                    case XK_Return:
                    {
                        process_keyboard_message(&new_keyboard_controller->buttons.start, is_down);
                    } break;
                    case XK_w:
                    {
                        process_keyboard_message(&new_keyboard_controller->buttons.move_up, is_down);
                    } break;
                    case XK_a:
                    {
                        process_keyboard_message(&new_keyboard_controller->buttons.move_left, is_down);
                    } break;
                    case XK_s:
                    {
                        process_keyboard_message(&new_keyboard_controller->buttons.move_down, is_down);
                    } break;
                    case XK_d:
                    {
                        process_keyboard_message(&new_keyboard_controller->buttons.move_right, is_down);
                    } break;
                }
                break;
            }
            case XCB_NO_EXPOSURE:
            {
                // No idea what these are, but they're spamming me.
                break;
            }
            case XCB_CLIENT_MESSAGE:
            {
                xcb_client_message_event_t* client_message_event = (xcb_client_message_event_t*)event;

                if (client_message_event->type == state->wm_protocols)
                {
                    if (client_message_event->data.data32[0] == state->wm_delete_window)
                    {
                        state->stopping = 1;
                        break;
                    }
                }
                break;
            }
            default:
            {
                break;
            }
        }
        free(event);
    };
}

PLATFORM_FAIL(xcb_fail)
{
    xcb_state *state = (xcb_state *)ctx;
    state->stopping = 1;
    state->stop_reason = strdup(failure_reason);
}

PLATFORM_GLGETPROCADDRESS(xcb_glgetprocaddress)
{
    return (void *)glXGetProcAddress(function_name);
}

PLATFORM_LOAD_ASSET(xcb_load_asset)
{
    xcb_state *state = (xcb_state *)ctx;
    char full_pathname[PATH_MAX];
    snprintf(full_pathname, sizeof(full_pathname), "%s/%s", state->asset_path, asset_path);

    FILE *asset = fopen(full_pathname, "r");
    fseek(asset, 0, SEEK_END);
    uint32_t file_size = ftell(asset);
    fseek(asset, 0, SEEK_SET);
    int size_read = fread(dest, 1, size, asset);
    return true;
}

static bool
find_asset_path(xcb_state *state)
{
    char dir[sizeof(state->binary_name)];
    strcpy(dir, state->binary_name);

    while(char *location_of_last_slash = strrchr(dir, '/')) {
        *location_of_last_slash = 0;

        strcpy(state->asset_path, dir);
        strcat(state->asset_path, "/data");
        struct stat statbuf = {};
        uint32_t stat_result = stat(state->asset_path, &statbuf);
        if (stat_result != 0)
        {
            continue;
        }
        return true;
    }
    return false;
}

// From glxinfo.cpp - This apparently unsticks mesa's glxChooseVisual for some
// unknown reason.
static void
mesa_hack(Display *dpy, int scrnum)
{
    static int attribs[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DEPTH_SIZE, 1,
        GLX_STENCIL_SIZE, 1,
        GLX_ACCUM_RED_SIZE, 1,
        GLX_ACCUM_GREEN_SIZE, 1,
        GLX_ACCUM_BLUE_SIZE, 1,
        GLX_ACCUM_ALPHA_SIZE, 1,
        GLX_DOUBLEBUFFER,
        None
    };
    XVisualInfo *visinfo;

    visinfo = glXChooseVisual(dpy, scrnum, attribs);
    if (visinfo)
    {
        XFree(visinfo);
    }
}

int
main()
{

    xcb_state state = {};
    get_binary_name(&state);
    find_asset_path(&state);

    char source_game_code_library_path[PATH_MAX];
    char *game_code_filename = (char *)"libgame.so";
    build_full_filename(&state, game_code_filename,
            sizeof(source_game_code_library_path),
            source_game_code_library_path);
    xcb_game_code game_code = {};
    load_game(&game_code, source_game_code_library_path);

    Display *display;

    display = XOpenDisplay(0);
    if(!display)
    {
        fprintf(stderr, "Can't open display\n");
        return -1;
    }
    int default_screen = DefaultScreen(display);

    mesa_hack(display, default_screen);

    int screenNum;
    state.connection = XGetXCBConnection(display);
    XSetEventQueueOwner(display, XCBOwnsEventQueue);

    /*
     * TODO(nbm): This is X-wide, so it really isn't a good option in reality.
     * We have to be careful and clean up at the end.  If we crash, auto-repeat
     * is left off.
     */
    {
        uint32_t values[1] = {XCB_AUTO_REPEAT_MODE_OFF};
        xcb_change_keyboard_control(state.connection, XCB_KB_AUTO_REPEAT_MODE, values);
    }

    state.key_symbols = xcb_key_symbols_alloc(state.connection);
    load_atoms(&state);
    state.setup = xcb_get_setup(state.connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(state.setup);
    xcb_screen_t *screen = iter.data;
    state.fmt = xcb_find_format(&state, 32, 24, 32);

    int monitor_refresh_hz = 60;
    float game_update_hz = (monitor_refresh_hz / 2.0f); // Should almost always be an int...
    long target_nanoseconds_per_frame = (1000 * 1000 * 1000) / game_update_hz;

    static int visual_attribs[] =
    {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 16,
        GLX_STENCIL_SIZE, 8,
        None
    };
    int fbcount;
    GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
    if (!fbc)
    {
        printf("Failed to retrieve a framebuffer config\n");
        exit(1);
    }
    GLXFBConfig fbconfig = fbc[0];
    XFree(fbc);

    int visualid;
    XVisualInfo *vi = glXGetVisualFromFBConfig(display, fbconfig);
    if (vi)
    {
        visualid = vi->visualid;
    }
    else
    {
        printf("Failed to get visual from fbconfig!\n");
        return -1;
    }
    GLXContext context;

    context = glXCreateNewContext(display, fbconfig, GLX_RGBA_TYPE, 0, True);

    /* Create XID's for colormap and window */
    xcb_colormap_t colormap = xcb_generate_id(state.connection);
    xcb_window_t window = xcb_generate_id(state.connection);
    state.window = window;

    /* Create colormap */
    xcb_create_colormap(
        state.connection,
        XCB_COLORMAP_ALLOC_NONE,
        colormap,
        screen->root,
        visualid
        );

    /* Create window */
    uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
    uint32_t valuelist[] = { eventmask, colormap, 0 };
    uint32_t valuemask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;

#define START_WIDTH 960
#define START_HEIGHT 540
    xcb_create_window(
        state.connection,
        XCB_COPY_FROM_PARENT,
        window,
        screen->root,
        0, 0,
        START_WIDTH, START_HEIGHT,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        visualid,
        valuemask,
        valuelist
        );


    // NOTE: window must be mapped before glXMakeContextCurrent
    xcb_map_window(state.connection, window);

    /* Create GLX Window */
    GLXDrawable drawable = 0;

    GLXWindow glxwindow =
        glXCreateWindow(
            display,
            fbconfig,
            window,
            0
            );

    if(!window)
    {
        xcb_destroy_window(state.connection, window);
        glXDestroyContext(display, context);

        fprintf(stderr, "glXDestroyContext failed\n");
        return -1;
    }

    drawable = glxwindow;

    /* make OpenGL context current */
    if(!glXMakeContextCurrent(display, drawable, drawable, context))
    {
        xcb_destroy_window(state.connection, window);
        glXDestroyContext(display, context);

        fprintf(stderr, "glXMakeContextCurrent failed\n");
        return -1;
    }

    xcb_icccm_set_wm_name(state.connection, state.window, XCB_ATOM_STRING,
            8, strlen("hello"), "hello");

    xcb_map_window(state.connection, state.window);
    xcb_atom_t protocols[] =
    {
        state.wm_delete_window,
    };
    xcb_icccm_set_wm_protocols(state.connection, state.window,
            state.wm_protocols, 1, protocols);

    xcb_size_hints_t hints = {};
    xcb_icccm_size_hints_set_max_size(&hints, START_WIDTH, START_HEIGHT);
    xcb_icccm_size_hints_set_min_size(&hints, START_WIDTH, START_HEIGHT);
    xcb_icccm_set_wm_size_hints(state.connection, state.window,
            XCB_ICCCM_WM_STATE_NORMAL, &hints);

    xcb_flush(state.connection);

    xcb_sound_output sound_output = {};
    sound_output.samples_per_second = 48000;
    sound_output.bytes_per_sample = sizeof(int16_t) * 2;
    sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;

    int16_t *sample_buffer = (int16_t  *)malloc(sound_output.secondary_buffer_size);

    hajonta_thread_context t = {};

    platform_memory memory = {};
    memory.size = 64 * 1024 * 1024;
    memory.memory = calloc(memory.size, sizeof(uint8_t));
    memory.platform_fail = xcb_fail;
    memory.platform_glgetprocaddress = xcb_glgetprocaddress;
    memory.platform_load_asset = xcb_load_asset;


    game_input input[2] = {};
    state.new_input = &input[0];
    state.old_input = &input[1];

    while(!state.stopping)
    {
        struct stat library_statbuf = {};
        stat(source_game_code_library_path, &library_statbuf);
        if (library_statbuf.st_mtime != game_code.last_updated)
        {
            unload_game(&game_code);
            load_game(&game_code, source_game_code_library_path);
        }

        state.new_input->delta_t = target_nanoseconds_per_frame / (1024.0 * 1024 * 1024);

        xcb_process_events(&state);

        if (state.stopping)
        {
            break;
        }

        game_sound_output output;
        output.samples_per_second = sound_output.samples_per_second;
        output.number_of_samples = sound_output.samples_per_second / 30;
        output.samples = sample_buffer;

        game_code.game_update_and_render((hajonta_thread_context *)&state, &memory, state.new_input, &output);
        glXSwapBuffers(display, drawable);
        timespec sleep_counter = {};
        sleep_counter.tv_nsec = 15 * 1024 * 1024;
        timespec remaining_sleep_counter = {};
        nanosleep(&sleep_counter, &remaining_sleep_counter);

        game_input *temp_input = state.new_input;
        state.new_input = state.old_input;
        state.old_input = temp_input;

        if (memory.quit)
        {
            state.stopping = true;
        }
    }

    if (state.stop_reason)
    {
        printf("%s\n", state.stop_reason);
    }

    // NOTE(nbm): Since auto-repeat seems to be a X-wide thing, let's be nice
    // and return it to where it was before?
    {
        uint32_t values[1] = {XCB_AUTO_REPEAT_MODE_DEFAULT};
        xcb_change_keyboard_control(state.connection, XCB_KB_AUTO_REPEAT_MODE, values);
    }

    xcb_flush(state.connection);
    xcb_disconnect(state.connection);
}
