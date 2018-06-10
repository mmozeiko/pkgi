#include "pkgi_filehttp.hpp"

void FileHttp::start(const std::string& url, uint64_t offset)
{
    f.open(url);
    f.seekg(offset, std::ios::beg);
}

int64_t FileHttp::read(uint8_t* buffer, uint64_t size)
{
    f.read(reinterpret_cast<char*>(buffer), size);
    return f.gcount();
}

int64_t FileHttp::get_length()
{
    const uint64_t pos = f.tellg();
    f.seekg(0, std::ios::end);
    const uint64_t size = f.tellg();
    f.seekg(pos, std::ios::beg);
    return size;
}

FileHttp::operator bool() const
{
    return f.is_open();
}
