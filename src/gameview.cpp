#include "gameview.hpp"

#include <fmt/format.h>

#include "dialog.hpp"
#include "imgui.hpp"
extern "C"
{
#include "style.h"
}

void GameView::render()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), 0);
    ImGui::SetNextWindowSize(ImVec2(VITA_WIDTH, VITA_HEIGHT), 0);

    ImGui::Begin(
            "#gameview",
            nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoInputs);

    ImGui::PushTextWrapPos(0.f);
    ImGui::Text(_item->name.c_str());
    ImGui::Text(_item->titleid.c_str());

    ImGui::Text(" ");

    ImGui::Text(fmt::format("Firmware version: {}", pkgi_get_system_version())
                        .c_str());
    ImGui::Text(fmt::format("Required firmware version: {}", _item->fw_version)
                        .c_str());

    ImGui::Text(" ");

    ImGui::Text(fmt::format(
                        "Installed game version: {}",
                        _game_version.empty() ? "not installed" : _game_version)
                        .c_str());
    if (_comppack_versions.present && _comppack_versions.base.empty() &&
        _comppack_versions.patch.empty())
    {
        ImGui::Text("Installed compatibility pack: unknown version");
    }
    else
    {
        ImGui::Text(fmt::format(
                            "Installed base compatibility pack: {}",
                            _comppack_versions.base.empty() ? "no" : "yes")
                            .c_str());
        ImGui::Text(fmt::format(
                            "Installed patch compatibility pack version: {}",
                            _comppack_versions.patch.empty()
                                    ? "none"
                                    : _comppack_versions.patch)
                            .c_str());
    }

    ImGui::Text(" ");

    printDiagnostic();

    ImGui::Text(" ");

    ImGui::PopTextWrapPos();

    if (!_downloader->is_in_queue(_item->content))
    {
        if (ImGui::Button("Install game###installgame"))
            start_download_package();
    }
    else
    {
        if (ImGui::Button("Cancel game installation###installgame"))
            cancel_download_package();
    }

    if (_base_comppack)
        if (ImGui::Button("Install base compatibility pack"))
            start_download_comppack(false);
    if (_patch_comppack)
        if (ImGui::Button(fmt::format(
                                  "Install compatibility pack {}",
                                  _patch_comppack->app_version)
                                  .c_str()))
            start_download_comppack(true);

    // HACK: comppack are identified by their titleid instead of content id
    if (_downloader->is_in_queue(_item->titleid))
        if (ImGui::Button("Cancel compatibility pack installations"))
            cancel_download_comppacks();

    if (ImGui::Button("Close"))
        _closed = true;

    ImGui::End();
}

static const auto Red = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
static const auto Yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
static const auto Green = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

void GameView::printDiagnostic()
{
    bool ok = true;
    auto const printError = [&](auto const& str) {
        ok = false;
        ImGui::TextColored(Red, str);
    };

    auto const systemVersion = pkgi_get_system_version();
    auto const& minSystemVersion = _item->fw_version;

    ImGui::Text("Diagnostic:");

    if (systemVersion < minSystemVersion)
    {
        if (!_comppack_versions.present)
            printError(
                    "- Your firmware is too old to play this game, you must "
                    "install the compatibility pack");
    }
    else
    {
        ImGui::Text(
                "- Your firmware is recent enough, no need for compatibility "
                "packs");
    }

    if (_comppack_versions.present && _comppack_versions.base.empty() &&
        _comppack_versions.patch.empty())
    {
        ImGui::TextColored(
                Yellow,
                "- A compatibility pack is installed but not by PKGj, please "
                "make sure it matches the installed version or reinstall it "
                "with PKGj");
        ok = false;
    }

    if (_comppack_versions.base.empty() && !_comppack_versions.patch.empty())
        printError(
                "- You have installed an update compitibility pack without "
                "installing the base pack, install the base pack first and "
                "reinstall the update compitibility pack.");

    std::string comppack_version;
    if (!_comppack_versions.patch.empty())
        comppack_version = _comppack_versions.patch;
    else if (!_comppack_versions.base.empty())
        comppack_version = _comppack_versions.base;

    if (_item->presence == PresenceInstalled && !comppack_version.empty() &&
        comppack_version < _game_version)
        printError(
                "- The version of the game does not match the installed "
                "compatibility pack. If you have updated the game, also "
                "install the update compitibility pack.");

    if (_item->presence == PresenceInstalled &&
        comppack_version > _game_version)
        printError(
                "- The version of the game does not match the installed "
                "compatibility pack. Downgrade to the base compatibility "
                "pack or update the game through the Live Area.");

    if (_item->presence != PresenceInstalled)
    {
        ImGui::Text("- Game not installed");
        ok = false;
    }

    if (ok)
        ImGui::TextColored(Green, "All green");
}

void GameView::refresh()
{
    LOGF("refreshing gameview");
    _game_version = pkgi_get_game_version(_item->titleid);
    _comppack_versions = pkgi_get_comppack_versions(_item->titleid);
}

void GameView::start_download_package()
{
    if (_item->presence == PresenceInstalled)
    {
        LOGF("[{}] {} - already installed", _item->titleid, _item->name);
        pkgi_dialog_error("Already installed");
        return;
    }

    pkgi_start_download(*_downloader, *_item);

    _item->presence = PresenceUnknown;
}

void GameView::cancel_download_package()
{
    _downloader->remove_from_queue(_item->content);
    _item->presence = PresenceUnknown;
}

void GameView::start_download_comppack(bool patch)
{
    const auto& entry = patch ? _patch_comppack : _base_comppack;

    _downloader->add(DownloadItem{CompPack,
                                  _item->name,
                                  _item->titleid,
                                  _config->comppack_url + entry->path,
                                  std::vector<uint8_t>{},
                                  std::vector<uint8_t>{},
                                  false,
                                  "ux0:",
                                  patch,
                                  entry->app_version});
}

void GameView::cancel_download_comppacks()
{
    // HACK: comppack are identified by their titleid instead of content id
    _downloader->remove_from_queue(_item->titleid);
}
