#pragma once

#include "http.hpp"
#include "sqlite.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <cstdint>

class CompPackDatabase
{
public:
    struct Item
    {
        std::string path;
        std::string app_version;
    };

    CompPackDatabase(const std::string& dbPath);

    void update(Http* http, const std::string& update_url);

    std::optional<Item> get(const std::string& titleid);

private:
    static constexpr auto MAX_DB_SIZE = 4 * 1024 * 1024;

    std::string _dbPath;

    SqlitePtr _sqliteDb = nullptr;

    void parse_entries(std::string& db_data);

    void reopen();
};
