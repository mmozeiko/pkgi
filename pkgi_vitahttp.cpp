#include "pkgi_vitahttp.hpp"

#include <fmt/format.h>

VitaHttp::~VitaHttp()
{
    if (_http)
        pkgi_http_close(_http);
}

void VitaHttp::start(const std::string& url, uint64_t offset)
{
    _http = pkgi_http_get(url.c_str(), nullptr, offset);
    if (!_http)
    {
        throw HttpError("cannot send HTTP request");
    }
}

int64_t VitaHttp::read(uint8_t* buffer, uint64_t size)
{
    const int64_t read = pkgi_http_read(_http, buffer, size);
    if (read < 0)
        throw HttpError(fmt::format(
                "HTTP download error {:#08x}",
                static_cast<uint32_t>(static_cast<int32_t>(read))));
    return read;
}

int64_t VitaHttp::get_length()
{
    int64_t http_length;
    pkgi_http_response_length(_http, &http_length);
    return http_length;
}

VitaHttp::operator bool() const
{
    return _http;
}
