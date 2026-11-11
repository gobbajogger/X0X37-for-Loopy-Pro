/* SPDX-License-Identifier: GPL-3.0-only */
/* Same packed bytes and sink-refusal prefixes after skipping a run candidate
 * that cannot beat a maximum-length hash match. The frozen compressor below
 * is also unchanged at a06e20189; helpers and buffers come from the existing LZ test.
 * Run in the authorized host environment:
 * cc -std=c99 -O2 -Wall -Wextra tests/host/lz_max_match_test.c -o /tmp/lz-max-match
 * /tmp/lz-max-match
 * Also run with -fsanitize=address,undefined -fno-omit-frame-pointer.
 * Wrong match lengths or distances change the packed bytes and are rejected.
 */
#define main lz_roundtrip_main
#include "lz_test.c"
#undef main

static int baseline_compress(const uint8_t *in, uint32_t n, lz_sink_t *o)
{
    uint32_t i = 0, lit = 0, k;
    if (n >= LZ_NONE)
        return -1;
    for (k = 0; k < (1u << LZ_HBITS); k++)
        lz_head[k] = LZ_NONE;
    while (i < n) {
        uint32_t best = 0, dist = 0;
        if (i + LZ_MIN <= n) {
            uint32_t h = lz_hash(in + i), c = lz_head[h], len;
            lz_head[h] = (uint16_t)i;
            if (c != LZ_NONE && (len = lz_match_len(in, n, c, i)) > best) {
                best = len;
                dist = i - c;
            }
            if (i && (len = lz_match_len(in, n, i - 1u, i)) > best) {   /* a run */
                best = len;
                dist = 1;
            }
        }
        if (best < LZ_MIN) {
            i++;
            continue;
        }
        if (lz_literals(o, in + lit, i - lit))
            return -1;
        {
            uint8_t m[3] = {(uint8_t)(0x80u | (best - LZ_MIN)), (uint8_t)(dist - 1u), (uint8_t)((dist - 1u) >> 8)};
            if (o->put(o->ctx, m, 3))
                return -1;
        }
        for (k = 1; k < best && i + k + LZ_MIN <= n; k++)       /* the positions inside the match */
            lz_head[lz_hash(in + i + k)] = (uint16_t)(i + k);
        i += best;
        lit = i;
    }
    return lz_literals(o, in + lit, i - lit);
}


static uint8_t expected[sizeof packed];
static uint32_t cases;

static int compare(uint32_t n, uint32_t limit)
{
    lz_sink_t sink = {put, 0};
    npacked = 0;
    cap = limit;
    int before = baseline_compress(in, n, &sink);
    uint32_t expected_n = npacked;
    memcpy(expected, packed, expected_n);
    npacked = 0;
    int after = lz_compress(in, n, &sink);
    cases++;
    if (before != after || npacked != expected_n ||
        memcmp(expected, packed, expected_n)) {
        fprintf(stderr, "FAIL n=%u cap=%u rc=%d/%d size=%u/%u\n",
                n, limit, before, after, expected_n, npacked);
        return 1;
    }
    return 0;
}

static int check(uint32_t n)
{
    if (compare(n, sizeof packed)) return 1;
    uint32_t full = npacked;
    const uint32_t limits[] = {0, 1, 2, 3, 127, 128, 129,
                              full ? full - 1u : 0u, full};
    for (uint32_t j = 0; j < sizeof limits / sizeof limits[0]; j++)
        if (compare(n, limits[j])) return 1;
    return 0;
}

int main(void)
{
    /* Run the existing roundtrip, malformed-stream and size-limit coverage. */
    if (lz_roundtrip_main()) return 1;
    for (uint32_t kind = 0; kind < 5; kind++) {
        for (uint32_t i = 0; i < MAXN; i++) {
            switch (kind) {
            case 0: in[i] = 0; break;                 /* maximum-length ties */
            case 1: in[i] = (uint8_t)(i % 7u); break; /* hash wins, run loses */
            case 2: in[i] = (uint8_t)(rnd() % 3u); break;
            case 3: in[i] = (uint8_t)rnd(); break;    /* collisions/literals */
            default: in[i] = i % 528u < 400u ? 0 : (uint8_t)rnd(); break;
            }
        }
        for (uint32_t n = 0; n <= 400u; n++)
            if (check(n)) return 1;
        const uint32_t lengths[] = {1024, 4096, 30000, MAXN, 0xFFFFu};
        for (uint32_t j = 0; j < sizeof lengths / sizeof lengths[0]; j++)
            if (check(lengths[j])) return 1;
    }
    printf("lz maximum-match differential: %u cases ok\n", cases);
    return 0;
}
