#pragma once

#include <string>

bool pkgi_is_installed(const char* titleid);
bool pkgi_update_is_installed(
        const std::string& titleid, const std::string& request_version);
bool pkgi_dlc_is_installed(const char* content);
bool pkgi_psm_is_installed(const char* titleid);
bool pkgi_psp_is_installed(const char* psppartition, const char* content);
bool pkgi_psx_is_installed(const char* psppartition, const char* content);
void pkgi_install(const char* contentid);
void pkgi_install_update(const char* contentid);
void pkgi_install_comppack(const char* titleid);
void pkgi_install_psmgame(const char* contentid);
void pkgi_install_pspgame(const char* partition, const char* contentid);
void pkgi_install_pspgame_as_iso(const char* partition, const char* contentid);
