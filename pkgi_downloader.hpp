#pragma once

#include <mutex>
#include <queue>
#include <vector>

#include "pkgi_thread.hpp"

struct DownloadItem
{
    std::string name;
    std::string content;
    std::string url;
    std::vector<uint8_t> rif;
    std::vector<uint8_t> digest;
};

class Downloader
{
public:
    Downloader(const Downloader&) = delete;
    Downloader(Downloader&&) = delete;
    Downloader& operator=(const Downloader&) = delete;
    Downloader& operator=(Downloader&&) = delete;

    Downloader();
    ~Downloader();

    void add(const DownloadItem& d);

private:
    using ScopeLock = std::lock_guard<Mutex>;

    Cond _cond;
    std::queue<DownloadItem> _queue;

    Thread _thread;
    bool _dying = false;

    void run();
    void do_download(const DownloadItem& item);
};
