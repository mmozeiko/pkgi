#pragma once

#include "pkgi_http.hpp"

extern "C" {
#include "pkgi.h"
}

class VitaHttp : public Http
{
public:
    ~VitaHttp();

    void start(const std::string& url, uint64_t offset) override;
    int64_t read(uint8_t* buffer, uint64_t size) override;

    int64_t get_length() override;

    explicit operator bool() const override;

private:
    pkgi_http* _http = nullptr;
};
