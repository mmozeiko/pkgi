#pragma once

#include "http.hpp"
#include "patchinfo.hpp"
#include "thread.hpp"

#include <optional>

class PatchInfoFetcher
{
public:
    enum class Status
    {
        Fetching,
        NoUpdate,
        Found,
        Error,
    };

    PatchInfoFetcher(std::string title_id);
    ~PatchInfoFetcher();

    Status get_status();
    std::optional<PatchInfo> get_patch_info();

private:
    Mutex _mutex;

    std::string _title_id;

    bool _abort{false};
    Status _status{Status::Fetching};
    std::unique_ptr<Http> _http;
    std::optional<PatchInfo> _patch_info;

    Thread _thread;

    void do_request();
};
