#pragma once

#include <string>
#include <vector>

struct CompPackVersion
{
    bool present;
    std::string base;
    std::string patch;
};

std::vector<std::string> pkgi_get_installed_games();
std::vector<std::string> pkgi_get_installed_themes();
std::string pkgi_get_game_version(const std::string& titleid);
CompPackVersion pkgi_get_comppack_versions(const std::string& titleid);
bool pkgi_dlc_is_installed(const char* content);
bool pkgi_psm_is_installed(const char* titleid);
bool pkgi_psp_is_installed(const char* psppartition, const char* content);
bool pkgi_psx_is_installed(const char* psppartition, const char* content);
void pkgi_install(const char* contentid);
void pkgi_install_update(const std::string& titleid);
void pkgi_install_comppack(
        const std::string& titleid, bool patch, const std::string& version);
void pkgi_install_psmgame(const char* contentid);
void pkgi_install_pspgame(const char* partition, const char* contentid);
void pkgi_install_pspgame_as_iso(const char* partition, const char* contentid);
void pkgi_install_pspdlc(const char* partition, const char* contentid);
