
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

    static const auto FLAGS_DEFAULT = Flags::ALLOW_DND;
    static const auto WIDTH_DEFAULT = 640u;
    static const auto HEIGHT_DEFAULT = 400u;
    static constexpr auto TITLE_DEFAULT = "Untitled";

    typedef RGFW_point Point2D;
    typedef RGFW_area Area2D;

    typedef void (*CallbackResize)(int32_t, int32_t);
    typedef void (*CallbackDisplay)();
    typedef void (*CallbackIdle)();
    typedef void (*CallbackKeyboard)(uint8_t, uint8_t, bool);
    typedef void (*CallbackMouseButton)(uint8_t, bool, double);
    typedef void (*CallbackMouseMotion)(int32_t, int32_t);

    bool Initialize(const char *title = TITLE_DEFAULT, uint32_t flags = FLAGS_DEFAULT, uint32_t width = WIDTH_DEFAULT, uint32_t height = HEIGHT_DEFAULT) {
        std::lock_guard<std::mutex> lock(windowMutex);

        if (window) {
            libqb_log_error("Window already created, cannot create another window"); // RGFW_TODO: sure we can, maybe we'll use it a future version of QB64-PE
        } else {
            window = RGFW_createWindow(title, RGFW_RECT(0, 0, width, height), flags);
            if (window) {
                RGFW_window_makeCurrent(window);

                window->userPtr = this;

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

    bool IsFullscreen() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                return RGFW_window_isFullscreen(window);
            }
        } else {
            libqb_log_error("Window not created, cannot check fullscreen");
        }

        return false;
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

    bool IsMaximized() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                return RGFW_window_isMaximized(window);
            }
        } else {
            libqb_log_error("Window not created, cannot check maximized");
        }

        return false;
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

    bool IsMinimized() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                return RGFW_window_isMinimized(window);
            }
        } else {
            libqb_log_error("Window not created, cannot check minimized");
        }

        return false;
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

    void Show() const {
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

    void Hide() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                RGFW_window_hide(window);
            }

            libqb_log_trace("Window hidden");
        } else {
            libqb_log_error("Window not created, cannot hide");
        }
    }

    bool IsHidden() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                return RGFW_window_isHidden(window);
            }
        } else {
            libqb_log_error("Window not created, cannot check hidden");
        }

        return false;
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
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                return {window->r.x, window->r.y};
            }
        } else {
            libqb_log_error("Window not created, cannot get position");
        }

        return {0, 0};
    }

    Area2D GetSize() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                return {uint32_t(window->r.w), uint32_t(window->r.h)};
            }
        } else {
            libqb_log_error("Window not created, cannot get size");
        }

        return {0u, 0u};
    }

    Area2D GetScreenSize() const {
        return RGFW_getScreenSize();
    }

    uintptr_t GetHandle() const {
        if (window) {
            {
                std::lock_guard<std::mutex> lock(windowMutex);
                return reinterpret_cast<uintptr_t>(window->src.window);
            }
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

    void PostRedisplay() {
        ++redisplayCounter;
    }

    void SetWindowResizeFunction(CallbackResize function) {
        if (function) {
            resizeFunction = function;

            RGFW_setWindowResizeCallback(ResizeFunction);

            libqb_log_trace("Window resize function set: %p", function);
        } else {
            libqb_log_error("Window resize function is NULL");
        }
    }

    void SetDisplayFunction(CallbackDisplay function) {
        if (function) {
            displayFunction = function;
            RGFW_setWindowRefreshCallback(RefreshFunction);

            libqb_log_trace("Display function set: %p", function);
        } else {
            libqb_log_error("Display function is NULL");
        }
    }

    void SetIdleFunction(CallbackIdle function) {
        if (function) {
            idleFunction = function;

            libqb_log_trace("Idle function set: %p", function);
        } else {
            libqb_log_error("Idle function is NULL");
        }
    }

    void SetKeyboardFunction(CallbackKeyboard function) {
        if (function) {
            keyboardFunction = function;
            RGFW_setKeyCallback(KeyboardFunction);

            libqb_log_trace("Keyboard function set: %p", function);
        } else {
            libqb_log_error("Keyboard function is NULL");
        }
    }

    void SetMouseButtonFunction(CallbackMouseButton function) {
        if (function) {
            mouseButtonFunction = function;
            RGFW_setMouseButtonCallback(MouseButtonFunction);

            libqb_log_trace("Mouse button function set: %p", function);
        } else {
            libqb_log_error("Mouse button function is NULL");
        }
    }

    void SetMouseMotionFunction(CallbackMouseMotion function) {
        if (function) {
            mouseMotionFunction = function;
            RGFW_setMousePosCallback(MouseMotionFunction);

            libqb_log_trace("Mouse motion function set: %p", function);
        } else {
            libqb_log_error("Mouse motion function is NULL");
        }
    }

    void MainLoop() {
        libqb_log_trace("Entering main loop");

        for (;;) {
            RGFW_window_checkEvents(window, 0);

            if (window->event.type == RGFW_quit) {
                break;
            }

            if (redisplayCounter) {
                --redisplayCounter;
                if (displayFunction) {
                    displayFunction();
                }
            }

            if (idleFunction) {
                idleFunction();
            }
        }

        libqb_log_trace("Exiting main loop");
    }

    static QB64PEWindow &Instance() {
        static QB64PEWindow instance;
        return instance;
    }

  private:
    QB64PEWindow()
        : window(nullptr), redisplayCounter(0), resizeFunction(nullptr), displayFunction(nullptr), idleFunction(nullptr), keyboardFunction(nullptr),
          mouseButtonFunction(nullptr), mouseMotionFunction(nullptr) {}

    ~QB64PEWindow() {
        ShutDown();
    }

    QB64PEWindow(const QB64PEWindow &) = delete;
    QB64PEWindow &operator=(const QB64PEWindow &) = delete;
    QB64PEWindow(QB64PEWindow &&) = delete;
    QB64PEWindow &operator=(QB64PEWindow &&) = delete;

    static void ResizeFunction(RGFW_window *window, RGFW_rect r) {
        auto instance = static_cast<QB64PEWindow *>(window->userPtr);
        if (instance->resizeFunction) {
            instance->resizeFunction(r.w, r.h);
        }
    }

    static void RefreshFunction(RGFW_window *window) {
        auto instance = static_cast<QB64PEWindow *>(window->userPtr);
        if (instance->displayFunction) {
            instance->displayFunction();
        }
    }

    static void KeyboardFunction(RGFW_window *window, u8 key, char keyChar, RGFW_keymod modifiers, b8 isPressed) {
        (void)keyChar;
        auto instance = static_cast<QB64PEWindow *>(window->userPtr);
        if (instance->keyboardFunction) {
            instance->keyboardFunction(key, modifiers, isPressed);
        }
    }

    static void MouseButtonFunction(RGFW_window *window, RGFW_mouseButton button, double scroll, b8 isPressed) {
        auto instance = static_cast<QB64PEWindow *>(window->userPtr);
        if (instance->mouseButtonFunction) {
            instance->mouseButtonFunction(button, isPressed, scroll);
        }
    }

    static void MouseMotionFunction(RGFW_window *window, RGFW_point point) {
        auto instance = static_cast<QB64PEWindow *>(window->userPtr);
        if (instance->mouseMotionFunction) {
            instance->mouseMotionFunction(point.x, point.y);
        }
    }

    RGFW_window *window; // RGFW_TODO: since RGFW allows multiple windows, check if we can support that in the future
    mutable std::mutex windowMutex;
    size_t redisplayCounter;
    CallbackResize resizeFunction;
    CallbackDisplay displayFunction;
    CallbackIdle idleFunction;
    CallbackKeyboard keyboardFunction;
    CallbackMouseButton mouseButtonFunction;
    CallbackMouseMotion mouseMotionFunction;
};

void MAIN_LOOP(void *);
void GLUT_KEYBOARD_FUNC(uint8_t key, uint8_t modifiers, bool isPressed);
void GLUT_DISPLAY_REQUEST();
void GLUT_MOUSE_FUNC(uint8_t button, bool isPressed, double scroll);
void GLUT_MOTION_FUNC(int32_t x, int32_t y);
void GLUT_RESHAPE_FUNC(int32_t width, int32_t height);
void GLUT_IDLEFUNC();

void glutReshapeWindow(int32_t width, int32_t height) {
    QB64PEWindow::Instance().Resize(width, height);
}

void glutFullScreen() {
    QB64PEWindow::Instance().SetFullscreen(true);
}

void glutPostRedisplay() {
    QB64PEWindow::Instance().PostRedisplay();
}

void glutSwapBuffers() {
    QB64PEWindow::Instance().SwapBuffers();
}

void glutDisplayFunc(void (*func)()) {
    QB64PEWindow::Instance().SetDisplayFunction(func);
}

void glutIdleFunc(void (*func)()) {
    QB64PEWindow::Instance().SetIdleFunction(func);
}

void glutReshapeFunc(void (*func)(int, int)) {
    QB64PEWindow::Instance().SetWindowResizeFunction(func);
}

void glutKeyboardFunc(void (*func)(uint8_t, uint8_t, bool)) {
    QB64PEWindow::Instance().SetKeyboardFunction(func);
}

void glutMouseFunc(void (*func)(uint8_t, bool, double)) {
    QB64PEWindow::Instance().SetMouseButtonFunction(func);
}

void glutMotionFunc(void (*func)(int32_t, int32_t)) {
    QB64PEWindow::Instance().SetMouseMotionFunction(func);
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

    QB64PEWindow::Instance().MainLoop();
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

void libqb_glut_set_cursor(int style) {
    QB64PEWindow::Instance().SetMouseCursor(QB64PEWindow::MouseCursorStyle(style));
}

void libqb_glut_warp_pointer(int x, int y) {
    QB64PEWindow::Instance().MoveMouse(x, y);
}

int libqb_glut_get(int id) {
    switch (id) {
    case GLUT_FULL_SCREEN:
        return QB64PEWindow::Instance().IsFullscreen();
        break;

    case GLUT_WINDOW_X:
        return QB64PEWindow::Instance().GetPosition().x;
        break;

    case GLUT_WINDOW_Y:
        return QB64PEWindow::Instance().GetPosition().y;
        break;

    case GLUT_WINDOW_WIDTH:
        return QB64PEWindow::Instance().GetSize().w;
        break;

    case GLUT_WINDOW_HEIGHT:
        return QB64PEWindow::Instance().GetSize().h;
        break;

    case GLUT_WINDOW_BORDER_WIDTH:
        libqb_log_trace("GLUT_WINDOW_BORDER_WIDTH not implemented");
        return 0;
        break;

    case GLUT_WINDOW_BORDER_HEIGHT:
        libqb_log_trace("GLUT_WINDOW_BORDER_HEIGHT not implemented");
        return 0;
        break;

    case GLUT_WINDOW_HEADER_HEIGHT:
        libqb_log_trace("GLUT_WINDOW_HEADER_HEIGHT not implemented");
        return 0;
        break;

    case GLUT_SCREEN_WIDTH:
        return QB64PEWindow::Instance().GetScreenSize().w;
        break;

    case GLUT_SCREEN_HEIGHT:
        return QB64PEWindow::Instance().GetScreenSize().h;
        break;
    }

    return 0;
}

void libqb_glut_iconify_window() {
    QB64PEWindow::Instance().Minimize();
}

void libqb_glut_position_window(int x, int y) {
    QB64PEWindow::Instance().Move(x, y);
}

void libqb_glut_show_window() {
    QB64PEWindow::Instance().Show();
}

void libqb_glut_hide_window() {
    QB64PEWindow::Instance().Hide();
}

void libqb_glut_set_window_title(const char *title) {
    QB64PEWindow::Instance().SetTitle(title);
}

void libqb_glut_exit_program(int exitcode) {
    QB64PEWindow::Instance().ShutDown();

    exit(exitcode);
}
