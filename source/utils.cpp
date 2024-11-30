#include "utils.h"

#include <stdio.h>
#include <malloc.h>

bool read_entire_file_c(const char *filepath, FileContents &file, bool null_terminated) {
    file.data = NULL;
    file.size = 0;

    FILE *f = NULL;
    if(fopen_s(&f, filepath, "rb") != 0) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    file.size = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* TODO: fail to load if file size is 0? */
    if(!file.size) {
        return false;
    }

    if(null_terminated) {
        file.data = (uint8_t *)malloc(file.size + 1);
        ASSERT(file.data, "read_entire_file: Failed to allocate memory.\n");
        fread(file.data, 1, file.size, f);
        ((char *)file.data)[file.size] = '\0';
    } else {
        file.data = (uint8_t *)malloc(file.size);
        ASSERT(file.data, "read_entire_file: Failed to allocate memory.\n");
        fread(file.data, 1, file.size, f);
    }

    fclose(f);
    return true;
}

bool read_entire_file(const std::string &filepath, FileContents &file, bool null_terminated) {
    return read_entire_file_c(filepath.c_str(), file, null_terminated);
}

bool write_entire_file(const char *filepath, uint8_t *buffer, size_t bytes) {
    ASSERT(buffer && bytes);

    FILE *f = NULL;
    if(fopen_s(&f, filepath, "wb") != 0) {
        return false;
    }

    size_t bytes_written = fwrite(buffer, sizeof(uint8_t), bytes, f);
    if(bytes_written != bytes) {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

void free_loaded_file(FileContents &file) {
    free(file.data);
    file.data = NULL;
    file.size = 0;
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

bool FileTime::get_times_c(const char *filepath, FileTime *last_write_time, FileTime *last_access_time, FileTime *creation_time) {
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

bool FileTime::get_times(const std::string &filepath, FileTime *last_write_time, FileTime *last_access_time, FileTime *creation_time) {
    return get_times_c(filepath.c_str(), last_write_time, last_access_time, creation_time);
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
