#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <cstdint>

#include "http.hpp"

class FileDownload
{
public:
    FileDownload(std::unique_ptr<Http> http);

    void download(
            const std::string& partition,
            const std::string& titleid,
            const std::string& url);

private:
    std::string root;

    std::unique_ptr<Http> _http;
    uint64_t download_size;
    uint64_t download_offset;
    std::string download_url;

    void* item_file;

    void start_download();
    void download_data(uint32_t size);
    void download_file();
};
