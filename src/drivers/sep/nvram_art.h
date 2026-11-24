// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 feiyehua

#ifndef NVRAM_ART_H
#define NVRAM_ART_H

#include <stddef.h>
#include <stdint.h>

#define NVRAM_ART_PAGE_SIZE 0x4000u

typedef struct {
    /* Borrowed bytes in /chosen/nvram-proxy-data; excludes trailing NUL. */
    const uint8_t *encoded_va;
    uint64_t encoded_pa;
    size_t encoded_len;

    /* Owned, physically contiguous, zero-padded 16 KiB page. */
    uint8_t *art_va;
    uint64_t art_pa;
    size_t art_len; /* Decoded bytes, NOT the allocation size. */
} nvram_art_t;

enum {
    NVRAM_ART_OK = 0,
    NVRAM_ART_EINVAL = -1,
    NVRAM_ART_ENOPROXY = -2,
    NVRAM_ART_ENOTFOUND = -3,
    NVRAM_ART_EFORMAT = -4,
    NVRAM_ART_ETOOBIG = -5,
    NVRAM_ART_ENOMEM = -6,
    NVRAM_ART_EALIGN = -7,
    NVRAM_ART_ENONCONTIG = -8
};

/*
 * Call after Pongo's DeviceTree and memory initialization.
 * out must not already own an allocation: release it before reusing it.
 * On failure, *out is zero. Pongo's allocator may panic on OOM.
 * Reads the boot-time NVRAM proxy only; never writes persistent NVRAM.
 */
int nvram_art_load(nvram_art_t *out);

/* Call only after all consumers of art_pa have finished. */
void nvram_art_release(nvram_art_t *art);

#endif
