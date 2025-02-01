
#include "libqb-common.h"

#include <GL/glew.h>
#include <RGFW.h>
#include <list>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unordered_map>

#include "completion.h"
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

class QB64PEWindow {
  public:
    enum MouseCursorStyle {
        NONE = -1,
        LEFT_ARROW = RGFW_mouseArrow,
        INFO = RGFW_mousePointingHand,
        TEXT = RGFW_mouseIbeam,
        CROSSHAIR = RGFW_mouseCrosshair,
        UP_DOWN = RGFW_mouseResizeNS,
        LEFT_RIGHT = RGFW_mouseResizeEW,
        TOP_LEFT_CORNER = RGFW_mouseResizeNESW,
        TOP_RIGHT_CORNER = RGFW_mouseResizeNWSE,
        WAIT = RGFW_mouseNotAllowed,
        HELP = RGFW_mouseNormal,
        CYCLE = RGFW_mouseResizeAll
    };

    bool Initialize(uint32_t width = WIDTH_DEFAULT, uint32_t height = HEIGHT_DEFAULT, const char *title = TITLE_DEFAULT) {
        std::lock_guard<std::mutex> lock(windowMutex);

        if (window) {
            libqb_log_error("Window already created, cannot create another window"); // sure we can, but not in this version
        } else {
            window = RGFW_createWindow(title, RGFW_RECT(0, 0, width, height), flags);
            if (window) {
                libqb_log_trace("Window created (%u x %u)", width, height);

                return true;
            } else {
                libqb_log_error("Failed to create window");
            }
        }

        return false;
    }

    bool Initialize(const char *title) {
        return Initialize(WIDTH_DEFAULT, HEIGHT_DEFAULT, title);
    }

    void ShutDown() {
        std::lock_guard<std::mutex> lock(windowMutex);

        if (window) {
            RGFW_window_close(window);
            window = nullptr;

            libqb_log_trace("Window closed");
        } else {
            libqb_log_error("Window not created, cannot close it");
        }
    }

    bool IsCreated() const {
        return window != nullptr;
    }

    void SetTitle(const char *title) const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                RGFW_window_setName(window, title);
            }

            libqb_log_trace("Window title set to '%s'", title);
        } else {
            libqb_log_error("Window not created, cannot set title");
        }
    }

    void SetFullscreen(bool fullscreen) const {
        if (window) {
            if (fullscreen) {
                {
                    std::lock_guard<std::mutex> lock(windowMutex);
                    RGFW_window_maximize(window);
                    RGFW_window_setBorder(window, false);
                }

                libqb_log_trace("Window set to fullscreen");
            } else {
                {
                    std::lock_guard<std::mutex> lock(windowMutex);
                    RGFW_window_restore(window);
                    RGFW_window_setBorder(window, true);
                }

                libqb_log_trace("Window set to windowed");
            }
        } else {
            libqb_log_error("Window not created, cannot set fullscreen");
        }
    }

    void Maximize() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                RGFW_window_maximize(window);
            }

            libqb_log_trace("Window maximized");
        } else {
            libqb_log_error("Window not created, cannot maximize");
        }
    }

    void Minimize() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                RGFW_window_minimize(window);
            }

            libqb_log_trace("Window minimized");
        } else {
            libqb_log_error("Window not created, cannot minimize");
        }
    }

    void Restore() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                RGFW_window_restore(window);
            }

            libqb_log_trace("Window restored");
        } else {
            libqb_log_error("Window not created, cannot restore");
        }
    }

    void Resize(uint32_t width, uint32_t height) const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                RGFW_window_resize(window, RGFW_AREA(width, height));
            }

            libqb_log_trace("Window resized (%u x %u)", width, height);
        } else {
            libqb_log_error("Window not created, cannot resize");
        }
    }

    void Move(int32_t x, int32_t y) const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                RGFW_window_move(window, RGFW_POINT(x, y));
            }

            libqb_log_trace("Window moved to (%d, %d)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move");
        }
    }

    std::pair<int32_t, int32_t> GetPosition() const {
        if (window) {
            return {window->r.x, window->r.y};
        } else {
            libqb_log_error("Window not created, cannot get position");
        }

        return {0, 0};
    }

    std::pair<uint32_t, uint32_t> GetSize() const {
        if (window) {
            return {window->r.w, window->r.h};
        } else {
            libqb_log_error("Window not created, cannot get size");
        }

        return {0, 0};
    }

    void Focus() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                RGFW_window_show(window);
            }

            libqb_log_trace("Window focused");
        } else {
            libqb_log_error("Window not created, cannot set focus");
        }
    }

    uintptr_t GetHandle() const {
        if (window) {
            return reinterpret_cast<uintptr_t>(window->src.window);
        } else {
            libqb_log_error("Window not created, cannot get handle");
        }

        return NULL;
    }

    void SetMouseCursor(MouseCursorStyle style) const {
        if (window) {
            if (style == MouseCursorStyle::NONE) {
                {
                    std::lock_guard<std::mutex> lock(windowMutex);
                    RGFW_window_mouseHold(window, RGFW_AREA(0, 0));
                    RGFW_window_showMouse(window, false);
                }

                libqb_log_trace("Mouse cursor grabbed & hidden");
            } else {
                {
                    std::lock_guard<std::mutex> lock(windowMutex);
                    RGFW_window_mouseUnhold(window);
                    RGFW_window_showMouse(window, true);
                    RGFW_window_setMouseStandard(window, RGFW_mouseIcons(style));
                }

                libqb_log_trace("Mouse cursor un-grabbed & set to %d", style);
            }
        } else {
            libqb_log_error("Window not created, cannot set mouse cursor");
        }
    }

    void SwapBuffers() const {
        if (window) {
            RGFW_window_swapBuffers(window);
        } else {
            libqb_log_error("Window not created, cannot swap buffers");
        }
    }

    static QB64PEWindow &Instance() {
        static QB64PEWindow instance;

        libqb_log_trace("Returning window instance");

        return instance;
    }

  private:
    static const uint32_t FLAGS_DEFAULT = RGFW_windowAllowDND | RGFW_windowScaleToMonitor;
    static const uint32_t WIDTH_DEFAULT = 640u;
    static const uint32_t HEIGHT_DEFAULT = 400u;
    static constexpr auto TITLE_DEFAULT = "Untitled";

    QB64PEWindow() : window(nullptr), flags(FLAGS_DEFAULT) {}

    ~QB64PEWindow() {
        ShutDown();
    }

    QB64PEWindow(const QB64PEWindow &) = delete;
    QB64PEWindow &operator=(const QB64PEWindow &) = delete;
    QB64PEWindow(QB64PEWindow &&) = delete;
    QB64PEWindow &operator=(QB64PEWindow &&) = delete;

    RGFW_window *window; // RGFW_TODO: since RGFW allows multiple windows, check if we can support that in the future
    RGFW_windowFlags flags;
    mutable std::mutex windowMutex;
};

void MAIN_LOOP(void *);
void GLUT_KEYBOARD_FUNC(unsigned char key, int x, int y);
void GLUT_DISPLAY_REQUEST();
void GLUT_KEYBOARDUP_FUNC(unsigned char key, int x, int y);
void GLUT_SPECIAL_FUNC(int key, int x, int y);
void GLUT_SPECIALUP_FUNC(int key, int x, int y);
void GLUT_MOUSE_FUNC(int glut_button, int state, int x, int y);
void GLUT_MOTION_FUNC(int x, int y);
void GLUT_PASSIVEMOTION_FUNC(int x, int y);
void GLUT_RESHAPE_FUNC(int width, int height);
void GLUT_IDLEFUNC();
void GLUT_MOUSEWHEEL_FUNC(int wheel, int direction, int x, int y);

static void glutWarning(const char *fmt, va_list lst) {
    // This keeps FreeGlut from dumping warnings to console
    (void)fmt;
    (void)lst;
}

void glutReshapeWindow(uint32_t width, uint32_t height) {
    // RGFW_TODO: GLUT runs in the GLUT main loop
    QB64PEWindow::Instance().Resize(width, height);
}

void glutFullScreen() {
    // RGFW_TODO: GLUT runs in the GLUT main loop
    QB64PEWindow::Instance().SetFullscreen(true);
}

void glutPostRedisplay() {
    libqb_log_error("glutPostRedisplay() called, but not implemented");
    exit(EXIT_FAILURE);
}

void glutSwapBuffers() {
    QB64PEWindow::Instance().SwapBuffers();
}

// Performs all of the FreeGLUT initialization except for calling glutMainLoop()
static void initialize_glut() {
    if (window_title) {
        if (!QB64PEWindow::Instance().Initialize(reinterpret_cast<char *>(window_title))) {
            gui_alert("Failed to initialize window");
            exit(EXIT_FAILURE);
        }
    } else {
        if (!QB64PEWindow::Instance().Initialize()) {
            gui_alert("Failed to initialize window");
            exit(EXIT_FAILURE);
        }
    }

    // RGFW_TODO: this needs to be implemented - glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);

    auto err = glewInit();
    if (GLEW_OK != err) {
        gui_alert((char *)glewGetErrorString(err));
    }

    if (glewIsSupported("GL_EXT_framebuffer_object"))
        framebufferobjects_supported = 1;

    glutDisplayFunc(GLUT_DISPLAY_REQUEST);

    glutIdleFunc(GLUT_IDLEFUNC);

    glutKeyboardFunc(GLUT_KEYBOARD_FUNC);
    glutKeyboardUpFunc(GLUT_KEYBOARDUP_FUNC);
    glutSpecialFunc(GLUT_SPECIAL_FUNC);
    glutSpecialUpFunc(GLUT_SPECIALUP_FUNC);
    glutMouseFunc(GLUT_MOUSE_FUNC);
    glutMotionFunc(GLUT_MOTION_FUNC);
    glutPassiveMotionFunc(GLUT_PASSIVEMOTION_FUNC);
    glutReshapeFunc(GLUT_RESHAPE_FUNC);
    glutMouseWheelFunc(GLUT_MOUSEWHEEL_FUNC);
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

void libqb_glut_presetup(int argc, char **argv) {
    if (!screen_hide) {
        initialize_glut(); // Initialize GLUT if the screen isn't hidden
        glut_is_started = true;
    } else {
        completion_init(&glut_thread_starter);
    }
}

void libqb_start_main_thread(int argc, char **argv) {

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
