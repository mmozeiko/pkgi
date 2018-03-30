extern "C" {
#include "pkgi.h"
#include "pkgi_style.h"
}

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PKGI_FOLDER "pkgi"
#define PKGI_APP_FOLDER "app"

void pkgi_log(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    printf("\n");
    va_end(args);
}

int pkgi_snprintf(char* buffer, uint32_t size, const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    int len = vsnprintf(buffer, size - 1, msg, args);
    va_end(args);
    buffer[len] = 0;
    return len;
}

void pkgi_vsnprintf(char* buffer, uint32_t size, const char* msg, va_list args)
{
    int len = vsnprintf(buffer, size - 1, msg, args);
    buffer[len] = 0;
}

char* pkgi_strstr(const char* str, const char* sub)
{
    return strstr(str, sub);
}

int pkgi_stricontains(const char* str, const char* sub)
{
    return strcasestr(str, sub) != NULL;
}

int pkgi_stricmp(const char* a, const char* b)
{
    return strcasecmp(a, b);
}

void pkgi_strncpy(char* dst, uint32_t size, const char* src)
{
    strncpy(dst, src, size);
}

char* pkgi_strrchr(const char* str, char ch)
{
    return strrchr(str, ch);
}

void pkgi_memcpy(void* dst, const void* src, uint32_t size)
{
    memcpy(dst, src, size);
}

void pkgi_memmove(void* dst, const void* src, uint32_t size)
{
    memmove(dst, src, size);
}

int pkgi_memequ(const void* a, const void* b, uint32_t size)
{
    return memcmp(a, b, size) == 0;
}

int pkgi_is_unsafe_mode(void)
{
    return 1;
}

const char* pkgi_get_temp_folder(void)
{
    return "tmp";
}

int pkgi_mkdirs(char* path)
{
    char* ptr = path;
    while (*ptr)
    {
        while (*ptr && *ptr != '/')
        {
            ptr++;
        }
        char last = *ptr;
        *ptr = 0;
        int ok = mkdir(path, 0755);
        if (ok < 0 && errno != EEXIST)
        {
            return 0;
        }
        if (last == 0)
        {
            break;
        }
        *ptr++ = last;
    }

    return 1;
}

void pkgi_rm(const char* file)
{
    unlink(file);
}

int pkgi_save(const char* name, const void* data, uint32_t size)
{
    int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return 0;

    int ret = 1;
    const char* data8 = static_cast<const char*>(data);
    while (size != 0)
    {
        int written = write(fd, data8, size);
        if (written <= 0)
        {
            ret = 0;
            break;
        }
        data8 += written;
        size -= written;
    }

    close(fd);
    return ret;
}

void* pkgi_create(const char* path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
    {
        return NULL;
    }

    return (void*)(intptr_t)fd;
}

int pkgi_read(void* f, void* buffer, uint32_t size)
{
    return read((intptr_t)f, buffer, size);
}

int pkgi_write(void* f, const void* buffer, uint32_t size)
{
    return write((intptr_t)f, buffer, size);
}

void pkgi_close(void* f)
{
    close((intptr_t)f);
}

uint32_t pkgi_time_msec()
{
    return time(NULL) * 1000;
}
