
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

    enum Flags : uint32_t {
        NO_INIT_API = RGFW_windowNoInitAPI,
        NO_BORDER = RGFW_windowNoBorder,
        NO_RESIZE = RGFW_windowNoResize,
        ALLOW_DND = RGFW_windowAllowDND,
        HIDE_MOUSE = RGFW_windowHideMouse,
        FULLSCREEN = RGFW_windowFullscreen,
        TRANSPARENT = RGFW_windowTransparent,
        CENTER = RGFW_windowCenter,
        OPENGL_SOFTWARE = RGFW_windowOpenglSoftware,
        COCOA_CHDIR_TO_RES = RGFW_windowCocoaCHDirToRes,
        SCALE_TO_MONITOR = RGFW_windowScaleToMonitor,
        HIDE = RGFW_windowHide
    };

    static const auto FLAGS_DEFAULT = Flags::ALLOW_DND | Flags::SCALE_TO_MONITOR;
    static const auto WIDTH_DEFAULT = 640u;
    static const auto HEIGHT_DEFAULT = 400u;
    static constexpr auto TITLE_DEFAULT = "Untitled";

    typedef RGFW_point Point2D;
    typedef RGFW_area Size2D;

    bool Initialize(const char *title = TITLE_DEFAULT, uint32_t flags = FLAGS_DEFAULT, uint32_t width = WIDTH_DEFAULT, uint32_t height = HEIGHT_DEFAULT) {
        std::lock_guard<std::mutex> lock(windowMutex);

        if (window) {
            libqb_log_error("Window already created, cannot create another window"); // sure we can, maybe we'll use it a future version of QB64-PE
        } else {
            window = RGFW_createWindow(title, RGFW_RECT(0, 0, width, height), flags);
            if (window) {
                RGFW_window_makeCurrent(window);

                libqb_log_trace("Window created (%u x %u)", width, height);

                return true;
            } else {
                libqb_log_error("Failed to create window");
            }
        }

        return false;
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

    Point2D GetPosition() const {
        if (window) {
            return {window->r.x, window->r.y};
        } else {
            libqb_log_error("Window not created, cannot get position");
        }

        return {0, 0};
    }

    Size2D GetSize() const {
        if (window) {
            return {uint32_t(window->r.w), uint32_t(window->r.h)};
        } else {
            libqb_log_error("Window not created, cannot get size");
        }

        return {0u, 0u};
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
                    RGFW_window_showMouse(window, false);
                    RGFW_window_mouseHold(window, RGFW_AREA(0, 0));
                }

                libqb_log_trace("Mouse cursor grabbed & hidden");
            } else {
                {
                    std::lock_guard<std::mutex> lock(windowMutex);
                    RGFW_window_showMouse(window, true);
                    RGFW_window_mouseUnhold(window);
                    RGFW_window_setMouseStandard(window, RGFW_mouseIcons(style));
                }

                libqb_log_trace("Mouse cursor un-grabbed & set to %d", style);
            }
        } else {
            libqb_log_error("Window not created, cannot set mouse cursor");
        }
    }

    void MoveMouse(int32_t x, int32_t y) const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                RGFW_window_moveMouse(window, RGFW_POINT(x, y));
            }

            libqb_log_trace("Mouse moved to (%d, %d)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move mouse");
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
    auto windowTitle = (window_title ? reinterpret_cast<const char *>(window_title) : QB64PEWindow::TITLE_DEFAULT);
    auto windowFlags = QB64PEWindow::FLAGS_DEFAULT | (screen_hide ? QB64PEWindow::Flags::HIDE : 0);

    if (!QB64PEWindow::Instance().Initialize(windowTitle, windowFlags)) {
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

static libqb_mutex *glut_msg_queue_lock = libqb_mutex_new();
static std::queue<glut_message *> glut_msg_queue;

// These values from GLUT are read on every process of the msg queue. Calls to
// libqb_glut_get() can then read from these values directly rather than wait
// for the GLUT thread to process the command.
static int glut_window_x, glut_window_y;
static int glut_window_border_width, glut_window_header_height;

bool libqb_queue_glut_message(glut_message *msg) {
    if (!libqb_is_glut_up()) {
        msg->finish();
        return false;
    }

    libqb_mutex_guard guard(glut_msg_queue_lock);

    glut_msg_queue.push(msg);

    return true;
}

void libqb_process_glut_queue() {
    libqb_mutex_guard guard(glut_msg_queue_lock);

    glut_window_x = glutGet(GLUT_WINDOW_X);
    glut_window_y = glutGet(GLUT_WINDOW_Y);
    glut_window_border_width = glutGet(GLUT_WINDOW_BORDER_WIDTH);
    glut_window_header_height = glutGet(GLUT_WINDOW_HEADER_HEIGHT);

    while (!glut_msg_queue.empty()) {
        glut_message *msg = glut_msg_queue.front();
        glut_msg_queue.pop();

        msg->execute();

        msg->finish();
    }
}

void libqb_glut_set_cursor(int style) {
    QB64PEWindow::Instance().SetMouseCursor(QB64PEWindow::MouseCursorStyle(style));
}

void libqb_glut_warp_pointer(int x, int y) {
    QB64PEWindow::Instance().MoveMouse(x, y);
}

int libqb_glut_get(int id) {
    if (is_static_glut_value(id)) {
        libqb_mutex_guard guard(glut_msg_queue_lock);
        return __get_static_glut_value(id);
    }

    glut_message_get msg(id);

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

void libqb_glut_iconify_window() {
    libqb_queue_glut_message(new glut_message_iconify_window());
}

void libqb_glut_position_window(int x, int y) {
    libqb_queue_glut_message(new glut_message_position_window(x, y));
}

void libqb_glut_show_window() {
    libqb_queue_glut_message(new glut_message_show_window());
}

void libqb_glut_hide_window() {
    libqb_queue_glut_message(new glut_message_hide_window());
}

void libqb_glut_set_window_title(const char *title) {
    libqb_queue_glut_message(new glut_message_set_window_title(title));
}

void libqb_glut_exit_program(int exitcode) {
    QB64PEWindow::Instance().ShutDown();

    exit(exitcode);
}
