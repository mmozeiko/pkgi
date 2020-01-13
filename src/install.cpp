#include "install.hpp"

#include "extractzip.hpp"
#include "file.hpp"
#include "log.hpp"
#include "sfo.hpp"
#include "sqlite.hpp"

#include <boost/scope_exit.hpp>

#include <psp2/io/fcntl.h>
#include <psp2/promoterutil.h>

std::vector<std::string> pkgi_get_installed_games()
{
    return pkgi_list_dir_contents("ux0:app");
}

std::vector<std::string> pkgi_get_installed_themes()
{
    return pkgi_list_dir_contents("ux0:theme");
}

namespace
{
std::string pkgi_extract_package_version(const std::string& package)
{
    const auto sfo = pkgi_load(fmt::format("{}/sce_sys/param.sfo", package));
    return pkgi_sfo_get_string(sfo.data(), sfo.size(), "APP_VER");
}
}

std::string pkgi_get_game_version(const std::string& titleid)
{
    const auto patch_dir = fmt::format("ux0:patch/{}", titleid);
    if (pkgi_file_exists(patch_dir.c_str()))
        return pkgi_extract_package_version(patch_dir);

    const auto game_dir = fmt::format("ux0:app/{}", titleid);
    if (pkgi_file_exists(game_dir.c_str()))
        return pkgi_extract_package_version(game_dir);

    return "";
}

bool pkgi_dlc_is_installed(const char* content)
{
    return pkgi_file_exists(
            fmt::format("ux0:addcont/{:.9}/{:.16}", content + 7, content + 20)
                    .c_str());
}

bool pkgi_psm_is_installed(const char* titleid)
{
    return pkgi_file_exists(fmt::format("ux0:psm/{}", titleid).c_str());
}

bool pkgi_psp_is_installed(const char* psppartition, const char* content)
{
    return pkgi_file_exists(
                   fmt::format(
                           "{}pspemu/ISO/{:.9}.iso", psppartition, content + 7)
                           .c_str()) ||
           pkgi_file_exists(fmt::format(
                                    "{}pspemu/PSP/GAME/{:.9}/EBOOT.PBP",
                                    psppartition,
                                    content + 7)
                                    .c_str());
}

bool pkgi_psx_is_installed(const char* psppartition, const char* content)
{
    return pkgi_file_exists(
            fmt::format("{}pspemu/PSP/GAME/{:.9}", psppartition, content + 7)
                    .c_str());
}

void pkgi_install(const char* contentid)
{
    char path[128];
    snprintf(path, sizeof(path), "ux0:pkgj/%s", contentid);

    LOG("calling scePromoterUtilityPromotePkgWithRif on %s", path);
    const auto res = scePromoterUtilityPromotePkgWithRif(path, 1);
    if (res < 0)
        throw formatEx<std::runtime_error>(
                "scePromoterUtilityPromotePkgWithRif failed: {:#08x}\n{}",
                static_cast<uint32_t>(res),
                static_cast<uint32_t>(res) == 0x80870004
                        ? "Please check your NoNpDrm installation"
                        : "");
}

void pkgi_install_update(const std::string& titleid)
{
    pkgi_mkdirs("ux0:patch");

    const auto src = fmt::format("ux0:pkgj/{}", titleid);

    LOGF("installing update from {}", src);
    const auto res = scePromoterUtilityPromotePkgWithRif(src.c_str(), 1);
    if (res < 0)
        throw formatEx<std::runtime_error>(
                "scePromoterUtilityPromotePkgWithRif failed: {:#08x}",
                static_cast<uint32_t>(res));
}

void pkgi_install_comppack(
        const std::string& titleid, bool patch, const std::string& version)
{
    const auto src = fmt::format("ux0:pkgj/{}-comp.ppk", titleid);
    const auto dest = fmt::format("ux0:rePatch/{}", titleid);

    if (!patch)
        pkgi_rm((dest + "/patch_comppack_version").c_str());

    pkgi_mkdirs(dest.c_str());

    LOGF("installing comp pack from {} to {}", src, dest);
    pkgi_extract_zip(src, dest);

    pkgi_save(
            fmt::format(
                    "{}/{}_comppack_version", dest, patch ? "patch" : "base"),
            version.data(),
            version.size());
}

CompPackVersion pkgi_get_comppack_versions(const std::string& titleid)
{
    const std::string dir = fmt::format("ux0:rePatch/{}", titleid);

    const bool present = pkgi_file_exists(dir.c_str());

    const auto base = [&] {
        try
        {
            const auto data = pkgi_load(dir + "/base_comppack_version");
            return std::string(data.begin(), data.end());
        }
        catch (const std::exception& e)
        {
            LOGF("no base comppack version: {}", e.what());
            // return empty string if not found
            return std::string{};
        }
    }();
    const auto patch = [&] {
        try
        {
            const auto data = pkgi_load(dir + "/patch_comppack_version");
            return std::string(data.begin(), data.end());
        }
        catch (const std::exception& e)
        {
            LOGF("no patch comppack version: {}", e.what());
            // return empty string if not found
            return std::string{};
        }
    }();

    return {present, base, patch};
}

void pkgi_install_psmgame(const char* contentid)
{
    const std::string base = "ux0:temp/game";
    pkgi_mkdirs(base.c_str());
    const auto titleid = fmt::format("{:.9}", contentid + 7);
    const auto src = fmt::format("ux0:pkgj/{}", contentid);
    const auto dest = fmt::format("{}/{}", base, titleid);

    LOGF("moving psm game from {} to {}", src, dest);
    int res = sceIoRename(src.c_str(), dest.c_str());
    if (res < 0)
        throw formatEx<std::runtime_error>(
                "failed to rename: {:#08x}", static_cast<uint32_t>(res));

    LOGF("promoting psm game at {}", dest);
    ScePromoterUtilityImportParams promote_args;
    memset(&promote_args, 0, sizeof(promote_args));

    strncpy(promote_args.path, base.c_str(), sizeof(promote_args.path)-1);
    strncpy(promote_args.titleid, titleid.c_str(), sizeof(promote_args.titleid)-1);
    promote_args.type = SCE_PKG_TYPE_PSM;
    promote_args.attribute = 0x1;

    res = scePromoterUtilityPromoteImport(&promote_args);
    if (res < 0)
        throw formatEx<std::runtime_error>(
            "scePromoterUtilityPromoteImport failed: {:#08x}",
            static_cast<uint32_t>(res));
}

void pkgi_install_pspgame(const char* partition, const char* contentid)
{
    LOG("Installing a PSP/PSX game");
    const auto path = fmt::format("{}pkgj/{}", partition, contentid);
    const auto dest =
            fmt::format("{}pspemu/PSP/GAME/{:.9}", partition, contentid + 7);

    pkgi_mkdirs(fmt::format("{}pspemu/PSP/GAME", partition).c_str());

    LOG("installing psx game at %s to %s", path.c_str(), dest.c_str());
    int res = sceIoRename(path.c_str(), dest.c_str());
    if (res < 0)
        throw std::runtime_error(fmt::format(
                "failed to rename: {:#08x}", static_cast<uint32_t>(res)));
}

static void pkgi_move_merge(const std::string& from, const std::string& to)
{
    LOGF("Merging {} and {}", from, to);
    const auto fromType = pkgi_get_inode_type(from);
    const auto toType = pkgi_get_inode_type(to);
    if (toType == InodeType::NotExist ||
        (fromType == InodeType::File && toType == InodeType::File))
        pkgi_rename(from, to);
    else if (fromType == InodeType::File && toType == InodeType::Directory)
        throw formatEx("cannot replace directory {} by file {}", to, from);
    else if (fromType == InodeType::Directory && toType == InodeType::File)
        throw formatEx("cannot replace directory {} by file {}", from, to);
    else if (fromType == InodeType::Directory && toType == InodeType::Directory)
    {
        const auto fromContents = pkgi_list_dir_contents(from);
        for (const auto& fromContent : fromContents)
            pkgi_move_merge(
                    fmt::format("{}/{}", from, fromContent),
                    fmt::format("{}/{}", to, fromContent));
    }
    else
        throw formatEx(
                "cannot merge {} ({}) and {} ({})",
                from,
                (int)fromType,
                to,
                (int)toType);
}

void pkgi_install_pspgame_as_iso(const char* partition, const char* contentid)
{
    const auto path = fmt::format("{}pkgj/{}", partition, contentid);
    const auto dest =
            fmt::format("{}pspemu/PSP/GAME/{:.9}", partition, contentid + 7);

    // this is actually a misnamed ISO file
    const auto eboot = fmt::format("{}/EBOOT.PBP", path);
    const auto content = fmt::format("{}/CONTENT.DAT", path);
    const auto pspkey = fmt::format("{}/PSP-KEY.EDAT", path);
    const auto isodest =
            fmt::format("{}pspemu/ISO/{:.9}.iso", partition, contentid + 7);

    pkgi_mkdirs(fmt::format("{}pspemu/ISO", partition).c_str());

    LOG("installing psp game at %s to %s", path.c_str(), dest.c_str());
    pkgi_rename(eboot.c_str(), isodest.c_str());

    const auto content_exists = pkgi_file_exists(content.c_str());
    const auto pspkey_exists = pkgi_file_exists(pspkey.c_str());
    if (content_exists || pspkey_exists)
        pkgi_mkdirs(dest.c_str());

    if (content_exists)
        pkgi_rename(
                content.c_str(), fmt::format("{}/CONTENT.DAT", dest).c_str());
    if (pspkey_exists)
        pkgi_rename(
                pspkey.c_str(), fmt::format("{}/PSP-KEY.EDAT", dest).c_str());

    pkgi_delete_dir(path);
}

void pkgi_install_pspdlc(const char* partition, const char* contentid)
{
    LOG("Installing a PSP DLC");
    const auto path = fmt::format("{}pkgj/{}", partition, contentid);
    const auto dest =
            fmt::format("{}pspemu/PSP/GAME/{:.9}", partition, contentid + 7);

    pkgi_mkdirs(fmt::format("{}pspemu/PSP/GAME", partition).c_str());

    LOG("installing psp dlc at %s to %s", path.c_str(), dest.c_str());
    pkgi_move_merge(path, dest);
    pkgi_delete_dir(path);
}
