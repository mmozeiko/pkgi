#include "file.hpp"

#include "log.hpp"

#include <fmt/format.h>

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/net/net.h>

#include <algorithm>

#define PKGI_ERRNO_EEXIST (int)(0x80010000 + SCE_NET_EEXIST)

void pkgi_mkdirs(const char* ppath)
{
    std::string path = ppath;
    path.push_back('/');
    auto ptr = path.begin();
    while (true)
    {
        ptr = std::find(ptr, path.end(), '/');
        if (ptr == path.end())
            break;

        char last = *ptr;
        *ptr = 0;
        LOG("mkdir %s", path.c_str());
        int err = sceIoMkdir(path.c_str(), 0777);
        if (err < 0 && err != PKGI_ERRNO_EEXIST)
            throw std::runtime_error(fmt::format(
                    "sceIoMkdir({}) failed:\n{:#08x}",
                    path.c_str(),
                    static_cast<uint32_t>(err)));
        *ptr = last;
        ++ptr;
    }
}

void pkgi_rm(const char* file)
{
    int err = sceIoRemove(file);
    if (err < 0)
    {
        LOG("error removing %s file, err=0x%08x", file, err);
    }
}

int64_t pkgi_get_size(const char* path)
{
    SceIoStat stat;
    int err = sceIoGetstat(path, &stat);
    if (err < 0)
    {
        LOG("cannot get size of %s, err=0x%08x", path, err);
        return -1;
    }
    return stat.st_size;
}

int pkgi_file_exists(const char* path)
{
    SceIoStat stat;
    return sceIoGetstat(path, &stat) >= 0;
}

void pkgi_rename(const char* from, const char* to)
{
    int res = sceIoRename(from, to);
    if (res < 0)
        throw std::runtime_error(fmt::format(
                "failed to rename from {} to {}:\n{:#08x}",
                from,
                to,
                static_cast<uint32_t>(res)));
}

void* pkgi_create(const char* path)
{
    LOG("sceIoOpen create on %s", path);
    SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0)
    {
        LOG("cannot create %s, err=0x%08x", path, fd);
        return NULL;
    }
    LOG("sceIoOpen returned fd=%d", fd);

    return (void*)(intptr_t)fd;
}

void* pkgi_openrw(const char* path)
{
    LOG("sceIoOpen openrw on %s", path);
    SceUID fd = sceIoOpen(path, SCE_O_RDWR, 0777);
    if (fd < 0)
    {
        LOG("cannot openrw %s, err=0x%08x", path, fd);
        return NULL;
    }
    LOG("sceIoOpen returned fd=%d", fd);

    return (void*)(intptr_t)fd;
}

void* pkgi_append(const char* path)
{
    LOG("sceIoOpen append on %s", path);
    SceUID fd =
            sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd < 0)
    {
        LOG("cannot append %s, err=0x%08x", path, fd);
        return NULL;
    }
    LOG("sceIoOpen returned fd=%d", fd);

    return (void*)(intptr_t)fd;
}

int64_t pkgi_seek(void* f, uint64_t offset)
{
    auto const pos = sceIoLseek((intptr_t)f, offset, SCE_SEEK_SET);
    if (pos < 0)
        throw formatEx<std::runtime_error>(
                "sceIoLseek error {:#08x}", static_cast<uint32_t>(pos));
    return pos;
}

int pkgi_read(void* f, void* buffer, uint32_t size)
{
    const auto read = sceIoRead((SceUID)(intptr_t)f, buffer, size);
    if (read < 0)
        throw formatEx<std::runtime_error>(
                "sceIoRead error {:#08x}", static_cast<uint32_t>(read));
    return read;
}

int pkgi_write(void* f, const void* buffer, uint32_t size)
{
    int write = sceIoWrite((SceUID)(intptr_t)f, buffer, size);
    if (write < 0)
        throw formatEx<std::runtime_error>(
                "sceIoWrite error {:#08x}", static_cast<uint32_t>(write));

    return write;
}

void pkgi_close(void* f)
{
    SceUID fd = (SceUID)(intptr_t)f;
    LOG("closing file %d", fd);
    int err = sceIoClose(fd);
    if (err < 0)
    {
        LOG("close error 0x%08x", err);
    }
}

std::vector<uint8_t> pkgi_load(const std::string& path)
{
    SceUID fd = sceIoOpen(path.c_str(), SCE_O_RDONLY, 0777);
    if (fd < 0)
        throw std::runtime_error(fmt::format(
                "sceIoOpen({}) failed:\n{:#08x}",
                path.c_str(),
                static_cast<uint32_t>(fd)));

    const auto size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);

    std::vector<uint8_t> data(size);

    const auto read = sceIoRead(fd, data.data(), data.size());
    if (read < 0)
        throw std::runtime_error(fmt::format(
                "sceIoRead({}) failed:\n{:#08x}",
                path.c_str(),
                static_cast<uint32_t>(read)));

    data.resize(read);

    sceIoClose(fd);

    return data;
}

int pkgi_save(const char* name, const void* data, uint32_t size)
{
    SceUID fd = sceIoOpen(name, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0)
    {
        return 0;
    }

    int ret = 1;
    const char* data8 = static_cast<const char*>(data);
    while (size != 0)
    {
        int written = sceIoWrite(fd, data8, size);
        if (written <= 0)
        {
            ret = 0;
            break;
        }
        data8 += written;
        size -= written;
    }

    sceIoClose(fd);
    return ret;
}
