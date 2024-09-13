#pragma once

#include "common.h"

union SDL_Event;

enum InputFlags : uint32_t {
    PRESSED  = 0x1,
    RELEASED = 0x2,
    IS_DOWN  = 0x4,
    REPEAT   = 0x8,
};

/* All defined keys */
#define INPUT_DEFINED_KEYS\
    DEFINE_KEY(W,      SDLK_w)\
    DEFINE_KEY(A,      SDLK_a)\
    DEFINE_KEY(S,      SDLK_s)\
    DEFINE_KEY(D,      SDLK_d)\
    DEFINE_KEY(LEFT,   SDLK_LEFT)\
    DEFINE_KEY(RIGHT,  SDLK_RIGHT)\
    DEFINE_KEY(UP,     SDLK_UP)\
    DEFINE_KEY(DOWN,   SDLK_DOWN)\
    DEFINE_KEY(ESCAPE, SDLK_ESCAPE)\
    DEFINE_KEY(LEFT_SHIFT, SDLK_LSHIFT)

/* Enum class of the defined keys */
#define DEFINE_KEY(name, sdlk) name,
enum class Key : int32_t {
    INPUT_DEFINED_KEYS
    _COUNT,
    _INVALID
};
#undef DEFINE_KEY

/* All defined mouse buttons */
#define INPUT_DEFINED_BUTTONS\
    DEFINE_BUTTON(LEFT, SDL_BUTTON_LEFT)

/* Enum of mouse buttons */
#define DEFINE_BUTTON(name, sdlb) name,
enum class Button : int32_t {
    INPUT_DEFINED_BUTTONS
    _COUNT,
    _INVALID
};
#undef DEFINE_BUTTON

class Input {
public:
    Input(void);

    bool key_check(Key key, uint32_t input_flags) const;
    
    bool key_pressed(Key key) const;

    bool key_released(Key key) const;

    bool key_is_down(Key key) const;

    bool key_repeat(Key key) const;

    bool key_pressed_or_repeat(Key key) const;

    bool button_check(Button button, uint32_t input_flags) const;

    bool button_pressed(Button button) const;

    bool button_released(Button button) const;

    bool button_is_down(Button button) const;

    int32_t mouse_rel_x(void) const;

    int32_t mouse_rel_y(void) const;

private:
    friend class Window;

    /* Called before event loop */
    void prepare_to_catch_input(void);

    /* Called in the event loop */
    void catch_input(const SDL_Event &event);

    uint32_t m_key_state[(int32_t)Key::_COUNT];

    uint32_t m_button_state[(int32_t)Button::_COUNT];
    int32_t m_mouse_x;
    int32_t m_mouse_y;
    int32_t m_mouse_rel_x;
    int32_t m_mouse_rel_y;
};

/* Static array of key names */
#define DEFINE_KEY(name, ...) "Key::"#name,
static inline const char *input_key_string[(int32_t)Key::_COUNT] = {
    INPUT_DEFINED_KEYS
};
#undef DEFINE_KEY

/* Static array of button names */
#define DEFINE_BUTTON(name, ...) "Button::"#name,
static inline const char *input_button_string[(int32_t)Button::_COUNT] = {
    INPUT_DEFINED_BUTTONS
};
#undef DEFINE_BUTTON

/* Convert Key to index with checks */
inline int32_t key_index(Key key) {
    const int32_t key_index = (int32_t)key;
    assert(key_index >= 0);
    assert(key_index < (int32_t)Key::_COUNT);
    return key_index;
}

/* Same for buttons */
inline int32_t button_index(Button button) {
    const int32_t button_index = (int32_t)button;
    ASSERT(button_index >= 0);
    ASSERT(button_index < (int32_t)Button::_COUNT);
    return button_index;
}
