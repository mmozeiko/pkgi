#include "db.hpp"

extern "C" {
#include "pkgi.h"
#include "sha256.h"
#include "utils.h"
}
#include "config.hpp"

#include <fmt/format.h>

#include <boost/scope_exit.hpp>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

#include <stddef.h>

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

static std::array<uint8_t, 32> pkgi_hexbytes(
        const char* digest, uint32_t length)
{
    std::array<uint8_t, 32> result;

    for (uint32_t i = 0; i < length; i++)
    {
        char ch1 = digest[2 * i];
        char ch2 = digest[2 * i + 1];
        if (ch1 == 0 || ch2 == 0)
            return result;

        result[i] = hexvalue(ch1) * 16 + hexvalue(ch2);
    }

    return result;
}

#define SQLITE_CHECK(call, errstr)                                     \
    do                                                                 \
    {                                                                  \
        auto err = call;                                               \
        if (err != SQLITE_OK)                                          \
            throw std::runtime_error(fmt::format(errstr ": {}", err)); \
    } while (false)

#define SQLITE_EXEC(handle, statement, errstr)                            \
    do                                                                    \
    {                                                                     \
        char* errmsg;                                                     \
        auto err = sqlite3_exec(                                          \
                handle.get(), statement, nullptr, nullptr, &errmsg);      \
        if (err != SQLITE_OK)                                             \
            throw std::runtime_error(fmt::format(errstr ": {}", errmsg)); \
    } while (false)

TitleDatabase::TitleDatabase(Mode mode, std::string const& dbPath) : mode(mode)
{
    LOG("opening database %s", dbPath.c_str());
    sqlite3* db;
    SQLITE_CHECK(sqlite3_open(dbPath.c_str(), &db), "can't open database");
    _sqliteDb.reset(db);

    SQLITE_EXEC(_sqliteDb, R"(
        CREATE TABLE IF NOT EXISTS titles (
            id INT PRIMARY KEY,
            content TEXT NOT NULL,
            name TEXT NOT NULL,
            name_org TEXT,
            zrif TEXT,
            url TEXT NOT NULL,
            digest BLOB,
            size INT,
            fw_version TEXT,
            last_modification DATETIME,
            region TEXT NOT NULL
        ))", "can't create table");
}

namespace
{
std::vector<const char*> pkgi_split_row(char** pptr, const char* end)
{
    auto& ptr = *pptr;

    std::vector<const char*> result;
    while (ptr != end && *ptr != '\n')
    {
        const char* field = ptr;
        while (ptr != end && *ptr != '\t' && *ptr != '\r')
            ++ptr;
        if (ptr == end)
        {
            result.push_back(field);
            break;
        }
        *ptr++ = 0;
        result.push_back(field);

        if (ptr == end)
        {
            result.push_back(field);
            break;
        }
    }
    while (ptr != end && *ptr++ != '\n')
        ;
    return result;
}

enum class Column
{
    Region,
    Content,
    Name,
    NameOrg,
    Zrif,
    Url,
    Digest,
    Size,
    FwVersion,
    LastModification,
};

int pkgi_get_column_number(Mode mode, Column column)
{
#define MAP_COL(name, i) \
    case Column::name:   \
        return i

    switch (mode)
    {
    case ModeGames:
        switch (column)
        {
            MAP_COL(Region, 1);
            MAP_COL(Name, 2);
            MAP_COL(Url, 3);
            MAP_COL(Zrif, 4);
            MAP_COL(Content, 5);
            MAP_COL(LastModification, 6);
            MAP_COL(NameOrg, 7);
            MAP_COL(Size, 8);
            MAP_COL(Digest, 9);
            MAP_COL(FwVersion, 10);
        default:
            throw std::runtime_error("invalid column");
        }
    case ModeUpdates:
        switch (column)
        {
            MAP_COL(Region, 1);
            MAP_COL(Name, 2);
            MAP_COL(Url, 5);
            MAP_COL(LastModification, 7);
            MAP_COL(Size, 8);
            MAP_COL(Digest, 9);
            MAP_COL(Content, -1);
            MAP_COL(NameOrg, -1);
            MAP_COL(Zrif, -1);
            MAP_COL(FwVersion, -1);
        default:
            throw std::runtime_error("invalid column");
        }
    case ModeDlcs:
        switch (column)
        {
            MAP_COL(Region, 1);
            MAP_COL(Name, 2);
            MAP_COL(Url, 3);
            MAP_COL(Zrif, 4);
            MAP_COL(Content, 5);
            MAP_COL(LastModification, 6);
            MAP_COL(Size, 7);
            MAP_COL(Digest, 8);
            MAP_COL(NameOrg, -1);
            MAP_COL(FwVersion, -1);
        default:
            throw std::runtime_error("invalid column");
        }
    case ModePsxGames:
        switch (column)
        {
            MAP_COL(Region, 1);
            MAP_COL(Name, 2);
            MAP_COL(Url, 3);
            MAP_COL(Content, 4);
            MAP_COL(LastModification, 5);
            MAP_COL(NameOrg, 6);
            MAP_COL(Size, 7);
            MAP_COL(Digest, 8);
            MAP_COL(Zrif, -1);
            MAP_COL(FwVersion, -1);
        default:
            throw std::runtime_error("invalid column");
        }
    case ModePspGames:
        switch (column)
        {
            MAP_COL(Region, 1);
            MAP_COL(Name, 3);
            MAP_COL(Url, 4);
            MAP_COL(Content, 5);
            MAP_COL(LastModification, 6);
            MAP_COL(Size, 9);
            MAP_COL(Digest, 10);
            MAP_COL(FwVersion, -1);
            MAP_COL(NameOrg, -1);
            MAP_COL(Zrif, -1);
        default:
            throw std::runtime_error("invalid column");
        }
    default:
        throw std::runtime_error("invalid mode");
    }
#undef MAP_COL
}

const char* get_or_empty(
        Mode mode, std::vector<const char*> const& v, Column column)
{
    const auto pos = pkgi_get_column_number(mode, column);
    if (pos < 0)
        return "";
    return v.at(pos);
}
}

void TitleDatabase::parse_tsv_file(std::string& db_data)
{
    SQLITE_EXEC(_sqliteDb, "BEGIN", "can't begin transaction");

    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (!std::uncaught_exception())
            SQLITE_EXEC(_sqliteDb, "END", "can't end transaction");
        else
        {
            char* errmsg;
            auto err = sqlite3_exec(
                    _sqliteDb.get(), "ROLLBACK", nullptr, nullptr, &errmsg);
            if (err != SQLITE_OK)
                LOG("sqlite error: %s", errmsg);
        }
    };

    SQLITE_EXEC(_sqliteDb, "DELETE FROM titles", "can't truncate table");

    sqlite3_stmt* stmt;
    SQLITE_CHECK(
            sqlite3_prepare_v2(
                    _sqliteDb.get(),
                    R"(INSERT INTO titles
                    (content, name, name_org, zrif, url, digest, size, fw_version, last_modification, region)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?))",
                    -1,
                    &stmt,
                    nullptr),
            "can't prepare SQL statement");
    BOOST_SCOPE_EXIT_ALL(&)
    {
        sqlite3_finalize(stmt);
    };

    char* ptr = db_data.data();
    char* end = db_data.data() + db_data.size();

    // skip header
    while (ptr < end && *ptr != '\n')
        ptr++;
    if (ptr == end)
        return;
    ptr++; // \n

    while (ptr < end && *ptr)
    {
        const auto fields = pkgi_split_row(&ptr, end);

        auto content = get_or_empty(mode, fields, Column::Content);
        const auto region = get_or_empty(mode, fields, Column::Region);
        const auto name = get_or_empty(mode, fields, Column::Name);
        const auto name_org = get_or_empty(mode, fields, Column::NameOrg);
        const auto url = get_or_empty(mode, fields, Column::Url);
        const auto zrif = get_or_empty(mode, fields, Column::Zrif);
        const auto digest = get_or_empty(mode, fields, Column::Digest);
        const auto size = get_or_empty(mode, fields, Column::Size);
        const auto fw_version = get_or_empty(mode, fields, Column::FwVersion);
        const auto last_modification =
                get_or_empty(mode, fields, Column::LastModification);

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

        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, content, strlen(content), nullptr);
        sqlite3_bind_text(stmt, 2, name, strlen(name), nullptr);
        sqlite3_bind_text(stmt, 3, name_org, strlen(name_org), nullptr);
        sqlite3_bind_text(stmt, 4, zrif, strlen(zrif), nullptr);
        sqlite3_bind_text(stmt, 5, url, strlen(url), nullptr);
        std::array<uint8_t, 32> digest_array;
        if (std::all_of(
                    digest, digest + 64, [](const auto c) { return c != 0; }))
        {
            digest_array = pkgi_hexbytes(digest, SHA256_DIGEST_SIZE);
            sqlite3_bind_blob(
                    stmt, 6, digest_array.data(), digest_array.size(), nullptr);
        }
        else
            sqlite3_bind_null(stmt, 6);
        sqlite3_bind_text(stmt, 7, size, strlen(size), nullptr);
        sqlite3_bind_text(stmt, 8, fw_version, strlen(fw_version), nullptr);
        sqlite3_bind_text(
                stmt, 9, last_modification, strlen(last_modification), nullptr);
        sqlite3_bind_text(stmt, 10, region, strlen(region), nullptr);

        auto err = sqlite3_step(stmt);
        if (err != SQLITE_DONE)
            throw std::runtime_error(
                    fmt::format("can't execute SQL statement: {}", err));
#undef NEXT_FIELD
    }
}

void TitleDatabase::update(Http* http, const char* update_url)
{
    std::string db_data;
    db_data.resize(MAX_DB_SIZE);
    db_total = 0;
    db_size = 0;
    db_item_count = 0;

    if (update_url[0] == 0)
        throw std::runtime_error("no update url");

    LOG("loading update from %s", update_url);

    http->start(update_url, 0);

    const auto length = http->get_length();

    if (length > (int64_t)db_data.size() - 1)
        throw std::runtime_error(
                "list is too large... check for newer pkgj version");

    if (length != 0)
        db_total = (uint32_t)length;

    for (;;)
    {
        uint32_t want = (uint32_t)min64(1 << 16, db_data.size() - 1 - db_size);
        int read = http->read(
                reinterpret_cast<uint8_t*>(db_data.data()) + db_size, want);
        if (read == 0)
            break;
        db_size += read;
    }

    if (db_size == 0)
        throw std::runtime_error(
                "list is empty... check for newer pkgi version");

    LOG("parsing items");

    db_data.resize(db_size);
    parse_tsv_file(db_data);

    LOG("finished parsing");
}

void TitleDatabase::reload()
{
    sqlite3_stmt* stmt;
    SQLITE_CHECK(
            sqlite3_prepare_v2(
                    _sqliteDb.get(),
                    R"(SELECT * FROM titles)",
                    -1,
                    &stmt,
                    nullptr),
            "can't prepare SQL statement");
    BOOST_SCOPE_EXIT_ALL(&)
    {
        sqlite3_finalize(stmt);
    };

    db_item_count = 0;
    db.clear();
    while (true)
    {
        auto const err = sqlite3_step(stmt);
        if (err == SQLITE_DONE)
            break;
        if (err != SQLITE_ROW)
            throw std::runtime_error(
                    fmt::format("can't execute SQL statement: {}", err));

        std::string content =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* name =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* name_org =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* zrif =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* url =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const auto bdigest =
                static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 6));
        const auto digest_size = sqlite3_column_bytes(stmt, 6);
        std::array<uint8_t, 32> digest;
        if (bdigest)
            std::copy_n(
                    bdigest,
                    std::min<int>(digest_size, digest.size()),
                    digest.begin());
        const auto size = sqlite3_column_int64(stmt, 7);

        db.push_back(DbItem{
                PresenceUnknown,
                content.size() >= 7 + 9 ? content.substr(7, 9) : "",
                content,
                0,
                name,
                name_org ? name_org : "",
                zrif ? zrif : "",
                url,
                static_cast<bool>(bdigest),
                bdigest ? digest : std::array<uint8_t, 32>{},
                size,
        });

        db_item[db.size() - 1] = db.size() - 1;
    }

    db_item_count = db.size();
    LOG("reloaded %d items", db_item_count);
}

void TitleDatabase::swap(uint32_t a, uint32_t b)
{
    auto const temp = db_item[a];
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
    GameRegion reg_a = pkgi_get_region(a->titleid);
    GameRegion reg_b = pkgi_get_region(b->titleid);

    int cmp = 0;
    if (sort == SortByTitle)
    {
        cmp = a->titleid < b->titleid;
    }
    else if (sort == SortByRegion)
    {
        cmp = reg_a == reg_b ? a->titleid < b->titleid : reg_a < reg_b;
    }
    else if (sort == SortByName)
    {
        cmp = pkgi_stricmp(a->name.c_str(), b->name.c_str()) < 0;
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

void TitleDatabase::heapify(
        uint32_t n,
        uint32_t index,
        DbSort sort,
        DbSortOrder order,
        uint32_t filter)
{
    uint32_t largest = index;
    uint32_t left = 2 * index + 1;
    uint32_t right = 2 * index + 2;

    if (left < n &&
        lower(&db[db_item[largest]], &db[db_item[left]], sort, order, filter))
    {
        largest = left;
    }

    if (right < n &&
        lower(&db[db_item[largest]], &db[db_item[right]], sort, order, filter))
    {
        largest = right;
    }

    if (largest != index)
    {
        swap(index, largest);
        heapify(n, largest, sort, order, filter);
    }
}

void TitleDatabase::configure(const char* search, const Config* config)
{
    uint32_t search_count;
    if (!search)
    {
        search_count = db.size();
    }
    else
    {
        uint32_t write = 0;
        for (uint32_t read = 0; read < db.size(); read++)
        {
            if (pkgi_stricontains(db[db_item[read]].name.c_str(), search))
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

            GameRegion region = pkgi_get_region(db[db_item[middle]].titleid);
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

void TitleDatabase::get_update_status(uint32_t* updated, uint32_t* total)
{
    *updated = db_size;
    *total = db_total;
}

uint32_t TitleDatabase::count(void)
{
    return db_item_count;
}

uint32_t TitleDatabase::total(void)
{
    return db.size();
}

DbItem* TitleDatabase::get(uint32_t index)
{
    return index < db_item_count ? &db[db_item[index]] : NULL;
}

DbItem* TitleDatabase::get_by_content(const char* content)
{
    for (size_t i = 0; i < db_item_count; ++i)
        if (db[db_item[i]].content == content)
            return &db[db_item[i]];
    return NULL;
}

GameRegion pkgi_get_region(const std::string& titleid)
{
    if (titleid.size() < 4)
        return RegionUnknown;

    uint32_t first = get32le((uint8_t*)titleid.c_str());

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
