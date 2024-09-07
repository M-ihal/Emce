#include "shader.h"

#include <stdio.h>
#include <glew.h>

Shader::Shader(void) {
	m_program_id = 0;
}

Shader::~Shader(void) {
	this->delete_program_if_exists();
}

void Shader::use_no_program(void) {
	GL_CHECK(glUseProgram(0));
}

void Shader::use_program(void) {
	GL_CHECK(glUseProgram(m_program_id));
}

static uint32_t compile_shader_source(const char *src, int32_t src_len, uint32_t src_type) {
	GL_CHECK(uint32_t shader_id = glCreateShader(src_type));
    if(!shader_id) {
        return 0;
    }

    GL_CHECK(glShaderSource(shader_id, 1, &src, &src_len));
    GL_CHECK(glCompileShader(shader_id));
    return shader_id;
}

bool Shader::load_from_memory(const char *vs, size_t vs_len, const char *fs, size_t fs_len, const char *gs, size_t gs_len) {
	GL_CHECK(m_program_id = glCreateProgram());
    if(!m_program_id) {
        fprintf(stderr, "[error] Shader: Failed to create program id.\n");
        return false;
    }

    uint32_t vs_id = compile_shader_source(vs, vs_len, GL_VERTEX_SHADER);
    uint32_t fs_id = compile_shader_source(fs, fs_len, GL_FRAGMENT_SHADER);
    uint32_t gs_id = gs ? compile_shader_source(fs, gs_len, GL_FRAGMENT_SHADER) : 0;

	GL_CHECK(glAttachShader(m_program_id, vs_id));
    GL_CHECK(glAttachShader(m_program_id, fs_id));
    GL_CHECK(if(gs_id) { glAttachShader(m_program_id, gs_id); })

	GL_CHECK(glLinkProgram(m_program_id));

	GL_CHECK(glDeleteShader(vs_id));
	GL_CHECK(glDeleteShader(fs_id));
	GL_CHECK(glDeleteShader(gs_id));

    int32_t link_success = 0;
    glGetProgramiv(m_program_id, GL_LINK_STATUS, &link_success);
    if(link_success != GL_TRUE) {
        char error_buffer[256];
        glGetProgramInfoLog(m_program_id, ARRAY_COUNT(error_buffer), 0, error_buffer);
        fprintf(stderr, "[error] Shader: Failed to link shader program, error below:\n%s-----------\n", error_buffer);
        this->delete_program_if_exists();
        return false;
    }

    fprintf(stdout, "[info] Shader: Created shader, ID: %d (%p)\n", m_program_id, this);
	return true;
}

bool Shader::load_from_file(const char *filepath) {
	/* Parse file to find sources */
	const char *vs = NULL;
	size_t vs_len  = 0;
	const char *fs = NULL;
	size_t fs_len  = 0;
	const char *gs = NULL;
	size_t gs_len  = 0;

	return this->load_from_memory(vs, vs_len, fs, fs_len, gs, gs_len);
}

void Shader::delete_program_if_exists(void) {
	if(m_program_id) {
		GL_CHECK(glDeleteProgram(m_program_id));
		m_program_id = 0;
	
		fprintf(stdout, "[info] Shader: Deleted shader (%p)\n", this);
	}
}

void Shader::upload_int(const char *name, int32_t value) {
	int32_t location = glGetUniformLocation(m_program_id, name);
	if(location != -1) {
		GL_CHECK(glUniform1i(location, value));
	}
}

void Shader::upload_float(const char *name, float value) {
	int32_t location = glGetUniformLocation(m_program_id, name);
	if(location != -1) {
		GL_CHECK(glUniform1f(location, value));
	}
}

void Shader::upload_vec2(const char *name, float values[2]) {
	int32_t location = glGetUniformLocation(m_program_id, name);
	if(location != -1) {
		GL_CHECK(glUniform2f(location, values[0], values[1]));
	}
}

void Shader::upload_vec3(const char *name, float values[2]) {
	int32_t location = glGetUniformLocation(m_program_id, name);
	if(location != -1) {
		GL_CHECK(glUniform3f(location, values[0], values[1], values[2]));
	}
}

void Shader::upload_vec4(const char *name, float values[2]) {
	int32_t location = glGetUniformLocation(m_program_id, name);
	if(location != -1) {
		GL_CHECK(glUniform4f(location, values[0], values[1], values[2], values[3]));
	}
}

bool Shader::is_valid_program(void) const {
	if(!m_program_id) {
		return false;
	}

	if(glIsProgram(m_program_id) != GL_TRUE) {
		return false;
	}

	return true;
}