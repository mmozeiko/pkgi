#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <vector>

#include "pkgi_thread.hpp"

enum Type
{
    Game,
    Update,
    Dlc,
};

struct DownloadItem
{
    Type type;
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
    void remove_from_queue(const std::string& contentid);
    bool is_in_queue(const std::string& titleid);
    std::optional<DownloadItem> get_current_download();

    std::function<void(const std::string& content)> refresh;

private:
    using ScopeLock = std::lock_guard<Mutex>;

    Cond _cond;
    std::deque<DownloadItem> _queue;

    DownloadItem _current_download;
    bool _cancel_current = false;

    Thread _thread;
    bool _dying = false;

    void run();
    void do_download(const DownloadItem& item);
};
