#include "input.h"

#include <SDL3/SDL.h>

// temp
#include <cstring>
#include <stdio.h>

Input::Input(void) {
    //ZERO_ARRAY(m_key_state);
    ZERO_STRUCT(*this);
}

bool Input::key_check(Key key, uint32_t input_flags) const {
    return !!(m_key_state[key_index(key)] & input_flags);
}

bool Input::key_pressed(Key key) const {
    return key_check(key, PRESSED);
}

bool Input::key_released(Key key) const {
    return key_check(key, RELEASED);
}

bool Input::key_is_down(Key key) const {
    return key_check(key, IS_DOWN);
}

bool Input::key_repeat(Key key) const {
    return key_check(key, REPEAT);
}

bool Input::key_pressed_or_repeat(Key key) const {
    return key_check(key, PRESSED | REPEAT);
}

bool Input::button_check(Button button, uint32_t input_flags) const {
    return !!(m_button_state[button_index(button)] & input_flags);
}

bool Input::button_pressed(Button button) const {
    return button_check(button, PRESSED);
}

bool Input::button_released(Button button) const {
    return button_check(button, RELEASED);
}

bool Input::button_is_down(Button button) const {
    return button_check(button, IS_DOWN);
}

int32_t Input::mouse_x(void) const {
    return m_mouse_x;
}

int32_t Input::mouse_y(void) const {
    return m_mouse_x;
}

int32_t Input::mouse_rel_x(void) const {
    return m_mouse_rel_x;
}

int32_t Input::mouse_rel_y(void) const {
    return m_mouse_rel_y;
}

int32_t Input::scroll_move(void) const {
    return m_scroll_move;
}

/* Function to get Key enum from SDL Keysym @todo Maybe do something better */
#define DEFINE_KEY(name, sdlk) case sdlk: return Key::name;
Key key_from_sdl_key_code(int32_t sdl_key_code) {
    switch(sdl_key_code) {
        default: return Key::_INVALID;
        INPUT_DEFINED_KEYS  
    }
}
#undef DEFINE_KEY

/* Same but for mouse button */
#define DEFINE_BUTTON(name, sdlb) case sdlb: return Button::name;
Button button_from_sdl_button(int32_t sdl_button) {
    switch(sdl_button) {
        default: return Button::_INVALID;
        INPUT_DEFINED_BUTTONS  
    }
}
#undef DEFINE_BUTTON

const char *const Input::get_text_input(void) const {
    return m_text_input;
}

void Input::prepare_to_catch_input(void) {
    for(int32_t key_index = 0; key_index < (int32_t)Key::_COUNT; ++key_index) { m_key_state[key_index] &= ~(PRESSED | RELEASED | REPEAT); }
    for(int32_t btn_index = 0; btn_index < (int32_t)Button::_COUNT; ++btn_index) { m_button_state[btn_index] &= ~(PRESSED | RELEASED); }

    m_mouse_rel_x = 0;
    m_mouse_rel_y = 0;
    m_scroll_move = 0;

    ZERO_ARRAY(m_text_input);
}

void Input::catch_input(const SDL_Event &event) {
    switch(event.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            const SDL_KeyboardEvent &kb_event = event.key;
            const SDL_Keycode &key_code = kb_event.key;

            const Key key = key_from_sdl_key_code((int32_t)key_code);
            if(key == Key::_INVALID) {
                break;
            }

            /* Set key state flags */
            const int32_t index = key_index(key);

            if((kb_event.type == SDL_EVENT_KEY_DOWN) && (kb_event.repeat != 1)) {
                m_key_state[index] |= InputFlags::PRESSED;
                m_key_state[index] |= InputFlags::IS_DOWN;
            } else if(kb_event.repeat == 1) {
                m_key_state[index] |= InputFlags::REPEAT;
            } else if(kb_event.type == SDL_EVENT_KEY_UP) {
                m_key_state[index] |= InputFlags::RELEASED;
                m_key_state[index] &= ~InputFlags::IS_DOWN;
            }
        } break;

        case SDL_EVENT_MOUSE_MOTION: {
            const SDL_MouseMotionEvent &motion = event.motion;
            m_mouse_x = motion.x;
            m_mouse_y = motion.y;
            m_mouse_rel_x = motion.xrel;
            m_mouse_rel_y = motion.yrel;
        } break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            const SDL_MouseButtonEvent &button_event = event.button;

            const Button button = button_from_sdl_button(button_event.button);
            if(button == Button::_INVALID) {
                break;
            }

            const int32_t index = button_index(button);

            if(button_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                m_button_state[index] |= PRESSED;
                m_button_state[index] |= IS_DOWN;
            } else if(button_event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                m_button_state[index] |= RELEASED;
                m_button_state[index] &= ~IS_DOWN;
            }
        } break;

        case SDL_EVENT_MOUSE_WHEEL: {
            const SDL_MouseWheelEvent &wheel_event = event.wheel;
            m_scroll_move = wheel_event.y;
        } break;

        case SDL_EVENT_TEXT_INPUT: {
            // @todo
            strcpy_s(m_text_input, event.text.text);
        } break;
    }
}
