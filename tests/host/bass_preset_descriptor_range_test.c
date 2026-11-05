/* SPDX-License-Identifier: GPL-3.0-only */
/* BEHAVIOR-CHANGING decode helper: accept actual descriptor maxima
 * (Chicago FxType 0..6) instead of requiring max==127.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../../firmware/src/app/bass_presets.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fprintf(stderr, "FAIL " __VA_ARGS__); fputc('\n', stderr); fails++; } } while (0)

static const x0x_param_t *param_fn(int bank, int param)
{
    return original_sound_param((unsigned)bank, (unsigned)param);
}

/* Old helper rule: reject unless max==127. */
static int decode_old(int bank, int index, uint8_t out[NPARAMS_MAX])
{
    if (bank < 0 || index < 0 || !out) return BASS_PRESET_INVALID;
    unsigned count = original_sound_count((unsigned)bank);
    if (!count || (unsigned)index >= original_sound_factories((unsigned)bank)) return BASS_PRESET_INVALID;
    if (!param_fn(bank, 0)) return BASS_PRESET_UNAVAILABLE;
    if (param_fn(bank, (int)count)) return BASS_PRESET_INVALID;
    for (unsigned i = 0; i < count; ++i) {
        const x0x_param_t *d = param_fn(bank, (int)i);
        if (!d || !d->name || d->max != 127 || d->def > d->max) return BASS_PRESET_INVALID;
    }
    return original_sound_init(out, (unsigned)bank, (unsigned)index) ? BASS_PRESET_OK : BASS_PRESET_INVALID;
}

int main(void)
{
    uint8_t out[NPARAMS_MAX];
    unsigned nchi = original_sound_factories(0);
    unsigned i;
    CHECK(nchi > 0, "chicago factory count");

    const x0x_param_t *fx = original_sound_param(0, 17);
    CHECK(fx && fx->max == 6, "chicago param17 max==6 (got %u)", fx ? fx->max : 0u);

    /* Old rule rejects every Chicago factory (FxType max 6). */
    CHECK(decode_old(0, 0, out) == BASS_PRESET_INVALID, "old rule rejects chicago 0");

    /* New helper accepts all Chicago factories. */
    for (i = 0; i < nchi && i < 81; ++i) {
        int rc = bass_preset_decode(0, (int)i, param_fn, out);
        CHECK(rc == BASS_PRESET_OK, "chicago factory %u decode rc=%d", i, rc);
    }

    if (original_sound_factories(1) > 0) {
        int rc = bass_preset_decode(1, 0, param_fn, out);
        CHECK(rc == BASS_PRESET_OK, "miami 0 decode rc=%d", rc);
    }

    CHECK(bass_preset_decode(0, 9999, param_fn, out) == BASS_PRESET_INVALID, "bad index");

    if (fails) {
        fprintf(stderr, "bass_preset_descriptor_range: %d FAIL\n", fails);
        return 1;
    }
    puts("bass_preset_descriptor_range: ok");
    return 0;
}
