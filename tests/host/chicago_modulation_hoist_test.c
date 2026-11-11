/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "chicago_modulation.h"

int before_original_chicago_ring_process(original_chicago_ring *, int16_t *, int16_t *, uint32_t);
int before_original_chicago_chorus_process(original_chicago_chorus *, int16_t *, int16_t *, uint32_t);
int before_original_chicago_phaser_process(original_chicago_phaser *, int16_t *, int16_t *, uint32_t);

static uint32_t rng;
static uint64_t samples, calls;
static uint32_t next(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}
static int pcm_equal(const int16_t *a, const int16_t *b, unsigned n)
{
    for (unsigned i = 0; i < n; ++i) if (a[i] != b[i]) return 0;
    return 1;
}
#define REQUIRE(c) do { if (!(c)) { \
    fprintf(stderr, "FAIL line=%d seed=%u block=%u\n", __LINE__, seed, block); return 1; \
} } while (0)

int main(void)
{
    static const unsigned sizes[] = {0, 1, 2, 31, 32, 33, 63, 64, 65, 127, 256, 1024};
    static const uint8_t activity[] = {0, 1, 255};
    const original_mod_event events[] = {{0, 4}, {0, 1}, {1, 4}, {31, 4}, {63, 4}, {64, 4}, {100, 2}, {511, 4}};
    int16_t al[1024], ar[1024], bl[1024], br[1024], input_l[1024], input_r[1024];
    int16_t ahl[1024], ahr[1024], bhl[1024], bhr[1024];
    original_chicago_ring ra, rb;
    original_chicago_chorus ca, cb;
    original_chicago_phaser pa, pb;
    unsigned seed, block;
    for (seed = 1; seed <= 8; ++seed) {
        rng = seed * UINT32_C(0x1234567);
        original_chicago_ring_init(&ra); rb = ra;
        REQUIRE(original_chicago_chorus_init(&ca, ahl, ahr));
        REQUIRE(original_chicago_chorus_init(&cb, bhl, bhr));
        original_chicago_phaser_init(&pa); pb = pa;
        for (block = 0; block < 2048; ++block) {
            unsigned n = sizes[next() % 12], e1 = block & 127, e2 = (block >> 7) * 8 + seed - 1;
            uint8_t active = activity[block % 3];
            ra.activity = rb.activity = ca.activity = cb.activity = pa.activity = pb.activity = active;
            /* Across seeds, cover all 128*128 edit pairs with live histories. */
            original_chicago_ring_set(&ra, 2, e1); original_chicago_ring_set(&rb, 2, e1);
            original_chicago_ring_set(&ra, 3, e2); original_chicago_ring_set(&rb, 3, e2);
            original_chicago_chorus_set(&ca, 2, e1); original_chicago_chorus_set(&cb, 2, e1);
            original_chicago_chorus_set(&ca, 3, e2); original_chicago_chorus_set(&cb, 3, e2);
            original_chicago_phaser_set(&pa, 2, e1); original_chicago_phaser_set(&pb, 2, e1);
            original_chicago_phaser_set(&pa, 3, e2); original_chicago_phaser_set(&pb, 3, e2);
            if (!(block % 37)) {
                original_chicago_ring_direct_event(&ra, 5); original_chicago_ring_direct_event(&rb, 5);
                original_chicago_chorus_direct_event(&ca, 2); original_chicago_chorus_direct_event(&cb, 2);
                original_chicago_phaser_direct_event(&pa, 5); original_chicago_phaser_direct_event(&pb, 5);
            }
            original_mod_queue q = {events, block % 5 ? 0 : 8, 0};
            ra.queue = rb.queue = ca.queue = cb.queue = pa.queue = pb.queue = q;
            for (unsigned i = 0; i < n; ++i) {
                uint16_t l = (uint16_t)next(), r = (uint16_t)next();
                if (!(block % 7)) l = r = 0;
                memcpy(input_l + i, &l, 2); memcpy(input_r + i, &r, 2);
            }
#define INPUT() do { \
    memcpy(al, input_l, n * sizeof(*al)); memcpy(bl, al, n * sizeof(*al)); \
    memcpy(ar, input_r, n * sizeof(*ar)); memcpy(br, ar, n * sizeof(*ar)); \
} while (0)
            INPUT();
            REQUIRE(before_original_chicago_ring_process(&ra, al, ar, n));
            REQUIRE(original_chicago_ring_process(&rb, bl, br, n));
            REQUIRE(pcm_equal(al, bl, n) && pcm_equal(ar, br, n));
            REQUIRE(!memcmp(&ra, &rb, sizeof(ra)));
            INPUT();
            REQUIRE(before_original_chicago_chorus_process(&ca, al, ar, n));
            REQUIRE(original_chicago_chorus_process(&cb, bl, br, n));
            REQUIRE(pcm_equal(al, bl, n) && pcm_equal(ar, br, n));
            REQUIRE(!memcmp(ahl, bhl, sizeof(ahl)) && !memcmp(ahr, bhr, sizeof(ahr)));
            original_chicago_chorus snapshot = cb;
            snapshot.left = ca.left; snapshot.right = ca.right; /* separate equivalent histories */
            REQUIRE(!memcmp(&ca, &snapshot, sizeof(ca)));
            INPUT();
            REQUIRE(before_original_chicago_phaser_process(&pa, al, ar, n));
            REQUIRE(original_chicago_phaser_process(&pb, bl, br, n));
            REQUIRE(pcm_equal(al, bl, n) && pcm_equal(ar, br, n));
            REQUIRE(!memcmp(&pa, &pb, sizeof(pa)));
            samples += 6u * n; calls += 3;
        }
    }
    printf("PASS modulation: %llu channel samples, %llu calls; PCM, state, histories bit-exact\n",
           (unsigned long long)samples, (unsigned long long)calls);
    return 0;
}
