//----------------------------------------------------------------------------------------------------------------------
// QB64-PE GLUT-like emulation layer
// This abstracts the underlying windowing library and provides a GLUT-like API for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <GL/glew.h>

#define RGFW_OPENGL
#include "../../parts/core/rgfw/RGFW.h"

#include <cstdint>
#include <cuchar>
#include <utility>

enum GLUTEmu_WindowFlag : RGFW_windowFlags {
    None = 0,
    NoBorder = RGFW_windowNoBorder,
    NoResize = RGFW_windowNoResize,
    AllowDND = RGFW_windowAllowDND,
    HideMouse = RGFW_windowHideMouse,
    Fullscreen = RGFW_windowFullscreen,
    Transparent = RGFW_windowTransparent,
    Center = RGFW_windowCenter,
    RawMouse = RGFW_windowRawMouse,
    ScaleToMonitor = RGFW_windowScaleToMonitor,
    Hide = RGFW_windowHide,
    Maximize = RGFW_windowMaximize,
    CenterCursor = RGFW_windowCenterCursor,
    Floating = RGFW_windowFloating,
    FocusOnShow = RGFW_windowFocusOnShow,
    Minimize = RGFW_windowMinimize,
    Focus = RGFW_windowFocus,
    CaptureMouse = RGFW_windowCaptureMouse,
    OpenGL = RGFW_windowOpenGL,
    EGL = RGFW_windowEGL,
    WindowedFullscreen = RGFW_windowedFullscreen,
    CaptureRawMouse = RGFW_windowCaptureRawMouse
};

enum class GLUTEmu_KeyboardKey : RGFW_key {
    Null = RGFW_keyNULL,
    Escape = RGFW_escape,
    Backtick = RGFW_backtick,
    Zero = RGFW_0,
    One = RGFW_1,
    Two = RGFW_2,
    Three = RGFW_3,
    Four = RGFW_4,
    Five = RGFW_5,
    Six = RGFW_6,
    Seven = RGFW_7,
    Eight = RGFW_8,
    Nine = RGFW_9,
    Minus = RGFW_minus,
    Equal = RGFW_equals,
    Backspace = RGFW_backSpace,
    Tab = RGFW_tab,
    Space = RGFW_space,
    A = RGFW_a,
    B = RGFW_b,
    C = RGFW_c,
    D = RGFW_d,
    E = RGFW_e,
    F = RGFW_f,
    G = RGFW_g,
    H = RGFW_h,
    I = RGFW_i,
    J = RGFW_j,
    K = RGFW_k,
    L = RGFW_l,
    M = RGFW_m,
    N = RGFW_n,
    O = RGFW_o,
    P = RGFW_p,
    Q = RGFW_q,
    R = RGFW_r,
    S = RGFW_s,
    T = RGFW_t,
    U = RGFW_u,
    V = RGFW_v,
    W = RGFW_w,
    X = RGFW_x,
    Y = RGFW_y,
    Z = RGFW_z,
    Period = RGFW_period,
    Comma = RGFW_comma,
    Slash = RGFW_slash,
    LeftBracket = RGFW_bracket,
    RightBracket = RGFW_closeBracket,
    Semicolon = RGFW_semicolon,
    Apostrophe = RGFW_apostrophe,
    Backslash = RGFW_backSlash,
    Enter = RGFW_enter,
    Delete = RGFW_delete,
    F1 = RGFW_F1,
    F2 = RGFW_F2,
    F3 = RGFW_F3,
    F4 = RGFW_F4,
    F5 = RGFW_F5,
    F6 = RGFW_F6,
    F7 = RGFW_F7,
    F8 = RGFW_F8,
    F9 = RGFW_F9,
    F10 = RGFW_F10,
    F11 = RGFW_F11,
    F12 = RGFW_F12,
    F13 = RGFW_F13,
    F14 = RGFW_F14,
    F15 = RGFW_F15,
    F16 = RGFW_F16,
    F17 = RGFW_F17,
    F18 = RGFW_F18,
    F19 = RGFW_F19,
    F20 = RGFW_F20,
    F21 = RGFW_F21,
    F22 = RGFW_F22,
    F23 = RGFW_F23,
    F24 = RGFW_F24,
    F25 = RGFW_F25,
    CapsLock = RGFW_capsLock,
    LeftShift = RGFW_shiftL,
    LeftControl = RGFW_controlL,
    LeftAlt = RGFW_altL,
    LeftSuper = RGFW_superL,
    RightShift = RGFW_shiftR,
    RightControl = RGFW_controlR,
    RightAlt = RGFW_altR,
    RightSuper = RGFW_superR,
    Up = RGFW_up,
    Down = RGFW_down,
    Left = RGFW_left,
    Right = RGFW_right,
    Insert = RGFW_insert,
    Menu = RGFW_menu,
    End = RGFW_end,
    Home = RGFW_home,
    PageUp = RGFW_pageUp,
    PageDown = RGFW_pageDown,
    NumLock = RGFW_numLock,
    KPDivide = RGFW_kpSlash,
    KPMultiply = RGFW_kpMultiply,
    KPPlus = RGFW_kpPlus,
    KPSubtract = RGFW_kpMinus,
    KPEqual = RGFW_kpEqual,
    KP0 = RGFW_kp0,
    KP1 = RGFW_kp1,
    KP2 = RGFW_kp2,
    KP3 = RGFW_kp3,
    KP4 = RGFW_kp4,
    KP5 = RGFW_kp5,
    KP6 = RGFW_kp6,
    KP7 = RGFW_kp7,
    KP8 = RGFW_kp8,
    KP9 = RGFW_kp9,
    KPPeriod = RGFW_kpPeriod,
    KPEnter = RGFW_kpReturn,
    ScrollLock = RGFW_scrollLock,
    PrintScreen = RGFW_printScreen,
    Pause = RGFW_pause,
    World1 = RGFW_world1,
    World2 = RGFW_world2
};

enum GLUTEmu_KeyboardKeyModifier : RGFW_keymod {
    Shift = RGFW_modShift,
    Control = RGFW_modControl,
    Alt = RGFW_modAlt,
    Super = RGFW_modSuper,
    CapsLock = RGFW_modCapsLock,
    NumLock = RGFW_modNumLock,
    ScrollLock = RGFW_modScrollLock
};

enum class GLUTEmu_MouseStandardCursor : RGFW_mouseIcons {
    Normal = RGFW_mouseNormal,
    Arrow = RGFW_mouseArrow,
    Text = RGFW_mouseText,
    Crosshair = RGFW_mouseCrosshair,
    PointingHand = RGFW_mousePointingHand,
    ResizeEW = RGFW_mouseResizeEW,
    ResizeNS = RGFW_mouseResizeNS,
    ResizeNWSE = RGFW_mouseResizeNWSE,
    ResizeNESW = RGFW_mouseResizeNESW,
    ResizeNW = RGFW_mouseResizeNW,
    ResizeN = RGFW_mouseResizeN,
    ResizeNE = RGFW_mouseResizeNE,
    ResizeE = RGFW_mouseResizeE,
    ResizeSE = RGFW_mouseResizeSE,
    ResizeS = RGFW_mouseResizeS,
    ResizeSW = RGFW_mouseResizeSW,
    ResizeW = RGFW_mouseResizeW,
    ResizeAll = RGFW_mouseResizeAll,
    NotAllowed = RGFW_mouseNotAllowed,
    Wait = RGFW_mouseWait,
    Progress = RGFW_mouseProgress
};

enum class GLUTEmu_MouseButton : RGFW_mouseButton {
    Left = RGFW_mouseLeft,
    Middle = RGFW_mouseMiddle,
    Right = RGFW_mouseRight,
    One = RGFW_mouseMisc1,
    Two = RGFW_mouseMisc2,
    Three = RGFW_mouseMisc3,
    Four = RGFW_mouseMisc4,
    Five = RGFW_mouseMisc5
};

enum class GLUTEmu_WindowFlashMode : RGFW_flashRequest { Stop = RGFW_flashCancel, Briefly = RGFW_flashBriefly, UntilFocused = RGFW_flashUntilFocused };

typedef void (*GLUTEmu_CallbackWindowClose)();
typedef void (*GLUTEmu_CallbackWindowMoved)(int32_t x, int32_t y);
typedef void (*GLUTEmu_CallbackWindowResized)(uint32_t width, uint32_t height);
typedef void (*GLUTEmu_CallbackWindowMaximized)(int32_t x, int32_t y, uint32_t width, uint32_t height);
typedef void (*GLUTEmu_CallbackWindowMinimized)();
typedef void (*GLUTEmu_CallbackWindowRestored)(int32_t x, int32_t y, uint32_t width, uint32_t height);
typedef void (*GLUTEmu_CallbackWindowFocused)(bool focused);
typedef void (*GLUTEmu_CallbackWindowScaled)(float scaleX, float scaleY);
typedef void (*GLUTEmu_CallbackWindowRefresh)();
typedef void (*GLUTEmu_CallbackWindowIdle)();
typedef void (*GLUTEmu_CallbackKeyboardButton)(GLUTEmu_KeyboardKey key, GLUTEmu_KeyboardKeyModifier modifiers, bool pressed, bool repeated);
typedef void (*GLUTEmu_CallbackKeyboardCharacter)(char32_t codepoint);
typedef void (*GLUTEmu_CallbackMousePosition)(int32_t x, int32_t y, float vecX, float vecY, bool isCaptured);
typedef void (*GLUTEmu_CallbackMouseButton)(int32_t x, int32_t y, GLUTEmu_MouseButton button, bool pressed);
typedef void (*GLUTEmu_CallbackMouseNotify)(int32_t x, int32_t y, bool entered);
typedef void (*GLUTEmu_CallbackMouseScroll)(int32_t x, int32_t y, float offsetX, float offsetY);
typedef void (*GLUTEmu_CallbackDragPosition)(int32_t x, int32_t y);
typedef void (*GLUTEmu_CallbackDropFiles)(char **fileNames, size_t count);
typedef void (*GLUTEmu_CallbackMonitorChanged)(const char *name, bool connected);

std::pair<float, float> GLUTEmu_MonitorGetScaleFactor();
std::pair<uint32_t, uint32_t> GLUTEmu_MonitorGetSize();
void GLUTEmu_MonitorSetChangedFunction(GLUTEmu_CallbackMonitorChanged func);
bool GLUTEmu_WindowCreate(const char *title, uint32_t width, uint32_t height, GLUTEmu_WindowFlag flags);
bool GLUTEmu_WindowIsCreated();
void GLUTEmu_WindowSetTitle(const char *title);
GLUTEmu_WindowFlag GLUTEmu_WindowGetFlags();
void GLUTEmu_WindowSetFlags(GLUTEmu_WindowFlag flags);
void GLUTEmu_WindowSetIcon(int32_t imageHandle);
void GLUTEmu_WindowFullScreen(bool fullscreen);
bool GLUTEmu_WindowIsFullscreen();
void GLUTEmu_WindowMaximize();
bool GLUTEmu_WindowIsMaximized();
void GLUTEmu_WindowMinimize();
bool GLUTEmu_WindowIsMinimized();
void GLUTEmu_WindowRestore();
void GLUTEmu_WindowHide(bool hide);
bool GLUTEmu_WindowIsHidden();
void GLUTEmu_WindowFocus();
bool GLUTEmu_WindowHasFocus();
void GLUTEmu_WindowRaise();
void GLUTEmu_WindowSetFloating(bool floating);
bool GLUTEmu_WindowIsFloating();
void GLUTEmu_WindowSetOpacity(uint8_t opacity);
void GLUTEmu_WindowSetBordered(bool bordered);
bool GLUTEmu_WindowHasBorder();
void GLUTEmu_WindowSetFlashMode(GLUTEmu_WindowFlashMode mode);
void GLUTEmu_WindowSetDragAndDrop(bool allow);
bool GLUTEmu_WindowAllowsDragAndDrop();
void GLUTEmu_WindowSetMousePassthrough(bool passthrough);
void GLUTEmu_WindowResize(uint32_t width, uint32_t height);
std::pair<uint32_t, uint32_t> GLUTEmu_WindowGetSize();
void GLUTEmu_WindowMove(int32_t x, int32_t y);
std::pair<int32_t, int32_t> GLUTEmu_WindowGetPosition();
void GLUTEmu_WindowCenter();
void GLUTEmu_WindowSetAspectRatio(uint32_t width, uint32_t height);
void GLUTEmu_WindowSetMinSize(uint32_t width, uint32_t height);
void GLUTEmu_WindowSetMaxSize(uint32_t width, uint32_t height);
void GLUTEmu_WindowScaleToMonitor();
void GLUTEmu_WindowSetShouldClose(bool shouldClose);
void GLUTEmu_WindowSwapBuffers();
void GLUTEmu_WindowRefresh();
const void *GLUTEmu_WindowGetNativeHandle(int32_t type);
void GLUTEmu_WindowSetCloseFunction(GLUTEmu_CallbackWindowClose func);
void GLUTEmu_WindowSetMovedFunction(GLUTEmu_CallbackWindowMoved func);
void GLUTEmu_WindowSetResizedFunction(GLUTEmu_CallbackWindowResized func);
void GLUTEmu_WindowSetMaximizedFunction(GLUTEmu_CallbackWindowMaximized func);
void GLUTEmu_WindowSetMinimizedFunction(GLUTEmu_CallbackWindowMinimized func);
void GLUTEmu_WindowSetRestoredFunction(GLUTEmu_CallbackWindowRestored func);
void GLUTEmu_WindowSetFocusedFunction(GLUTEmu_CallbackWindowFocused func);
void GLUTEmu_WindowSetScaledFunction(GLUTEmu_CallbackWindowScaled func);
void GLUTEmu_WindowSetRefreshFunction(GLUTEmu_CallbackWindowRefresh func);
void GLUTEmu_WindowSetIdleFunction(GLUTEmu_CallbackWindowIdle func);
void GLUTEmu_KeyboardSetButtonFunction(GLUTEmu_CallbackKeyboardButton func);
void GLUTEmu_KeyboardSetCharacterFunction(GLUTEmu_CallbackKeyboardCharacter func);
bool GLUTEmu_KeyboardIsKeyModifierSet(GLUTEmu_KeyboardKeyModifier modifier);
bool GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor style);
bool GLUTEmu_MouseSetCustomCursor(int32_t imageHandle);
void GLUTEmu_MouseHide(bool hide);
bool GLUTEmu_MouseIsHidden();
void GLUTEmu_MouseCapture(bool capture);
bool GLUTEmu_MouseIsCaptured();
void GLUTEmu_MouseMove(int32_t x, int32_t y);
void GLUTEmu_MouseSetPositionFunction(GLUTEmu_CallbackMousePosition func);
void GLUTEmu_MouseSetButtonFunction(GLUTEmu_CallbackMouseButton func);
void GLUTEmu_MouseSetNotifyFunction(GLUTEmu_CallbackMouseNotify func);
void GLUTEmu_MouseSetScrollFunction(GLUTEmu_CallbackMouseScroll func);
void GLUTEmu_DragSetPositionFunction(GLUTEmu_CallbackDragPosition func);
void GLUTEmu_DropSetFilesFunction(GLUTEmu_CallbackDropFiles func);
void GLUTEmu_MainLoop();
