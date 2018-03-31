#include "pkgi_download.hpp"
#include "pkgi_filehttp.hpp"
extern "C" {
#include "pkgi_zrif.h"
}

#include <boost/algorithm/hex.hpp>

#include <fmt/format.h>

#include <memory>

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <filename> <zrif> <sha256>", argv[0]);
        return 1;
    }

    std::vector<uint8_t> digest;
    boost::algorithm::unhex(std::string(argv[3]), std::back_inserter(digest));

    uint8_t rif[PKGI_RIF_SIZE];
    char message[256];
    if (argv[2][0] && !pkgi_zrif_decode(argv[2], rif, message, sizeof(message)))
        throw std::runtime_error(fmt::format("can't decode zrif: {}", message));

    Download d(std::make_unique<FileHttp>());

    d.update_progress_cb = [](const Download& d) {};
    d.update_status = [](auto&&) {};
    d.is_canceled = [] { return false; };

    d.pkgi_download(
            argv[1], argv[1], argv[2][0] ? rif : nullptr, digest.data());

    return 0;
}
