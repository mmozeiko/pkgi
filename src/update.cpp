#include "dialog.hpp"
#include "pkgi.hpp"
#include "vitahttp.hpp"

#include <vector>

#define PKGJ_UPDATE_URL \
    "https://raw.githubusercontent.com/blastrock/pkgj/last/version"

namespace
{
void update_thread()
{
    try
    {
        LOGF("checking latest pkgi version at {}", PKGJ_UPDATE_URL);

        VitaHttp http;
        http.start(PKGJ_UPDATE_URL, 0);
        std::vector<uint8_t> last_versionb(10);
        last_versionb.resize(
                http.read(last_versionb.data(), last_versionb.size()));
        std::string last_version(last_versionb.begin(), last_versionb.end());

        LOGF("last version is {}", last_version);

        if (last_version != PKGI_VERSION)
        {
            LOG("new version available");

            pkgi_dialog_message(
                    fmt::format(
                            "New pkgj version {} is available!", last_version)
                            .c_str());
        }
    }
    catch (const std::exception& e)
    {
        LOGF("error in update thread: {}", e.what());
    }
}
}

void start_update_thread()
{
    pkgi_start_thread("pkgj_update", &update_thread);
}