/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define X0X_DRUM_TABLES_DEFINE
#include "dsp/fxbus.h"
const char *const x0x_dist_names[8] = {"Off", "Diode", "Clip", "SAT", "BFZ", "PDIST", "Fold", "Crush"};
void reference_fxbus_init(fxbus_t *, int16_t *, int);
void reference_fxbus_set(fxbus_t *, int, int);
void reference_fxbus_process(fxbus_t *, const float *, const float *, const float *, float *, int);
void reference_fxbus_process_st(fxbus_t *, const float *, const float *, const float *, const float *, float *, float *, int);
static fxbus_t a, b;
static int16_t line_a[16384], line_b[16384];
static void same(void)
{
    /* Separate line ownership is the only expected pointer difference. */
    fxbus_t normalized = b;
    normalized.dbuf = a.dbuf;
    assert(!memcmp(&a, &normalized, sizeof a));
    assert(!memcmp(line_a, line_b, sizeof line_a));
}
int main(void)
{
    for (int stereo = 0; stereo < 2; ++stereo)
    for (int tape = 0; tape < 2; ++tape)
    for (int position = 0; position < 3; ++position) {
        reference_fxbus_init(&a, line_a, 16384); fxbus_init(&b, line_b, 16384);
        int params[] = {FX_DL_TYPE, FX_DL_PING, FX_DIST, FX_COMP, FX_RV_DECAY, FX_DL_FDBK};
        int values[] = {tape, 1, position == 0 ? 0 : 7, position == 0 ? 0 : 90, 0, 0};
        for (unsigned i = 0; i < sizeof params / sizeof *params; ++i) {
            reference_fxbus_set(&a, params[i], values[i]); fxbus_set(&b, params[i], values[i]);
        }
        for (int block = 0; block < 800; ++block) {
            int sizes[] = {0, 1, 31, 64, 127, 255, 256};
            int n = sizes[block % 7];
            float dry[256] = {0}, right[256] = {0}, rev[256] = {0}, dly[256] = {0};
            float l[256], r[256], cl[256], cr[256];
            for (int i = 0; i < 256; ++i) {
                l[i] = cl[i] = 123.0f; r[i] = cr[i] = -456.0f;
                if (i & 1) rev[i] = dly[i] = -0.0f;
            }
            /* Tail finishes before a new attack; scan hits first/middle/last. */
            if (n && (block == 5 || block == 750)) {
                int p = position == 0 ? 0 : position == 1 ? n / 2 : n - 1;
                rev[p] = 0.7f; dly[p] = -0.8f;
                dry[p] = 0.1f; right[p] = -0.2f;
            }
            if (stereo) {
                reference_fxbus_process_st(&a, dry, right, rev, dly, l, r, n);
                fxbus_process_st(&b, dry, right, rev, dly, cl, cr, n);
            } else {
                reference_fxbus_process(&a, dry, rev, dly, l, n);
                fxbus_process(&b, dry, rev, dly, cl, n);
            }
            assert(!memcmp(l, cl, sizeof l)); assert(!memcmp(r, cr, sizeof r)); same();
            if (block == 749) assert(a.dl_quiet >= a.dlen);
        }
    }
    puts("FX bus state/line/audio silence-tail-restart equivalence: PASS");
}
