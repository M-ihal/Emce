#pragma once

#include "common.h"
#include "window.h"
#include "input.h"
#include "camera.h"

// Can forward declare to not include here?
#include <vector>
#include <string>

#define CONSOLE_COMMAND_PROC(proc) void proc(const std::vector<std::string> &args, Window &window, Camera &camera)
typedef CONSOLE_COMMAND_PROC(console_command_proc);
#define CONSOLE_COMMAND_LAMBDA [](const std::vector<std::string> &args, Window &window, Camera &camera) -> void

struct ConsoleCommand {
    std::string command;
    console_command_proc *proc;
};

namespace Console {
    void initialize(void);
    void destroy(void);
    void update(const Input &input, Window &window, Camera &camera);
    void render(int32_t window_w, int32_t window_h);
    void add_to_history(const char *string);
    void set_open_state(bool open);
    bool is_open(void);
    void register_command(ConsoleCommand command);
}
