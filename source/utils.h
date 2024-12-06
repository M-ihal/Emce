#pragma once

#include "common.h"
#include <string>

struct FileContents {
    uint8_t *data = NULL;
    size_t   size = 0;
};

bool read_entire_file_c(const char *filepath, FileContents &file, bool null_terminated = false);
bool read_entire_file(const std::string &filepath, FileContents &file, bool null_terminated = false);
bool write_entire_file(const char *filepath, uint8_t *buffer, size_t length);
void free_loaded_file(FileContents &file);

struct FileTime {
    uint16_t year         = 0;
    uint16_t month        = 0;
    uint16_t day          = 0;
    uint16_t hour         = 0;
    uint16_t minute       = 0;
    uint16_t second       = 0;
    uint16_t milliseconds = 0;

    static bool get_times_c(const char *filepath, FileTime *last_write_time, FileTime *last_access_time, FileTime *creation_time);
    static bool get_times(const std::string &filepath, FileTime *last_write_time, FileTime *last_access_time, FileTime *creation_time);
};

bool operator==(const FileTime &l, const FileTime &r);
bool operator!=(const FileTime &l, const FileTime &r);
