#pragma once

#define STRING_VIU_CPP_HELPERS
#include <string_viu.h>

#include "common.h"
#include "window.h"
#include "input.h"
#include "game.h"

// Can forward declare to not include here?
#include <vector>

/*
    Arguments passed to command procedure:
        - args: vector of strings (max 3 for now)
        - window: reference to window class
        - game: reference to game class
*/

#define CONSOLE_COMMAND_PROC_ARGS const std::vector<StringViu> &args, Window &window, Game &game
#define CONSOLE_COMMAND_PROC(proc) void proc(CONSOLE_COMMAND_PROC_ARGS)
typedef CONSOLE_COMMAND_PROC(console_command_proc);
#define CONSOLE_COMMAND_LAMBDA [](CONSOLE_COMMAND_PROC_ARGS) -> void

struct ConsoleCommand {
    console_command_proc *proc;
};

namespace Console {
    void initialize(void);
    void destroy(void);
    void update(const Input &input, Window &window, Game &game, double delta_time);
    void render(int32_t window_w, int32_t window_h);
    void add_to_history(const char *format, ...); // @todo
    void set_open_state(bool open);
    bool is_open(void);
    void set_command(const char name[64], ConsoleCommand command);
}
