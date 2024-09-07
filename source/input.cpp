#include "input.h"

#include <SDL.h>

Input::Input(void) {
	ZERO_ARRAY(m_key_state);
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

/* Function to get Key enum from SDL Keysym @todo Maybe do something better */
#define DEFINE_KEY(name, sdlk) case sdlk: return Key::name;
Key key_from_sdl_key_sym(int32_t sdl_key_sym) {
	switch(sdl_key_sym) {
		default: return Key::_INVALID;
		INPUT_DEFINED_KEYS	
	}
}
#undef DEFINE_KEY

void Input::prepare_to_catch_input(void) {
	for(int32_t key_index = 0; key_index < (int32_t)Key::_COUNT; ++key_index) { m_key_state[key_index] &= ~(PRESSED | RELEASED | REPEAT); }
}

void Input::catch_input(const SDL_Event &event) {
	switch(event.type) {
		case SDL_KEYDOWN:
        case SDL_KEYUP: {
            const SDL_KeyboardEvent &kb_event = event.key;
            const SDL_Keysym &key_sym = kb_event.keysym;

            const Key key = key_from_sdl_key_sym((int32_t)key_sym.sym);
            if(key == Key::_INVALID) {
                break;
            }

            /* Set key state flags */
            const int32_t key_index = (int32_t)key;

            if((kb_event.type == SDL_KEYDOWN) && (kb_event.repeat != 1)) {
            	m_key_state[key_index] |= InputFlags::PRESSED;
            	m_key_state[key_index] |= InputFlags::IS_DOWN;
            } else if(kb_event.repeat == 1) {
				m_key_state[key_index] |= InputFlags::REPEAT;
            } else if(kb_event.type == SDL_KEYUP) {
				m_key_state[key_index] |= InputFlags::RELEASED;
				m_key_state[key_index] &= ~InputFlags::IS_DOWN;
            }
        } break;
	}
}