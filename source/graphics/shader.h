#pragma once

#include "common.h"

class Shader {
public:
	CLASS_COPY_DISABLE(Shader);
//	CLASS_MOVE_ALLOW(Shader);

	Shader(void);
	
	~Shader(void);

	/* Unbind any shader program */
	static void use_no_program(void);

	void use_program(void);

	bool load_from_memory(const char *vs, size_t vs_len, const char *fs, size_t fs_len, const char *gs = NULL, size_t gs_len = 0);

	bool load_from_file(const char *filepath);

	void upload_int(const char *name, int32_t value);

	void upload_float(const char *name, float value);

	void upload_vec2(const char *name, float values[2]);

	void upload_vec3(const char *name, float values[2]);

	void upload_vec4(const char *name, float values[2]);

    void upload_mat4(const char *name, float values[4][4]);

	bool is_valid_program(void) const;

	// @temp
	uint32_t program_id(void) { return m_program_id; };

private:
	void delete_program_if_exists(void);

	uint32_t m_program_id;
};
