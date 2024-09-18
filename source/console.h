#pragma once

#include "common.h"
#include "window.h"
#include "input.h"
#include "camera.h"

#define CONSOLE_COMMAND_PROC(proc) void proc(Window &window, Camera &camera)
typedef CONSOLE_COMMAND_PROC(console_command_proc);
#define CONSOLE_COMMAND_LAMBDA [](Window &window, Camera &camera) -> void

struct ConsoleCommand {
    char command[32];
    console_command_proc *proc;
};

// @todo Kinda rewrite

namespace Console {
    void initialize(void);
    void destroy(void);
    void update(const Input &input, Window &window, Camera &camera);
    void render(int32_t window_w, int32_t window_h);
    void set_open_state(bool open);
    bool is_open(void);
    void register_command(ConsoleCommand command);
}
