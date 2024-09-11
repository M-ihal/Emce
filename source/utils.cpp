#include "utils.h"

#include <stdio.h>
#include <malloc.h>

LoadedFile::LoadedFile(const char *filepath, bool null_terminated) {
    ASSERT(filepath);

    m_sanity_check = false;
    m_data = NULL;
    m_size = 0;

    FILE *file = NULL;
    if(fopen_s(&file, filepath, "rb") != 0) {
        return;
    }

    fseek(file, 0, SEEK_END);
    m_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* TODO: fail to load if file size is 0? */
    if(!m_size) {
        return;
    }

    if(null_terminated) {
        m_data = malloc(m_size + 1);
        ASSERT(m_data);
        fread(m_data, 1, m_size, file);
        ((char *)m_data)[m_size] = '\0';
    } else {
        m_data = malloc(m_size);
        ASSERT(m_data);
        fread(m_data, 1, m_size, file);
    }

    fclose(file);
    m_sanity_check = true;
}

LoadedFile::~LoadedFile(void) {
    if(m_data) {
        free(m_data);
        m_data = NULL;
    }
}

const void *LoadedFile::data(void) const {
    return m_data;
}

const size_t LoadedFile::size(void) const {
    return m_size;
}

bool LoadedFile::is_loaded(void) const {
    return m_data && m_size && m_sanity_check;
}

#if defined(_WIN32)
#include <windows.h>

inline static FileTime convert_SYSTEMTIME(const SYSTEMTIME *sys_time) {
    FileTime time;
    time.year         = sys_time->wYear;
    time.month        = sys_time->wMonth;
    time.day          = sys_time->wDay;
    time.hour         = sys_time->wHour;
    time.minute       = sys_time->wMinute;
    time.second       = sys_time->wSecond;
    time.milliseconds = sys_time->wMilliseconds;
    return time;
}

bool FileTime::get_times(char *filepath, FileTime *last_write_time, FileTime *last_access_time, FileTime *creation_time) {
    HANDLE file_handle = CreateFile(filepath, 0, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    FILETIME _last_write_time;
    FILETIME _creation_time;
    FILETIME _last_access_time;

    bool success = !!GetFileTime(file_handle, &_creation_time, &_last_access_time, &_last_write_time);
    if(!success) {
        return false;
    }
    CloseHandle(file_handle);

    SYSTEMTIME sys_last_write_time;
    SYSTEMTIME sys_creation_time;
    SYSTEMTIME sys_last_access_time;
    
    FileTimeToSystemTime(&_last_write_time, &sys_last_write_time);
    FileTimeToSystemTime(&_creation_time, &sys_creation_time);
    FileTimeToSystemTime(&_last_access_time, &sys_last_access_time);

    if(last_write_time) {
        *last_write_time = convert_SYSTEMTIME(&sys_last_write_time);
    }
    if(creation_time) {
        *creation_time = convert_SYSTEMTIME(&sys_creation_time);
    }
    if(last_access_time) {
        *last_access_time = convert_SYSTEMTIME(&sys_last_access_time);
    }

    return true;
}
#endif

bool operator==(const FileTime &l, const FileTime &r) {
    return (
        l.year         == r.year &&
        l.month        == r.month &&
        l.day          == r.day &&
        l.hour         == r.hour &&
        l.minute       == r.minute &&
        l.second       == r.second &&
        l.milliseconds == r.milliseconds
    );
}

bool operator!=(const FileTime &l, const FileTime &r) {
    return !(l == r);
}
