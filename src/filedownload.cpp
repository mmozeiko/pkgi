#include "filedownload.hpp"
#include "download.hpp"

extern "C"
{
#include "utils.h"
}
#include "file.hpp"
#include "pkgi.hpp"

#include <fmt/format.h>

#include <boost/scope_exit.hpp>

#include <cereal/archives/binary.hpp>

#include <fstream>

#include <cstddef>

FileDownload::FileDownload(std::unique_ptr<Http> http) : _http(std::move(http))
{
}

void FileDownload::update_progress()
{
    update_progress_cb(download_offset, download_size);
}

void FileDownload::start_download()
{
    LOGF("requesting {}", download_url);
    _http->start(download_url, 0);

    const auto http_length = _http->get_length();
    if (http_length < 0)
        throw DownloadError("HTTP response has unknown length");

    download_size = http_length;
    LOGF("http response length = {}", download_size);
}

void FileDownload::download_data(uint32_t size)
{
    if (is_canceled())
        throw std::runtime_error("download was canceled");

    if (size == 0)
        return;

    update_progress();

    std::vector<uint8_t> buffer(size);
    {
        size_t pos = 0;
        while (pos < size)
        {
            const int read = _http->read(buffer.data() + pos, size - pos);
            if (read == 0)
                throw DownloadError("HTTP connection closed");
            pos += read;
        }
    }

    download_offset += size;

    pkgi_write(item_file, buffer.data(), buffer.size());
}

void FileDownload::download_file()
{
    LOG("downloading encrypted files");

    LOGF("creating {} file", root);
    item_file = pkgi_create(root.c_str());
    if (!item_file)
        throw formatEx<DownloadError>("cannot create file {}", root);

    BOOST_SCOPE_EXIT_ALL(&)
    {
        pkgi_close(item_file);
    };

    start_download();

    static constexpr auto SAVE_PERIOD = 512 * 1024;

    while (download_offset < download_size)
    {
        const uint32_t read =
                (uint32_t)min64(SAVE_PERIOD, download_size - download_offset);
        download_data(read);
    }
}

void FileDownload::download(
        const std::string& partition,
        const std::string& titleid,
        const std::string& url)
{
    root = fmt::format("{}pkgj/{}-comp.ppk", partition, titleid);
    LOGF("temp installation folder: {}", root);

    download_size = 0;
    download_offset = 0;
    download_url = url;

    download_file();
}
