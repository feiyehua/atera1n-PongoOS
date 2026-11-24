// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 feiyehua

#include <pongo.h>
#include "nvram_art.h"

/* CHRP partition headers contain a LE16 size in units of 16 bytes. */
static size_t chrp_size(const uint8_t *header)
{
    return ((size_t)header[2] | ((size_t)header[3] << 8)) * 16u;
}

static int is_variable_partition(const uint8_t *header)
{
    static const char names[][12] = { "common", "system" };
    return memcmp(header + 4, names[0], 12) == 0 ||
           memcmp(header + 4, names[1], 12) == 0;
}

/* Parse entries rather than matching a key embedded inside another value. */
static int find_art(const uint8_t *proxy, size_t proxy_len,
                    const uint8_t **value, size_t *value_len)
{
    static const char key[] = "com.apple.System.sep.art=";
    const size_t key_len = sizeof(key) - 1;
    const uint8_t *found = NULL;
    size_t found_len = 0;

    for (size_t off = 0; off < proxy_len;) {
        if (proxy_len - off < 16)
            return NVRAM_ART_EFORMAT;
        const uint8_t *header = proxy + off;
        size_t partition_len = chrp_size(header);
        if (partition_len < 16 || partition_len > proxy_len - off)
            return NVRAM_ART_EFORMAT;

        /* The initial "nvram" header is skipped using its own size (0x20). */
        if (is_variable_partition(header)) {
            size_t end = off + partition_len;
            for (size_t pos = off + 16; pos < end && proxy[pos];) {
                const uint8_t *entry = proxy + pos;
                const uint8_t *nul = memchr(entry, 0, end - pos);
                if (!nul)
                    return NVRAM_ART_EFORMAT;
                size_t entry_len = (size_t)(nul - entry);
                if (entry_len >= key_len &&
                    memcmp(entry, key, key_len) == 0) {
                    if (found) /* Refuse ambiguous duplicate variables. */
                        return NVRAM_ART_EFORMAT;
                    found = entry + key_len;
                    found_len = entry_len - key_len;
                }
                pos += entry_len + 1;
            }
        }
        off += partition_len;
    }
    if (!found)
        return NVRAM_ART_ENOTFOUND;
    if (!found_len)
        return NVRAM_ART_EFORMAT;
    *value = found;
    *value_len = found_len;
    return NVRAM_ART_OK;
}

/* dst == NULL performs a bounded sizing/validation pass before allocation. */
static int unescape_art(const uint8_t *src, size_t len,
                        uint8_t *dst, size_t *decoded_len)
{
    size_t used = 0;
    for (size_t i = 0; i < len;) {
        uint8_t byte = src[i++];
        size_t count = 1;
        if (byte == 0xff) {
            if (i == len)
                return NVRAM_ART_EFORMAT;
            uint8_t code = src[i++];
            if (!code)
                return NVRAM_ART_EFORMAT;
            count = code & 0x7fu;
            byte = (code & 0x80u) ? 0xff : 0;
        }
        if (count > NVRAM_ART_PAGE_SIZE - used)
            return NVRAM_ART_ETOOBIG;
        if (dst)
            memset(dst + used, byte, count);
        used += count;
    }
    if (!used)
        return NVRAM_ART_EFORMAT;
    *decoded_len = used;
    return NVRAM_ART_OK;
}

/* Check that the reported source PA/length really describes one RAM span. */
static int source_phys_span(const uint8_t *va, size_t len, uint64_t *pa_out)
{
    uintptr_t base = (uintptr_t)va;
    uint64_t pa = vatophys((uint64_t)base);
    size_t off = 0;
    while (off < len) {
        if (vatophys((uint64_t)(base + off)) != pa + off)
            return NVRAM_ART_ENONCONTIG;
        /* 4 KiB boundaries also cover systems using 16 KiB mappings. */
        size_t step = 0x1000u - ((base + off) & 0xfffu);
        if (step >= len - off)
            break;
        off += step;
    }
    *pa_out = pa;
    return NVRAM_ART_OK;
}

int nvram_art_load(nvram_art_t *out)
{
    if (!out)
        return NVRAM_ART_EINVAL;
    memset(out, 0, sizeof(*out));

    if (!gDeviceTree)
        return NVRAM_ART_ENOPROXY;
    dt_node_t *chosen = dt_find(gDeviceTree, "/chosen");
    if (!chosen)
        return NVRAM_ART_ENOPROXY;
    size_t proxy_len = 0;
    const uint8_t *proxy = dt_prop(chosen, "nvram-proxy-data", &proxy_len);
    if (!proxy || !proxy_len)
        return NVRAM_ART_ENOPROXY;

    nvram_art_t result = {0};
    int rc = find_art(proxy, proxy_len, &result.encoded_va,
                      &result.encoded_len);
    if (rc != NVRAM_ART_OK)
        return rc;
    rc = unescape_art(result.encoded_va, result.encoded_len,
                       NULL, &result.art_len);
    if (rc != NVRAM_ART_OK)
        return rc;
    /* DeviceTree pointers are not assumed to be in the static mapping. */
    rc = source_phys_span(result.encoded_va, result.encoded_len,
                          &result.encoded_pa);
    if (rc != NVRAM_ART_OK)
        return rc;

    result.art_va = alloc_contig(NVRAM_ART_PAGE_SIZE);
    if (!result.art_va)
        return NVRAM_ART_ENOMEM;
    result.art_pa = vatophys_static(result.art_va);
    if (((uintptr_t)result.art_va & (NVRAM_ART_PAGE_SIZE - 1u)) ||
        (result.art_pa & (NVRAM_ART_PAGE_SIZE - 1u))) {
        free_contig(result.art_va, NVRAM_ART_PAGE_SIZE);
        return NVRAM_ART_EALIGN;
    }
    memset(result.art_va, 0, NVRAM_ART_PAGE_SIZE);
    rc = unescape_art(result.encoded_va, result.encoded_len,
                       result.art_va, &result.art_len);
    if (rc != NVRAM_ART_OK) {
        free_contig(result.art_va, NVRAM_ART_PAGE_SIZE);
        return rc;
    }
    /* Publish CPU-written bytes for a consumer using the physical address. */
    cache_clean(result.art_va, NVRAM_ART_PAGE_SIZE);
    *out = result;
    return NVRAM_ART_OK;
}

void nvram_art_release(nvram_art_t *art)
{
    if (!art)
        return;
    if (art->art_va)
        free_contig(art->art_va, NVRAM_ART_PAGE_SIZE);
    memset(art, 0, sizeof(*art));
}
