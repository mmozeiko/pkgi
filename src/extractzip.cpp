#include "extractzip.hpp"

#include "file.hpp"
#include "pkgi.hpp"

#include <zip.h>

#include <boost/scope_exit.hpp>

void pkgi_extract_zip(const std::string& zip_file, const std::string& dest)
{
    int err;
    const auto zip_fd = zip_open(zip_file.c_str(), ZIP_RDONLY, &err);
    if (!zip_fd)
        throw formatEx<std::runtime_error>(
                "failed to open zip {}:\n{}", zip_file, err);
    BOOST_SCOPE_EXIT_ALL(&)
    {
        zip_close(zip_fd);
    };

    const auto num_entries = zip_get_num_entries(zip_fd, 0);
    for (auto i = 0; i < num_entries; ++i)
    {
        struct zip_stat stat;
        if (zip_stat_index(zip_fd, i, 0, &stat) != 0)
            throw formatEx<std::runtime_error>(
                    "can't zip_stat index {} of {}:\n{}",
                    i,
                    zip_file,
                    zip_strerror(zip_fd));

        if (!(stat.valid & ZIP_STAT_NAME))
            throw std::runtime_error("unsupported zip: no file name");
        if (!(stat.valid & ZIP_STAT_SIZE))
            throw std::runtime_error("unsupported zip: no file size");

        std::string path = stat.name;
        if (path[path.size() - 1] == '/')
        {
            LOGF("creating directory {}", path);
            path.substr(0, path.size() - 1);
            pkgi_mkdirs((dest + '/' + path).c_str());
        }
        else
        {
            LOGF("uncompressing file {}", path);
            const auto comp_fd = zip_fopen_index(zip_fd, i, 0);
            if (!comp_fd)
                throw formatEx<std::runtime_error>(
                        "can't zip_fopen index {} of {}:\n{}",
                        i,
                        zip_file,
                        zip_strerror(zip_fd));
            BOOST_SCOPE_EXIT_ALL(&)
            {
                zip_fclose(comp_fd);
            };

            const auto out_fd = pkgi_create((dest + '/' + path).c_str());
            if (!out_fd)
                throw formatEx<std::runtime_error>("can't open file {}", path);
            BOOST_SCOPE_EXIT_ALL(&)
            {
                pkgi_close(out_fd);
            };

            static constexpr auto READ_SIZE = 1 * 1024 * 1024;
            std::vector<uint8_t> buffer(READ_SIZE);

            uint64_t pos = 0;
            while (pos < stat.size)
            {
                const auto to_read =
                        std::min<uint64_t>(buffer.size(), stat.size - pos);
                const auto readed = zip_fread(comp_fd, buffer.data(), to_read);
                pkgi_write(out_fd, buffer.data(), readed);
                pos += readed;
            }
        }
    }
}
