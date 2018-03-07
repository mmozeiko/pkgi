#pragma once

#include "pkgi_db.h"

typedef struct Config
{
    DbSort sort;
    DbSortOrder order;
    uint32_t filter;
    int no_version_check;
} Config;

void pkgi_load_config(
        Config* config,
        char* games_url,
        uint32_t games_len,
        char* updates_url,
        uint32_t updates_len,
        char* dlcs_url,
        uint32_t dlcs_len);
void pkgi_save_config(
        const Config* config,
        const char* games_url,
        const char* updates_url,
        const char* dlcs_url);
