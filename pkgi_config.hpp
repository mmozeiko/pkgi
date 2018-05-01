#pragma once

#include "pkgi_db.h"

#include <string>

typedef struct Config
{
    DbSort sort;
    DbSortOrder order;
    uint32_t filter;
    int no_version_check;

    std::string games_url;
    std::string updates_url;
    std::string dlcs_url;
    std::string psx_games_url;
    std::string psp_games_url;
} Config;

Config pkgi_load_config();
void pkgi_save_config(const Config& config);
