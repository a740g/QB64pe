
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

class glut_message_get_window_title : public glut_message {
  public:
    const char *response_value;

    glut_message_get_window_title() : glut_message(true), response_value(nullptr) {}

    void execute() {
        response_value = GLUTEmu_WindowGetTitle();
    }
};

const char *libqb_glut_get_window_title() {
    glut_message_get_window_title msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_fullscreen : public glut_message {
  public:
    bool fullscreen;

    glut_message_fullscreen(bool _fullscreen) : glut_message(false), fullscreen(_fullscreen) {}

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
        GLUTEmu_ShowWindow();
    }
};

void libqb_glut_show_window() {
    libqb_queue_glut_message(new glut_message_show_window());
}

class glut_message_hide_window : public glut_message {
  public:
    glut_message_hide_window() : glut_message(false) {}

    void execute() {
        GLUTEmu_HideWindow();
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
        response_value = GLUTEmu_WindowIsVisible();
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
    int width, height;

    glut_message_resize_window(int _width, int _height) : glut_message(false), width(_width), height(_height) {}

    void execute() {
        GLUTEmu_WindowResize(width, height);
    }
};

void libqb_glut_resize_window(int width, int height) {
    libqb_queue_glut_message(new glut_message_resize_window(width, height));
}

class glut_message_get_window_size : public glut_message {
  public:
    std::pair<int, int> response_value;

    glut_message_get_window_size() : glut_message(true), response_value(0, 0) {}

    void execute() {
        response_value = GLUTEmu_WindowGetSize();
    }
};

std::pair<int, int> libqb_glut_get_window_size() {
    glut_message_get_window_size msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_move_window : public glut_message {
  public:
    int x, y;

    glut_message_move_window(int _x, int _y) : glut_message(false), x(_x), y(_y) {}

    void execute() {
        GLUTEmu_WindowMove(x, y);
    }
};

void libqb_glut_move_window(int x, int y) {
    libqb_queue_glut_message(new glut_message_move_window(x, y));
}

class glut_message_get_window_position : public glut_message {
  public:
    std::pair<int, int> response_value;

    glut_message_get_window_position() : glut_message(true), response_value(0, 0) {}

    void execute() {
        response_value = GLUTEmu_WindowGetPosition();
    }
};

std::pair<int, int> libqb_glut_get_window_position() {
    glut_message_get_window_position msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_set_window_aspect_ratio : public glut_message {
  public:
    int width, height;

    glut_message_set_window_aspect_ratio(int _width, int _height) : glut_message(false), width(_width), height(_height) {}

    void execute() {
        GLUTEmu_WindowSetAspectRatio(width, height);
    }
};

void libqb_glut_set_window_aspect_ratio(int width, int height) {
    libqb_queue_glut_message(new glut_message_set_window_aspect_ratio(width, height));
}

class glut_message_set_window_size_limits : public glut_message {
  public:
    int minWidth, minHeight, maxWidth, maxHeight;

    glut_message_set_window_size_limits(int _minWidth, int _minHeight, int _maxWidth, int _maxHeight)
        : glut_message(false), minWidth(_minWidth), minHeight(_minHeight), maxWidth(_maxWidth), maxHeight(_maxHeight) {}

    void execute() {
        GLUTEmu_WindowSetSizeLimits(minWidth, minHeight, maxWidth, maxHeight);
    }
};

void libqb_glut_set_window_size_limits(int minWidth, int minHeight, int maxWidth, int maxHeight) {
    libqb_queue_glut_message(new glut_message_set_window_size_limits(minWidth, minHeight, maxWidth, maxHeight));
}

class glut_message_set_cursor : public glut_message {
  public:
    GLUTEmu_MouseStandardCursor style;

    glut_message_set_cursor(GLUTEmu_MouseStandardCursor _style) : glut_message(false), style(_style) {}

    void execute() {
        GLUTEmu_MouseSetStandardCursor(style);
    }
};

void libqb_glut_set_cursor(GLUTEmu_MouseStandardCursor style) {
    libqb_queue_glut_message(new glut_message_set_cursor(style));
}

class glut_message_set_cursor_mode : public glut_message {
  public:
    GLUTEnum_MouseCursorMode mode;

    glut_message_set_cursor_mode(GLUTEnum_MouseCursorMode _mode) : glut_message(false), mode(_mode) {}

    void execute() {
        GLUTEmu_MouseSetCursorMode(mode);
    }
};

void libqb_glut_set_cursor_mode(GLUTEnum_MouseCursorMode mode) {
    libqb_queue_glut_message(new glut_message_set_cursor_mode(mode));
}

class glut_message_get_cursor_mode : public glut_message {
  public:
    GLUTEnum_MouseCursorMode mode;

    glut_message_get_cursor_mode() : glut_message(true), mode(GLUTEnum_MouseCursorMode::Normal) {}

    void execute() {
        mode = GLUTEmu_MouseGetCursorMode();
    }
};

GLUTEnum_MouseCursorMode libqb_glut_get_cursor_mode() {
    glut_message_get_cursor_mode msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.mode;
}

class glut_message_move_mouse : public glut_message {
  public:
    double x, y;

    glut_message_move_mouse(double _x, double _y) : glut_message(false), x(_x), y(_y) {}

    void execute() {
        GLUTEmu_MouseMove(x, y);
    }
};

void libqb_glut_move_mouse(double x, double y) {
    libqb_queue_glut_message(new glut_message_move_mouse(x, y));
}

class glut_message_get_mouse_position : public glut_message {
  public:
    std::pair<double, double> response_value;

    glut_message_get_mouse_position() : glut_message(true), response_value(0.0, 0.0) {}

    void execute() {
        response_value = GLUTEmu_MouseGetPosition();
    }
};

std::pair<double, double> libqb_glut_get_mouse_position() {
    glut_message_get_mouse_position msg;

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

class glut_message_get_screen_size : public glut_message {
  public:
    std::pair<int, int> response_value;

    glut_message_get_screen_size() : glut_message(true), response_value(0, 0) {}

    void execute() {
        response_value = GLUTEmu_ScreenGetSize();
    }
};

std::pair<int, int> libqb_glut_get_screen_size() {
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
