#pragma once

#include "http.hpp"

#include <optional>
#include <string>

struct PatchInfo
{
    std::string version;
    std::string url;
};

std::optional<PatchInfo> pkgi_download_patch_info(
        Http* http, const std::string& titleid);
