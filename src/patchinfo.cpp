#include "patchinfo.hpp"

#include "sha256.hpp"

namespace
{
constexpr uint8_t HMAC_KEY[32] = {
        0xE5, 0xE2, 0x78, 0xAA, 0x1E, 0xE3, 0x40, 0x82, 0xA0, 0x88, 0x27,
        0x9C, 0x83, 0xF9, 0xBB, 0xC8, 0x06, 0x82, 0x1C, 0x52, 0xF2, 0xAB,
        0x5D, 0x2B, 0x4A, 0xBD, 0x99, 0x54, 0x50, 0x35, 0x51, 0x14,
};

std::string get_link_for_title(const std::string& titleid)
{
    uint8_t hmac[SHA256_MAC_LEN];

    const auto uniqdata = "np_" + titleid;

    hmac_sha256(
            HMAC_KEY,
            sizeof(HMAC_KEY),
            reinterpret_cast<const uint8_t*>(uniqdata.data()),
            uniqdata.size(),
            hmac);

    const auto link = fmt::format(
            "https://gs-sec.ww.np.dl.playstation.net/pl/np/{}/{}/{}-ver.xml",
            titleid,
            pkgi_tohex(std::vector<uint8_t>(hmac, hmac + SHA256_MAC_LEN)),
            titleid);

    return link;
}

std::optional<std::vector<uint8_t>> download_data(
        Http* http, const std::string& url)
{
    std::vector<uint8_t> data;
    http->start(url, 0);
    if (http->get_status() == 404)
        return std::nullopt;
    size_t pos = 0;
    while (true)
    {
        if (pos == data.size())
            data.resize(pos + 4096);
        const auto read = http->read(data.data() + pos, data.size() - pos);
        if (read == 0)
            break;
        pos += read;
    }
    data.resize(pos);
    return data;
}

PatchInfo get_last_patch(const std::string& xml)
{
    static constexpr char PackageStr[] = "<package";
    static constexpr char HybridPackageStr[] = "<hybrid_package";
    static constexpr char VersionStr[] = "version=\"";
    static constexpr char UrlStr[] = "url=\"";

    const auto last_package = xml.rfind(PackageStr);
    const auto last_hybrid_package = xml.find(HybridPackageStr, last_package);
    const auto version =
            xml.find(VersionStr, last_package) + sizeof(VersionStr) - 1;
    const auto versionEnd = xml.find('"', version);
    const auto package_of_interest = last_hybrid_package == std::string::npos
                                             ? last_package
                                             : last_hybrid_package;
    const auto url = xml.find(UrlStr, package_of_interest) + sizeof(UrlStr) - 1;
    const auto urlEnd = xml.find('"', url);

    return PatchInfo{
            xml.substr(version, versionEnd - version),
            xml.substr(url, urlEnd - url),
    };
}
}

std::optional<PatchInfo> pkgi_download_patch_info(
        Http* http, const std::string& titleid)
{
    const auto info_link = get_link_for_title(titleid);
    const auto xml = download_data(http, info_link);
    if (!xml || xml->empty())
        return std::nullopt;
    const auto patch_info =
            get_last_patch(std::string(xml->begin(), xml->end()));

    return patch_info;
}
