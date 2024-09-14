#pragma once

#include "common.h"
#include "utils.h"

/*
    TODO: Better logging when loading and hotloading shaders
*/

class Shader {
public:
    CLASS_COPY_DISABLE(Shader);

    Shader(void);
    
    ~Shader(void);

    static void use_no_program(void);

    void use_program(void) const;

    bool load_from_memory(const char *vs, size_t vs_len, const char *fs, size_t fs_len, const char *gs = NULL, size_t gs_len = 0);

    bool load_from_file(const char *filepath);

    void upload_int(const char *name, int32_t value) const;

    void upload_float(const char *name, float value) const;

    void upload_vec2(const char *name, float values[2]) const;

    void upload_vec3(const char *name, float values[2]) const;

    void upload_vec4(const char *name, float values[2]) const;

    void upload_mat4(const char *name, float values[4][4]) const;

    bool is_valid_program(void) const;

private:
    void delete_program_if_exists(bool log = true);

    uint32_t m_program_id;
};

class ShaderFile : public Shader {
public:
    // CLASS_COPY_DISABLE(Shader);

    ShaderFile(const char *filepath);

    ~ShaderFile(void);

    void hotload(void);

private:
    FileTime m_last_time;
    char m_filepath[64];
};
