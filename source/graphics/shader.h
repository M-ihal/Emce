#pragma once

#include "common.h"
#include "utils.h"
#include "math_types.h"

#include <string>

/* Unbind shader program */
static void use_no_program(void);

class Shader {
public:
    CLASS_COPY_DISABLE(Shader);

    Shader(void) = default;

    /* Bind shader */
    void use_program(void) const;

    /* Create the shader from memory, should be called at init or after deletion */
    bool load_from_memory(const char *vs, size_t vs_len, const char *fs, size_t fs_len, const char *gs = NULL, size_t gs_len = 0);

    /* Load the shader from file, should be called at init or after deletion */
    /* Shader sources need to be separated by @shader_vertex, @shader_fragment, @shader_geometry, placed before each shader */
    bool load_from_file(const std::string &filepath);

    /* Delete the shader */
    void delete_shader(void);

    /* Check if the shader's program is valid */
    bool is_valid_program(void) const;

    /* Uniform procs */
    void upload_int(const char *name, int32_t value) const;
    void upload_float(const char *name, float value) const;
    void upload_vec2(const char *name, float values[2]) const;
    void upload_vec3(const char *name, float values[3]) const;
    void upload_vec4(const char *name, float values[4]) const;
    void upload_mat4(const char *name, const mat4 &mat) const;

private:
    uint32_t m_program_id = 0;
};

/* Shader abstraction that allows hotloading from file */
class ShaderFile : public Shader {
public:
    CLASS_COPY_DISABLE(ShaderFile);

    ShaderFile(void) = default;

    /* Set filepath and load shader from it */
    void set_filepath_and_load(const std::string &filepath);

    /* Delete shader and clear data */
    void delete_shader_file(void);
 
    /* Deletes and reloads shader from the file */
    void hotload(void);

private:
    FileTime    m_last_time;
    std::string m_filepath;
};
