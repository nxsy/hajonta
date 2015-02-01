#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <android/log.h>
#include "android_native_app_glue.h"

#include "hajonta/platform/common.h"
#include "hajonta/game.cpp"

struct pan_state {
    bool in_pan;
    v2 start_pos;
    v2 magnitude;
    v2 stick;
};

struct motion_state {
    pan_state pan;
};

struct user_data {
    char app_name[64];
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    AAssetManager *asset_manager;

    bool drawable;
    bool stopping;
    char *stop_reason;

    motion_state motion;

    game_input *new_input;
    game_input *old_input;
};

char *cmd_names[] = {
    "APP_CMD_INPUT_CHANGED",
    "APP_CMD_INIT_WINDOW",
    "APP_CMD_TERM_WINDOW",
    "APP_CMD_WINDOW_RESIZED",
    "APP_CMD_WINDOW_REDRAW_NEEDED",
    "APP_CMD_CONTENT_RECT_CHANGED",
    "APP_CMD_GAINED_FOCUS",
    "APP_CMD_LOST_FOCUS",
    "APP_CMD_CONFIG_CHANGED",
    "APP_CMD_LOW_MEMORY",
    "APP_CMD_START",
    "APP_CMD_RESUME",
    "APP_CMD_SAVE_STATE",
    "APP_CMD_PAUSE",
    "APP_CMD_STOP",
    "APP_CMD_DESTROY",
};

void init(android_app *app)
{
    user_data *p = (user_data *)app->userData;

    p->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(p->display, 0, 0);

    int attrib_list[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig config;
    int num_config;
    eglChooseConfig(p->display, attrib_list, &config, 1, &num_config);

    int format;
    eglGetConfigAttrib(p->display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);
    p->surface = eglCreateWindowSurface(p->display, config, app->window, 0);

    const int context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    eglBindAPI(EGL_OPENGL_ES_API);
    p->context = eglCreateContext(p->display, config, EGL_NO_CONTEXT, context_attribs);

    eglMakeCurrent(p->display, p->surface, p->surface, p->context);
    p->drawable = 1;
}

void term(android_app *app)
{
    user_data *p = (user_data *)app->userData;
    p->drawable = 0;
    eglMakeCurrent(p->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(p->display, p->context);
    eglDestroySurface(p->display, p->surface);
    eglTerminate(p->display);
}

void on_app_cmd(android_app *app, int32_t cmd) {
    user_data *p = (user_data *)app->userData;
    if (cmd < sizeof(cmd_names))
    {
        __android_log_print(ANDROID_LOG_INFO, p->app_name, "cmd is %s", cmd_names[cmd]);
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, p->app_name, "unknown cmd is %d", cmd);
    }
    if (cmd == APP_CMD_INIT_WINDOW)
    {
        init(app);
    }
    if (cmd == APP_CMD_TERM_WINDOW)
    {
        term(app);
    }
    if (cmd == APP_CMD_DESTROY)
    {
        exit(0);
    }
}

int32_t on_motion_event(android_app *app, AInputEvent *event)
{
    /*
    AMotionEvent_getAction();
    AMotionEvent_getEventTime();
    AMotionEvent_getPointerCount();
    AMotionEvent_getPointerId();
    AMotionEvent_getX();
    AMotionEvent_getY();
    AMOTION_EVENT_ACTION_DOWN
    AMOTION_EVENT_ACTION_UP
    AMOTION_EVENT_ACTION_MOVE
    AMOTION_EVENT_ACTION_CANCEL
    AMOTION_EVENT_ACTION_POINTER_DOWN
    AMOTION_EVENT_ACTION_POINTER_UP
    */
    user_data *p = (user_data *)app->userData;
    pan_state *pan = &p->motion.pan;

    pan->stick = {};

    uint action = AMotionEvent_getAction(event);
    uint num_pointers = AMotionEvent_getPointerCount(event);
    if (num_pointers != 2 || action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
    {
        pan->in_pan = 0;
    }
    else
    {
        uint pointer_id_0 = AMotionEvent_getPointerId(event, 0);
        uint pointer_id_1 = AMotionEvent_getPointerId(event, 1);
        v2 pointer_pos_0 = {AMotionEvent_getX(event, pointer_id_0), AMotionEvent_getY(event, pointer_id_0)};
        v2 pointer_pos_1 = {AMotionEvent_getX(event, pointer_id_1), AMotionEvent_getY(event, pointer_id_1)};
        v2 center_pos = v2mul(v2add(pointer_pos_0, pointer_pos_1), 0.5f);

        if (!pan->in_pan)
        {
            pan->in_pan = 1;
            pan->start_pos = center_pos;
        }

        if (pan->in_pan)
        {
            v2 distance = v2sub(center_pos, pan->start_pos);
            if ((abs(distance.x) > 100) || (abs(distance.y) > 100))
            {
                if (abs(distance.x) > 100)
                {
                    pan->stick.x = distance.x > 0 ? 1.0 : -1.0;
                }
                if (abs(distance.y) > 100)
                {
                    pan->stick.y = distance.y < 0 ? 1.0 : -1.0;
                }
            }
        }
    }
    return 1;
}

static void
process_keyboard_message(game_button_state *new_state, bool is_down)
{
    if (new_state->ended_down != is_down)
    {
        new_state->ended_down = is_down;
    }
}

int32_t on_key_event(android_app *app, AInputEvent *event)
{
    user_data *p = (user_data *)app->userData;
    game_controller_state *old_keyboard_controller = get_controller(p->old_input, 0);
    game_controller_state *new_keyboard_controller = get_controller(p->new_input, 0);

    uint action = AKeyEvent_getAction(event);
    if (action == AKEY_EVENT_ACTION_MULTIPLE)
    {
        return 1;
    }
    bool is_down = action == AKEY_EVENT_ACTION_DOWN;

    int keycode = AKeyEvent_getKeyCode(event);
    int meta_state = AKeyEvent_getMetaState(event);
    if (keycode == 4)
    {
        return 0;
    }
    else if ((keycode == 51) || (keycode == 19))
    {
        process_keyboard_message(&new_keyboard_controller->buttons.move_up, is_down);
    }
    else if ((keycode == 29) || (keycode == 21))
    {
        process_keyboard_message(&new_keyboard_controller->buttons.move_left, is_down);
    }
    else if ((keycode == 47) || (keycode == 20))
    {
        process_keyboard_message(&new_keyboard_controller->buttons.move_down, is_down);
    }
    else if ((keycode == 32) || (keycode == 22))
    {
        process_keyboard_message(&new_keyboard_controller->buttons.move_right, is_down);
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, p->app_name, "key event: down %d, keycode %d, meta_state %x", is_down, keycode, meta_state);
    }
    return 1;
}

int32_t on_input_event(android_app *app, AInputEvent *event) {
    user_data *p = (user_data *)app->userData;
    int event_type = AInputEvent_getType(event);

    switch (event_type)
    {
        case AINPUT_EVENT_TYPE_KEY:
        {
            return on_key_event(app, event);
        }
        case AINPUT_EVENT_TYPE_MOTION:
        {
            return on_motion_event(app, event);
        }
        default:
        {
            __android_log_print(ANDROID_LOG_INFO, p->app_name, "unknown event_type was %d", event_type);
            break;
        }
    }
    return 0;
}

void
draw(android_app *app)
{
    user_data *p = (user_data *)app->userData;
    if (!p->drawable)
    {
        return;
    }
    eglSwapBuffers(p->display, p->surface);
}

static void
process_button(bool down, game_button_state *old_state, game_button_state *new_state)
{
    new_state->ended_down = down;
}

static void
platform_process_events(android_app *app, game_input *new_input, game_input *old_input)
{
    game_controller_state *old_controller = get_controller(old_input, 1);
    game_controller_state *new_controller = get_controller(new_input, 1);

    user_data *p = (user_data *)app->userData;
    pan_state *pan = &p->motion.pan;

    process_button((pan->stick.x > 0), &old_controller->buttons.move_right, &new_controller->buttons.move_right);
    process_button((pan->stick.x < 0), &old_controller->buttons.move_left, &new_controller->buttons.move_left);
    process_button((pan->stick.y > 0), &old_controller->buttons.move_up, &new_controller->buttons.move_up);
    process_button((pan->stick.y < 0), &old_controller->buttons.move_down, &new_controller->buttons.move_down);
}

PLATFORM_LOAD_ASSET(ndk_load_asset)
{
    user_data *state = (user_data *)ctx;

    AAsset *asset = AAssetManager_open(state->asset_manager, asset_path, AASSET_MODE_BUFFER);

    if (asset == 0)
    {
        __android_log_print(ANDROID_LOG_INFO, "org.nxsy.ndk_handmade", "Failed to open file %s", asset_path);
        return false;
    }

    uint64_t asset_size = AAsset_getLength64(asset);
    if (asset_size != size)
    {
        __android_log_print(ANDROID_LOG_INFO, "org.nxsy.ndk_handmade", "File size mismatch: Got %d, expected %d", asset_size, size);
        return false;
    }

    char *buf = (char *)malloc(asset_size + 1);
    AAsset_read(asset, dest, size);
    AAsset_close(asset);

    return true;
}

PLATFORM_FAIL(platform_fail)
{
    user_data *state = (user_data *)ctx;
    __android_log_print(ANDROID_LOG_INFO, state->app_name, "%s", failure_reason);
    state->stopping = 1;
    state->stop_reason = strdup(failure_reason);
}

PLATFORM_DEBUG_MESSAGE(platform_debug_message)
{
    user_data *state = (user_data *)ctx;
    __android_log_print(ANDROID_LOG_INFO, state->app_name, "%s", message);
}

void android_main(android_app *app) {
    app_dummy();

    user_data p = {};
    strcpy(p.app_name, "org.nxsy.hajonta.ndk");
    app->userData = &p;
    p.asset_manager = app->activity->assetManager;

    app->onAppCmd = on_app_cmd;
    app->onInputEvent = on_input_event;
    uint64_t counter;
    uint start_row = 0;
    uint start_col = 0;

    platform_memory m = {};
    m.size = 64 * 1024 * 1024;
    m.memory = calloc(m.size, sizeof(uint8_t));
    m.platform_fail = platform_fail;
    m.platform_debug_message = platform_debug_message;
    m.platform_load_asset = ndk_load_asset;

    hajonta_thread_context t = {};

    game_input input[2] = {};
    p.new_input = &input[0];
    p.old_input = &input[1];

    int monitor_refresh_hz = 60;
    float game_update_hz = (monitor_refresh_hz / 2.0f); // Should almost always be an int...
    long target_nanoseconds_per_frame = (1000 * 1000 * 1000) / game_update_hz;

    while (!p.stopping && ++counter) {
        timespec start_time = {};
        clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

        game_controller_state *old_keyboard_controller = get_controller(p.old_input, 0);
        game_controller_state *new_keyboard_controller = get_controller(p.new_input, 0);
        *new_keyboard_controller = {};
        new_keyboard_controller->is_active = true;
        for (
                uint button_index = 0;
                button_index < harray_count(new_keyboard_controller->_buttons);
                ++button_index)
        {
            new_keyboard_controller->_buttons[button_index].ended_down =
                old_keyboard_controller->_buttons[button_index].ended_down;
        }

        game_controller_state *pan_controller = get_controller(p.new_input, 1);
        pan_controller->is_active = true;

        int poll_result, events;
        android_poll_source *source;

        while((poll_result = ALooper_pollAll(0, 0, &events, (void**)&source)) >= 0)
        {
            source->process(app, source);
        }

        switch (poll_result)
        {
            case ALOOPER_POLL_WAKE:
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was ALOOPER_POLL_WAKE");
                break;
            }
            case ALOOPER_POLL_CALLBACK:
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was ALOOPER_POLL_CALLBACK");
                break;
            }
            case ALOOPER_POLL_TIMEOUT:
            {
                //__android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was ALOOPER_POLL_TIMEOUT");
                break;
            }
            case ALOOPER_POLL_ERROR:
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was ALOOPER_POLL_ERROR");
                break;
            }
            default:
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was %d", poll_result);
                break;
            }
        }

        p.new_input->delta_t = target_nanoseconds_per_frame / (1024.0 * 1024 * 1024);

        platform_process_events(app, p.new_input, p.old_input);

        game_sound_output sound_output;
        sound_output.samples_per_second = 48000;
        sound_output.channels = 2;
        sound_output.number_of_samples = 48000 / 60;

        if (p.drawable)
        {
            game_update_and_render((hajonta_thread_context *)&p, &m, p.new_input, &sound_output);
        }
        draw(app);

        timespec end_time = {};
        clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

        end_time.tv_sec -= start_time.tv_sec;
        start_time.tv_sec -= start_time.tv_sec;

        int64_t time_taken = ((end_time.tv_sec * 1000000000 + end_time.tv_nsec) -
            (start_time.tv_sec * 1000000000 + start_time.tv_nsec));

        int64_t time_to_sleep = 33 * 1000000;
        if (time_taken <= time_to_sleep)
        {
            timespec sleep_time = {};
            sleep_time.tv_nsec = time_to_sleep - time_taken;
            timespec remainder = {};
            nanosleep(&sleep_time, &remainder);
        }
        else
        {
            if (counter % 10 == 0)
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "Skipped frame!  Took %" PRId64 " ns total", time_taken);
            }
        }

        game_input *temp_input = p.new_input;
        p.new_input = p.old_input;
        p.old_input = temp_input;
    }
}
