#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include <stdint.h>

extern "C" {
#include "pkgi_aes128.h"
#include "pkgi_sha256.h"
}

#include "pkgi_http.hpp"

#define PKGI_RIF_SIZE 512
#define PKG_HEADER_SIZE 192
#define PKG_HEADER_EXT_SIZE 64
#define PKG_TAIL_SIZE 480

class DownloadError : public std::exception
{
public:
    DownloadError(std::string msg) : _msg(std::move(msg))
    {
    }

    virtual const char* what() const noexcept override
    {
        return _msg.c_str();
    }

private:
    std::string _msg;
};

class Download
{
public:
    Download(std::unique_ptr<Http> http);

    int pkgi_download(
            const char* content,
            const char* url,
            const uint8_t* rif,
            const uint8_t* digest);

    // private:
    // temporary unpack folder ux0:pkgi/TITLE
    char root[256];

    std::unique_ptr<Http> _http;
    const char* download_content;
    const char* download_url;

    uint64_t download_offset; // pkg absolute offset
    uint64_t download_size;   // pkg total size (from http request)

    uint8_t iv[AES_BLOCK_SIZE];
    aes128_ctx aes;
    sha256_ctx sha;

    void* item_file;     // current file handle
    char item_name[256]; // current file name
    char item_path[256]; // current file path
    int item_index;      // current item

    // head.bin contents, kept in memory while downloading
    uint8_t head[4 * 1024 * 1024];
    uint32_t head_size;

    // temporary buffer for downloads
    uint8_t down[64 * 1024];

    // pkg header
    uint32_t meta_offset;
    uint32_t meta_count;
    uint32_t index_count;
    uint64_t total_size;
    uint64_t enc_offset;
    uint64_t enc_size;
    uint32_t index_size;

    uint32_t content_type;

    // encrypted files
    uint64_t encrypted_base;   // offset in pkg where it starts
    uint64_t encrypted_offset; // offset from beginning of file
    uint64_t decrypted_size;   // size that's left to write into decrypted file

    // UI stuff
    uint32_t info_start;
    uint32_t info_update;
    std::function<void(const Download& dl)> update_progress_cb;
    std::function<void(const std::string& status)> update_status;
    std::function<bool()> is_canceled;

    void update_progress();
    void download_start(void);
    int download_data(uint8_t* buffer, uint32_t size, int encrypted, int save);
    int create_file(void);
    int download_head(const uint8_t* rif);
    void download_normal_file(uint64_t encrypted_size);
    void download_psp_file(uint64_t item_size);
    int download_files(void);
    int download_tail(void);
    int create_stat();
    int check_integrity(const uint8_t* digest);
    int create_rif(const uint8_t* rif);
};
