#pragma once

#include <stdint.h>

#define PKGI_RIF_SIZE 512

extern "C"
{
int pkgi_download(const char* content, const char* url, const uint8_t* rif, const uint8_t* digest);
}
