#pragma once

#include <stdint.h>

int pkgi_zrif_decode(const char* str, uint8_t* rif, char* error, uint32_t error_size);
