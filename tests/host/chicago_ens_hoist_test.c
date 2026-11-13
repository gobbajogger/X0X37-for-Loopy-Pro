/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare shipping Ensemble with the pinned, unmodified process function. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "chicago_ens.h"
#include "ens_before.h"

static uint32_t rng;
static uint64_t samples, calls;
static uint32_t next(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}

static int compare(chi_ens_t *old, chi_ens_t *now, int n)
{
    float a[1024], b[1024];
    int i;
    for (i = 0; i < n; ++i) {
        a[i] = (float)((int32_t)(next() % 65537u) - 32768) / 16384.0f;
        if ((calls & 7u) == 0) a[i] = 0.0f;
        b[i] = a[i];
    }
    ens_before_process(old, a, n);
    chi_ens_process(now, b, n);
    for (i = 0; i < n; ++i) {
        if (memcmp(&a[i], &b[i], sizeof(float))) {
            fprintf(stderr, "FAIL output call=%llu sample=%d\n", (unsigned long long)calls, i);
            return 0;
        }
    }
    if (memcmp(old, now, sizeof(*old))) {
        fprintf(stderr, "FAIL state call=%llu\n", (unsigned long long)calls);
        return 0;
    }
    samples += (unsigned)n; ++calls;
    return 1;
}

int main(void)
{
    static const int sizes[] = {0, 1, 2, 31, 32, 33, 63, 64, 127, 256, 1024};
    static const float rates[] = {0.0f, 1.0f, 8000.0f, 44100.0f, 48000.0f, 96000.0f};
    chi_ens_t old, now;
    unsigned seed, e1, e2, block;
    for (seed = 1; seed <= 8; ++seed) {
        rng = seed * UINT32_C(0x1234567);
        memset(&old, 0, sizeof(old)); chi_ens_init(&old); now = old;
        if (!compare(&old, &now, 32)) return 1; /* unset wet=0 */
        /* Every edit pair, changing parameters while histories keep running. */
        for (e1 = 0; e1 < 128; ++e1) for (e2 = 0; e2 < 128; ++e2) {
            float sr = rates[(e1 + e2 + seed) % 6];
            chi_ens_set(&old, e1, e2, sr); chi_ens_set(&now, e1, e2, sr);
            if (!compare(&old, &now, sizes[(e1 + e2) % 11])) return 1;
        }
        for (block = 0; block < 512; ++block) {
            if (!(block % 7)) {
                e1 = next() % 128; e2 = next() % 128;
                chi_ens_set(&old, e1, e2, 44100); chi_ens_set(&now, e1, e2, 44100);
            }
            if (!(block % 53)) { chi_ens_init(&old); chi_ens_init(&now); }
            if (!compare(&old, &now, sizes[next() % 11])) return 1;
        }
    }
    printf("PASS Ensemble: %llu samples, %llu calls; output and full state bit-exact\n",
           (unsigned long long)samples, (unsigned long long)calls);
    return 0;
}
