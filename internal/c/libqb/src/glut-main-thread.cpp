
#include "libqb-common.h"

#include <GL/glew.h>
#include <list>
#include <queue>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unordered_map>

#include "completion.h"
#include "glut-emu.h"
#include "glut-thread.h"
#include "gui.h"
#include "logging.h"
#include "mutex.h"
#include "thread.h"

// FIXME: These extern variable and function definitions should probably go
// somewhere more global so that they can be referenced by libqb.cpp
extern uint8_t *window_title;
extern int32_t framebufferobjects_supported;
extern int32_t screen_hide;

void MAIN_LOOP(void *);
void GLUT_KEYBOARD_FUNC(uint8_t key, uint8_t modifiers, bool isPressed);
void GLUT_DISPLAY_REQUEST();
void GLUT_MOUSE_FUNC(uint8_t button, bool isPressed, int32_t scroll, int32_t x, int32_t y);
void GLUT_MOTION_FUNC(int32_t x, int32_t y, int32_t dx, int32_t dy);
void GLUT_RESHAPE_FUNC(int32_t width, int32_t height);
void GLUT_IDLEFUNC();

// Performs all of the FreeGLUT initialization except for calling glutMainLoop()
static void initialize_glut() {
    auto windowTitle = (window_title ? reinterpret_cast<const char *>(window_title) : "Untitled");
    auto windowFlags = GLUT_WINDOW_FLAG_ALLOW_DND | (screen_hide ? GLUT_WINDOW_FLAG_HIDE : 0);

    if (!glutInitWindow(640, 400, windowTitle, windowFlags)) {
        gui_alert("Failed to initialize window");
        exit(EXIT_FAILURE);
    }

    auto err = glewInit();
    if (GLEW_OK != err) {
        gui_alert(reinterpret_cast<const char *>(glewGetErrorString(err)));
        exit(EXIT_FAILURE);
    }

    if (GLEW_EXT_framebuffer_object) {
        framebufferobjects_supported = 1;

        libqb_log_trace("GLEW_EXT_framebuffer_object supported");
    }

    // RGFW_TODO: check implementation - glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    RGFW_setGLSamples(4);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glutDisplayFunc(GLUT_DISPLAY_REQUEST);
    glutIdleFunc(GLUT_IDLEFUNC);
    glutKeyboardFunc(GLUT_KEYBOARD_FUNC);
    glutMouseFunc(GLUT_MOUSE_FUNC);
    glutMotionFunc(GLUT_MOTION_FUNC);
    glutReshapeFunc(GLUT_RESHAPE_FUNC);
}

static bool glut_is_started;
static struct completion glut_thread_starter;
static struct completion *glut_thread_initialized;

void libqb_start_glut_thread() {
    if (glut_is_started)
        return;

    struct completion init;
    completion_init(&init);

    glut_thread_initialized = &init;

    completion_finish(&glut_thread_starter);

    completion_wait(&init);
    completion_clear(&init);
}

// Checks whether the GLUT thread is running
bool libqb_is_glut_up() {
    return glut_is_started;
}

void libqb_glut_presetup() {
    if (!screen_hide) {
        initialize_glut(); // Initialize GLUT if the screen isn't hidden
        glut_is_started = true;
    } else {
        completion_init(&glut_thread_starter);
    }
}

void libqb_start_main_thread() {

    // Start the 'MAIN_LOOP' in a separate thread, as GLUT has to run on the
    // initial thread.
    struct libqb_thread *main_loop = libqb_thread_new();
    libqb_thread_start(main_loop, MAIN_LOOP, NULL);

    // This happens for $SCREENHIDE programs. This thread waits on the
    // `glut_thread_starter` completion, which will get completed if a
    // _ScreenShow is used.
    if (!glut_is_started) {
        completion_wait(&glut_thread_starter);

        initialize_glut();
        glut_is_started = true;

        if (glut_thread_initialized)
            completion_finish(glut_thread_initialized);
    }

    glutMainLoop();
}

// Due to GLUT making use of cleanup via atexit, we have to call exit() from
// the same thread handling the GLUT logic so that the atexit handler also runs
// from that thread (not doing that can result in a segfault due to using GLUT
// from two threads at the same time).
//
// This is accomplished by simply queuing a GLUT message that calls exit() for us.
void libqb_exit(int exitcode) {
    libqb_log_info("Program exiting with code: %d\n", exitcode);
    // If GLUT isn't running then we're free to do the exit() call from here
    if (!libqb_is_glut_up())
        exit(exitcode);

    libqb_glut_exit_program(exitcode);
}
