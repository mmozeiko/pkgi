#pragma once

#include "http.hpp"

#include <fstream>
#include <string>

class FileHttp : public Http
{
public:
    FileHttp(const std::string& path = {});

    void start(const std::string& url, uint64_t offset) override;
    int64_t read(uint8_t* buffer, uint64_t size) override;
    void abort() override;

    int get_status() override;
    int64_t get_length() override;

    explicit operator bool() const override;

private:
    std::string override_path;
    std::ifstream f;
};
