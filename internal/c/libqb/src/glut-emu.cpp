//----------------------------------------------------------------------------------------------------------------------
// QB64-PE GLUT-like emulation layer
// This abstracts the underlying windowing library and provides a GLUT-like API for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#include "libqb-common.h"

#include "glut-emu.h"
#include "logging.h"

#if defined(QB64_WINDOWS)
#    define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(QB64_MACOSX)
#    define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(QB64_LINUX)
#    include <X11/XKBlib.h>
#    define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

class GLUTEmu {
  public:
    template <typename T> void WindowSetHint(GLUTEmu_WindowHint hint, T value) const {
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, unsigned int> || std::is_same_v<T, bool>) {
            glfwWindowHint(int(hint), value);
            libqb_log_trace("Window hint set: %d = %d", hint, value);
        } else if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>) {
            glfwWindowHintString(int(hint), value);
            libqb_log_trace("Window hint set: %d = '%s'", hint, value);
        } else {
            static_assert(!sizeof(T), "Unsupported type");
        }
    }

    bool WindowInitialize(int width, int height, const char *title) {
        if (isInitialized) {
            if (window) {
                // GLFW_TODO: sure we can; maybe we'll use it in a future version of QB64-PE
                libqb_log_warn("Window already created, cannot create another window");
            } else {
                window = glfwCreateWindow(width, height, title, nullptr, nullptr);
                if (window) {
                    windowClientWidth = width;
                    windowClientHeight = height;

                    glfwSetWindowUserPointer(window, this);

                    glfwMakeContextCurrent(window);

                    auto glewError = glewInit();
                    if (GLEW_OK != glewError) {
                        libqb_log_error("Failed to initialize GLEW: %s", glewGetErrorString(glewError));

                        glfwDestroyWindow(window);
                        libqb_log_error("Window destroyed due to GLEW initialization failure");
                        window = nullptr;

                        return false;
                    }

                    glfwSwapInterval(1);

                    glfwGetWindowContentScale(window, &windowXScale, &windowYScale);

                    glfwSetWindowContentScaleCallback(window, [](GLFWwindow *win, float xScale, float yScale) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->windowXScale = xScale;
                        instance->windowYScale = yScale;
                    });

                    int fbWidth, fbHeight;
                    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

                    if (fbWidth != width || fbHeight != height) {
                        libqb_log_warn("Framebuffer size (%d x %d) does not match requested size (%d x %d)", fbWidth, fbHeight, width, height);

                        WindowResize(width, height);
                    }

                    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int width, int height) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        if (instance->windowClientWidth != width || instance->windowClientHeight != height) {
                            libqb_log_warn("Framebuffer size (%d x %d) does not match requested size (%d x %d)", width, height, instance->windowClientWidth,
                                           instance->windowClientHeight);

                            instance->WindowResize(width, height);
                        }
                    });

                    monitor = WindowGetCurrentMonitor();

                    glfwSetWindowPosCallback(window, [](GLFWwindow *win, int x, int y) {
                        (void)x, (void)y;
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->monitor = instance->WindowGetCurrentMonitor();
                    });

                    // Set a hook into the keyboard button callback to enable some backward compatibility
                    glfwSetKeyCallback(window, [](GLFWwindow *win, int key, int scancode, int action, int mods) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
#if defined(QB64_MACOSX) || defined(QB64_LINUX)
                        if (key == GLFW_KEY_SCROLL_LOCK && action == GLFW_RELEASE) {
                            instance->keyboardScollLockState = !instance->keyboardScollLockState;
                        }
#endif
                        instance->keyboardModifiers = instance->KeyboardUpdateLockKeyModifier(GLFW_KEY_SCROLL_LOCK, mods);
                        if (instance->keyboardButtonFunction) {
                            instance->keyboardButtonFunction(GLUTEmu_KeyboardKey(key), scancode, GLUTEmu_ButtonAction(action));
                        }
                    });

                    glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

                    keyboardModifiers = KeyboardUpdateLockKeyModifier(GLFW_KEY_CAPS_LOCK, keyboardModifiers);
                    keyboardModifiers = KeyboardUpdateLockKeyModifier(GLFW_KEY_NUM_LOCK, keyboardModifiers);
                    keyboardModifiers = KeyboardUpdateLockKeyModifier(GLFW_KEY_SCROLL_LOCK, keyboardModifiers);

                    if (glfwRawMouseMotionSupported()) {
                        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                        libqb_log_trace("Raw mouse motion supported and enabled");
                    } else {
                        libqb_log_trace("Raw mouse motion not supported");
                    }

                    mouseCursorMode = GLUTEnum_MouseCursorMode(glfwGetInputMode(window, GLFW_CURSOR));

                    // Set a hook into the mouse button callback to enable some backward compatibility
                    glfwSetMouseButtonCallback(window, [](GLFWwindow *win, int button, int action, int mods) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->keyboardModifiers = instance->KeyboardUpdateLockKeyModifier(GLFW_KEY_SCROLL_LOCK, mods);
                        if (instance->mouseButtonFunction) {
                            instance->mouseButtonFunction(GLUTEmu_MouseButton(button), GLUTEmu_ButtonAction(action));
                        }
                    });

                    libqb_log_trace("Window created (%d x %d)", width, height);

                    return true;
                } else {
                    libqb_log_error("Failed to create window");
                }
            }
        } else {
            libqb_log_error("GLFW not initialized, cannot create window");
        }

        return false;
    }

    bool WindowIsCreated() const {
        return window != nullptr;
    }

    void WindowSetTitle(const char *title) const {
        if (window) {
            glfwSetWindowTitle(window, title);
        } else {
            libqb_log_error("Window not created, cannot set title");
        }
    }

    const char *WindowGetTitle() const {
        if (window) {
            return glfwGetWindowTitle(window);
        } else {
            libqb_log_error("Window not created, cannot get title");
        }

        return nullptr;
    }

    void WindowFullscreen(bool fullscreen) {
        if (window) {
            if (fullscreen) {
                if (!isWindowFullscreen) {
                    glfwGetWindowPos(window, &windowX, &windowY);
                    glfwGetWindowSize(window, &windowWidth, &windowHeight);
                    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                    isWindowFullscreen = true;
                    windowShouldRefresh = true;

                    libqb_log_trace("Window set to fullscreen mode");
                } else {
                    libqb_log_trace("Window already in fullscreen mode, ignoring request");
                }
            } else {
                if (isWindowFullscreen) {
                    glfwSetWindowMonitor(window, nullptr, windowX, windowY, windowWidth, windowHeight, 0);
                    isWindowFullscreen = false;
                    windowShouldRefresh = true;

                    libqb_log_trace("Window set to windowed mode");
                } else {
                    libqb_log_trace("Window already in windowed mode, ignoring request");
                }
            }
        } else {
            libqb_log_error("Window not created, cannot set fullscreen");
        }
    }

    bool WindowIsFullscreen() const {
        if (window) {
            return isWindowFullscreen;
        } else {
            libqb_log_error("Window not created, cannot check fullscreen");
        }

        return false;
    }

    void WindowMaximize() {
        if (window) {
            glfwMaximizeWindow(window);

            windowShouldRefresh = true;

            libqb_log_trace("Window maximized");
        } else {
            libqb_log_error("Window not created, cannot maximize");
        }
    }

    bool WindowIsMaximized() const {
        if (window) {
            return bool(glfwGetWindowAttrib(window, GLFW_MAXIMIZED));
        } else {
            libqb_log_error("Window not created, cannot check maximized");
        }

        return false;
    }

    void WindowMinimize() {
        if (window) {
            glfwIconifyWindow(window);

            windowShouldRefresh = false;

            libqb_log_trace("Window minimized");
        } else {
            libqb_log_error("Window not created, cannot minimize");
        }
    }

    bool WindowIsMinimized() const {
        if (window) {
            return bool(glfwGetWindowAttrib(window, GLFW_ICONIFIED));
        } else {
            libqb_log_error("Window not created, cannot check minimized");
        }

        return false;
    }

    void WindowRestore() {
        if (window) {
            glfwRestoreWindow(window);

            windowShouldRefresh = true;

            libqb_log_trace("Window restored");
        } else {
            libqb_log_error("Window not created, cannot restore");
        }
    }

    void WindowShow() {
        if (window) {
            glfwShowWindow(window);

            windowShouldRefresh = true;

            libqb_log_trace("Window focused");
        } else {
            libqb_log_error("Window not created, cannot set focus");
        }
    }

    void WindowHide() {
        if (window) {
            glfwHideWindow(window);

            windowShouldRefresh = false;

            libqb_log_trace("Window hidden");
        } else {
            libqb_log_error("Window not created, cannot hide");
        }
    }

    bool WindowIsVisible() const {
        if (window) {
            return bool(glfwGetWindowAttrib(window, GLFW_VISIBLE));
        } else {
            libqb_log_error("Window not created, cannot check visibility");
        }

        return false;
    }

    void WindowFocus() {
        if (window) {
            glfwFocusWindow(window);

            windowShouldRefresh = true;

            libqb_log_trace("Window focused");
        } else {
            libqb_log_error("Window not created, cannot focus");
        }
    }

    bool WindowHasFocus() const {
        if (window) {
            return bool(glfwGetWindowAttrib(window, GLFW_FOCUSED));
        } else {
            libqb_log_error("Window not created, cannot check focus");
        }

        return false;
    }

    void WindowResize(int width, int height) {
        if (window) {
            windowClientWidth = width;
            windowClientHeight = height;

            glfwSetWindowSize(window, ToScreenCoordsX(width), ToScreenCoordsY(height));

            windowShouldRefresh = true;

            libqb_log_trace("Window resized (%d x %d)", width, height);
        } else {
            libqb_log_error("Window not created, cannot resize");
        }
    }

    std::pair<int, int> WindowGetSize() const {
        if (window) {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            return {ToPixelCoordsX(width), ToPixelCoordsY(height)};
        } else {
            libqb_log_error("Window not created, cannot get size");
        }

        return {0, 0};
    }

    void WindowMove(int x, int y) const {
        if (window) {
            glfwSetWindowPos(window, ToScreenCoordsX(x), ToScreenCoordsY(y));

            libqb_log_trace("Window moved to (%d, %d)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move");
        }
    }

    std::pair<int, int> WindowGetPosition() const {
        if (window) {
            int x, y;
            glfwGetWindowPos(window, &x, &y);
            return {ToPixelCoordsX(x), ToPixelCoordsY(y)};
        } else {
            libqb_log_error("Window not created, cannot get position");
        }

        return {0, 0};
    }

    void WindowSetAspectRatio(int width, int height) const {
        if (window) {
            width = (width < 0 ? GLFW_DONT_CARE : width);
            height = (height < 0 ? GLFW_DONT_CARE : height);
            glfwSetWindowAspectRatio(window, width, height);

            libqb_log_trace("Window aspect ratio set to %d:%d", width, height);
        } else {
            libqb_log_error("Window not created, cannot set aspect ratio");
        }
    }

    void WindowSetSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight) const {
        if (window) {
            // Convert to screen coordinates
            minWidth = (minWidth < 0 ? GLFW_DONT_CARE : ToScreenCoordsX(minWidth));
            minHeight = (minHeight < 0 ? GLFW_DONT_CARE : ToScreenCoordsY(minHeight));
            maxWidth = (maxWidth < 0 ? GLFW_DONT_CARE : ToScreenCoordsX(maxWidth));
            maxHeight = (maxHeight < 0 ? GLFW_DONT_CARE : ToScreenCoordsY(maxHeight));

            glfwSetWindowSizeLimits(window, minWidth, minHeight, maxWidth, maxHeight);

            libqb_log_trace("Window size limits set to (%d, %d) to (%d, %d)", minWidth, minHeight, maxWidth, maxHeight);
        } else {
            libqb_log_error("Window not created, cannot set minimum size");
        }
    }

    void WindowSetShouldClose(bool shouldClose) const {
        if (window) {
            glfwSetWindowShouldClose(window, shouldClose);

            if (shouldClose) {
                libqb_log_trace("Window set to close");
            } else {
                libqb_log_trace("Window set to not close");
            }
        } else {
            libqb_log_error("Window not created, cannot set should close");
        }
    }

    void WindowSwapBuffers() const {
        if (window) {
            glfwSwapBuffers(window);
        } else {
            libqb_log_error("Window not created, cannot swap buffers");
        }
    }

    void WindowRefresh() {
        windowShouldRefresh = true;
    }

    const void *WindowGetNativeHandle() const {
        if (window) {
#if defined(QB64_WINDOWS)
            return reinterpret_cast<const void *>(glfwGetWin32Window(window));
#elif defined(QB64_MACOSX)
            return reinterpret_cast<const void *>(glfwGetCocoaWindow(window));
#elif defined(QB64_LINUX)
            return reinterpret_cast<const void *>(glfwGetX11Window(window));
#else
#    error "Unsupported platform for window handle retrieval"
#endif
        }

        else {
            libqb_log_error("Window not created, cannot get handle");
        }

        return nullptr;
    }

    void WindowSetCloseFunction(GLUTEmu_CallbackWindowClose function) {
        if (window) {
            windowCloseFunction = function;

            glfwSetWindowCloseCallback(window, [](GLFWwindow *win) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->windowCloseFunction) {
                    instance->windowCloseFunction();
                }
            });

            libqb_log_trace("Window close function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set close function");
        }
    }

    void WindowSetResizedFunction(GLUTEmu_CallbackWindowResized function) {
        if (window) {
            windowResizedFunction = function;

            glfwSetWindowSizeCallback(window, [](GLFWwindow *win, int width, int height) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->windowResizedFunction) {
                    instance->windowResizedFunction(instance->ToPixelCoordsX(width), instance->ToPixelCoordsY(height));
                }
            });

            libqb_log_trace("Window resize function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set resize function");
        }
    }

    void WindowSetMaximizedFunction(GLUTEmu_CallbackWindowMaximized function) {
        if (window) {
            windowMaximizedFunction = function;

            glfwSetWindowMaximizeCallback(window, [](GLFWwindow *win, int maximized) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->windowMaximizedFunction) {
                    instance->windowMaximizedFunction(bool(maximized));
                }
            });

            libqb_log_trace("Window maximized function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set maximized function");
        }
    }

    void WindowSetRefreshFunction(GLUTEmu_CallbackWindowRefresh function) {
        if (window) {
            windowRefreshFunction = function;

            glfwSetWindowRefreshCallback(window, [](GLFWwindow *win) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                instance->windowShouldRefresh = true;
                /*
                if (instance->windowRefreshFunction) {
                    instance->windowRefreshFunction();
                }
                */
            });

            libqb_log_trace("Display function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set refresh function");
        }
    }

    void WindowSetIdleFunction(GLUTEmu_CallbackWindowIdle function) {
        if (window) {
            windowIdleFunction = function;

            libqb_log_trace("Idle function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set idle function");
        }
    }

    void KeyboardSetButtonFunction(GLUTEmu_CallbackKeyboardButton function) {
        if (window) {
            keyboardButtonFunction = function;

            // We are already listening for keyboard events

            libqb_log_trace("Keyboard function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set keyboard function");
        }
    }

    bool KeyboardIsKeyModifierSet(GLUTEmu_KeyboardKeyModifier modifier) const {
        if (window) {
            return bool(keyboardModifiers & int(modifier));
        } else {
            libqb_log_error("Window not created, cannot get modifiers");
        }

        return false;
    }

    void KeyboardSetCharFunction(GLUTEmu_CallbackKeyboardChar function) {
        if (window) {
            keyboardCharFunction = function;

            glfwSetCharCallback(window, [](GLFWwindow *win, unsigned int codepoint) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->keyboardCharFunction) {
                    instance->keyboardCharFunction(codepoint);
                }
            });

            libqb_log_trace("Keyboard char function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set keyboard char function");
        }
    }

    void MouseSetStandardCursor(GLUTEmu_MouseStandardCursor style) {
        if (window) {
            if (mouseCursor) {
                glfwDestroyCursor(mouseCursor);
                mouseCursor = nullptr;
            }

            mouseCursor = glfwCreateStandardCursor(int(style));

            if (mouseCursor) {
                glfwSetCursor(window, mouseCursor);

                libqb_log_trace("Mouse cursor set to standard style %d", style);
            } else {
                libqb_log_error("Failed to create standard cursor of style %d", style);
            }
        } else {
            libqb_log_error("Window not created, cannot set mouse cursor");
        }
    }

    // GLFW_TODO: Check if we need a MouseSetCustomCursor() function that allows custom bitmap cursors

    void MouseSetCursorMode(GLUTEnum_MouseCursorMode mode) {
        if (window) {
            glfwSetInputMode(window, GLFW_CURSOR, int(mode));
            mouseCursorMode = GLUTEnum_MouseCursorMode(glfwGetInputMode(window, GLFW_CURSOR));

            libqb_log_trace("Mouse cursor mode set to %d", int(mode));
        } else {
            libqb_log_error("Window not created, cannot set mouse cursor mode");
        }
    }

    GLUTEnum_MouseCursorMode MouseGetCursorMode() const {
        if (window) {
            return mouseCursorMode;
        } else {
            libqb_log_error("Window not created, cannot get mouse cursor mode");
        }

        return GLUTEnum_MouseCursorMode::Normal;
    }

    void MouseMove(double x, double y) const {
        if (window) {
            if (mouseCursorMode == GLUTEnum_MouseCursorMode::Disabled) {
                glfwSetCursorPos(window, x, y);
            } else {
                glfwSetCursorPos(window, ToScreenCoordsX(x), ToScreenCoordsY(y));
            }

            libqb_log_trace("Mouse moved to (%f, %f)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move mouse");
        }
    }

    std::pair<double, double> MouseGetPosition() const {
        if (window) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            if (mouseCursorMode == GLUTEnum_MouseCursorMode::Disabled) {
                return {x, y};
            } else {
                return {ToPixelCoordsX(x), ToPixelCoordsY(y)};
            }
        } else {
            libqb_log_error("Window not created, cannot get mouse position");
        }

        return {0.0, 0.0};
    }

    void MouseSetMotionFunction(GLUTEmu_CallbackMouseMotion function) {
        if (window) {
            mouseMotionFunction = function;

            glfwSetCursorPosCallback(window, [](GLFWwindow *win, double xPos, double yPos) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->mouseMotionFunction) {
                    if (instance->mouseCursorMode == GLUTEnum_MouseCursorMode::Disabled) {
                        instance->mouseMotionFunction(xPos, yPos, true);
                    } else {
                        instance->mouseMotionFunction(instance->ToPixelCoordsX(xPos), instance->ToPixelCoordsY(yPos), false);
                    }
                }
            });

            libqb_log_trace("Mouse motion function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse motion function");
        }
    }

    void MouseSetButtonFunction(GLUTEmu_CallbackMouseButton function) {
        if (window) {
            mouseButtonFunction = function;

            // We are already listening for mouse events

            libqb_log_trace("Mouse button function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse button function");
        }
    }

    void MouseSetScrollFunction(GLUTEmu_CallbackMouseScroll function) {
        if (window) {
            mouseScrollFunction = function;

            glfwSetScrollCallback(window, [](GLFWwindow *win, double xOffset, double yOffset) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->mouseScrollFunction) {
                    instance->mouseScrollFunction(xOffset, yOffset);
                }
            });

            libqb_log_trace("Mouse scroll function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse scroll function");
        }
    }

    std::pair<int, int> ScreenGetSize() const {
        if (window) {
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);
            return {ToPixelCoordsX(mode->width), ToPixelCoordsY(mode->height)};
        } else {
            libqb_log_error("Window not created, cannot get screen size");
        }

        return {0, 0};
    }

    // GLFW_TODO: Implement drag and drop functionality.
    // Idea: use std::queue<std::string> to store file paths (enforcing a limit - INT32_MAX?) and process them in the main loop.

    void MainLoop() {
        libqb_log_trace("Entering main loop");

        while (!glfwWindowShouldClose(window)) {
            if (windowShouldRefresh) {
                windowShouldRefresh = false;

                if (windowRefreshFunction) {
                    windowRefreshFunction();
                }
            }

            if (windowIdleFunction) {
                glfwPollEvents();

                windowIdleFunction();
            } else {
                glfwWaitEvents();
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
        : monitor(nullptr), window(nullptr), windowShouldRefresh(true), isWindowFullscreen(false), windowX(0), windowY(0), windowWidth(0), windowHeight(0),
          windowClientWidth(0), windowClientHeight(0), windowXScale(1.0f), windowYScale(1.0f), mouseCursor(nullptr),
          mouseCursorMode(GLUTEnum_MouseCursorMode::Normal), keyboardModifiers(0), windowCloseFunction(nullptr), windowResizedFunction(nullptr),
          windowMaximizedFunction(nullptr), windowRefreshFunction(nullptr), windowIdleFunction(nullptr), keyboardButtonFunction(nullptr),
          keyboardCharFunction(nullptr), mouseButtonFunction(nullptr), mouseMotionFunction(nullptr), mouseScrollFunction(nullptr) {

#if defined(QB64_MACOSX) || defined(QB64_LINUX)
        bool keyboardScollLockState = false;
#endif

        glfwSetErrorCallback([](int error_code, const char *description) { libqb_log_error("GLFW Error %d: %s", error_code, description); });

        isInitialized = glfwInit();
        if (isInitialized) {
            libqb_log_trace("GLFW (%s) initialized", glfwGetVersionString());
        } else {
            libqb_log_error("Failed to initialize GLFW");
        }
    }

    ~GLUTEmu() {
        if (mouseCursor) {
            glfwDestroyCursor(mouseCursor);
            mouseCursor = nullptr;
            libqb_log_trace("Mouse cursor destroyed");
        }

        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
            libqb_log_trace("Window closed");
        }

        if (isInitialized) {
            glfwTerminate();
            isInitialized = false;
            libqb_log_trace("GLFW terminated");
        }
    }

    GLUTEmu(const GLUTEmu &) = delete;
    GLUTEmu &operator=(const GLUTEmu &) = delete;
    GLUTEmu(GLUTEmu &&) = delete;
    GLUTEmu &operator=(GLUTEmu &&) = delete;

    template <typename T> T ToPixelCoordsX(T x) const {
        return static_cast<T>(x * windowXScale);
    }

    template <typename T> T ToPixelCoordsY(T y) const {
        return static_cast<T>(y * windowYScale);
    }

    template <typename T> T ToScreenCoordsX(T x) const {
        return static_cast<T>(x / windowXScale);
    }

    template <typename T> T ToScreenCoordsY(T y) const {
        return static_cast<T>(y / windowYScale);
    }

    GLFWmonitor *WindowGetCurrentMonitor() const {
        if (window) {
            int wx, wy, ww, wh;
            glfwGetWindowPos(window, &wx, &wy);
            glfwGetWindowSize(window, &ww, &wh);

            int nmonitors;
            GLFWmonitor **monitors = glfwGetMonitors(&nmonitors);
            GLFWmonitor *bestMonitor = nullptr;
            int bestOverlap = 0;

            for (int i = 0; i < nmonitors; i++) {
                int mx, my;
                glfwGetMonitorPos(monitors[i], &mx, &my);
                const GLFWvidmode *mode = glfwGetVideoMode(monitors[i]);
                int mw = mode->width;
                int mh = mode->height;

                int overlap = (wx + ww > mx && wx < mx + mw ? (wx + ww < mx + mw ? wx + ww : mx + mw) - (wx > mx ? wx : mx) : 0) *
                              (wy + wh > my && wy < my + mh ? (wy + wh < my + mh ? wy + wh : my + mh) - (wy > my ? wy : my) : 0);

                if (overlap > bestOverlap) {
                    bestOverlap = overlap;
                    bestMonitor = monitors[i];
                }
            }

            return bestMonitor ? bestMonitor : glfwGetPrimaryMonitor();
        } else {
            libqb_log_error("Window not created, cannot get current monitor");
        }

        return nullptr;
    }

    int KeyboardUpdateLockKeyModifier(int key, int mods) {
#if defined(QB64_WINDOWS)
        switch (key) {
        case GLFW_KEY_SCROLL_LOCK:
            mods = (GetKeyState(VK_SCROLL) & 0x0001) ? (mods | int(GLUTEmu_KeyboardKeyModifier::ScrollLock))
                                                     : (mods & ~int(GLUTEmu_KeyboardKeyModifier::ScrollLock));
            break;

        case GLFW_KEY_CAPS_LOCK:
            mods =
                (GetKeyState(VK_CAPITAL) & 0x0001) ? (mods | int(GLUTEmu_KeyboardKeyModifier::CapsLock)) : (mods & ~int(GLUTEmu_KeyboardKeyModifier::CapsLock));
            break;

        case GLFW_KEY_NUM_LOCK:
            mods =
                (GetKeyState(VK_NUMLOCK) & 0x0001) ? (mods | int(GLUTEmu_KeyboardKeyModifier::NumLock)) : (mods & ~int(GLUTEmu_KeyboardKeyModifier::NumLock));
            break;
        }
#elif defined(QB64_LINUX)
        unsigned int n = 0;
        if (XkbGetIndicatorState(glfwGetX11Display(), XkbUseCoreKbd, &n) == Success) {
            switch (key) {
            case GLFW_KEY_SCROLL_LOCK:
                mods = (n & 0x04) ? (mods | int(GLUTEmu_KeyboardKeyModifier::ScrollLock)) : (mods & ~int(GLUTEmu_KeyboardKeyModifier::ScrollLock));
                break;

            case GLFW_KEY_CAPS_LOCK:
                mods = (n & 0x01) ? (mods | int(GLUTEmu_KeyboardKeyModifier::CapsLock)) : (mods & ~int(GLUTEmu_KeyboardKeyModifier::CapsLock));
                break;

            case GLFW_KEY_NUM_LOCK:
                mods = (n & 0x02) ? (mods | int(GLUTEmu_KeyboardKeyModifier::NumLock)) : (mods & ~int(LUTEmu_KeyboardKeyModifier::NumLock));
                break;
            }
        } else // note the else here
#elif defined(QB64_MACOSX) || defined(QB64_LINUX)
        {
            // No indicator API, toggle manually
            if (key == GLFW_KEY_SCROLL_LOCK) {
                // We need this for scroll lock only since GLFW supports caps lock and num lock natively
                mods = keyboardScollLockState ? (mods | int(GLUTEmu_KeyboardKeyModifier::ScrollLock)) : (mods & ~int(GLUTEmu_KeyboardKeyModifier::ScrollLock));
            } else {
                libqb_log_warn("Unsupported platform for keyboard lock key modifier retrieval");
            }
        }
#endif

        return mods;
    }

    bool isInitialized;                        // whether GLFW is initialized
    GLFWmonitor *monitor;                      // current monitor
    GLFWwindow *window;                        // GLFW_TODO: since GLFW allows multiple windows, check if we can support that in the future
    bool windowShouldRefresh;                  // whether the window contents should be refreshed
    bool isWindowFullscreen;                   // whether the window is in fullscreen mode
    int windowX, windowY;                      // window position in screen coordinates
    int windowWidth, windowHeight;             // window size in screen coordinates
    int windowClientWidth, windowClientHeight; // client area size in pixels
    float windowXScale, windowYScale;          // window scaling factors
    GLFWcursor *mouseCursor;                   // current mouse cursor
    GLUTEnum_MouseCursorMode mouseCursorMode;  // current mouse cursor mode (normal, hidden, disabled, etc.)
    int keyboardModifiers;                     // current keyboard modifiers
#if defined(QB64_MACOSX) || defined(QB64_LINUX)
    bool keyboardScollLockState; // Scroll Lock state for macOS and Linux
#endif
    GLUTEmu_CallbackWindowClose windowCloseFunction;
    GLUTEmu_CallbackWindowResized windowResizedFunction;
    GLUTEmu_CallbackWindowMaximized windowMaximizedFunction;
    GLUTEmu_CallbackWindowRefresh windowRefreshFunction;
    GLUTEmu_CallbackWindowIdle windowIdleFunction;
    GLUTEmu_CallbackKeyboardButton keyboardButtonFunction;
    GLUTEmu_CallbackKeyboardChar keyboardCharFunction;
    GLUTEmu_CallbackMouseButton mouseButtonFunction;
    GLUTEmu_CallbackMouseMotion mouseMotionFunction;
    GLUTEmu_CallbackMouseScroll mouseScrollFunction;
};

template <typename T> void GLUTEmu_WindowSetHint(GLUTEmu_WindowHint hint, const T value) {
    GLUTEmu::Instance().WindowSetHint(hint, value);
}

// We only need to instantiate the template for the types we need
template void GLUTEmu_WindowSetHint<bool>(GLUTEmu_WindowHint, bool);
template void GLUTEmu_WindowSetHint<int>(GLUTEmu_WindowHint, int);
template void GLUTEmu_WindowSetHint<char *>(GLUTEmu_WindowHint, char *);
template void GLUTEmu_WindowSetHint<const char *>(GLUTEmu_WindowHint, const char *);

bool GLUTEmu_WindowInitialize(int width, int height, const char *title) {
    return GLUTEmu::Instance().WindowInitialize(width, height, title);
}

bool GLUTEmu_WindowIsCreated() {
    return GLUTEmu::Instance().WindowIsCreated();
}

void GLUTEmu_WindowSetTitle(const char *title) {
    GLUTEmu::Instance().WindowSetTitle(title);
}

const char *GLUTEmu_WindowGetTitle() {
    return GLUTEmu::Instance().WindowGetTitle();
}

void GLUTEmu_WindowFullScreen(bool fullscreen) {
    GLUTEmu::Instance().WindowFullscreen(fullscreen);
}

bool GLUTEmu_WindowIsFullscreen() {
    return GLUTEmu::Instance().WindowIsFullscreen();
}

void GLUTEmu_WindowMaximize() {
    GLUTEmu::Instance().WindowMaximize();
}

bool GLUTEmu_WindowIsMaximized() {
    return GLUTEmu::Instance().WindowIsMaximized();
}

void GLUTEmu_WindowMinimize() {
    GLUTEmu::Instance().WindowMinimize();
}

bool GLUTEmu_WindowIsMinimized() {
    return GLUTEmu::Instance().WindowIsMinimized();
}

void GLUTEmu_WindowRestore() {
    GLUTEmu::Instance().WindowRestore();
}

void GLUTEmu_ShowWindow() {
    GLUTEmu::Instance().WindowShow();
}

void GLUTEmu_HideWindow() {
    GLUTEmu::Instance().WindowHide();
}

bool GLUTEmu_WindowIsVisible() {
    return GLUTEmu::Instance().WindowIsVisible();
}

void GLUTEmu_WindowFocus() {
    GLUTEmu::Instance().WindowFocus();
}

bool GLUTEmu_WindowHasFocus() {
    return GLUTEmu::Instance().WindowHasFocus();
}

void GLUTEmu_WindowResize(int width, int height) {
    GLUTEmu::Instance().WindowResize(width, height);
}

std::pair<int, int> GLUTEmu_WindowGetSize() {
    return GLUTEmu::Instance().WindowGetSize();
}

void GLUTEmu_WindowMove(int x, int y) {
    GLUTEmu::Instance().WindowMove(x, y);
}

std::pair<int, int> GLUTEmu_WindowGetPosition() {
    return GLUTEmu::Instance().WindowGetPosition();
}

void GLUTEmu_WindowSetAspectRatio(int width, int height) {
    GLUTEmu::Instance().WindowSetAspectRatio(width, height);
}

void GLUTEmu_WindowSetSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight) {
    GLUTEmu::Instance().WindowSetSizeLimits(minWidth, minHeight, maxWidth, maxHeight);
}

void GLUTEmu_WindowSetShouldClose(bool shouldClose) {
    GLUTEmu::Instance().WindowSetShouldClose(shouldClose);
}

void GLUTEmu_WindowSwapBuffers() {
    GLUTEmu::Instance().WindowSwapBuffers();
}

void GLUTEmu_WindowRefresh() {
    GLUTEmu::Instance().WindowRefresh();
}

const void *GLUTEmu_WindowGetNativeHandle() {
    return GLUTEmu::Instance().WindowGetNativeHandle();
}

void GLUTEmu_WindowSetCloseFunction(GLUTEmu_CallbackWindowClose func) {
    GLUTEmu::Instance().WindowSetCloseFunction(func);
}

void GLUTEmu_WindowSetResizedFunction(GLUTEmu_CallbackWindowResized func) {
    GLUTEmu::Instance().WindowSetResizedFunction(func);
}

void GLUTEmu_WindowSetMaximizedFunction(GLUTEmu_CallbackWindowMaximized func) {
    GLUTEmu::Instance().WindowSetMaximizedFunction(func);
}

void GLUTEmu_WindowSetRefreshFunction(GLUTEmu_CallbackWindowRefresh func) {
    GLUTEmu::Instance().WindowSetRefreshFunction(func);
}

void GLUTEmu_WindowSetIdleFunction(GLUTEmu_CallbackWindowIdle func) {
    GLUTEmu::Instance().WindowSetIdleFunction(func);
}

void GLUTEmu_KeyboardSetButtonFunction(GLUTEmu_CallbackKeyboardButton func) {
    GLUTEmu::Instance().KeyboardSetButtonFunction(func);
}

bool GLUTEmu_KeyboardIsKeyModifierSet(GLUTEmu_KeyboardKeyModifier modifier) {
    return GLUTEmu::Instance().KeyboardIsKeyModifierSet(modifier);
}

void GLUTEmu_KeyboardSetCharFunction(GLUTEmu_CallbackKeyboardChar func) {
    GLUTEmu::Instance().KeyboardSetCharFunction(func);
}

void GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor style) {
    GLUTEmu::Instance().MouseSetStandardCursor(style);
}

void GLUTEmu_MouseSetCursorMode(GLUTEnum_MouseCursorMode mode) {
    GLUTEmu::Instance().MouseSetCursorMode(mode);
}

GLUTEnum_MouseCursorMode GLUTEmu_MouseGetCursorMode() {
    return GLUTEmu::Instance().MouseGetCursorMode();
}

void GLUTEmu_MouseMove(double x, double y) {
    GLUTEmu::Instance().MouseMove(x, y);
}

std::pair<double, double> GLUTEmu_MouseGetPosition() {
    return GLUTEmu::Instance().MouseGetPosition();
}

void GLUTEmu_MouseSetMotionFunction(GLUTEmu_CallbackMouseMotion func) {
    GLUTEmu::Instance().MouseSetMotionFunction(func);
}

void GLUTEmu_MouseSetButtonFunction(GLUTEmu_CallbackMouseButton func) {
    GLUTEmu::Instance().MouseSetButtonFunction(func);
}

void GLUTEmu_MouseSetScrollFunction(GLUTEmu_CallbackMouseScroll func) {
    GLUTEmu::Instance().MouseSetScrollFunction(func);
}

std::pair<int, int> GLUTEmu_ScreenGetSize() {
    return GLUTEmu::Instance().ScreenGetSize();
}

void GLUTEmu_MainLoop() {
    GLUTEmu::Instance().MainLoop();
}
