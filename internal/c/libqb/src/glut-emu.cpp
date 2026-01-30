//--------------------------------------------------------------------------------------------------------
// QB64-PE GLUT-like emulation layer
// This abstracts the underlying windowing library and provides a GLUT-like API for QB64-PE
//
// TODO:
// Custom bitmap cursor support
// Custom window icon support
// Desktop image capture support
// Mouse capture/release support
//--------------------------------------------------------------------------------------------------------

#include "glut-emu.h"
#include "libqb-common.h"
#include "logging.h"

class GLUTEmu {
  public:
    std::pair<float, float> MonitorGetScaleFactor() const {
        float scaleX, scaleY;

        if (window && RGFW_monitor_getScale(RGFW_window_getMonitor(window), &scaleX, &scaleY)) {
            return {scaleX, scaleY};
        } else if (RGFW_monitor_getScale(RGFW_getPrimaryMonitor(), &scaleX, &scaleY)) {
            libqb_log_warn("Failed to get monitor scale factor, using primary monitor instead");
            return {scaleX, scaleY};
        } else {
            libqb_log_error("Failed to get monitor scale factor");
        }

        return {1.0f, 1.0f};
    }

    std::pair<uint32_t, uint32_t> MonitorGetSize() const {
        RGFW_monitorMode mode;

        if (!window || !RGFW_monitor_getMode(RGFW_window_getMonitor(window), &mode)) {
            libqb_log_warn("Window not created, getting primary screen size");
            if (!RGFW_monitor_getMode(RGFW_getPrimaryMonitor(), &mode)) {
                libqb_log_error("Failed to get primary monitor mode");
                return {0, 0};
            }
        }

        return {mode.w, mode.h};
    }

    void MonitorSetChangedFunction(GLUTEmu_CallbackMonitorChanged func) {
        if (window) {
            monitorChangedFunction = func;

            RGFW_setMonitorCallback([](RGFW_window *win, const RGFW_monitor *monitor, RGFW_bool connected) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->monitorChangedFunction) {
                    instance->monitorChangedFunction(monitor->name, connected);
                }
            });
        } else {
            libqb_log_error("Window not created, cannot set monitor changed function");
        }
    }

    bool WindowCreate(const char *title, uint32_t width, uint32_t height, GLUTEmu_WindowFlag flags) {
        if (window) {
            // GLFW_TODO: sure we can; maybe we'll use it in a future version of QB64-PE
            libqb_log_error("Window already created, cannot create another window");
        } else {
            window = RGFW_createWindow(title, 0, 0, width, height, flags);
            if (window) {
                RGFW_window_setUserPtr(window, this);
                RGFW_window_setExitKey(window, RGFW_keyNULL); // disable the default exit key behavior
                RGFW_window_makeCurrentWindow_OpenGL(window);
                RGFW_window_makeCurrentContext_OpenGL(window);
                RGFW_window_swapInterval_OpenGL(window, 1); // enable vsync

                // Set a hook into the keyboard button callback to keep track of the modifier keys
                RGFW_setKeyCallback([](RGFW_window *win, u8 key, RGFW_keymod mod, RGFW_bool repeat, RGFW_bool pressed) {
                    auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                    instance->keyboardModifiers = mod;
                    if (instance->keyboardButtonFunction) {
                        instance->keyboardButtonFunction(GLUTEmu_KeyboardKey(key), GLUTEmu_KeyboardKeyModifier(mod), pressed, repeat);
                    }
                });

                libqb_log_trace("Window created (%u x %u)", width, height);

                return true;
            } else {
                libqb_log_error("Failed to create window");
            }
        }

        return false;
    }

    bool WindowIsCreated() const {
        return window != nullptr;
    }

    void WindowSetTitle(const char *title) const {
        if (window) {
            RGFW_window_setName(window, title);
        } else {
            libqb_log_error("Window not created, cannot set title");
        }
    }

    GLUTEmu_WindowFlag WindowGetFlags() const {
        if (window) {
            return static_cast<GLUTEmu_WindowFlag>(RGFW_window_getFlags(window));
        } else {
            libqb_log_error("Window not created, cannot get flags");
        }

        return GLUTEmu_WindowFlag::None;
    }

    void WindowSetFlags(GLUTEmu_WindowFlag flags) const {
        if (window) {
            RGFW_window_setFlags(window, flags);
        } else {
            libqb_log_error("Window not created, cannot set flags");
        }
    }

    void WindowSetIcon(int32_t imageHandle) const {
        if (window) {
            // RGFW_TODO: implement icon support
            libqb_log_warn("WindowSetIcon is not implemented");
        } else {
            libqb_log_error("Window not created, cannot set icon");
        }
    }

    void WindowFullscreen(bool fullscreen) {
        if (window) {
            auto isFullscreen = RGFW_window_isFullscreen(window);

            if (fullscreen && !isFullscreen) {
                libqb_log_trace("Entering fullscreen mode");

                RGFW_window_setFullscreen(window, true);

                windowShouldRefresh = true;
            } else if (!fullscreen && isFullscreen) {
                libqb_log_trace("Exiting fullscreen mode");

                RGFW_window_setFullscreen(window, false);

                windowShouldRefresh = true;
            } else {
                libqb_log_trace("Fullscreen = %d, ignoring request", fullscreen);
            }
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

            windowShouldRefresh = true;

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

            windowShouldRefresh = false;

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

            windowShouldRefresh = true;

            libqb_log_trace("Window restored");
        } else {
            libqb_log_error("Window not created, cannot restore");
        }
    }

    void WindowShow() {
        if (window) {
            RGFW_window_show(window);

            windowShouldRefresh = true;

            libqb_log_trace("Window focused");
        } else {
            libqb_log_error("Window not created, cannot set focus");
        }
    }

    void WindowHide(bool hide) {
        if (window) {
            if (hide) {
                RGFW_window_hide(window);
                windowShouldRefresh = false;
            } else {
                RGFW_window_show(window);
                windowShouldRefresh = true;
            }

            libqb_log_trace("Window %s", hide ? "hidden" : "shown");
        } else {
            libqb_log_error("Window not created, cannot hide");
        }
    }

    bool WindowIsHidden() const {
        if (window) {
            return RGFW_window_isHidden(window);
        } else {
            libqb_log_error("Window not created, cannot check visibility");
        }

        return false;
    }

    void WindowFocus() {
        if (window) {
            RGFW_window_focus(window);

            windowShouldRefresh = true;

            libqb_log_trace("Window focused");
        } else {
            libqb_log_error("Window not created, cannot focus");
        }
    }

    bool WindowHasFocus() const {
        if (window) {
            return RGFW_window_isInFocus(window);
        } else {
            libqb_log_error("Window not created, cannot check focus");
        }

        return false;
    }

    void WindowRaise() {
        if (window) {
            RGFW_window_raise(window);

            windowShouldRefresh = true;

            libqb_log_trace("Window raised");
        } else {
            libqb_log_error("Window not created, cannot raise");
        }
    }

    void WindowSetFloating(bool floating) {
        if (window) {
            RGFW_window_setFloating(window, floating);

            windowShouldRefresh = true;

            libqb_log_trace("Window floating state set to %s", floating ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set floating state");
        }
    }

    bool WindowIsFloating() const {
        if (window) {
            return RGFW_window_isFloating(window);
        } else {
            libqb_log_error("Window not created, cannot check floating state");
        }

        return false;
    }

    void WindowSetOpacity(float opacity) {
        if (window) {
            RGFW_window_setOpacity(window, opacity);

            windowShouldRefresh = true;

            libqb_log_trace("Window opacity set to %f", opacity);
        } else {
            libqb_log_error("Window not created, cannot set opacity");
        }
    }

    void WindowSetBordered(bool bordered) {
        if (window) {
            RGFW_window_setBorder(window, bordered);

            windowShouldRefresh = true;

            libqb_log_trace("Window border state set to %s", bordered ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set border state");
        }
    }

    bool WindowHasBorder() const {
        if (window) {
            return !RGFW_window_borderless(window);
        } else {
            libqb_log_error("Window not created, cannot check border state");
        }

        return false;
    }

    void WindowSetFlashMode(GLUTEmu_WindowFlashMode mode) const {
        if (window) {
            RGFW_window_flash(window, RGFW_flashRequest(mode));

            libqb_log_trace("Window flash mode set to %d", int(mode));
        } else {
            libqb_log_error("Window not created, cannot set flash mode");
        }
    }

    void WindowSetDragAndDrop(bool allow) const {
        if (window) {
            RGFW_window_setDND(window, allow);

            libqb_log_trace("Window drag and drop set to %s", allow ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set drag and drop");
        }
    }

    bool WindowAllowsDragAndDrop() const {
        if (window) {
            return RGFW_window_allowsDND(window);
        } else {
            libqb_log_error("Window not created, cannot check drag and drop");
        }

        return false;
    }

    void WindowSetMousePassthrough(bool passthrough) const {
        if (window) {
            RGFW_window_setMousePassthrough(window, passthrough);

            libqb_log_trace("Window mouse passthrough set to %s", passthrough ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set mouse passthrough");
        }
    }

    void WindowResize(uint32_t width, uint32_t height) {
        if (window) {
            RGFW_window_resize(window, width, height);

            windowShouldRefresh = true;

            libqb_log_trace("Window resized (%u x %u)", width, height);
        } else {
            libqb_log_error("Window not created, cannot resize");
        }
    }

    std::pair<uint32_t, uint32_t> WindowGetSize() const {
        int32_t w, h;

        if (window && RGFW_window_getSize(window, &w, &h)) {
            return {w, h};
        } else {
            libqb_log_error("Window not created, cannot get size");
        }

        return {0, 0};
    }

    void WindowMove(int32_t x, int32_t y) {
        if (window) {
            RGFW_window_move(window, x, y);

            windowShouldRefresh = true;

            libqb_log_trace("Window moved to (%d, %d)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move");
        }
    }

    std::pair<int32_t, int32_t> WindowGetPosition() const {
        int32_t x, y;

        if (window && RGFW_window_getPosition(window, &x, &y)) {
            return {x, y};
        } else {
            libqb_log_error("Window not created, cannot get position");
        }

        return {0, 0};
    }

    void WindowCenter() {
        if (window) {
            RGFW_window_center(window);

            windowShouldRefresh = true;

            libqb_log_trace("Window centered");
        } else {
            libqb_log_error("Window not created, cannot center");
        }
    }

    void WindowSetAspectRatio(uint32_t width, uint32_t height) {
        if (window) {
            RGFW_window_setAspectRatio(window, width, height);

            windowShouldRefresh = true;

            libqb_log_trace("Window aspect ratio set to %u:%u", width, height);
        } else {
            libqb_log_error("Window not created, cannot set aspect ratio");
        }
    }

    void WindowSetMinSize(uint32_t width, uint32_t height) const {
        if (window) {
            RGFW_window_setMinSize(window, width, height);

            libqb_log_trace("Window minimum size set to (%u x %u)", width, height);
        } else {
            libqb_log_error("Window not created, cannot set minimum size");
        }
    }

    void WindowSetMaxSize(uint32_t width, uint32_t height) const {
        if (window) {
            RGFW_window_setMaxSize(window, width, height);

            libqb_log_trace("Window maximum size set to (%u x %u)", width, height);
        } else {
            libqb_log_error("Window not created, cannot set maximum size");
        }
    }

    void WindowScaleToMonitor() {
        if (window) {
            RGFW_window_scaleToMonitor(window);

            windowShouldRefresh = true;

            libqb_log_trace("Window scaled to monitor");
        } else {
            libqb_log_error("Window not created, cannot scale to monitor");
        }
    }

    void WindowSetShouldClose(bool shouldClose) const {
        if (window) {
            RGFW_window_setShouldClose(window, shouldClose);

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
            RGFW_window_swapBuffers_OpenGL(window);
        } else {
            libqb_log_error("Window not created, cannot swap buffers");
        }
    }

    void WindowRefresh() {
        windowShouldRefresh = true;
    }

    const void *WindowGetNativeHandle(int32_t type) const {
        // RGFW_TODO: Expand to return more platform-specific handles as needed (WindowGetNativeHandle(int type))
        if (window) {
            switch (type) {
            case 1:
#if defined(QB64_WINDOWS)
                return reinterpret_cast<const void *>(RGFW_window_getHDC(window));
#elif defined(QB64_MACOSX)
                return reinterpret_cast<const void *>(RGFW_window_getView_OSX(window));
#else
                return reinterpret_cast<const void *>(RGFW_getDisplay_X11());
#endif
                break;

            default:
#if defined(QB64_WINDOWS)
                return reinterpret_cast<const void *>(RGFW_window_getHWND(window));
#elif defined(QB64_MACOSX)
                return reinterpret_cast<const void *>(RGFW_window_getWindow_OSX(window));
#else
                return reinterpret_cast<const void *>(RGFW_window_getWindow_X11(window));
#endif
            }
        } else {
            libqb_log_error("Window not created, cannot get handle");
        }

        return nullptr;
    }

    void WindowSetCloseFunction(GLUTEmu_CallbackWindowClose function) {
        if (window) {
            windowCloseFunction = function;

            RGFW_setWindowQuitCallback([](RGFW_window *win) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->windowCloseFunction) {
                    instance->windowCloseFunction();
                }
            });

            libqb_log_trace("Window close function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set close function");
        }
    }

    void WindowSetMovedFunction(GLUTEmu_CallbackWindowMoved function) {
        if (window) {
            windowMovedFunction = function;

            RGFW_setWindowMovedCallback([](RGFW_window *win, i32 x, i32 y) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->windowMovedFunction) {
                    instance->windowMovedFunction(x, y);
                }
            });

            libqb_log_trace("Window move function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set move function");
        }
    }

    void WindowSetResizedFunction(GLUTEmu_CallbackWindowResized function) {
        if (window) {
            windowResizedFunction = function;

            RGFW_setWindowResizedCallback([](RGFW_window *win, i32 w, i32 h) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->windowResizedFunction) {
                    instance->windowResizedFunction(w, h);
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

            RGFW_setWindowMaximizedCallback([](RGFW_window *win, i32 x, i32 y, i32 w, i32 h) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->windowMaximizedFunction) {
                    instance->windowMaximizedFunction(x, y, w, h);
                }
            });

            libqb_log_trace("Window maximized function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set maximized function");
        }
    }

    void WindowSetMinimizedFunction(GLUTEmu_CallbackWindowMinimized function) {
        if (window) {
            windowMinimizedFunction = function;

            RGFW_setWindowMinimizedCallback([](RGFW_window *win) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->windowMinimizedFunction) {
                    instance->windowMinimizedFunction();
                }
            });

            libqb_log_trace("Window minimized function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set minimized function");
        }
    }

    void WindowSetRestoredFunction(GLUTEmu_CallbackWindowRestored function) {
        if (window) {
            windowRestoredFunction = function;

            RGFW_setWindowRestoredCallback([](RGFW_window *win, i32 x, i32 y, i32 w, i32 h) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->windowRestoredFunction) {
                    instance->windowRestoredFunction(x, y, w, h);
                }
            });

            libqb_log_trace("Window restored function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set restored function");
        }
    }

    void WindowSetFocusedFunction(GLUTEmu_CallbackWindowFocused function) {
        if (window) {
            windowFocusedFunction = function;

            RGFW_setFocusCallback([](RGFW_window *win, RGFW_bool inFocus) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->windowFocusedFunction) {
                    instance->windowFocusedFunction(inFocus);
                }
            });

            libqb_log_trace("Window focused function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set focused function");
        }
    }

    void WindowSetScaledFunction(GLUTEmu_CallbackWindowScaled function) {
        if (window) {
            windowScaledFunction = function;

            RGFW_setScaleUpdatedCallback([](RGFW_window *win, float scaleX, float scaleY) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->windowScaledFunction) {
                    instance->windowScaledFunction(scaleX, scaleY);
                }
            });

            libqb_log_trace("Window scaled function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set scaled function");
        }
    }

    void WindowSetRefreshFunction(GLUTEmu_CallbackWindowRefresh function) {
        if (window) {
            windowRefreshFunction = function;

            RGFW_setWindowRefreshCallback([](RGFW_window *win) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
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

    void KeyboardSetCharacterFunction(GLUTEmu_CallbackKeyboardCharacter function) {
        if (window) {
            keyboardCharacterFunction = function;

            RGFW_setKeyCharCallback([](RGFW_window *win, u32 codepoint) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->keyboardCharacterFunction) {
                    instance->keyboardCharacterFunction(static_cast<char32_t>(codepoint));
                }
            });

            libqb_log_trace("Keyboard char function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set keyboard char function");
        }
    }

    bool KeyboardIsKeyModifierSet(GLUTEmu_KeyboardKeyModifier modifier) const {
        return bool(keyboardModifiers & RGFW_keymod(modifier));
    }

    bool MouseSetStandardCursor(GLUTEmu_MouseStandardCursor style) {
        if (window) {
            if (cursor) {
                RGFW_window_setMouseDefault(window);
                RGFW_freeMouse(cursor);
                cursor = nullptr;
                libqb_log_trace("Custom mouse cursor freed");
            }

            if (RGFW_window_setMouseStandard(window, RGFW_mouseIcons(style))) {
                libqb_log_trace("Mouse cursor set to standard style %d", int(style));
                return true;
            } else {
                libqb_log_error("Failed to set standard cursor of style %d", int(style));
                if (RGFW_window_setMouseDefault(window)) {
                    libqb_log_trace("Mouse cursor set to default");
                } else {
                    libqb_log_error("Failed to set default cursor");
                }
            }
        } else {
            libqb_log_error("Window not created, cannot set mouse cursor");
        }

        return false;
    }

    bool MouseSetCustomCursor(int32_t imageHandle) {
        if (window) {
            // RGFW_TODO: implement custom bitmap cursor support
            libqb_log_warn("MouseSetCustomCursor is not implemented");
            return false;
        } else {
            libqb_log_error("Window not created, cannot set custom mouse cursor");
        }

        return false;
    }

    void MouseHide(bool hide) const {
        if (window) {
            RGFW_window_showMouse(window, !hide);

            libqb_log_trace("Mouse cursor %s", hide ? "hidden" : "shown");
        }
    }

    bool MouseIsHidden() const {
        return window ? RGFW_window_isMouseHidden(window) : false;
    }

    void MouseCapture(bool capture) const {
        if (window) {
            RGFW_window_captureRawMouse(window, capture);

            libqb_log_trace("Mouse capture %s", capture ? "enabled" : "disabled");
        }
    }

    bool MouseIsCaptured() const {
        return window ? RGFW_window_isCaptured(window) && RGFW_window_isRawMouseMode(window) : false;
    }

    void MouseMove(int32_t x, int32_t y) const {
        if (window) {
            RGFW_window_moveMouse(window, x, y);

            libqb_log_trace("Mouse moved to (%d, %d)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move mouse");
        }
    }

    void MouseSetPositionFunction(GLUTEmu_CallbackMousePosition function) {
        if (window) {
            mousePositionFunction = function;

            RGFW_setMousePosCallback([](RGFW_window *win, i32 x, i32 y, float vecX, float vecY) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->mousePositionFunction) {
                    instance->mousePositionFunction(x, y, vecX, vecY, RGFW_window_isCaptured(win) && RGFW_window_isRawMouseMode(win));
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

            RGFW_setMouseButtonCallback([](RGFW_window *win, RGFW_mouseButton button, RGFW_bool pressed) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->mouseButtonFunction) {
                    instance->mouseButtonFunction(GLUTEmu_MouseButton(button), pressed);
                }
            });

            libqb_log_trace("Mouse button function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse button function");
        }
    }

    void MouseSetNotifyFunction(GLUTEmu_CallbackMouseNotify function) {
        if (window) {
            mouseNotifyFunction = function;

            RGFW_setMouseNotifyCallback([](RGFW_window *win, i32 x, i32 y, RGFW_bool status) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->mouseNotifyFunction) {
                    instance->mouseNotifyFunction(x, y, status);
                }
            });

            libqb_log_trace("Mouse notify function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse notify function");
        }
    }

    void MouseSetScrollFunction(GLUTEmu_CallbackMouseScroll function) {
        if (window) {
            mouseScrollFunction = function;

            RGFW_setMouseScrollCallback([](RGFW_window *win, float x, float y) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->mouseScrollFunction) {
                    instance->mouseScrollFunction(x, y);
                }
            });

            libqb_log_trace("Mouse scroll function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse scroll function");
        }
    }

    void DragSetPositionFunction(GLUTEmu_CallbackDragPosition function) {
        if (window) {
            dragPositionFunction = function;

            RGFW_setDataDragCallback([](RGFW_window *win, i32 x, i32 y) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->dragPositionFunction) {
                    instance->dragPositionFunction(x, y);
                }
            });

            libqb_log_trace("Drag position function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set drag position function");
        }
    }

    void DropSetFilesFunction(GLUTEmu_CallbackDropFiles function) {
        if (window) {
            dropFilesFunction = function;

            RGFW_setDataDropCallback([](RGFW_window *win, char **files, size_t count) {
                auto instance = reinterpret_cast<GLUTEmu *>(RGFW_window_getUserPtr(win));
                if (instance->dropFilesFunction) {
                    instance->dropFilesFunction(files, count);
                }
            });

            libqb_log_trace("Drop files function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set drop files function");
        }
    }

    void MainLoop() {
        libqb_log_trace("Entering main loop");

        while (!RGFW_window_shouldClose(window)) {
            if (windowShouldRefresh) {
                windowShouldRefresh = false;
                if (windowRefreshFunction) {
                    windowRefreshFunction();
                }
            }

            if (windowIdleFunction) {
                windowIdleFunction();
            }

            RGFW_pollEvents();
        }

        libqb_log_trace("Exiting main loop");
    }

    static GLUTEmu &Instance() {
        static GLUTEmu instance;
        return instance;
    }

  private:
    GLUTEmu()
        : window(nullptr), windowShouldRefresh(true), keyboardModifiers(0), cursor(nullptr), windowCloseFunction(nullptr), windowMovedFunction(nullptr),
          windowResizedFunction(nullptr), windowMaximizedFunction(nullptr), windowMinimizedFunction(nullptr), windowRestoredFunction(nullptr),
          windowFocusedFunction(nullptr), windowScaledFunction(nullptr), windowRefreshFunction(nullptr), windowIdleFunction(nullptr),
          keyboardButtonFunction(nullptr), keyboardCharacterFunction(nullptr), mousePositionFunction(nullptr), mouseButtonFunction(nullptr),
          mouseNotifyFunction(nullptr), mouseScrollFunction(nullptr), dragPositionFunction(nullptr), dropFilesFunction(nullptr), monitorChangedFunction(nullptr)

    {
        // Listen for RGFW debug messages and route them to libqb logging
        RGFW_setDebugCallback([](RGFW_debugType type, RGFW_errorCode err, const char *msg) {
            switch (type) {
            case RGFW_typeError:
                libqb_log_error("RGFW error %d: %s", int(err), msg);
                break;
            case RGFW_typeWarning:
                libqb_log_warn("RGFW warning %d: %s", int(err), msg);
                break;
            case RGFW_typeInfo:
                libqb_log_trace("RGFW info %d: %s", int(err), msg);
                break;
            default:
                libqb_log_trace("RGFW debug %d: %s", int(err), msg);
            }
        });

        RGFW_init();

        libqb_log_trace("RGFW initialized");
    }

    ~GLUTEmu() {
        MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::Arrow);

        if (window) {
            RGFW_window_close(window);
            window = nullptr;
            libqb_log_trace("Window closed");
        }
    }

    GLUTEmu(const GLUTEmu &) = delete;
    GLUTEmu &operator=(const GLUTEmu &) = delete;
    GLUTEmu(GLUTEmu &&) = delete;
    GLUTEmu &operator=(GLUTEmu &&) = delete;

    RGFW_window *window;           // RGFW_TODO: since RGFW allows multiple windows, check if we can support that in the future
    bool windowShouldRefresh;      // whether the window contents should be refreshed
    RGFW_keymod keyboardModifiers; // current keyboard modifiers
    RGFW_mouse *cursor;            // current mouse cursor
    GLUTEmu_CallbackWindowClose windowCloseFunction;
    GLUTEmu_CallbackWindowMoved windowMovedFunction;
    GLUTEmu_CallbackWindowResized windowResizedFunction;
    GLUTEmu_CallbackWindowMaximized windowMaximizedFunction;
    GLUTEmu_CallbackWindowMinimized windowMinimizedFunction;
    GLUTEmu_CallbackWindowRestored windowRestoredFunction;
    GLUTEmu_CallbackWindowFocused windowFocusedFunction;
    GLUTEmu_CallbackWindowScaled windowScaledFunction;
    GLUTEmu_CallbackWindowRefresh windowRefreshFunction;
    GLUTEmu_CallbackWindowIdle windowIdleFunction;
    GLUTEmu_CallbackKeyboardButton keyboardButtonFunction;
    GLUTEmu_CallbackKeyboardCharacter keyboardCharacterFunction;
    GLUTEmu_CallbackMousePosition mousePositionFunction;
    GLUTEmu_CallbackMouseButton mouseButtonFunction;
    GLUTEmu_CallbackMouseNotify mouseNotifyFunction;
    GLUTEmu_CallbackMouseScroll mouseScrollFunction;
    GLUTEmu_CallbackDragPosition dragPositionFunction;
    GLUTEmu_CallbackDropFiles dropFilesFunction;
    GLUTEmu_CallbackMonitorChanged monitorChangedFunction;
};

std::pair<float, float> GLUTEmu_MonitorGetScaleFactor() {
    return GLUTEmu::Instance().MonitorGetScaleFactor();
}

bool GLUTEmu_WindowCreate(const char *title, uint32_t width, uint32_t height, GLUTEmu_WindowFlag flags) {
    return GLUTEmu::Instance().WindowCreate(title, width, height, flags);
}

bool GLUTEmu_WindowIsCreated() {
    return GLUTEmu::Instance().WindowIsCreated();
}

void GLUTEmu_WindowSetTitle(const char *title) {
    GLUTEmu::Instance().WindowSetTitle(title);
}

GLUTEmu_WindowFlag GLUTEmu_WindowGetFlags() {
    return GLUTEmu::Instance().WindowGetFlags();
}

void GLUTEmu_WindowSetFlags(GLUTEmu_WindowFlag flags) {
    GLUTEmu::Instance().WindowSetFlags(flags);
}

void GLUTEmu_WindowSetIcon(int32_t imageHandle) {
    GLUTEmu::Instance().WindowSetIcon(imageHandle);
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

void GLUTEmu_WindowShow() {
    GLUTEmu::Instance().WindowShow();
}

void GLUTEmu_WindowHide(bool hide) {
    GLUTEmu::Instance().WindowHide(hide);
}

bool GLUTEmu_WindowIsHidden() {
    return GLUTEmu::Instance().WindowIsHidden();
}

void GLUTEmu_WindowFocus() {
    GLUTEmu::Instance().WindowFocus();
}

bool GLUTEmu_WindowHasFocus() {
    return GLUTEmu::Instance().WindowHasFocus();
}

void GLUTEmu_WindowRaise() {
    GLUTEmu::Instance().WindowRaise();
}

void GLUTEmu_WindowSetFloating(bool floating) {
    GLUTEmu::Instance().WindowSetFloating(floating);
}

bool GLUTEmu_WindowIsFloating() {
    return GLUTEmu::Instance().WindowIsFloating();
}

void GLUTEmu_WindowSetOpacity(uint8_t opacity) {
    GLUTEmu::Instance().WindowSetOpacity(opacity);
}

void GLUTEmu_WindowSetBordered(bool bordered) {
    GLUTEmu::Instance().WindowSetBordered(bordered);
}

bool GLUTEmu_WindowHasBorder() {
    return GLUTEmu::Instance().WindowHasBorder();
}

void GLUTEmu_WindowSetFlashMode(GLUTEmu_WindowFlashMode mode) {
    GLUTEmu::Instance().WindowSetFlashMode(mode);
}

void GLUTEmu_WindowSetDragAndDrop(bool allow) {
    GLUTEmu::Instance().WindowSetDragAndDrop(allow);
}

bool GLUTEmu_WindowAllowsDragAndDrop() {
    return GLUTEmu::Instance().WindowAllowsDragAndDrop();
}

void GLUTEmu_WindowSetMousePassthrough(bool passthrough) {
    GLUTEmu::Instance().WindowSetMousePassthrough(passthrough);
}

void GLUTEmu_WindowResize(uint32_t width, uint32_t height) {
    GLUTEmu::Instance().WindowResize(width, height);
}

std::pair<uint32_t, uint32_t> GLUTEmu_WindowGetSize() {
    return GLUTEmu::Instance().WindowGetSize();
}

void GLUTEmu_WindowMove(int32_t x, int32_t y) {
    GLUTEmu::Instance().WindowMove(x, y);
}

std::pair<int32_t, int32_t> GLUTEmu_WindowGetPosition() {
    return GLUTEmu::Instance().WindowGetPosition();
}

void GLUTEmu_WindowCenter() {
    GLUTEmu::Instance().WindowCenter();
}

void GLUTEmu_WindowSetAspectRatio(uint32_t width, uint32_t height) {
    GLUTEmu::Instance().WindowSetAspectRatio(width, height);
}

void GLUTEmu_WindowSetMinSize(uint32_t width, uint32_t height) {
    GLUTEmu::Instance().WindowSetMinSize(width, height);
}

void GLUTEmu_WindowSetMaxSize(uint32_t width, uint32_t height) {
    GLUTEmu::Instance().WindowSetMaxSize(width, height);
}

void GLUTEmu_WindowScaleToMonitor() {
    GLUTEmu::Instance().WindowScaleToMonitor();
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

const void *GLUTEmu_WindowGetNativeHandle(int32_t type) {
    return GLUTEmu::Instance().WindowGetNativeHandle(type);
}

void GLUTEmu_WindowSetCloseFunction(GLUTEmu_CallbackWindowClose func) {
    GLUTEmu::Instance().WindowSetCloseFunction(func);
}

void GLUTEmu_WindowSetMovedFunction(GLUTEmu_CallbackWindowMoved func) {
    GLUTEmu::Instance().WindowSetMovedFunction(func);
}

void GLUTEmu_WindowSetResizedFunction(GLUTEmu_CallbackWindowResized func) {
    GLUTEmu::Instance().WindowSetResizedFunction(func);
}

void GLUTEmu_WindowSetMaximizedFunction(GLUTEmu_CallbackWindowMaximized func) {
    GLUTEmu::Instance().WindowSetMaximizedFunction(func);
}

void GLUTEmu_WindowSetMinimizedFunction(GLUTEmu_CallbackWindowMinimized func) {
    GLUTEmu::Instance().WindowSetMinimizedFunction(func);
}

void GLUTEmu_WindowSetRestoredFunction(GLUTEmu_CallbackWindowRestored func) {
    GLUTEmu::Instance().WindowSetRestoredFunction(func);
}

void GLUTEmu_WindowSetFocusedFunction(GLUTEmu_CallbackWindowFocused func) {
    GLUTEmu::Instance().WindowSetFocusedFunction(func);
}

void GLUTEmu_WindowSetScaledFunction(GLUTEmu_CallbackWindowScaled func) {
    GLUTEmu::Instance().WindowSetScaledFunction(func);
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

void GLUTEmu_KeyboardSetCharacterFunction(GLUTEmu_CallbackKeyboardCharacter func) {
    GLUTEmu::Instance().KeyboardSetCharacterFunction(func);
}

bool GLUTEmu_KeyboardIsKeyModifierSet(GLUTEmu_KeyboardKeyModifier modifier) {
    return GLUTEmu::Instance().KeyboardIsKeyModifierSet(modifier);
}

bool GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor style) {
    return GLUTEmu::Instance().MouseSetStandardCursor(style);
}

bool GLUTEmu_MouseSetCustomCursor(int32_t imageHandle) {
    return GLUTEmu::Instance().MouseSetCustomCursor(imageHandle);
}

void GLUTEmu_MouseHide(bool hide) {
    GLUTEmu::Instance().MouseHide(hide);
}

bool GLUTEmu_MouseIsHidden() {
    return GLUTEmu::Instance().MouseIsHidden();
}

void GLUTEmu_MouseCapture(bool capture) {
    GLUTEmu::Instance().MouseCapture(capture);
}

bool GLUTEmu_MouseIsCaptured() {
    return GLUTEmu::Instance().MouseIsCaptured();
}

void GLUTEmu_MouseMove(int32_t x, int32_t y) {
    GLUTEmu::Instance().MouseMove(x, y);
}

void GLUTEmu_MouseSetPositionFunction(GLUTEmu_CallbackMousePosition func) {
    GLUTEmu::Instance().MouseSetPositionFunction(func);
}

void GLUTEmu_MouseSetButtonFunction(GLUTEmu_CallbackMouseButton func) {
    GLUTEmu::Instance().MouseSetButtonFunction(func);
}

void GLUTEmu_MouseSetNotifyFunction(GLUTEmu_CallbackMouseNotify func) {
    GLUTEmu::Instance().MouseSetNotifyFunction(func);
}

void GLUTEmu_MouseSetScrollFunction(GLUTEmu_CallbackMouseScroll func) {
    GLUTEmu::Instance().MouseSetScrollFunction(func);
}

std::pair<uint32_t, uint32_t> GLUTEmu_MonitorGetSize() {
    return GLUTEmu::Instance().MonitorGetSize();
}

void GLUTEmu_MonitorSetChangedFunction(GLUTEmu_CallbackMonitorChanged func) {
    GLUTEmu::Instance().MonitorSetChangedFunction(func);
}

void GLUTEmu_DragSetPositionFunction(GLUTEmu_CallbackDragPosition func) {
    GLUTEmu::Instance().DragSetPositionFunction(func);
}

void GLUTEmu_DropSetFilesFunction(GLUTEmu_CallbackDropFiles func) {
    GLUTEmu::Instance().DropSetFilesFunction(func);
}

void GLUTEmu_MainLoop() {
    GLUTEmu::Instance().MainLoop();
}
