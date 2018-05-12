#pragma once

#include <string>

#include <cstdint>

typedef enum {
    PresenceUnknown,
    PresenceIncomplete,
    PresenceInstalling,
    PresenceInstalled,
    PresenceMissing,
    PresenceGamePresent,
} DbPresence;

typedef enum {
    SortByTitle,
    SortByRegion,
    SortByName,
    SortBySize,
} DbSort;

typedef enum {
    SortAscending,
    SortDescending,
} DbSortOrder;

typedef enum {
    DbFilterRegionASA = 0x01,
    DbFilterRegionEUR = 0x02,
    DbFilterRegionJPN = 0x04,
    DbFilterRegionUSA = 0x08,

    // TODO: implement these two
    DbFilterInstalled = 0x10,
    DbFilterMissing = 0x20,

    DbFilterAllRegions = DbFilterRegionUSA | DbFilterRegionEUR |
                         DbFilterRegionJPN | DbFilterRegionASA,
    DbFilterAll = DbFilterAllRegions | DbFilterInstalled | DbFilterMissing,
} DbFilter;

typedef struct
{
    DbPresence presence;
    const char* content;
    uint32_t flags;
    const char* name;
    const char* name_org;
    const char* zrif;
    const char* url;
    const uint8_t* digest;
    int64_t size;
} DbItem;

typedef enum {
    RegionASA,
    RegionEUR,
    RegionJPN,
    RegionUSA,
    RegionUnknown,
} GameRegion;

typedef enum {
    ModeGames,
    ModeUpdates,
    ModeDlcs,
    ModePsxGames,
    ModePspGames,
} Mode;

typedef struct Config Config;

class TitleDatabase
{
public:
    void update(const char* update_url, Mode mode);
    void get_update_status(uint32_t* updated, uint32_t* total);

    void configure(const char* search, const Config* config);

    uint32_t count();
    uint32_t total();
    DbItem* get(uint32_t index);
    DbItem* get_by_content(const char* content);

    Mode get_mode();

private:
    static constexpr auto MAX_DB_SIZE = 4 * 1024 * 1024;
    static constexpr auto MAX_DB_ITEMS = 8192;

    Mode mode;
    std::string db_data;
    uint32_t db_total;
    uint32_t db_size;

    DbItem db[MAX_DB_ITEMS];
    uint32_t db_count;

    DbItem* db_item[MAX_DB_SIZE];
    uint32_t db_item_count;

    void parse_tsv_file();
    void swap(uint32_t a, uint32_t b);
    void heapify(
            uint32_t n,
            uint32_t index,
            DbSort sort,
            DbSortOrder order,
            uint32_t filter);
};

GameRegion pkgi_get_region(const char* content);
