#include "shader.h"

#include <stdio.h>
#include <string.h>
#include <glew.h>

// TEMP  TODO: Rewrite the thing, or use something else
#define STRING_VIEW_IMPLEMENTATION
#include <string_view.h>

#include "utils.h"

static bool find_shader_sources(const void  *file_data, 
                                const size_t file_size, 
                                string_view_t &out_vert_view, 
                                string_view_t &out_frag_view, 
                                string_view_t &out_geom_view);


Shader::Shader(void) {
    m_program_id = 0;
}

Shader::~Shader(void) {
    this->delete_program_if_exists();
}

void Shader::use_no_program(void) {
    GL_CHECK(glUseProgram(0));
}

void Shader::use_program(void) const {
    /* What TODO */
    if(!this->is_valid_program()) {
        Shader::use_no_program();
    }

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
    this->delete_program_if_exists();

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
        this->delete_program_if_exists(false);
        return false;
    }

    fprintf(stdout, "[info] Shader: Created shader, ID: %d (%p)\n", m_program_id, this);
    return true;
}

bool Shader::load_from_file(const char *filepath) {
    fprintf(stdout, "[info] Shader: Loading shader from file, path: %s\n", filepath);

    FileContents file;
    if(!read_entire_file(filepath, file, true)) {
        fprintf(stderr, "[error] Shader: Failed to read shader file, path: \"%s\"\n", filepath);
        return false;
    }

    /* Parse file to find sources */
    string_view_t vs;
    string_view_t fs;
    string_view_t gs;
    const bool parse_success = find_shader_sources(file.data, file.size, vs, fs, gs);
    if(!parse_success) {
        fprintf(stderr, "[error] Shader: Failed to parse shader file, path: %s\n", filepath);
        free_loaded_file(file);
        return false;
    }
    
    const bool success = this->load_from_memory(vs.pointer, vs.length, fs.pointer, fs.length, gs.pointer, gs.length);
    free_loaded_file(file);
    return success;
}

void Shader::delete_program_if_exists(bool log) {
    if(m_program_id) {
        GL_CHECK(glDeleteProgram(m_program_id));
        if(log) {
            fprintf(stdout, "[info] Shader: Deleted shader program, ID: %d (%p)\n", m_program_id, this);
        }
        m_program_id = 0;
    }
}

void Shader::upload_int(const char *name, int32_t value) const {
    int32_t location = glGetUniformLocation(m_program_id, name);
    if(location != -1) {
        GL_CHECK(glUniform1i(location, value));
    }
}

void Shader::upload_float(const char *name, float value) const {
    int32_t location = glGetUniformLocation(m_program_id, name);
    if(location != -1) {
        GL_CHECK(glUniform1f(location, value));
    }
}

void Shader::upload_vec2(const char *name, float values[2]) const {
    int32_t location = glGetUniformLocation(m_program_id, name);
    if(location != -1) {
        GL_CHECK(glUniform2f(location, values[0], values[1]));
    }
}

void Shader::upload_vec3(const char *name, float values[2]) const {
    int32_t location = glGetUniformLocation(m_program_id, name);
    if(location != -1) {
        GL_CHECK(glUniform3f(location, values[0], values[1], values[2]));
    }
}

void Shader::upload_vec4(const char *name, float values[2]) const {
    int32_t location = glGetUniformLocation(m_program_id, name);
    if(location != -1) {
        GL_CHECK(glUniform4f(location, values[0], values[1], values[2], values[3]));
    }
}

void Shader::upload_mat4(const char *name, float values[4][4]) const {
    int32_t location = glGetUniformLocation(m_program_id, name);
    if(location != -1) {
       GL_CHECK(glUniformMatrix4fv(location, 1, false, (float *)values));
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

ShaderFile::ShaderFile(const char *filepath) {
    strcpy_s(m_filepath, ARRAY_COUNT(m_filepath), filepath);

    this->load_from_file(m_filepath);
    FileTime::get_times(m_filepath, &m_last_time, NULL, NULL);
}

ShaderFile::~ShaderFile(void) {
}

void ShaderFile::hotload(void) {
    if(strlen(m_filepath) == 0) {
        return;
    }

    FileTime time_now;
    bool hacky_check_if_file_exists = FileTime::get_times(m_filepath, &time_now, NULL, NULL);

    if(!hacky_check_if_file_exists) {
        return;
    }

    bool should_hotload = time_now != m_last_time;
    if(!should_hotload) {
        return;
    }

    fprintf(stdout, "[info] Hotload: Hotloading shader: \n");

    bool hotloaded = this->load_from_file(m_filepath);
    if(!hotloaded) {
        fprintf(stdout, "[error] Hotload: Failed to hotload the shader.\n");
    }

    m_last_time = time_now;
}

/* Returns true if atleast vertex and fragment shaders were found */
static bool find_shader_sources(const void  *file_data, 
                                const size_t file_size, 
                                string_view_t &out_vert_view, 
                                string_view_t &out_frag_view, 
                                string_view_t &out_geom_view) {

    constexpr const char shader_token_start[] = "@shader_";
    constexpr const char shader_vert_token[]  = "@shader_vertex";
    constexpr const char shader_frag_token[]  = "@shader_fragment";
    constexpr const char shader_geom_token[]  = "@shader_geometry";

    /* Zero the out views */
    out_vert_view = s_view_zero;
    out_frag_view = s_view_zero;
    out_geom_view = s_view_zero;

    const string_view_t file_view = string_view((char *)file_data, file_size);

    /* Find tokens */
    const size_t token_1 = s_view_find(file_view, shader_token_start);
    if(token_1 == s_view_invalid_idx) {
        return false;
    }

    const size_t token_2 = s_view_find(file_view, shader_token_start, token_1 + strlen(shader_token_start));
    if(token_2 == s_view_invalid_idx) {
        return false;
    }

    const size_t token_3 = s_view_find(file_view, shader_token_start, token_2 + strlen(shader_token_start));

    /* Determine which shader source is which */
    string_view_t vert_view = s_view_zero;
    string_view_t frag_view = s_view_zero;
    string_view_t geom_view = s_view_zero;

    bool redefined_shaders_error = false;
    auto assign_shader = [&] (string_view_t view) -> void {
        auto assign_inner = [&] (string_view_t *view, const char *token, string_view_t *dest) -> bool {
            if(s_view_begins_with(*view, token, view)) {
                if(dest->length) {
                    redefined_shaders_error = true;
                    return true;
                }
                *dest = *view;
                return true;
            }
            return false;
        };

        if(assign_inner(&view, shader_vert_token, &vert_view)) { return; }
        if(assign_inner(&view, shader_frag_token, &frag_view)) { return; }
        if(assign_inner(&view, shader_geom_token, &geom_view)) { return; }
    };
    
    /* First view is guaranteed */
    assign_shader(s_view_substring(file_view, token_1, token_2 - 1));

    /* Calc second and _optionally_ third shader view */
    if(token_3 == s_view_invalid_idx) {
        assign_shader(s_view_substring(file_view, token_2, file_view.length - 1));
    } else {
        assign_shader(s_view_substring(file_view, token_2, token_3 - 1));
        assign_shader(s_view_substring(file_view, token_3, file_view.length - 1));
    }
    
    /* Error checks */
    if(redefined_shaders_error) {    
        return false;
    }

    if(!vert_view.length || !frag_view.length) {
        return false;
    }

    /* Fill the out views */
    out_vert_view = vert_view;
    out_frag_view = frag_view;
    out_geom_view = geom_view;
    return true;
}
