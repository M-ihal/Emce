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
	DEFINE_KEY(ESCAPE, SDLK_ESCAPE)

/* Enum class of the defined keys */
#define DEFINE_KEY(name, sdlk) name,
enum class Key : int32_t {
	INPUT_DEFINED_KEYS
	_COUNT,
	_INVALID
};
#undef DEFINE_KEY

class Input {
public:
	Input(void);

	bool key_check(Key key, uint32_t input_flags) const;
	
	bool key_pressed(Key key) const;

	bool key_released(Key key) const;

	bool key_is_down(Key key) const;

	bool key_repeat(Key key) const;

	bool key_pressed_or_repeat(Key key) const;

private:
	friend class Window;

	/* Called before event loop */
	void prepare_to_catch_input(void);

	/* Called in the event loop */
	void catch_input(const SDL_Event &event);

	uint32_t m_key_state[(int32_t)Key::_COUNT];
};

/* Static array of key names */
#define DEFINE_KEY(name, ...) "KEY::"#name,
static inline const char *input_key_string[(int32_t)Key::_COUNT] = {
	INPUT_DEFINED_KEYS
};
#undef DEFINE_KEY

/* Convert Key to index with checks */
inline int32_t key_index(Key key) {
	int32_t key_index = (int32_t)key;
	assert(key_index >= 0);
	assert(key_index < (int32_t)Key::_COUNT);
	return key_index;
}