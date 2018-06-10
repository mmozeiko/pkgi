#pragma once

#include "pkgi_http.hpp"

#include <fstream>

class FileHttp : public Http
{
public:
    void start(const std::string& url, uint64_t offset) override;
    int64_t read(uint8_t* buffer, uint64_t size) override;

    int64_t get_length() override;

    explicit operator bool() const override;

private:
    std::ifstream f;
};
