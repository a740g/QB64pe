//----------------------------------------------------------------------------------------------------------------------
// QB64-PE GLUT emulation layer
// This abstracts the underlying windowing library and provides a GLUT-like API for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#include "glut-emu.h"
#include "logging.h"
#include <cmath>

#if defined(_WIN32)
#    include <windows.h>
#endif

class GLUTEmu {
  public:
    typedef void (*CallbackWindowResize)(int32_t, int32_t);
    typedef void (*CallbackWindowDisplay)();
    typedef void (*CallbackWindowIdle)();
    typedef void (*CallbackKeyboardButton)(uint8_t, uint8_t, bool);
    typedef void (*CallbackMouseButton)(uint8_t, bool, int32_t, int32_t, int32_t);
    typedef void (*CallbackMouseMotion)(int32_t, int32_t, int32_t, int32_t);

    bool WindowInitialize(uint32_t width, uint32_t height, const char *title, uint32_t flags) {
        if (window) {
            libqb_log_error("Window already created, cannot create another window"); // RGFW_TODO: sure we can; maybe we'll use it a future version of QB64-PE
        } else {
            window = RGFW_createWindow(title, RGFW_RECT(0, 0, width, height), flags);
            if (window) {
                window->userPtr = this;

                RGFW_window_makeCurrent(window);

                mouseCurrentPosition = RGFW_window_getMousePoint(window);
                mousePreviousPosition = mouseCurrentPosition;

                ComputeWindowBorderSize();

                libqb_log_trace("Window created (%u x %u)", width, height);

                return true;
            } else {
                libqb_log_error("Failed to create window");
            }
        }

        return false;
    }

    void WindowShutDown() {
        if (window) {
            RGFW_window_close(window);
            window = nullptr;

            libqb_log_trace("Window closed");
        } else {
            libqb_log_error("Window not created, cannot close it");
        }
    }

    bool WindowIsCreated() const {
        return window != nullptr;
    }

    void WindowSetTitle(const char *title) const {
        if (window) {
            RGFW_window_setName(window, title);

            libqb_log_trace("Window title set to '%s'", title);
        } else {
            libqb_log_error("Window not created, cannot set title");
        }
    }

    void WindowSetFullscreen(bool fullscreen) {
        if (window) {
            if (fullscreen) {
                RGFW_window_maximize(window);
                RGFW_window_setBorder(window, false);

                libqb_log_trace("Window set to fullscreen");
            } else {
                RGFW_window_restore(window);
                RGFW_window_setBorder(window, true);

                libqb_log_trace("Window set to windowed");
            }

            ComputeWindowBorderSize();

        } else {
            libqb_log_error("Window not created, cannot set fullscreen");
        }
    }

    bool WindowIsFullscreen() const {
        if (window) {
            return RGFW_window_isFullscreen(window);
        } else {
            libqb_log_error("Window not created, cannot check fullscreen");
        }

        return false;
    }

    void WindowMaximize() {
        if (window) {
            RGFW_window_maximize(window);

            ComputeWindowBorderSize();

            libqb_log_trace("Window maximized");
        } else {
            libqb_log_error("Window not created, cannot maximize");
        }
    }

    bool WindowIsMaximized() const {
        if (window) {
            return RGFW_window_isMaximized(window);
        } else {
            libqb_log_error("Window not created, cannot check maximized");
        }

        return false;
    }

    void WindowMinimize() {
        if (window) {
            RGFW_window_minimize(window);

            ComputeWindowBorderSize();

            libqb_log_trace("Window minimized");
        } else {
            libqb_log_error("Window not created, cannot minimize");
        }
    }

    bool WindowIsMinimized() const {
        if (window) {
            return RGFW_window_isMinimized(window);
        } else {
            libqb_log_error("Window not created, cannot check minimized");
        }

        return false;
    }

    void WindowRestore() {
        if (window) {
            RGFW_window_restore(window);

            ComputeWindowBorderSize();

            libqb_log_trace("Window restored");
        } else {
            libqb_log_error("Window not created, cannot restore");
        }
    }

    void WindowShow() {
        if (window) {
            RGFW_window_show(window);

            ComputeWindowBorderSize();

            libqb_log_trace("Window focused");
        } else {
            libqb_log_error("Window not created, cannot set focus");
        }
    }

    void WindowHide() {
        if (window) {
            RGFW_window_hide(window);

            ComputeWindowBorderSize();

            libqb_log_trace("Window hidden");
        } else {
            libqb_log_error("Window not created, cannot hide");
        }
    }

    bool WindowIsHidden() const {
        if (window) {
            return RGFW_window_isHidden(window);
        } else {
            libqb_log_error("Window not created, cannot check hidden");
        }

        return false;
    }

    void WindowResize(uint32_t width, uint32_t height) {
        if (window) {
            RGFW_window_resize(window, RGFW_AREA(width, height));

            ComputeWindowBorderSize();

            libqb_log_trace("Window resized (%u x %u)", width, height);
        } else {
            libqb_log_error("Window not created, cannot resize");
        }
    }

    void WindowMove(int32_t x, int32_t y) {
        if (window) {
            RGFW_window_move(window, RGFW_POINT(x, y));

            ComputeWindowBorderSize();

            libqb_log_trace("Window moved to (%d, %d)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move");
        }
    }

    RGFW_point WindowGetPosition() const {
        if (window) {
            return {window->r.x - windowBorderSize.x, window->r.y - windowBorderSize.y};
        } else {
            libqb_log_error("Window not created, cannot get position");
        }

        return {0, 0};
    }

    RGFW_area WindowGetSize() const {
        if (window) {
            return {uint32_t(window->r.w), uint32_t(window->r.h)};
        } else {
            libqb_log_error("Window not created, cannot get size");
        }

        return {0u, 0u};
    }

    void *WindowGetHandle() const {
        if (window) {
            return window->src.window;
        } else {
            libqb_log_error("Window not created, cannot get handle");
        }

        return nullptr;
    }

    void WindowSwapBuffers() const {
        if (window) {
            RGFW_window_swapBuffers(window);
        } else {
            libqb_log_error("Window not created, cannot swap buffers");
        }
    }

    void WindowPostRedisplay() {
        ++windowRedisplayCounter;
    }

    void MouseSetCursor(int32_t style) {
        if (window) {
            if (style == GLUT_CURSOR_NONE) {
                RGFW_window_mouseHold(window, RGFW_AREA(0, 0));
                RGFW_window_showMouse(window, false);
                mouseCaptured = true;

                libqb_log_trace("Mouse cursor grabbed & hidden");
            } else {
                RGFW_window_mouseUnhold(window);
                RGFW_window_showMouse(window, true);
                RGFW_window_setMouseStandard(window, RGFW_mouseIcons(style));
                mouseCaptured = false;

                libqb_log_trace("Mouse cursor un-grabbed & set to %d", style);
            }
        } else {
            libqb_log_error("Window not created, cannot set mouse cursor");
        }
    }

    void MouseMove(int32_t x, int32_t y) const {
        if (window) {
            RGFW_window_moveMouse(window, RGFW_POINT(x, y));

            libqb_log_trace("Mouse moved to (%d, %d)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move mouse");
        }
    }

    void WindowSetResizeFunction(CallbackWindowResize function) {
        if (function) {
            windowResizeFunction = function;

            RGFW_setWindowResizeCallback(WindowResizeCallback);

            libqb_log_trace("Window resize function set: %p", function);
        } else {
            libqb_log_error("Window resize function is NULL");
        }
    }

    void WindowSetDisplayFunction(CallbackWindowDisplay function) {
        if (function) {
            windowDisplayFunction = function;
            RGFW_setWindowRefreshCallback(WindowRefreshCallback);

            libqb_log_trace("Display function set: %p", function);
        } else {
            libqb_log_error("Display function is NULL");
        }
    }

    void WindowSetIdleFunction(CallbackWindowIdle function) {
        if (function) {
            windowIdleFunction = function;

            libqb_log_trace("Idle function set: %p", function);
        } else {
            libqb_log_error("Idle function is NULL");
        }
    }

    void KeyboardSetButtonFunction(CallbackKeyboardButton function) {
        if (function) {
            keyboardButtonFunction = function;
            RGFW_setKeyCallback(KeyboardButtonCallback);

            libqb_log_trace("Keyboard function set: %p", function);
        } else {
            libqb_log_error("Keyboard function is NULL");
        }
    }

    void MouseSetButtonFunction(CallbackMouseButton function) {
        if (function) {
            mouseButtonFunction = function;
            RGFW_setMouseButtonCallback(MouseButtonCallback);

            libqb_log_trace("Mouse button function set: %p", function);
        } else {
            libqb_log_error("Mouse button function is NULL");
        }
    }

    void MouseSetMotionFunction(CallbackMouseMotion function) {
        if (function) {
            mouseMotionFunction = function;
            RGFW_setMousePosCallback(MouseMotionCallback);

            libqb_log_trace("Mouse motion function set: %p", function);
        } else {
            libqb_log_error("Mouse motion function is NULL");
        }
    }

    RGFW_area ScreenGetSize() const {
        return RGFW_getScreenSize();
    }

    void MainLoop() {
        libqb_log_trace("Entering main loop");

        for (;;) {
            RGFW_window_eventWait(window, 0);

            while (RGFW_window_checkEvent(window) && window->event.type != RGFW_quit) {
                if (window->event.type == RGFW_mousePosChanged && mouseMotionFunction && mouseCaptured) {
                    mousePreviousPosition = mouseCurrentPosition;
                    RGFW_point mouseRelativePosition = window->event.point;
                    mouseCurrentPosition.x += mouseRelativePosition.x;
                    mouseCurrentPosition.y += mouseRelativePosition.y;
                    mouseMotionFunction(mouseCurrentPosition.x, mouseCurrentPosition.y, mouseRelativePosition.x, mouseRelativePosition.y);
                }
            }

            if (window->event.type == RGFW_quit) {
                break;
            }

            if (windowRedisplayCounter) {
                --windowRedisplayCounter;
                if (windowDisplayFunction) {
                    windowDisplayFunction();
                }
            }

            if (windowIdleFunction) {
                windowIdleFunction();
            }
        }

        libqb_log_trace("Exiting main loop");
    }

    static GLUTEmu &Instance() {
        static GLUTEmu instance;
        return instance;
    }

  private:
    GLUTEmu()
        : window(nullptr), windowRedisplayCounter(0), mouseCaptured(false), windowResizeFunction(nullptr), windowDisplayFunction(nullptr),
          windowIdleFunction(nullptr), keyboardButtonFunction(nullptr), mouseButtonFunction(nullptr), mouseMotionFunction(nullptr) {
        windowBorderSize = {0, 0};
        mouseCurrentPosition = {0, 0};
        mousePreviousPosition = {0, 0};
    }

    ~GLUTEmu() {
        WindowShutDown();
    }

    GLUTEmu(const GLUTEmu &) = delete;
    GLUTEmu &operator=(const GLUTEmu &) = delete;
    GLUTEmu(GLUTEmu &&) = delete;
    GLUTEmu &operator=(GLUTEmu &&) = delete;

    static void WindowResizeCallback(RGFW_window *window, RGFW_rect r) {
        auto instance = static_cast<GLUTEmu *>(window->userPtr);
        if (instance->windowResizeFunction) {
            instance->windowResizeFunction(r.w, r.h);
        }
    }

    static void WindowRefreshCallback(RGFW_window *window) {
        auto instance = static_cast<GLUTEmu *>(window->userPtr);
        if (instance->windowDisplayFunction) {
            instance->windowDisplayFunction();
        }
    }

    static void KeyboardButtonCallback(RGFW_window *window, u8 key, char keyChar, RGFW_keymod modifiers, b8 isPressed) {
        (void)keyChar;

        auto instance = static_cast<GLUTEmu *>(window->userPtr);
        if (instance->keyboardButtonFunction) {
            instance->keyboardButtonFunction(key, modifiers, isPressed);
        }
    }

    static void MouseButtonCallback(RGFW_window *window, RGFW_mouseButton button, double scroll, b8 isPressed) {
        auto instance = static_cast<GLUTEmu *>(window->userPtr);
        if (instance->mouseButtonFunction) {
            instance->mouseButtonFunction(button, isPressed, std::lround(scroll), instance->mouseCurrentPosition.x, instance->mouseCurrentPosition.y);
        }
    }

    static void MouseMotionCallback(RGFW_window *window, RGFW_point point) {
        auto instance = static_cast<GLUTEmu *>(window->userPtr);
        if (instance->mouseMotionFunction && !instance->mouseCaptured) {
            instance->mousePreviousPosition = instance->mouseCurrentPosition;
            instance->mouseCurrentPosition = point;
            RGFW_point mouseRelativePosition;
            mouseRelativePosition.x = point.x - instance->mousePreviousPosition.x;
            mouseRelativePosition.y = point.y - instance->mousePreviousPosition.y;
            instance->mouseMotionFunction(instance->mouseCurrentPosition.x, instance->mouseCurrentPosition.y, mouseRelativePosition.x, mouseRelativePosition.y);
        }
    }

#if defined(_WIN32)

    void ComputeWindowBorderSize() {
        RECT windowRect;
        GetWindowRect(window->src.window, &windowRect);

        windowBorderSize.x = window->r.x - windowRect.left;
        windowBorderSize.y = window->r.y - windowRect.top;
    }

#elif defined(__APPLE__)

#elif defined(__linux__)

#else
#    error Unsupported platform
#endif

    RGFW_window *window; // RGFW_TODO: since RGFW allows multiple windows, check if we can support that in the future
    size_t windowRedisplayCounter;
    bool mouseCaptured;
    RGFW_point windowBorderSize;
    RGFW_point mouseCurrentPosition;
    RGFW_point mousePreviousPosition;
    CallbackWindowResize windowResizeFunction;
    CallbackWindowDisplay windowDisplayFunction;
    CallbackWindowIdle windowIdleFunction;
    CallbackKeyboardButton keyboardButtonFunction;
    CallbackMouseButton mouseButtonFunction;
    CallbackMouseMotion mouseMotionFunction;
};

bool glutInitWindow(uint32_t width, uint32_t height, const char *title, uint32_t flags) {
    return GLUTEmu::Instance().WindowInitialize(width, height, title, flags);
}

void glutReshapeWindow(int32_t width, int32_t height) {
    GLUTEmu::Instance().WindowResize(width, height);
}

void glutFullScreen() {
    GLUTEmu::Instance().WindowSetFullscreen(true);
}

void glutPostRedisplay() {
    GLUTEmu::Instance().WindowPostRedisplay();
}

void glutSwapBuffers() {
    GLUTEmu::Instance().WindowSwapBuffers();
}

void glutDisplayFunc(void (*func)()) {
    GLUTEmu::Instance().WindowSetDisplayFunction(func);
}

void glutIdleFunc(void (*func)()) {
    GLUTEmu::Instance().WindowSetIdleFunction(func);
}

void glutReshapeFunc(void (*func)(int32_t, int32_t)) {
    GLUTEmu::Instance().WindowSetResizeFunction(func);
}

void glutKeyboardFunc(void (*func)(uint8_t, uint8_t, bool)) {
    GLUTEmu::Instance().KeyboardSetButtonFunction(func);
}

void glutMouseFunc(void (*func)(uint8_t, bool, int32_t, int32_t, int32_t)) {
    GLUTEmu::Instance().MouseSetButtonFunction(func);
}

void glutMotionFunc(void (*func)(int32_t, int32_t, int32_t, int32_t)) {
    GLUTEmu::Instance().MouseSetMotionFunction(func);
}

void glutMainLoop() {
    GLUTEmu::Instance().MainLoop();
}

void glutSetCursor(int32_t style) {
    GLUTEmu::Instance().MouseSetCursor(style);
}

void glutWarpPointer(int32_t x, int32_t y) {
    GLUTEmu::Instance().MouseMove(x, y);
}

int32_t glutGet(uint32_t id) {
    switch (id) {
    case GLUT_FULL_SCREEN:
        return GLUTEmu::Instance().WindowIsFullscreen();
        break;

    case GLUT_WINDOW_X:
        return GLUTEmu::Instance().WindowGetPosition().x;
        break;

    case GLUT_WINDOW_Y:
        return GLUTEmu::Instance().WindowGetPosition().y;
        break;

    case GLUT_WINDOW_WIDTH:
        return GLUTEmu::Instance().WindowGetSize().w;
        break;

    case GLUT_WINDOW_HEIGHT:
        return GLUTEmu::Instance().WindowGetSize().h;
        break;

    case GLUT_SCREEN_WIDTH:
        return GLUTEmu::Instance().ScreenGetSize().w;
        break;

    case GLUT_SCREEN_HEIGHT:
        return GLUTEmu::Instance().ScreenGetSize().h;
        break;
    }

    return 0;
}

void glutIconifyWindow() {
    GLUTEmu::Instance().WindowMinimize();
}

void glutPositionWindow(int32_t x, int32_t y) {
    GLUTEmu::Instance().WindowMove(x, y);
}

void glutShowWindow() {
    GLUTEmu::Instance().WindowShow();
}

void glutHideWindow() {
    GLUTEmu::Instance().WindowHide();
}

void glutSetWindowTitle(const char *title) {
    GLUTEmu::Instance().WindowSetTitle(title);
}

void *glutGetWindowHandle() {
    return GLUTEmu::Instance().WindowGetHandle();
}
