#pragma once

#include <cstdint>
#include <string>

std::string pkgi_sfo_get_string(
        const uint8_t* buffer, size_t size, const std::string& name);
