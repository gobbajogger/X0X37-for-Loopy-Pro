/* SPDX-License-Identifier: GPL-3.0-only */
/* Optional break-loop pack in the free tail of the app code area.
 * tools/break_pack.py writes the same bytes. Each loop is its whole IMA ADPCM
 * stream from the first sample. A cut or a shared prefix does not parse. */
#pragma once
#include <stdint.h>

#define BRK_PACK_MAX 8

typedef struct {
    char name[8];
    const uint8_t *adpcm;
    uint32_t n;
    uint32_t nbytes;
    uint32_t rate;
    uint8_t bars;
} brk_pack_loop_t;

static uint32_t brk_pack_crc(const uint8_t *p, uint32_t n)
{
    uint32_t crc = 0xFFFFFFFFu, i, b;
    for (i = 0; i < n; i++) {
        crc ^= p[i];
        for (b = 0; b < 8u; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static uint32_t brk_pack_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int brk_pack_magic(const uint8_t *p)
{
    return p[0] == 'X' && p[1] == '0' && p[2] == 'X' && p[3] == 'B' &&
           p[4] == 'R' && p[5] == 'K' && p[6] == 'P' && p[7] == 'K';
}

static int brk_pack_has0(const uint8_t *p)
{
    uint32_t i;
    for (i = 0; i < 8u; i++)
        if (p[i] == 0)
            return 1;
    return 0;
}

/* One pack at `at`. 0 = no. On success, *end is the first byte after the pack. */
static int brk_pack_at(const uint8_t *base, uint32_t limit, uint32_t at, brk_pack_loop_t *loops, int cap,
                       uint32_t *end)
{
    uint16_t version, count;
    uint32_t rate, crc, pos, i, ns, nbytes;
    uint8_t bars;
    if (at + 20u > limit || !brk_pack_magic(base + at))
        return 0;
    version = (uint16_t)(base[at + 8] | (base[at + 9] << 8));
    count = (uint16_t)(base[at + 10] | (base[at + 11] << 8));
    rate = brk_pack_u32(base + at + 12);
    crc = brk_pack_u32(base + at + 16);
    if (version != 1u || count < 1u || count > BRK_PACK_MAX || (rate != 44100u && rate != 22050u))
        return 0;
    if (count > (uint32_t)cap)
        return 0;
    pos = at + 20u;
    for (i = 0; i < count; i++) {
        uint32_t j;
        if (pos + 20u > limit)
            return 0;
        ns = brk_pack_u32(base + pos + 8);
        bars = base[pos + 12];
        nbytes = brk_pack_u32(base + pos + 16);
        if (ns < 64u || nbytes != (ns + 1u) / 2u || pos + 20u + nbytes > limit)
            return 0;
        if (bars != 1u && bars != 2u && bars != 4u && bars != 8u)
            return 0;
        if (!brk_pack_has0(base + pos))
            return 0;
        for (j = 0; j < 8u; j++)
            loops[i].name[j] = (char)base[pos + j];
        loops[i].adpcm = base + pos + 20u;
        loops[i].n = ns;
        loops[i].nbytes = nbytes;
        loops[i].rate = rate;
        loops[i].bars = bars;
        pos += 20u + nbytes;
    }
    if (brk_pack_crc(base + at + 20u, pos - (at + 20u)) != crc)
        return 0;
    for (i = pos; i < limit; i++)
        if (base[i] != 0xFFu)
            return 0;
    *end = pos;
    return (int)count;
}

/* The pack in [base, base + limit), or 0. The bytes after it, up to limit, are 0xFF. */
static int brk_pack_parse(const uint8_t *base, uint32_t limit, brk_pack_loop_t *loops, int cap)
{
    uint32_t at, end;
    if (limit < 20u)
        return 0;
    at = (limit - 20u) & ~15u;
    for (;;) {
        int n = brk_pack_at(base, limit, at, loops, cap, &end);
        if (n)
            return n;
        if (at == 0u)
            break;
        at -= 16u;
    }
    return 0;
}
