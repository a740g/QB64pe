#pragma once

#include <RGFW.h>

enum {
    GLUT_CURSOR_NONE = -1,
    GLUT_CURSOR_LEFT_ARROW = RGFW_mouseArrow,
    GLUT_CURSOR_INFO = RGFW_mousePointingHand,
    GLUT_CURSOR_TEXT = RGFW_mouseIbeam,
    GLUT_CURSOR_CROSSHAIR = RGFW_mouseCrosshair,
    GLUT_CURSOR_UP_DOWN = RGFW_mouseResizeNS,
    GLUT_CURSOR_LEFT_RIGHT = RGFW_mouseResizeEW,
    GLUT_CURSOR_TOP_LEFT_CORNER = RGFW_mouseResizeNESW,
    GLUT_CURSOR_TOP_RIGHT_CORNER = RGFW_mouseResizeNWSE,
    GLUT_CURSOR_WAIT = RGFW_mouseNotAllowed,
    GLUT_CURSOR_HELP = RGFW_mouseNormal,
    GLUT_CURSOR_CYCLE = RGFW_mouseResizeAll
};

enum {
    GLUT_FULL_SCREEN,
    GLUT_WINDOW_X,
    GLUT_WINDOW_Y,
    GLUT_WINDOW_WIDTH,
    GLUT_WINDOW_HEIGHT,
    GLUT_WINDOW_BORDER_WIDTH,
    GLUT_WINDOW_BORDER_HEIGHT,
    GLUT_WINDOW_HEADER_HEIGHT,
    GLUT_SCREEN_WIDTH,
    GLUT_SCREEN_HEIGHT
};

enum {
    GLUT_MODIFIER_CAPS_LOCK = RGFW_modCapsLock,
    GLUT_MODIFIER_NUM_LOCK = RGFW_modNumLock,
    GLUT_MODIFIER_CONTROL = RGFW_modControl,
    GLUT_MODIFIER_ALT = RGFW_modAlt,
    GLUT_MODIFIER_SHIFT = RGFW_modShift,
    GLUT_MODIFIER_SUPER = RGFW_modSuper
};

enum {
    GLUT_KEY_NULL = RGFW_keyNULL,
    GLUT_KEY_ESCAPE = RGFW_escape,
    GLUT_KEY_BACKTICK = RGFW_backtick,
    GLUT_KEY_0 = RGFW_0,
    GLUT_KEY_1 = RGFW_1,
    GLUT_KEY_2 = RGFW_2,
    GLUT_KEY_3 = RGFW_3,
    GLUT_KEY_4 = RGFW_4,
    GLUT_KEY_5 = RGFW_5,
    GLUT_KEY_6 = RGFW_6,
    GLUT_KEY_7 = RGFW_7,
    GLUT_KEY_8 = RGFW_8,
    GLUT_KEY_9 = RGFW_9,
    GLUT_KEY_MINUS = RGFW_minus,
    GLUT_KEY_EQUALS = RGFW_equals,
    GLUT_KEY_BACKSPACE = RGFW_backSpace,
    GLUT_KEY_TAB = RGFW_tab,
    GLUT_KEY_SPACE = RGFW_space,
    GLUT_KEY_A = RGFW_a,
    GLUT_KEY_B = RGFW_b,
    GLUT_KEY_C = RGFW_c,
    GLUT_KEY_D = RGFW_d,
    GLUT_KEY_E = RGFW_e,
    GLUT_KEY_F = RGFW_f,
    GLUT_KEY_G = RGFW_g,
    GLUT_KEY_H = RGFW_h,
    GLUT_KEY_I = RGFW_i,
    GLUT_KEY_J = RGFW_j,
    GLUT_KEY_K = RGFW_k,
    GLUT_KEY_L = RGFW_l,
    GLUT_KEY_M = RGFW_m,
    GLUT_KEY_N = RGFW_n,
    GLUT_KEY_O = RGFW_o,
    GLUT_KEY_P = RGFW_p,
    GLUT_KEY_Q = RGFW_q,
    GLUT_KEY_R = RGFW_r,
    GLUT_KEY_S = RGFW_s,
    GLUT_KEY_T = RGFW_t,
    GLUT_KEY_U = RGFW_u,
    GLUT_KEY_V = RGFW_v,
    GLUT_KEY_W = RGFW_w,
    GLUT_KEY_X = RGFW_x,
    GLUT_KEY_Y = RGFW_y,
    GLUT_KEY_Z = RGFW_z,
    GLUT_KEY_PERIOD = RGFW_period,
    GLUT_KEY_COMMA = RGFW_comma,
    GLUT_KEY_SLASH = RGFW_slash,
    GLUT_KEY_BRACKET = RGFW_bracket,
    GLUT_KEY_CLOSE_BRACKET = RGFW_closeBracket,
    GLUT_KEY_SEMICOLON = RGFW_semicolon,
    GLUT_KEY_APOSTROPHE = RGFW_apostrophe,
    GLUT_KEY_BACKSLASH = RGFW_backSlash,
    GLUT_KEY_RETURN = RGFW_return,
    GLUT_KEY_DELETE = RGFW_delete,

    GLUT_KEY_F1 = RGFW_F1,
    GLUT_KEY_F2 = RGFW_F2,
    GLUT_KEY_F3 = RGFW_F3,
    GLUT_KEY_F4 = RGFW_F4,
    GLUT_KEY_F5 = RGFW_F5,
    GLUT_KEY_F6 = RGFW_F6,
    GLUT_KEY_F7 = RGFW_F7,
    GLUT_KEY_F8 = RGFW_F8,
    GLUT_KEY_F9 = RGFW_F9,
    GLUT_KEY_F10 = RGFW_F10,
    GLUT_KEY_F11 = RGFW_F11,
    GLUT_KEY_F12 = RGFW_F12,

    GLUT_KEY_CAPS_LOCK = RGFW_capsLock,
    GLUT_KEY_SHIFT_L = RGFW_shiftL,
    GLUT_KEY_CONTROL_L = RGFW_controlL,
    GLUT_KEY_ALT_L = RGFW_altL,
    GLUT_KEY_SUPER_L = RGFW_superL,
    GLUT_KEY_SHIFT_R = RGFW_shiftR,
    GLUT_KEY_CONTROL_R = RGFW_controlR,
    GLUT_KEY_ALT_R = RGFW_altR,
    GLUT_KEY_SUPER_R = RGFW_superR,

    GLUT_KEY_UP = RGFW_up,
    GLUT_KEY_DOWN = RGFW_down,
    GLUT_KEY_LEFT = RGFW_left,
    GLUT_KEY_RIGHT = RGFW_right,

    GLUT_KEY_INSERT = RGFW_insert,
    GLUT_KEY_END = RGFW_end,
    GLUT_KEY_HOME = RGFW_home,
    GLUT_KEY_PAGE_UP = RGFW_pageUp,
    GLUT_KEY_PAGE_DOWN = RGFW_pageDown,

    GLUT_KEY_NUM_LOCK = RGFW_numLock,
    GLUT_KEY_KP_SLASH = RGFW_KP_Slash,
    GLUT_KEY_MULTIPLY = RGFW_multiply,
    GLUT_KEY_KP_MINUS = RGFW_KP_Minus,
    GLUT_KEY_KP_1 = RGFW_KP_1,
    GLUT_KEY_KP_2 = RGFW_KP_2,
    GLUT_KEY_KP_3 = RGFW_KP_3,
    GLUT_KEY_KP_4 = RGFW_KP_4,
    GLUT_KEY_KP_5 = RGFW_KP_5,
    GLUT_KEY_KP_6 = RGFW_KP_6,
    GLUT_KEY_KP_7 = RGFW_KP_7,
    GLUT_KEY_KP_8 = RGFW_KP_8,
    GLUT_KEY_KP_9 = RGFW_KP_9,
    GLUT_KEY_KP_0 = RGFW_KP_0,
    GLUT_KEY_KP_PERIOD = RGFW_KP_Period,
    GLUT_KEY_KP_RETURN = RGFW_KP_Return
};

// GLUT emulation functions
void glutPostRedisplay();
void glutReshapeWindow(int32_t width, int32_t height);
void glutFullScreen();
void glutSwapBuffers();
void glutDisplayFunc(void (*func)());
void glutIdleFunc(void (*func)());
void glutKeyboardFunc(void (*func)(uint8_t, uint8_t, bool));
void glutMouseFunc(void (*func)(uint8_t, bool, double));
void glutMotionFunc(void (*func)(int32_t, int32_t));

// Called to potentially setup GLUT before starting the program.
void libqb_glut_presetup();

// Starts the "main thread", including handling all the GLUT setup.
void libqb_start_main_thread();

// Used to support _ScreenShow, which can start the GLUT thread after the
// program is started
void libqb_start_glut_thread();

// Indicates whether GLUT is currently running (and thus whether we're able to
// do any GLUT-related stuff
bool libqb_is_glut_up();

// Called to properly exit the program. Necessary because GLUT requires a
// special care to not seg-fault when exiting the program.
void libqb_exit(int);

// These functions perform the same actions as their corresponding glut* functions.
// They tell the GLUT thread to perform the command, returning the result if applicable
void libqb_glut_set_cursor(int style);
void libqb_glut_warp_pointer(int x, int y);
int libqb_glut_get(int id);
void libqb_glut_iconify_window();
void libqb_glut_position_window(int x, int y);
void libqb_glut_show_window();
void libqb_glut_hide_window();
void libqb_glut_set_window_title(const char *title);
void libqb_glut_exit_program(int exitcode);

// Convenience macros, exists a function depending on the state of GLUT
#define NEEDS_GLUT(error_result)                                                                                                                               \
    do {                                                                                                                                                       \
        if (!libqb_is_glut_up()) {                                                                                                                             \
            error(5);                                                                                                                                          \
            return error_result;                                                                                                                               \
        }                                                                                                                                                      \
    } while (0)

#define OPTIONAL_GLUT(result)                                                                                                                                  \
    do {                                                                                                                                                       \
        if (!libqb_is_glut_up())                                                                                                                               \
            return result;                                                                                                                                     \
    } while (0)
