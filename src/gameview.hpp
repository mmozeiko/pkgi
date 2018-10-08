#pragma once

#include "comppackdb.hpp"
#include "config.hpp"
#include "db.hpp"
#include "downloader.hpp"

#include <optional>

class GameView
{
public:
    GameView(
            const Config* config,
            Downloader* downloader,
            DbItem* item,
            std::optional<CompPackDatabase::Item> base_comppack,
            std::optional<CompPackDatabase::Item> patch_comppack)
        : _config(config)
        , _downloader(downloader)
        , _item(item)
        , _base_comppack(base_comppack)
        , _patch_comppack(patch_comppack)
    {
        refresh();
    }

    const DbItem* get_item() const
    {
        return _item;
    }

    void render();
    void refresh();

    bool is_closed() const
    {
        return _closed;
    }

private:
    const Config* _config;
    Downloader* _downloader;

    DbItem* _item;
    std::optional<CompPackDatabase::Item> _base_comppack;
    std::optional<CompPackDatabase::Item> _patch_comppack;

    std::string _game_version;
    std::tuple<std::string, std::string> _comppack_versions;

    bool _closed{false};

    void printDiagnostic();
    void start_download_package();
    void cancel_download_package();
    void start_download_comppack(bool patch);
    void cancel_download_comppacks();
};
