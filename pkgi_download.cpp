extern "C" {
#include "pkgi.h"
#include "pkgi_utils.h"
}
#include "pkgi_download.hpp"

#include <fmt/format.h>

#include <boost/scope_exit.hpp>

#include <stddef.h>

static constexpr auto ISO_SECTOR_SIZE = 2048;

// clang-format off
static const uint8_t pkg_psp_key[] = { 0x07, 0xf2, 0xc6, 0x82, 0x90, 0xb5, 0x0d, 0x2c, 0x33, 0x81, 0x8d, 0x70, 0x9b, 0x60, 0xe6, 0x2b };
static const uint8_t pkg_vita_2[] = { 0xe3, 0x1a, 0x70, 0xc9, 0xce, 0x1d, 0xd7, 0x2b, 0xf3, 0xc0, 0x62, 0x29, 0x63, 0xf2, 0xec, 0xcb };
static const uint8_t pkg_vita_3[] = { 0x42, 0x3a, 0xca, 0x3a, 0x2b, 0xd5, 0x64, 0x9f, 0x96, 0x86, 0xab, 0xad, 0x6f, 0xd8, 0x80, 0x1f };
static const uint8_t pkg_vita_4[] = { 0xaf, 0x07, 0xfd, 0x59, 0x65, 0x25, 0x27, 0xba, 0xf1, 0x33, 0x89, 0x66, 0x8b, 0x17, 0xd9, 0xea };
static const uint8_t kirk7_key38[] = { 0x12, 0x46, 0x8d, 0x7e, 0x1c, 0x42, 0x20, 0x9b, 0xba, 0x54, 0x26, 0x83, 0x5e, 0xb0, 0x33, 0x03 };
static const uint8_t kirk7_key39[] = { 0xc4, 0x3b, 0xb6, 0xd6, 0x53, 0xee, 0x67, 0x49, 0x3e, 0xa9, 0x5f, 0xbc, 0x0c, 0xed, 0x6f, 0x8a };
static const uint8_t kirk7_key63[] = { 0x9c, 0x9b, 0x13, 0x72, 0xf8, 0xc6, 0x40, 0xcf, 0x1c, 0x62, 0xf5, 0xd5, 0x92, 0xdd, 0xb5, 0x82 };
static const uint8_t amctl_hashkey_3[] = { 0xe3, 0x50, 0xed, 0x1d, 0x91, 0x0a, 0x1f, 0xd0, 0x29, 0xbb, 0x1c, 0x3e, 0xf3, 0x40, 0x77, 0xfb };
static const uint8_t amctl_hashkey_4[] = { 0x13, 0x5f, 0xa4, 0x7c, 0xab, 0x39, 0x5b, 0xa4, 0x76, 0xb8, 0xcc, 0xa9, 0x8f, 0x3a, 0x04, 0x45 };
static const uint8_t amctl_hashkey_5[] = { 0x67, 0x8d, 0x7f, 0xa3, 0x2a, 0x9c, 0xa0, 0xd1, 0x50, 0x8a, 0xd8, 0x38, 0x5e, 0x4b, 0x01, 0x7e };
// clang-format on

Download::Download(std::unique_ptr<Http> http) : _http(std::move(http))
{
}

void Download::download_start(void)
{
    LOG("resuming pkg download from %llu offset", download_offset);
    info_update = pkgi_time_msec() + 1000;
    update_status("Downloading");
}

void Download::update_progress()
{
    uint32_t info_now = pkgi_time_msec();
    if (info_now >= info_update)
    {
        update_progress_cb(*this);
        info_update = info_now + 500;
    }
}

// lzrc decompression code from libkirk by tpu
typedef struct
{
    // input stream
    const uint8_t* input;
    uint32_t in_ptr;
    uint32_t in_len;

    // output stream
    uint8_t* output;
    uint32_t out_ptr;
    uint32_t out_len;

    // range decode
    uint32_t range;
    uint32_t code;
    uint32_t out_code;
    uint8_t lc;

    uint8_t bm_literal[8][256];
    uint8_t bm_dist_bits[8][39];
    uint8_t bm_dist[18][8];
    uint8_t bm_match[8][8];
    uint8_t bm_len[8][31];
} lzrc_decode;

static void rc_init(
        lzrc_decode* rc, void* out, int out_len, const void* in, int in_len)
{
    if (in_len < 5)
    {
        throw DownloadError(
                "internal error - lzrc input underflow! pkg may be corrupted");
    }

    rc->input = static_cast<const uint8_t*>(in);
    rc->in_len = in_len;
    rc->in_ptr = 5;

    rc->output = static_cast<uint8_t*>(out);
    rc->out_len = out_len;
    rc->out_ptr = 0;

    rc->range = 0xffffffff;
    rc->lc = rc->input[0];
    rc->code = get32be(rc->input + 1);
    rc->out_code = 0xffffffff;

    memset(rc->bm_literal, 0x80, sizeof(rc->bm_literal));
    memset(rc->bm_dist_bits, 0x80, sizeof(rc->bm_dist_bits));
    memset(rc->bm_dist, 0x80, sizeof(rc->bm_dist));
    memset(rc->bm_match, 0x80, sizeof(rc->bm_match));
    memset(rc->bm_len, 0x80, sizeof(rc->bm_len));
}

static void normalize(lzrc_decode* rc)
{
    if (rc->range < 0x01000000)
    {
        rc->range <<= 8;
        rc->code = (rc->code << 8) + rc->input[rc->in_ptr];
        rc->in_ptr++;
    }
}

static int rc_bit(lzrc_decode* rc, uint8_t* prob)
{
    uint32_t bound;

    normalize(rc);

    bound = (rc->range >> 8) * (*prob);
    *prob -= *prob >> 3;

    if (rc->code < bound)
    {
        rc->range = bound;
        *prob += 31;
        return 1;
    }
    else
    {
        rc->code -= bound;
        rc->range -= bound;
        return 0;
    }
}

static int rc_bittree(lzrc_decode* rc, uint8_t* probs, int limit)
{
    int number = 1;

    do
    {
        number = (number << 1) + rc_bit(rc, probs + number);
    } while (number < limit);

    return number;
}

static int rc_number(lzrc_decode* rc, uint8_t* prob, uint32_t n)
{
    int number = 1;

    if (n > 3)
    {
        number = (number << 1) + rc_bit(rc, prob + 3);
        if (n > 4)
        {
            number = (number << 1) + rc_bit(rc, prob + 3);
            if (n > 5)
            {
                // direct bits
                normalize(rc);

                for (uint32_t i = 0; i < n - 5; i++)
                {
                    rc->range >>= 1;
                    number <<= 1;
                    if (rc->code < rc->range)
                    {
                        number += 1;
                    }
                    else
                    {
                        rc->code -= rc->range;
                    }
                }
            }
        }
    }

    if (n > 0)
    {
        number = (number << 1) + rc_bit(rc, prob);
        if (n > 1)
        {
            number = (number << 1) + rc_bit(rc, prob + 1);
            if (n > 2)
            {
                number = (number << 1) + rc_bit(rc, prob + 2);
            }
        }
    }

    return number;
}

static int lzrc_decompress(void* out, int out_len, const void* in, int in_len)
{
    lzrc_decode rc;
    rc_init(&rc, out, out_len, in, in_len);

    if (rc.lc & 0x80)
    {
        // plain text
        memcpy(rc.output, rc.input + 5, rc.code);
        return rc.code;
    }

    int rc_state = 0;
    uint8_t last_byte = 0;

    for (;;)
    {
        uint32_t match_step = 0;

        int bit = rc_bit(&rc, &rc.bm_match[rc_state][match_step]);
        if (bit == 0) // literal
        {
            if (rc_state > 0)
            {
                rc_state -= 1;
            }

            int byte = rc_bittree(
                    &rc,
                    &rc.bm_literal[((last_byte >> rc.lc) & 0x07)][0],
                    0x100);
            byte -= 0x100;

            if (rc.out_ptr == rc.out_len)
            {
                throw DownloadError(
                        "internal error - lzrc output overflow! pkg may be "
                        "corrupted");
            }
            rc.output[rc.out_ptr++] = (uint8_t)byte;
            last_byte = (uint8_t)byte;
        }
        else // match
        {
            // find bits of match length
            uint32_t len_bits = 0;
            for (int i = 0; i < 7; i++)
            {
                match_step += 1;
                bit = rc_bit(&rc, &rc.bm_match[rc_state][match_step]);
                if (bit == 0)
                {
                    break;
                }
                len_bits += 1;
            }

            // find match length
            uint32_t match_len;
            if (len_bits == 0)
            {
                match_len = 1;
            }
            else
            {
                uint32_t len_state = ((len_bits - 1) << 2) +
                                     ((rc.out_ptr << (len_bits - 1)) & 0x03);
                match_len = rc_number(
                        &rc, &rc.bm_len[rc_state][len_state], len_bits);
                if (match_len == 0xFF)
                {
                    // end of stream
                    return rc.out_ptr;
                }
            }

            // find number of bits of match distance
            uint32_t dist_state = 0;
            uint32_t limit = 8;
            if (match_len > 2)
            {
                dist_state += 7;
                limit = 44;
            }
            int dist_bits = rc_bittree(
                    &rc, &rc.bm_dist_bits[len_bits][dist_state], limit);
            dist_bits -= limit;

            // find match distance
            uint32_t match_dist;
            if (dist_bits > 0)
            {
                match_dist =
                        rc_number(&rc, &rc.bm_dist[dist_bits][0], dist_bits);
            }
            else
            {
                match_dist = 1;
            }

            // copy match bytes
            if (match_dist > rc.out_ptr)
            {
                throw DownloadError(
                        "internal error - lzrc match_dist out of range! pkg "
                        "may be corrupted");
            }

            if (rc.out_ptr + match_len + 1 > rc.out_len)
            {
                throw DownloadError(
                        "internal error - lzrc output overflow! pkg may be "
                        "corrupted");
            }

            const uint8_t* match_src = rc.output + rc.out_ptr - match_dist;
            for (uint32_t i = 0; i <= match_len; i++)
            {
                rc.output[rc.out_ptr++] = *match_src++;
            }
            last_byte = match_src[-1];

            rc_state = 6 + ((rc.out_ptr + 1) & 1);
        }
    }
}
static void init_psp_decrypt(
        aes128_ctx* key,
        uint8_t* iv,
        int eboot,
        const uint8_t* mac,
        const uint8_t* header,
        uint32_t offset1,
        uint32_t offset2)
{
    uint8_t tmp[16];
    aes128_init_dec(key, kirk7_key63);
    if (eboot)
    {
        aes128_decrypt(key, header + offset1, tmp);
    }
    else
    {
        memcpy(tmp, header + offset1, 16);
    }

    aes128_ctx aes;
    aes128_init_dec(&aes, kirk7_key38);
    aes128_decrypt(&aes, tmp, tmp);

    for (size_t i = 0; i < 16; i++)
    {
        iv[i] = mac[i] ^ tmp[i] ^ header[offset2 + i] ^ amctl_hashkey_3[i] ^
                amctl_hashkey_5[i];
    }
    aes128_init_dec(&aes, kirk7_key39);
    aes128_decrypt(&aes, iv, iv);

    for (size_t i = 0; i < 16; i++)
    {
        iv[i] ^= amctl_hashkey_4[i];
    }
}

int Download::download_data(
        uint8_t* buffer, uint32_t size, int encrypted, int save)
{
    if (is_canceled())
        return 0;

    if (size == 0)
        return 0;

    update_progress();

    if (!*_http)
    {
        LOG("requesting %s @ %llu", download_url, download_offset);
        _http->start(download_url, download_offset);

        const int64_t http_length = _http->get_length();
        if (http_length < 0)
        {
            throw DownloadError("HTTP response has unknown length");
        }

        download_size = http_length + download_offset;

        LOG("http response length = %lld, total pkg size = %llu",
            http_length,
            download_size);
        info_start = pkgi_time_msec();
        info_update = pkgi_time_msec() + 500;
    }

    int read = _http->read(buffer, size);
    if (read < 0)
    {
        char error[256];
        pkgi_snprintf(error, sizeof(error), "HTTP download error 0x%08x", read);
        throw DownloadError(error);
    }
    else if (read == 0)
    {
        throw DownloadError("HTTP connection closed");
    }
    download_offset += read;

    sha256_update(&sha, buffer, read);

    if (encrypted)
    {
        aes128_ctr(&aes, iv, encrypted_base + encrypted_offset, buffer, read);
        encrypted_offset += read;
    }

    if (save)
    {
        uint32_t write;
        if (encrypted)
        {
            write = (uint32_t)min64(decrypted_size, read);
            decrypted_size -= write;
        }
        else
        {
            write = read;
        }

        if (!pkgi_write(item_file, buffer, write))
        {
            char error[256];
            pkgi_snprintf(
                    error, sizeof(error), "failed to write to %s", item_path);
            throw DownloadError(error);
        }
    }

    return read;
}

// this includes creating of all the parent folders necessary to actually
// create file
int Download::create_file(void)
{
    char folder[256];
    pkgi_strncpy(folder, sizeof(folder), item_path);
    char* last = pkgi_strrchr(folder, '/');
    *last = 0;

    pkgi_mkdirs(folder);

    LOG("creating %s file", item_name);
    item_file = pkgi_create(item_path);
    if (!item_file)
    {
        char error[256];
        pkgi_snprintf(error, sizeof(error), "cannot create file %s", item_name);
        throw DownloadError(error);
    }

    return 1;
}

int Download::download_head(const uint8_t* rif)
{
    LOG("downloading pkg head");

    pkgi_strncpy(item_name, sizeof(item_name), "Preparing...");
    pkgi_snprintf(
            item_path, sizeof(item_path), "%s/sce_sys/package/head.bin", root);

    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (item_file)
        {
            pkgi_close(item_file);
            item_file = NULL;
        }
    };

    if (!create_file())
    {
        char error[256];
        pkgi_snprintf(error, sizeof(error), "cannot create %s", item_path);
        throw DownloadError(error);
    }

    head_size = PKG_HEADER_SIZE + PKG_HEADER_EXT_SIZE;
    uint32_t head_offset = 0;
    while (head_offset != head_size)
    {
        int size = download_data(
                head + head_offset, head_size - head_offset, 0, 1);
        if (size <= 0)
        {
            return 0;
        }
        head_offset += size;
    }

    if (get32be(head) != 0x7f504b47 ||
        get32be(head + PKG_HEADER_SIZE) != 0x7F657874)
    {
        throw DownloadError("wrong pkg header");
    }

    if (rif && !pkgi_memequ(rif + 0x10, head + 0x30, 0x30))
    {
        throw DownloadError("zRIF content id doesn't match pkg");
    }

    meta_offset = get32be(head + 8);
    meta_count = get32be(head + 12);
    index_count = get32be(head + 20);
    total_size = get64be(head + 24);
    enc_offset = get64be(head + 32);
    enc_size = get64be(head + 40);
    LOG("meta_offset=%u meta_count=%u index_count=%u total_size=%llu "
        "enc_offset=%llu enc_size=%llu",
        meta_offset,
        meta_count,
        index_count,
        total_size,
        enc_offset,
        enc_size);

    if (enc_offset > sizeof(head))
    {
        LOG("pkg file head is too large");
        throw DownloadError("pkg is not supported, head.bin is too big");
    }

    pkgi_memcpy(iv, head + 0x70, sizeof(iv));

    uint8_t key[AES_BLOCK_SIZE];
    int key_type = head[0xe7] & 7;
    if (key_type == 1)
    {
        pkgi_memcpy(key, pkg_psp_key, sizeof(key));
    }
    else if (key_type == 2)
    {
        aes128_ctx ctx;
        aes128_init(&ctx, pkg_vita_2);
        aes128_encrypt(&ctx, iv, key);
    }
    else if (key_type == 3)
    {
        aes128_ctx ctx;
        aes128_init(&ctx, pkg_vita_3);
        aes128_encrypt(&ctx, iv, key);
    }
    else if (key_type == 4)
    {
        aes128_ctx ctx;
        aes128_init(&ctx, pkg_vita_4);
        aes128_encrypt(&ctx, iv, key);
    }
    else
        throw DownloadError("invalid key type " + std::to_string(key_type));

    aes128_ctr_init(&aes, key);

    uint32_t target_size = (uint32_t)enc_offset;
    while (head_size != target_size)
    {
        int size =
                download_data(head + head_size, target_size - head_size, 0, 1);
        if (size <= 0)
        {
            return 0;
        }
        head_size += size;
    }

    index_size = 0;

    uint32_t offset = meta_offset;
    for (uint32_t i = 0; i < meta_count; i++)
    {
        if (offset + 16 >= enc_offset)
        {
            throw DownloadError("pkg file is too small or corrupted");
        }

        uint32_t type = get32be(head + offset + 0);
        uint32_t size = get32be(head + offset + 4);

        if (type == 2)
        {
            content_type = get32be(head + offset + 8);
            // 6 PSX game
            // 7 PSP game
            // 21 PSV game (or update)
            // 22 PSV DLC
            if (content_type != 21 && content_type != 22 && content_type != 6 &&
                content_type != 7)
            {
                throw DownloadError(
                        "unsupported package type: " +
                        std::to_string(content_type));
            }
        }
        else if (type == 13)
        {
            // index_offset = get32be(head + offset + 8);
            index_size = get32be(head + offset + 12);
        }
        offset += 8 + size;
    }

    target_size = (uint32_t)(enc_offset + index_count * 32);
    if (target_size > sizeof(head))
    {
        LOG("pkg file head is too large");
        throw DownloadError("pkg is not supported, head.bin is too big");
    }

    while (head_size != target_size)
    {
        int size =
                download_data(head + head_size, target_size - head_size, 0, 1);
        if (size <= 0)
        {
            return 0;
        }
        head_size += size;
    }

    uint64_t item_offset;
    {
        uint8_t item[32];
        pkgi_memcpy(item, head + enc_offset, sizeof(item));
        aes128_ctr(&aes, iv, 0, item, sizeof(item));

        item_offset = get64be(item + 8);
    }

    if (index_size && item_offset != index_size)
    {
        throw DownloadError(
                "assertion error: read-ahead mismatch, expected: " +
                std::to_string(index_size) +
                ", got: " + std::to_string(item_offset));
    }

    target_size = (uint32_t)(enc_offset + item_offset);
    if (target_size > sizeof(head))
    {
        LOG("pkg file head is too large");
        throw DownloadError("pkg is not supported, head.bin is too big");
    }

    while (head_size != target_size)
    {
        int size =
                download_data(head + head_size, target_size - head_size, 0, 1);
        if (size <= 0)
        {
            return 0;
        }
        head_size += size;
    }

    LOG("head.bin downloaded");
    return 1;
}

void Download::download_normal_file(uint64_t encrypted_size)
{
    while (encrypted_offset != encrypted_size)
    {
        uint32_t read = (uint32_t)min64(
                sizeof(down), encrypted_size - encrypted_offset);
        int size = download_data(down, read, 1, 1);
        if (size <= 0)
            throw DownloadError("unspecified download failure");
    }
}

void Download::download_psp_file(uint64_t item_size)
{
    if (item_size < 0x28)
        throw DownloadError("eboot.pbp file is too small");

    uint8_t eboot_header[0x28];
    download_data(eboot_header, sizeof(eboot_header), 1, 0);

    if (memcmp(eboot_header, "\x00PBP", 4) != 0)
        throw DownloadError("wrong eboot.pbp header magic");

    uint32_t const psar_offset = get32le(eboot_header + 0x24);
    if (psar_offset + 256 > item_size)
        throw DownloadError("eboot.pbp file is to short");
    if (psar_offset % 16 != 0)
        throw DownloadError("psar_offset is not aligned");

    {
        if (psar_offset < encrypted_offset)
            throw DownloadError("can't seek backward to psar_offset");
        std::vector<uint8_t> tmp(psar_offset - encrypted_offset);
        download_data(tmp.data(), tmp.size(), 1, 0);
    }

    std::vector<uint8_t> psar_header(256);
    download_data(psar_header.data(), psar_header.size(), 1, 0);

    if (memcmp(psar_header.data(), "NPUMDIMG", 8) != 0)
        throw DownloadError("wrong data.psar header magic");

    uint32_t const iso_block = get32le(psar_header.data() + 0x0c);
    if (iso_block > 16)
        throw DownloadError(fmt::format(
                "unsupported data.psar block size %u, max %u supported",
                iso_block,
                16));

    uint8_t mac[16];
    aes128_cmac(kirk7_key38, psar_header.data(), 0xc0, mac);

    aes128_ctx psp_key;
    uint8_t psp_iv[16];
    init_psp_decrypt(&psp_key, psp_iv, 1, mac, psar_header.data(), 0xc0, 0xa0);
    aes128_psp_decrypt(&psp_key, psp_iv, 0, psar_header.data() + 0x40, 0x60);

    uint32_t iso_start = get32le(psar_header.data() + 0x54);
    uint32_t iso_end = get32le(psar_header.data() + 0x64);
    uint32_t iso_total = iso_end - iso_start - 1;
    uint32_t block_count = (iso_total + iso_block - 1) / iso_block;

    uint32_t iso_table = get32le(psar_header.data() + 0x6c);

    if (iso_table + block_count * 32 > item_size)
        throw DownloadError("offset table in data.psar file is too large");

    uint64_t const table_offset = psar_offset + iso_table;
    {
        if (table_offset < encrypted_offset)
            throw DownloadError("can't seek backward to table_offset");
        std::vector<uint8_t> tmp(table_offset - encrypted_offset);
        download_data(tmp.data(), tmp.size(), 1, 0);
    }

    std::vector<std::array<uint8_t, 32>> tables(block_count);
    for (auto& table : tables)
        download_data(table.data(), table.size(), 1, 0);

    for (uint32_t i = 0; i < block_count; i++)
    {
        auto const& table = tables[i];

        uint32_t t[8];
        for (size_t k = 0; k < 8; k++)
            t[k] = get32le(table.data() + k * 4);

        uint32_t block_offset = t[4] ^ t[2] ^ t[3];
        uint32_t block_size = t[5] ^ t[1] ^ t[2];
        uint32_t block_flags = t[6] ^ t[0] ^ t[3];

        if (psar_offset + block_size > item_size)
            throw DownloadError(fmt::format(
                    "iso block size/offset is to large: {} > {}",
                    psar_offset + block_size,
                    item_size));

        std::vector<uint8_t> data(16 * ISO_SECTOR_SIZE);

        uint64_t abs_offset = psar_offset + block_offset;
        {
            if (abs_offset < encrypted_offset)
                throw DownloadError("can't seek backward to abs_offset");
            std::vector<uint8_t> tmp(abs_offset - encrypted_offset);
            download_data(tmp.data(), tmp.size(), 1, 0);
        }
        download_data(data.data(), block_size, 1, 0);

        if ((block_flags & 4) == 0)
        {
            aes128_psp_decrypt(
                    &psp_key,
                    psp_iv,
                    block_offset / 16,
                    data.data(),
                    block_size);
        }

        if (block_size == iso_block * ISO_SECTOR_SIZE)
        {
            if (!pkgi_write(item_file, data.data(), block_size))
                throw DownloadError(
                        fmt::format("failed to write to %s", item_path));
        }
        else
        {
            std::vector<uint8_t> uncompressed(16 * ISO_SECTOR_SIZE);
            auto const out_size = lzrc_decompress(
                    uncompressed.data(),
                    uncompressed.size(),
                    data.data(),
                    block_size);
            if (out_size != int(iso_block) * ISO_SECTOR_SIZE)
            {
                throw DownloadError(
                        "internal error - lzrc decompression failed! "
                        "pkg may be corrupted");
            }
            if (!pkgi_write(item_file, uncompressed.data(), out_size))
                throw DownloadError(
                        fmt::format("failed to write to %s", item_path));
        }
    }
}

int Download::download_files(void)
{
    LOG("downloading encrypted files");

    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (item_file)
        {
            pkgi_close(item_file);
            item_file = NULL;
        }
    };

    for (uint32_t index = 0; index < index_count; index++)
    {
        uint8_t item[32];
        pkgi_memcpy(
                item, head + enc_offset + sizeof(item) * index, sizeof(item));
        aes128_ctr(&aes, iv, sizeof(item) * index, item, sizeof(item));

        uint32_t name_offset = get32be(item + 0);
        uint32_t name_size = get32be(item + 4);
        uint64_t item_offset = get64be(item + 8);
        uint64_t item_size = get64be(item + 16);
        uint8_t type = item[27];

        if (name_size > sizeof(item_name) - 1 ||
            enc_offset + name_offset + name_size > total_size)
        {
            throw DownloadError("pkg file is too small or corrupted");
        }

        pkgi_memcpy(item_name, head + enc_offset + name_offset, name_size);
        aes128_ctr(&aes, iv, name_offset, (uint8_t*)item_name, name_size);
        item_name[name_size] = 0;

        uint64_t encrypted_size = (item_size + AES_BLOCK_SIZE - 1) &
                                  ~((uint64_t)AES_BLOCK_SIZE - 1);
        decrypted_size = item_size;
        encrypted_base = item_offset;
        encrypted_offset = 0;
        item_index = index;

        LOG("[%u/%u] %s item_offset=%llu item_size=%llu type=%u",
            item_index + 1,
            index_count,
            item_name,
            item_offset,
            item_size,
            type);

        if (content_type == 6 || content_type == 7)
        {
            if (std::string(item_name) == "USRDIR/CONTENT/DOCUMENT.DAT")
                pkgi_snprintf(
                        item_path, sizeof(item_path), "%s/DOCUMENT.DAT", root);
            else if (std::string(item_name) == "USRDIR/CONTENT/EBOOT.PBP")
                pkgi_snprintf(
                        item_path, sizeof(item_path), "%s/EBOOT.PBP", root);
            else
            {
                while (encrypted_offset != encrypted_size)
                {
                    uint32_t read = (uint32_t)min64(
                            sizeof(down), encrypted_size - encrypted_offset);
                    int size = download_data(down, read, 1, 0);
                    if (size <= 0)
                    {
                        return 0;
                    }
                }
                continue;
            }
        }
        else
            pkgi_snprintf(
                    item_path, sizeof(item_path), "%s/%s", root, item_name);

        if (type == 4)
        {
            pkgi_mkdirs(item_path);
            continue;
        }
        else if (type == 18)
        {
            continue;
        }

        if (!create_file())
        {
            char error[256];
            pkgi_snprintf(error, sizeof(error), "cannot create %s", item_name);
            throw DownloadError(error);
        }

        if (enc_offset + item_offset + encrypted_offset != download_offset)
        {
            throw DownloadError(
                    "pkg is not supported, files are in wrong order, "
                    "expected: " +
                    std::to_string(
                            enc_offset + item_offset + encrypted_offset) +
                    ", actual: " + std::to_string(download_offset));
        }

        if (enc_offset + item_offset + item_size > total_size)
        {
            throw DownloadError("pkg file is too small or corrupted");
        }

        if (content_type == 7 &&
            std::string(item_name) == "USRDIR/CONTENT/EBOOT.PBP")
            download_psp_file(item_size);
        else
            download_normal_file(encrypted_size);

        pkgi_close(item_file);
        item_file = NULL;
    }

    item_index = -1;

    LOG("all files decrypted");
    return 1;
}

int Download::download_tail(void)
{
    LOG("downloading tail.bin");

    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (item_file)
        {
            pkgi_close(item_file);
            item_file = NULL;
        }
    };

    pkgi_strncpy(item_name, sizeof(item_name), "Finishing...");
    pkgi_snprintf(
            item_path, sizeof(item_path), "%s/sce_sys/package/tail.bin", root);

    if (!create_file())
    {
        char error[256];
        pkgi_snprintf(error, sizeof(error), "cannot create %s", item_path);
        throw DownloadError(error);
    }

    uint64_t tail_offset = enc_offset + enc_size;
    while (download_offset < tail_offset)
    {
        uint32_t read =
                (uint32_t)min64(sizeof(down), tail_offset - download_offset);
        int size = download_data(down, read, 0, 0);
        if (size <= 0)
        {
            return 0;
        }
    }

    while (download_offset != total_size)
    {
        uint32_t read =
                (uint32_t)min64(sizeof(down), total_size - download_offset);
        int size = download_data(down, read, 0, content_type != 6);
        if (size <= 0)
        {
            return 0;
        }
    }

    LOG("tail.bin downloaded");
    return 1;
}

int Download::check_integrity(const uint8_t* digest)
{
    if (!digest)
    {
        LOG("no integrity provided, skipping check");
        return 1;
    }

    uint8_t check[SHA256_DIGEST_SIZE];
    sha256_finish(&sha, check);

    LOG("checking integrity of pkg");
    if (!pkgi_memequ(digest, check, SHA256_DIGEST_SIZE))
    {
        LOG("pkg integrity is wrong, removing head.bin & resume data");

        char path[256];
        pkgi_snprintf(path, sizeof(path), "%s/sce_sys/package/head.bin", root);
        pkgi_rm(path);

        throw DownloadError("pkg integrity failed, try downloading again");
    }

    LOG("pkg integrity check succeeded");
    return 1;
}

int Download::create_stat()
{
    LOG("creating stat.bin");
    update_status("Creating stat.bin");

    char path[256];
    pkgi_snprintf(path, sizeof(path), "%s/sce_sys/package/stat.bin", root);

    uint8_t stat[768] = {0};
    if (!pkgi_save(path, stat, sizeof(stat)))
    {
        char error[256];
        pkgi_snprintf(error, sizeof(error), "cannot save rif to %s", path);
        throw DownloadError(error);
    }

    LOG("stat.bin created");
    return 1;
}

int Download::create_rif(const uint8_t* rif)
{
    LOG("creating work.bin");
    update_status("Creating work.bin");

    char path[256];
    pkgi_snprintf(path, sizeof(path), "%s/sce_sys/package/work.bin", root);

    if (!pkgi_save(path, rif, PKGI_RIF_SIZE))
    {
        char error[256];
        pkgi_snprintf(error, sizeof(error), "cannot save rif to %s", path);
        throw DownloadError(error);
    }

    LOG("work.bin created");
    return 1;
}

int Download::pkgi_download(
        const char* content,
        const char* url,
        const uint8_t* rif,
        const uint8_t* digest)
{
    pkgi_snprintf(root, sizeof(root), "%s/%s", pkgi_get_temp_folder(), content);
    LOG("temp installation folder: %s", root);

    update_status("Downloading");
    sha256_init(&sha);

    item_file = NULL;
    item_index = -1;
    download_size = 0;
    download_offset = 0;
    download_content = content;
    download_url = url;

    info_start = pkgi_time_msec();
    info_update = info_start + 1000;

    if (!download_head(rif))
        return 0;
    if (!download_files())
        return 0;
    if (!download_tail())
        return 0;
    if (content_type != 6 && content_type != 7)
    {
        if (!create_stat())
            return 0;
    }
    if (!check_integrity(digest))
        return 0;
    if (rif)
    {
        if (!create_rif(rif))
            return 0;
    }

    return 1;
}
