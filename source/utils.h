#pragma once

#include "common.h"

class LoadedFile {
public:
    CLASS_COPY_DISABLE(LoadedFile);

    explicit LoadedFile(const char *filepath, bool null_terminated = false);
    ~LoadedFile(void);

    const void  *data(void) const;
    const size_t size(void) const;

    bool is_loaded(void) const;

private:
    void  *m_data;
    size_t m_size;
    bool   m_sanity_check;
};

struct FileTime {
    uint16_t year         = 0;
    uint16_t month        = 0;
    uint16_t day          = 0;
    uint16_t hour         = 0;
    uint16_t minute       = 0;
    uint16_t second       = 0;
    uint16_t milliseconds = 0;

    static bool get_times(char *filepath, FileTime *last_write_time, FileTime *last_access_time, FileTime *creation_time);
};

bool operator==(const FileTime &l, const FileTime &r);
bool operator!=(const FileTime &l, const FileTime &r);
