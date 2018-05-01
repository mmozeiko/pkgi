extern "C" {
#include "pkgi_db.h"
#include "pkgi.h"
#include "pkgi_sha256.h"
#include "pkgi_utils.h"
}
#include "pkgi_config.hpp"
#include "pkgi_vitahttp.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

#include <stddef.h>

#define MAX_DB_SIZE (4 * 1024 * 1024)
#define MAX_DB_ITEMS 8192

static Mode mode;
static std::string db_data;
static uint32_t db_total;
static uint32_t db_size;

static DbItem db[MAX_DB_ITEMS];
static uint32_t db_count;

static DbItem* db_item[MAX_DB_SIZE];
static uint32_t db_item_count;

static int64_t pkgi_strtoll(const char* str)
{
    int64_t res = 0;
    const char* s = str;
    if (*s && *s == '-')
    {
        s++;
    }
    while (*s)
    {
        res = res * 10 + (*s - '0');
        s++;
    }

    return str[0] == '-' ? -res : res;
}

static uint8_t hexvalue(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    else if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    return 0;
}

static uint8_t* pkgi_hexbytes(const char* digest, uint32_t length)
{
    uint8_t* result = (uint8_t*)digest;

    for (uint32_t i = 0; i < length; i++)
    {
        char ch1 = digest[2 * i];
        char ch2 = digest[2 * i + 1];
        if (ch1 == 0 || ch2 == 0)
        {
            return NULL;
        }

        result[i] = hexvalue(ch1) * 16 + hexvalue(ch2);
    }

    return result;
}

static void parse_tsv_file()
{
    char* ptr = db_data.data();
    char* end = db_data.data() + db_data.size();

    // skip header
    while (ptr < end && *ptr != '\r')
        ptr++;
    ptr++; // \r
    ptr++; // \n

    while (ptr < end && *ptr)
    {
#define NEXT_FIELD()                      \
    do                                    \
    {                                     \
        while (ptr < end && *ptr != '\t') \
            ptr++;                        \
        if (ptr == end)                   \
            break;                        \
        *ptr++ = 0;                       \
    } while (0)

        const char* zrif = "";
        const char* content = "";
        const char* name_org = "";

        NEXT_FIELD(); // Title ID
        NEXT_FIELD(); // Region

        if (mode == ModePspGames)
        {
            NEXT_FIELD(); // Type
        }

        const char* name = ptr;
        NEXT_FIELD();

        if (mode == ModeUpdates)
        {
            NEXT_FIELD(); // Update Version
            NEXT_FIELD(); // FW VERSION
        }

        const char* url = ptr;
        NEXT_FIELD();

        if (mode == ModeUpdates)
        {
            NEXT_FIELD(); // NoNPDRM mirror
        }

        if (mode == ModeGames || mode == ModeDlcs)
        {
            zrif = ptr;
            NEXT_FIELD();

            content = ptr;
            NEXT_FIELD();
        }
        else if (mode == ModePsxGames || mode == ModePspGames)
        {
            content = ptr;
            NEXT_FIELD();
        }

        NEXT_FIELD(); // Last Modification Date

        if (mode == ModeGames || mode == ModePsxGames)
        {
            name_org = ptr;
            NEXT_FIELD();
        }
        else if (mode == ModePspGames)
        {
            NEXT_FIELD(); // RAP
            NEXT_FIELD(); // Download RAP
        }

        const char* size = ptr;
        NEXT_FIELD();

        const char* digest = ptr;
        while (ptr < end && *ptr != '\n' && *ptr != '\r')
            ptr++;
        if (ptr != end)
        {
            *ptr++ = 0;

            if (*ptr == '\n')
                ptr++;
        }

        if (*url == '\0' || std::string(url) == "MISSING" ||
            std::string(url) == "CART ONLY" || std::string(zrif) == "MISSING")
            continue;

        if (mode == ModeUpdates)
        {
            std::reverse_iterator<const char*> rbegin(url + strlen(url) - 1);
            std::reverse_iterator<const char*> rend(url - 1);
            auto const it =
                    std::find_if(rbegin, rend, [](char c) { return c == '/'; });
            if (it != rend)
                content = it.base();
        }

        db[db_count].presence = PresenceUnknown;
        db[db_count].content = content;
        db[db_count].flags = 0;
        db[db_count].name = name;
        db[db_count].name_org = name_org[0] == 0 ? name : name_org;
        db[db_count].zrif = zrif[0] == 0 ? NULL : zrif;
        db[db_count].url = url;
        db[db_count].size = pkgi_strtoll(size);
        db[db_count].digest = pkgi_hexbytes(digest, SHA256_DIGEST_SIZE);
        db_item[db_count] = db + db_count;
        db_count++;

        if (db_count == MAX_DB_ITEMS)
            break;
#undef NEXT_FIELD
    }

    db_item_count = db_count;
}

void pkgi_db_update(const char* update_url, Mode amode)
{
    db_data.resize(MAX_DB_SIZE);
    db_total = 0;
    db_size = 0;
    db_count = 0;
    db_item_count = 0;

    mode = amode;

    char path[256];
    pkgi_snprintf(path, sizeof(path), "%s/pkgi.txt", pkgi_get_config_folder());

    LOG("loading update from %s", path);
    int loaded = pkgi_load(path, db_data.data(), db_data.size() - 1);
    if (loaded > 0)
    {
        db_size = loaded;
    }
    else if (update_url[0] != 0)
    {
        LOG("loading update from %s", update_url);

        VitaHttp http;
        http.start(update_url, 0);

        const auto length = http.get_length();

        if (length > (int64_t)db_data.size() - 1)
            throw std::runtime_error(
                    "list is too large... check for newer pkgj version");

        if (length != 0)
            db_total = (uint32_t)length;

        for (;;)
        {
            uint32_t want =
                    (uint32_t)min64(1 << 16, db_data.size() - 1 - db_size);
            int read = http.read(
                    reinterpret_cast<uint8_t*>(db_data.data()) + db_size, want);
            if (read == 0)
                break;
            db_size += read;
        }

        if (db_size == 0)
            throw std::runtime_error(
                    "list is empty... check for newer pkgi version");
    }
    else
    {
        throw std::runtime_error(
                "pkgi.txt file missing or bad config.txt file?");
    }

    LOG("parsing items");

    db_data.resize(db_size);
    parse_tsv_file();

    LOG("finished parsing, %u total items", db_count);
}

static void swap(uint32_t a, uint32_t b)
{
    DbItem* temp = db_item[a];
    db_item[a] = db_item[b];
    db_item[b] = temp;
}

static int matches(GameRegion region, uint32_t filter)
{
    return (region == RegionASA && (filter & DbFilterRegionASA)) ||
           (region == RegionEUR && (filter & DbFilterRegionEUR)) ||
           (region == RegionJPN && (filter & DbFilterRegionJPN)) ||
           (region == RegionUSA && (filter & DbFilterRegionUSA)) ||
           (region == RegionUnknown);
}

static int lower(
        const DbItem* a,
        const DbItem* b,
        DbSort sort,
        DbSortOrder order,
        uint32_t filter)
{
    GameRegion reg_a = pkgi_get_region(a->content);
    GameRegion reg_b = pkgi_get_region(b->content);

    int cmp = 0;
    if (sort == SortByTitle)
    {
        cmp = pkgi_stricmp(a->content + 7, b->content + 7) < 0;
    }
    else if (sort == SortByRegion)
    {
        cmp = reg_a == reg_b ? pkgi_stricmp(a->content + 7, b->content + 7) < 0
                             : reg_a < reg_b;
    }
    else if (sort == SortByName)
    {
        cmp = pkgi_stricmp(a->name, b->name) < 0;
    }
    else if (sort == SortBySize)
    {
        cmp = a->size < b->size;
    }

    int matches_a = matches(reg_a, filter);
    int matches_b = matches(reg_b, filter);

    if (matches_a == matches_b)
    {
        return order == SortAscending ? cmp : !cmp;
    }
    else if (matches_a)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void heapify(
        uint32_t n,
        uint32_t index,
        DbSort sort,
        DbSortOrder order,
        uint32_t filter)
{
    uint32_t largest = index;
    uint32_t left = 2 * index + 1;
    uint32_t right = 2 * index + 2;

    if (left < n && lower(db_item[largest], db_item[left], sort, order, filter))
    {
        largest = left;
    }

    if (right < n &&
        lower(db_item[largest], db_item[right], sort, order, filter))
    {
        largest = right;
    }

    if (largest != index)
    {
        swap(index, largest);
        heapify(n, largest, sort, order, filter);
    }
}

void pkgi_db_configure(const char* search, const Config* config)
{
    uint32_t search_count;
    if (!search)
    {
        search_count = db_count;
    }
    else
    {
        uint32_t write = 0;
        for (uint32_t read = 0; read < db_count; read++)
        {
            if (pkgi_stricontains(db_item[read]->name, search))
            {
                if (write < read)
                {
                    swap(read, write);
                }
                write++;
            }
        }
        search_count = write;
    }

    if (search_count == 0)
    {
        db_item_count = 0;
        return;
    }

    for (int i = search_count / 2 - 1; i >= 0; i--)
    {
        heapify(search_count, i, config->sort, config->order, config->filter);
    }

    for (int i = search_count - 1; i >= 0; i--)
    {
        swap(i, 0);
        heapify(i, 0, config->sort, config->order, config->filter);
    }

    if (config->filter == DbFilterAll)
    {
        db_item_count = search_count;
    }
    else
    {
        uint32_t low = 0;
        uint32_t high = search_count - 1;
        while (low <= high)
        {
            // this never overflows because of MAX_DB_ITEMS
            uint32_t middle = (low + high) / 2;

            GameRegion region = pkgi_get_region(db_item[middle]->content);
            if (matches(region, config->filter))
            {
                low = middle + 1;
            }
            else
            {
                if (middle == 0)
                {
                    break;
                }
                high = middle - 1;
            }
        }
        db_item_count = low;
    }
}

void pkgi_db_get_update_status(uint32_t* updated, uint32_t* total)
{
    *updated = db_size;
    *total = db_total;
}

uint32_t pkgi_db_count(void)
{
    return db_item_count;
}

uint32_t pkgi_db_total(void)
{
    return db_count;
}

DbItem* pkgi_db_get(uint32_t index)
{
    return index < db_item_count ? db_item[index] : NULL;
}

DbItem* pkgi_db_get_by_content(const char* content)
{
    for (size_t i = 0; i < db_item_count; ++i)
        if (pkgi_stricmp(db_item[i]->content, content) == 0)
            return db_item[i];
    return NULL;
}

GameRegion pkgi_get_region(const char* content)
{
    uint32_t first = get32le((uint8_t*)content + 7);

#define ID(a, b, c, d)                                    \
    (uint32_t)(                                           \
            ((uint8_t)(d) << 24) | ((uint8_t)(c) << 16) | \
            ((uint8_t)(b) << 8) | ((uint8_t)(a)))

    switch (first)
    {
    case ID('V', 'C', 'A', 'S'):
    case ID('P', 'C', 'S', 'H'):
    case ID('V', 'L', 'A', 'S'):
    case ID('P', 'C', 'S', 'D'):
    case ID('N', 'P', 'H', 'I'):
    case ID('N', 'P', 'H', 'J'):
    case ID('N', 'P', 'H', 'G'):
    case ID('N', 'P', 'H', 'H'):
    case ID('N', 'P', 'H', 'Z'):
    case ID('U', 'C', 'A', 'S'):
        return RegionASA;

    case ID('P', 'C', 'S', 'F'):
    case ID('P', 'C', 'S', 'B'):
    case ID('N', 'P', 'E', 'E'):
    case ID('N', 'P', 'E', 'F'):
    case ID('N', 'P', 'E', 'G'):
    case ID('N', 'P', 'E', 'H'):
    case ID('N', 'P', 'E', 'X'):
    case ID('N', 'P', 'E', 'Z'):
    case ID('U', 'C', 'E', 'S'):
    case ID('U', 'L', 'E', 'S'):
        return RegionEUR;

    case ID('P', 'C', 'S', 'C'):
    case ID('V', 'C', 'J', 'S'):
    case ID('P', 'C', 'S', 'G'):
    case ID('V', 'L', 'J', 'S'):
    case ID('V', 'L', 'J', 'M'):
    case ID('N', 'P', 'J', 'I'):
    case ID('N', 'P', 'J', 'J'):
    case ID('N', 'P', 'J', 'G'):
    case ID('N', 'P', 'J', 'H'):
    case ID('N', 'P', 'J', 'Q'):
        return RegionJPN;

    case ID('P', 'C', 'S', 'E'):
    case ID('P', 'C', 'S', 'A'):
    case ID('N', 'P', 'U', 'F'):
    case ID('N', 'P', 'U', 'I'):
    case ID('N', 'P', 'U', 'J'):
    case ID('N', 'P', 'U', 'G'):
    case ID('N', 'P', 'U', 'H'):
    case ID('N', 'P', 'U', 'X'):
    case ID('N', 'P', 'U', 'Z'):
    case ID('U', 'C', 'U', 'S'):
    case ID('U', 'L', 'U', 'S'):
        return RegionUSA;

    default:
        return RegionUnknown;
    }

#undef ID
}

Mode pkgi_db_get_mode()
{
    return mode;
}
