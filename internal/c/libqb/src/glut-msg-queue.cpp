
#include "libqb-common.h"

#include "completion.h"
#include "glut-thread.h"
#include <cstdlib>
#include <cstring>
#include <queue>

class glut_message {
  private:
    completion *finished = nullptr;

    void initCompletion() {
        finished = new completion();
        completion_init(finished);
    }

  protected:
    glut_message(bool withCompletion) {
        if (withCompletion) {
            initCompletion();
        }
    }

  public:
    // Calling this indicates to the creator of the message that it has been
    // completed, and any response data is available to be read.
    //
    // If `finished` is NULL that means nobody is waiting for the response. In
    // that situation we're free to simply delete the object.
    void finish() {
        if (finished) {
            completion_finish(finished);
        } else {
            delete this;
        }
    }

    void wait_for_response() {
        completion_wait(finished);
    }

    virtual ~glut_message() {
        if (finished) {
            completion_wait(finished); // Should be a NOP, but better to check anyway
            completion_clear(finished);

            delete finished;
        }
    }

    virtual void execute() = 0;
};

static libqb_mutex *glut_msg_queue_lock = libqb_mutex_new();
static std::queue<glut_message *> glut_msg_queue;

// Queues a glut_message to be processed. Returns false if the message was not queued.
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

    while (!glut_msg_queue.empty()) {
        glut_message *msg = glut_msg_queue.front();
        glut_msg_queue.pop();

        msg->execute();

        msg->finish();
    }
}

class glut_message_set_window_title : public glut_message {
  public:
    char *newTitle;

    glut_message_set_window_title(const char *title) : glut_message(false) {
        newTitle = strdup(title);
    }

    void execute() {
        GLUTEmu_WindowSetTitle(newTitle);
    }

    virtual ~glut_message_set_window_title() {
        free(newTitle);
    }
};

void libqb_glut_set_window_title(const char *title) {
    libqb_queue_glut_message(new glut_message_set_window_title(title));
}

class glut_message_fullscreen : public glut_message {
  public:
    bool fullscreen;

    glut_message_fullscreen(bool fullscreen) : glut_message(false), fullscreen(fullscreen) {}

    void execute() {
        GLUTEmu_WindowFullScreen(fullscreen);
    }
};

void libqb_glut_set_fullscreen(bool fullscreen) {
    libqb_queue_glut_message(new glut_message_fullscreen(fullscreen));
}

class glut_message_is_fullscreen : public glut_message {
  public:
    bool response_value;

    glut_message_is_fullscreen() : glut_message(true), response_value(false) {}

    void execute() {
        response_value = GLUTEmu_WindowIsFullscreen();
    }
};

bool libqb_glut_is_fullscreen() {
    glut_message_is_fullscreen msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_maximize_window : public glut_message {
  public:
    glut_message_maximize_window() : glut_message(false) {}

    void execute() {
        GLUTEmu_WindowMaximize();
    }
};

void libqb_glut_maximize_window() {
    libqb_queue_glut_message(new glut_message_maximize_window());
}

class glut_message_is_window_maximized : public glut_message {
  public:
    bool response_value;

    glut_message_is_window_maximized() : glut_message(true), response_value(false) {}

    void execute() {
        response_value = GLUTEmu_WindowIsMaximized();
    }
};

bool libqb_glut_is_window_maximized() {
    glut_message_is_window_maximized msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_minimize_window : public glut_message {
  public:
    glut_message_minimize_window() : glut_message(false) {}

    void execute() {
        GLUTEmu_WindowMinimize();
    }
};

void libqb_glut_minimize_window() {
    libqb_queue_glut_message(new glut_message_minimize_window());
}

class glut_message_is_window_minimized : public glut_message {
  public:
    bool response_value;

    glut_message_is_window_minimized() : glut_message(true), response_value(false) {}

    void execute() {
        response_value = GLUTEmu_WindowIsMinimized();
    }
};

bool libqb_glut_is_window_minimized() {
    glut_message_is_window_minimized msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_restore_window : public glut_message {
  public:
    glut_message_restore_window() : glut_message(false) {}

    void execute() {
        GLUTEmu_WindowRestore();
    }
};

void libqb_glut_restore_window() {
    libqb_queue_glut_message(new glut_message_restore_window());
}

class glut_message_show_window : public glut_message {
  public:
    glut_message_show_window() : glut_message(false) {}

    void execute() {
        GLUTEmu_WindowHide(false);
    }
};

void libqb_glut_show_window() {
    libqb_queue_glut_message(new glut_message_show_window());
}

class glut_message_hide_window : public glut_message {
  public:
    glut_message_hide_window() : glut_message(false) {}

    void execute() {
        GLUTEmu_WindowHide(true);
    }
};

void libqb_glut_hide_window() {
    libqb_queue_glut_message(new glut_message_hide_window());
}

class glut_message_is_window_visible : public glut_message {
  public:
    bool response_value;

    glut_message_is_window_visible() : glut_message(true), response_value(false) {}

    void execute() {
        response_value = !GLUTEmu_WindowIsHidden();
    }
};

bool libqb_glut_is_window_visible() {
    glut_message_is_window_visible msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_focus_window : public glut_message {
  public:
    glut_message_focus_window() : glut_message(false) {}

    void execute() {
        GLUTEmu_WindowFocus();
    }
};

void libqb_glut_focus_window() {
    libqb_queue_glut_message(new glut_message_focus_window());
}

class glut_message_window_has_focus : public glut_message {
  public:
    bool response_value;

    glut_message_window_has_focus() : glut_message(true), response_value(false) {}

    void execute() {
        response_value = GLUTEmu_WindowHasFocus();
    }
};

bool libqb_glut_window_has_focus() {
    glut_message_window_has_focus msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_resize_window : public glut_message {
  public:
    uint32_t width, height;

    glut_message_resize_window(uint32_t width, uint32_t height) : glut_message(false), width(width), height(height) {}

    void execute() {
        GLUTEmu_WindowResize(width, height);
    }
};

void libqb_glut_resize_window(uint32_t width, uint32_t height) {
    libqb_queue_glut_message(new glut_message_resize_window(width, height));
}

class glut_message_get_window_size : public glut_message {
  public:
    std::pair<uint32_t, uint32_t> response_value;

    glut_message_get_window_size() : glut_message(true), response_value(0, 0) {}

    void execute() {
        response_value = GLUTEmu_WindowGetSize();
    }
};

std::pair<uint32_t, uint32_t> libqb_glut_get_window_size() {
    glut_message_get_window_size msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_move_window : public glut_message {
  public:
    int32_t x, y;

    glut_message_move_window(int32_t x, int32_t y) : glut_message(false), x(x), y(y) {}

    void execute() {
        GLUTEmu_WindowMove(x, y);
    }
};

void libqb_glut_move_window(int32_t x, int32_t y) {
    libqb_queue_glut_message(new glut_message_move_window(x, y));
}

class glut_message_get_window_position : public glut_message {
  public:
    std::pair<int32_t, int32_t> response_value;

    glut_message_get_window_position() : glut_message(true), response_value(0, 0) {}

    void execute() {
        response_value = GLUTEmu_WindowGetPosition();
    }
};

std::pair<int32_t, int32_t> libqb_glut_get_window_position() {
    glut_message_get_window_position msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_set_window_aspect_ratio : public glut_message {
  public:
    uint32_t width, height;

    glut_message_set_window_aspect_ratio(uint32_t width, uint32_t height) : glut_message(false), width(width), height(height) {}

    void execute() {
        GLUTEmu_WindowSetAspectRatio(width, height);
    }
};

void libqb_glut_set_window_aspect_ratio(uint32_t width, uint32_t height) {
    libqb_queue_glut_message(new glut_message_set_window_aspect_ratio(width, height));
}

class glut_message_set_window_min_size : public glut_message {
  public:
    uint32_t width, height;

    glut_message_set_window_min_size(uint32_t width, uint32_t height) : glut_message(false), width(width), height(height) {}

    void execute() {
        GLUTEmu_WindowSetMinSize(width, height);
    }
};

void libqb_glut_set_window_min_size(uint32_t width, uint32_t height) {
    libqb_queue_glut_message(new glut_message_set_window_min_size(width, height));
}

class glut_message_set_window_max_size : public glut_message {
  public:
    uint32_t width, height;

    glut_message_set_window_max_size(uint32_t width, uint32_t height) : glut_message(false), width(width), height(height) {}

    void execute() {
        GLUTEmu_WindowSetMaxSize(width, height);
    }
};

void libqb_glut_set_window_max_size(uint32_t width, uint32_t height) {
    libqb_queue_glut_message(new glut_message_set_window_max_size(width, height));
}

class glut_message_set_cursor : public glut_message {
  public:
    GLUTEmu_MouseStandardCursor style;

    glut_message_set_cursor(GLUTEmu_MouseStandardCursor style) : glut_message(false), style(style) {}

    void execute() {
        GLUTEmu_MouseSetStandardCursor(style);
    }
};

void libqb_glut_set_cursor(GLUTEmu_MouseStandardCursor style) {
    libqb_queue_glut_message(new glut_message_set_cursor(style));
}

class glut_message_set_hide_cursor : public glut_message {
  public:
    bool hide;

    glut_message_set_hide_cursor(bool hide) : glut_message(false), hide(hide) {}

    void execute() {
        GLUTEmu_MouseHide(hide);
    }
};

void libqb_glut_hide_cursor(bool hide) {
    libqb_queue_glut_message(new glut_message_set_hide_cursor(hide));
}

class glut_message_is_cursor_hidden : public glut_message {
  public:
    bool response_value;

    glut_message_is_cursor_hidden() : glut_message(true), response_value(false) {}

    void execute() {
        response_value = GLUTEmu_MouseIsHidden();
    }
};

bool libqb_glut_is_cursor_hidden() {
    glut_message_is_cursor_hidden msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_capture_mouse : public glut_message {
  public:
    bool capture;

    glut_message_capture_mouse(bool capture) : glut_message(false), capture(capture) {}

    void execute() {
        GLUTEmu_MouseCapture(capture);
    }
};

void libqb_glut_capture_mouse(bool capture) {
    libqb_queue_glut_message(new glut_message_capture_mouse(capture));
}

class glut_message_is_mouse_captured : public glut_message {
  public:
    bool response_value;

    glut_message_is_mouse_captured() : glut_message(true), response_value(false) {}

    void execute() {
        response_value = GLUTEmu_MouseIsCaptured();
    }
};

bool libqb_glut_is_mouse_captured() {
    glut_message_is_mouse_captured msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_move_mouse : public glut_message {
  public:
    int32_t x, y;

    glut_message_move_mouse(double _x, double _y) : glut_message(false), x(_x), y(_y) {}

    void execute() {
        GLUTEmu_MouseMove(x, y);
    }
};

void libqb_glut_move_mouse(int32_t x, int32_t y) {
    libqb_queue_glut_message(new glut_message_move_mouse(x, y));
}

class glut_message_get_screen_size : public glut_message {
  public:
    std::pair<uint32_t, uint32_t> response_value;

    glut_message_get_screen_size() : glut_message(true), response_value(0, 0) {}

    void execute() {
        response_value = GLUTEmu_MonitorGetSize();
    }
};

std::pair<uint32_t, uint32_t> libqb_glut_get_screen_size() {
    glut_message_get_screen_size msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_exit_program : public glut_message {
  public:
    int exitCode;

    glut_message_exit_program(int _exitCode) : glut_message(true), exitCode(_exitCode) {}

    void execute() {
        GLUTEmu_WindowSetShouldClose(true);
        exit(exitCode);
    }
};

void libqb_glut_exit_program(int exitcode) {
    glut_message_exit_program msg(exitcode);

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    // Should never return
    exit(exitcode);
}
