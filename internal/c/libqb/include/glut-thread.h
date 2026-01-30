#pragma once

#include "glut-emu.h"

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

// Called at consistent intervals from a GLUT callback
void libqb_process_glut_queue();

// Called to properly exit the program. Necessary because GLUT requires a
// special care to not seg-fault when exiting the program.
void libqb_exit(int);

// These functions perform the same actions as their corresponding glut* functions.
// They tell the GLUT thread to perform the command, returning the result if applicable
void libqb_glut_set_window_title(const char *title);
void libqb_glut_set_fullscreen(bool fullscreen);
bool libqb_glut_is_fullscreen();
void libqb_glut_maximize_window();
bool libqb_glut_is_window_maximized();
void libqb_glut_minimize_window();
bool libqb_glut_is_window_minimized();
void libqb_glut_restore_window();
void libqb_glut_show_window();
void libqb_glut_hide_window();
bool libqb_glut_is_window_visible();
void libqb_glut_focus_window();
bool libqb_glut_window_has_focus();
void libqb_glut_resize_window(uint32_t width, uint32_t height);
std::pair<uint32_t, uint32_t> libqb_glut_get_window_size();
void libqb_glut_move_window(int32_t x, int32_t y);
std::pair<int32_t, int32_t> libqb_glut_get_window_position();
void libqb_glut_set_window_aspect_ratio(uint32_t width, uint32_t height);
void libqb_glut_set_window_min_size(uint32_t width, uint32_t height);
void libqb_glut_set_window_max_size(uint32_t width, uint32_t height);
void libqb_glut_set_cursor(GLUTEmu_MouseStandardCursor style);
void libqb_glut_hide_cursor(bool hide);
bool libqb_glut_is_cursor_hidden();
void libqb_glut_capture_mouse(bool capture);
bool libqb_glut_is_mouse_captured();
void libqb_glut_move_mouse(int32_t x, int32_t y);
std::pair<int32_t, int32_t> libqb_glut_get_mouse_position();
std::pair<uint32_t, uint32_t> libqb_glut_get_screen_size();
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
