#include "pkgi_db.hpp"
#include "pkgi_download.hpp"
#include "pkgi_filehttp.hpp"
extern "C" {
#include "pkgi_zrif.h"
}

#include <boost/algorithm/hex.hpp>

#include <fmt/format.h>

#include <memory>

static constexpr auto USAGE =
        "Usage: %s [extract <filename> <zrif> <sha256>] [refreshlist PSV "
        "url]\n";

int extract(int argc, char* argv[])
{
    if (argc != 5)
    {
        printf(USAGE, argv[0]);
        return 1;
    }

    std::vector<uint8_t> digest;
    boost::algorithm::unhex(std::string(argv[4]), std::back_inserter(digest));

    uint8_t rif[PKGI_RIF_SIZE];
    char message[256];
    if (argv[3][0] && !pkgi_zrif_decode(argv[3], rif, message, sizeof(message)))
        throw std::runtime_error(fmt::format("can't decode zrif: {}", message));

    Download d(std::make_unique<FileHttp>());

    d.save_as_iso = false;
    d.update_progress_cb = [](const Download&) {};
    d.update_status = [](auto&&) {};
    d.is_canceled = [] { return false; };

    d.pkgi_download(
            "tmp", argv[2], argv[2], argv[3][0] ? rif : nullptr, digest.data());

    return 0;
}

int refreshlist(int argc, char* argv[])
{
    if (argc != 4)
    {
        printf(USAGE, argv[0]);
        return 1;
    }

    auto const http = std::make_unique<FileHttp>();

    auto db = std::make_unique<TitleDatabase>(ModeGames, "db.db");
    db->update(http.get(), argv[3]);

    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf(USAGE, argv[0]);
        return 1;
    }

    if (std::string(argv[1]) == "extract")
        return extract(argc, argv);
    if (std::string(argv[1]) == "refreshlist")
        return refreshlist(argc, argv);

    printf(USAGE, argv[0]);
    return 1;
}
