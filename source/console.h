#pragma once

#include "common.h"
#include "framebuffer.h"
#include "font.h"
#include "vertex_array.h"
#include "shader.h"
#include "text_batcher.h"

// Can forward declare to not include here?
#include <vector>

#include "meh_hash.h"

class Window;
class Game;
class Input;
struct StringViu;

constexpr int32_t CONSOLE_HISTORY_MAX       = 32;
constexpr int32_t CONSOLE_INPUT_MAX         = 64;
constexpr int32_t CONSOLE_COMMAND_NAME_MAX  = 64;
constexpr float   CONSOLE_CURSOR_BLINK_TIME = 0.5f;

struct CommandTableKey {
    char string[CONSOLE_COMMAND_NAME_MAX]; // This includes the null terminator
};

#define CONSOLE_COMMAND_PROC_ARGS class Console &console, const std::vector<StringViu> &args, Game &game
#define CONSOLE_COMMAND_PROC(proc) void proc(CONSOLE_COMMAND_PROC_ARGS)
typedef CONSOLE_COMMAND_PROC(console_command_proc);
#define CONSOLE_COMMAND_LAMBDA [](CONSOLE_COMMAND_PROC_ARGS) -> void

uint64_t console_command_hash_func(const CommandTableKey &a);
bool     console_command_comp_func(const CommandTableKey &a, const CommandTableKey &b);
typedef meh::Table<CommandTableKey, console_command_proc *, console_command_hash_func, console_command_comp_func> ConsoleCommandTable;

class Console {
public:
    CLASS_COPY_DISABLE(Console);

    Console(void)  = default;
    ~Console(void) = default;

    void initialize(void);

    void destroy(void);

    void update(Game &game, const Input &input, double delta_time);

    void render(int32_t width, int32_t height);

    void add_to_history(const char *format, ...);

    void clear_history(void);

    void set_open_state(bool open);

    bool is_open(void);

    void set_command(const char command_name[CONSOLE_COMMAND_NAME_MAX], console_command_proc *command_proc);

private:
    bool m_initialized     = false;
    bool m_is_open         = false;

    std::string m_input;
    std::string m_history[CONSOLE_HISTORY_MAX];
    std::string m_last_entered;

    ConsoleCommandTable m_commands;

    bool  m_cursor_blink   = false;
    float m_cursor_blink_t = 0.0f;

    Font        m_font;
    TextBatcher m_batcher;
    ShaderFile  m_quad_shader; // @temp
    VertexArray m_quad_vao;    // @temp
};

inline uint64_t console_command_hash_func(const CommandTableKey &a) {
    uint64_t hash = 0;
    for(uint32_t index = 0; index < strlen(a.string); ++index) {
        hash += a.string[index] * (index + 1) + 7;
    }
    return hash;
}

inline bool console_command_comp_func(const CommandTableKey &a, const CommandTableKey &b) {
    return !strcmp(a.string, b.string);
}
