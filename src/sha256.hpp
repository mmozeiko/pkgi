#pragma once

extern "C"
{
#include "utils.h"
}
#include <cstddef>

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32
#define SHA256_MAC_LEN 32

typedef struct
{
    uint8_t buffer[SHA256_BLOCK_SIZE] GCC_ALIGN(16);
    uint32_t state[8];
    uint64_t count;
} sha256_ctx;

void sha256_init(sha256_ctx* ctx);
void sha256_update(sha256_ctx* ctx, const uint8_t* buffer, uint32_t size);
void sha256_finish(sha256_ctx* ctx, uint8_t* digest);
void sha256_vector(
        size_t num_elem,
        const uint8_t* addr[],
        const size_t* len,
        uint8_t* mac);
void hmac_sha256_vector(
        const uint8_t* key,
        size_t key_len,
        size_t num_elem,
        const uint8_t* addr[],
        const size_t* len,
        uint8_t* mac);
void hmac_sha256(
        const uint8_t* key,
        size_t key_len,
        const uint8_t* data,
        size_t data_len,
        uint8_t* mac);
