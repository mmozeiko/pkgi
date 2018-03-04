#include "pkgi_downloader.hpp"

#include "pkgi_download.hpp"

Downloader::Downloader()
    : _cond("downloader_cond"), _thread("downloader_thread", [this] { run(); })
{
    LOG("new downloader");
}

Downloader::~Downloader()
{
    LOG("destroying downloader");
    _dying = true;
    _cond.notify_one();
    _thread.join();
    LOG("downloader destroyed");
}

void Downloader::add(const DownloadItem& d)
{
    LOG("adding download %s", d.name.c_str());
    {
        ScopeLock _(_cond.get_mutex());
        _queue.push(d);
    }
    _cond.notify_one();
}

void Downloader::run()
{
    while (true)
    {
        DownloadItem item;
        {
            ScopeLock _(_cond.get_mutex());

            if (_dying)
                return;
            else if (!_queue.empty())
            {
                item = _queue.front();
                _queue.pop();
            }
            else
                _cond.wait();
        }

        if (!item.content.empty())
            do_download(item);
    }
}

void Downloader::do_download(const DownloadItem& item)
{
    ScopeProcessLock _;
    LOG("downloading %s", item.name.c_str());
    auto download = std::make_unique<Download>();
    download->update_progress_cb = [](auto&&) {};
    download->update_status = [](auto&&) {};
    download->is_canceled = [this] { return _dying; };
    if (!download->pkgi_download(
                item.content.c_str(),
                item.url.c_str(),
                item.rif.empty() ? nullptr : item.rif.data(),
                item.digest.data()))
        return;
    LOG("download of %s completed!", item.name.c_str());
    if (!pkgi_install(item.content.substr(7, 9).c_str()))
        return;
    LOG("install of %s completed!", item.name.c_str());
}
