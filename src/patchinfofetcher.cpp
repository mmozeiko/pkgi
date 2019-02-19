#include "patchinfofetcher.hpp"

#include "vitahttp.hpp"

#include <mutex>

PatchInfoFetcher::PatchInfoFetcher(std::string title_id)
    : _mutex("patch_info_fetcher_mutex")
    , _title_id(std::move(title_id))
    , _thread("patch_info_fetcher", [this] { do_request(); })
{
}

PatchInfoFetcher::~PatchInfoFetcher()
{
    Http* http;
    {
        std::lock_guard<Mutex> lock(_mutex);
        _abort = true;
        http = _http.get();
    }
    if (http)
        http->abort();
    _thread.join();
}

PatchInfoFetcher::Status PatchInfoFetcher::get_status()
{
    std::lock_guard<Mutex> lock(_mutex);
    return _status;
}

std::optional<PatchInfo> PatchInfoFetcher::get_patch_info()
{
    std::lock_guard<Mutex> lock(_mutex);
    return _patch_info;
}

void PatchInfoFetcher::do_request()
{
    try
    {
        {
            std::lock_guard<Mutex> lock(_mutex);
            if (_abort)
                return;
            _http = std::make_unique<VitaHttp>();
        }
        const auto patch_info =
                pkgi_download_patch_info(_http.get(), _title_id);
        {
            std::lock_guard<Mutex> lock(_mutex);
            if (!patch_info)
                _status = Status::NoUpdate;
            else
                _status = Status::Found;
            _patch_info = patch_info;
            _http = nullptr;
        }
    }
    catch (const std::exception& e)
    {
        LOGF("Failed to fetch patch info: {}", e.what());
        std::lock_guard<Mutex> lock(_mutex);
        _status = Status::Error;
        _http = nullptr;
    }
}
