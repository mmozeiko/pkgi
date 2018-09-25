#include "install.hpp"

#include "extractzip.hpp"
#include "file.hpp"
#include "log.hpp"
#include "sfo.hpp"
#include "sqlite.hpp"

#include <boost/scope_exit.hpp>

#include <psp2/io/fcntl.h>
#include <psp2/promoterutil.h>

int pkgi_is_installed(const char* titleid)
{
    int ret = -1;
    LOG("calling scePromoterUtilityCheckExist on %s", titleid);
    int res = scePromoterUtilityCheckExist(titleid, &ret);
    LOG("res=%d ret=%d", res, ret);
    return res == 0;
}

bool pkgi_update_is_installed(
        const std::string& titleid, const std::string& request_version)
{
    const auto patch_dir = fmt::format("ux0:patch/{}", titleid);

    if (!pkgi_file_exists(patch_dir.c_str()))
        return false;

    const auto sfo = pkgi_load(fmt::format("{}/sce_sys/param.sfo", patch_dir));
    const auto installed_version =
            pkgi_sfo_get_string(sfo.data(), sfo.size(), "APP_VER");

    const auto full_request_version = fmt::format("{:0>5}", request_version);

    if (installed_version != full_request_version)
        return false;

    return true;
}

int pkgi_dlc_is_installed(const char* content)
{
    return pkgi_file_exists(
            fmt::format("ux0:addcont/{:.9}/{:.16}", content + 7, content + 20)
                    .c_str());
}

int pkgi_psm_is_installed(const char* titleid)
{
    return pkgi_file_exists(fmt::format("ux0:psm/{}", titleid).c_str());
}

int pkgi_psp_is_installed(const char* psppartition, const char* content)
{
    return pkgi_file_exists(
                   fmt::format(
                           "{}pspemu/ISO/{:.9}.iso", psppartition, content + 7)
                           .c_str()) ||
           pkgi_file_exists(
                   fmt::format(
                           "{}pspemu/PSP/GAME/{:.9}", psppartition, content + 7)
                           .c_str());
}

int pkgi_psx_is_installed(const char* psppartition, const char* content)
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

void pkgi_install_update(const char* contentid)
{
    pkgi_mkdirs("ux0:patch");

    const auto titleid = fmt::format("{:.9}", contentid + 7);
    const auto src = fmt::format("ux0:pkgj/{}", contentid);
    const auto dest = fmt::format("ux0:patch/{}", titleid);

    LOGF("deleting previous patch at {}", dest);
    pkgi_delete_dir(dest);

    LOGF("installing update from {} to {}", src, dest);
    const auto res = sceIoRename(src.c_str(), dest.c_str());
    if (res < 0)
        throw formatEx<std::runtime_error>(
                "failed to rename: {:#08x}", static_cast<uint32_t>(res));

    const auto sfo = pkgi_load(fmt::format("{}/sce_sys/param.sfo", dest));
    const auto version = pkgi_sfo_get_string(sfo.data(), sfo.size(), "APP_VER");

    LOGF("found version is {}", version);
    if (version.empty())
        throw std::runtime_error("no version field found in param.sfo");
    if (version.size() != 5)
        throw formatEx<std::runtime_error>(
                "version field of incorrect size: {}", version.size());

    SqlitePtr _sqliteDb;
    sqlite3* raw_appdb;
    SQLITE_CHECK(
            sqlite3_open("ur0:shell/db/app.db", &raw_appdb),
            "can't open app.db database");
    _sqliteDb.reset(raw_appdb);

    sqlite3_stmt* stmt;
    SQLITE_CHECK(
            sqlite3_prepare_v2(
                    _sqliteDb.get(),
                    R"(UPDATE tbl_appinfo
                    SET val = ?
                    WHERE titleId = ? AND key = 3168212510)",
                    -1,
                    &stmt,
                    nullptr),
            "can't prepare version update SQL statement");
    BOOST_SCOPE_EXIT_ALL(&)
    {
        sqlite3_finalize(stmt);
    };

    sqlite3_bind_text(stmt, 1, version.data(), version.size(), nullptr);
    sqlite3_bind_text(stmt, 2, titleid.data(), titleid.size(), nullptr);

    const auto err = sqlite3_step(stmt);
    if (err != SQLITE_DONE)
        throw formatEx<std::runtime_error>(
                "can't execute version update SQL statement:\n{}",
                sqlite3_errmsg(_sqliteDb.get()));
}

void pkgi_install_comppack(const char* titleid)
{
    const auto src = fmt::format("ux0:pkgj/{}-comp.ppk", titleid);
    const auto dest = fmt::format("ux0:rePatch/{}", titleid);

    pkgi_mkdirs(dest.c_str());

    pkgi_mkdirs(dest.c_str());

    LOGF("installing comp pack from {} to {}", src, dest);
    pkgi_extract_zip(src, dest);
}

void pkgi_install_psmgame(const char* contentid)
{
    pkgi_mkdirs("ux0:psm");
    const auto titleid = fmt::format("{:.9}", contentid + 7);
    const auto src = fmt::format("ux0:pkgj/{}", contentid);
    const auto dest = fmt::format("ux0:psm/{}", titleid);

    LOGF("installing psm game from {} to {}", src, dest);
    const auto res = sceIoRename(src.c_str(), dest.c_str());
    if (res < 0)
        throw formatEx<std::runtime_error>(
                "failed to rename: {:#08x}", static_cast<uint32_t>(res));
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
