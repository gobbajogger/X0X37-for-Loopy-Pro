/* SPDX-License-Identifier: GPL-3.0-only */
/* Natural note/release activity regression. No panic, all-off, raw-key edits,
 * or render skipping is allowed to make a release become idle. */
#include "original_chicago_backend.h"
#include "original_miami_backend.h"
#include "chicago_fx2_bank.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { N = 256, RELEASE_BLOCKS = 16 };
static unsigned failures, cases, frames;
static uint64_t audio_hash = UINT64_C(1469598103934665603);
static uint64_t key_hash = UINT64_C(1469598103934665603);

static void hash_bytes(uint64_t *h, const void *data, size_t size)
{
    const unsigned char *p = data;
    for (size_t i = 0; i < size; ++i) *h = (*h ^ p[i]) * UINT64_C(1099511628211);
}
static void check_activity(const char *name, unsigned factory, unsigned mode, int actual, int expected)
{
    if (!!actual != !!expected) {
        printf("FAIL %s factory=%u mode=%u expected=%d actual=%d\n", name, factory, mode, expected, actual);
        ++failures;
    }
}

static void miami_block(original_miami_backend *s, original_miami_backend_workspace *w)
{
    assert(original_miami_backend_render_words(s, &w->fx, w->left, w->right, N));
    hash_bytes(&audio_hash, w->left, N * sizeof *w->left);
    hash_bytes(&audio_hash, w->right, N * sizeof *w->right);
    frames += N;
}

static void miami_releases(void)
{
    original_miami_backend s;
    uint32_t words[2][N];
    int32_t dry[2][N];
    int16_t scratch[2][N];
    original_miami_backend_workspace w = {words[0], words[1],
        {dry[0], dry[1], scratch[0], scratch[1], N}};
    for (unsigned factory = 0; factory < 30; ++factory) for (unsigned sustain = 0; sustain < 2; ++sustain) {
        assert(original_miami_backend_init(&s, factory, 137));
        assert(original_miami_backend_set_macro(&s, 17, (float)sustain));
        assert(original_miami_backend_prepare(&s));
        check_activity("Miami cold", factory, sustain, original_miami_backend_active(&s), 0);
        assert(original_miami_backend_note_on(&s, 60, 110));
        miami_block(&s, &w); miami_block(&s, &w);
        assert(original_miami_backend_prepare(&s));
        assert(original_miami_backend_note_off(&s, 60));
        assert(!s.keys.physical_count && s.keys.latched_count == 1);
        check_activity("Miami release tail", factory, sustain, original_miami_backend_active(&s), 1);
        if (sustain) {
            for (unsigned i = 0; i < 4; ++i) miami_block(&s, &w);
            assert(s.allocator.held_active.count && s.voices[0].active);
            check_activity("Miami sustain", factory, sustain, original_miami_backend_active(&s), 1);
            assert(original_miami_backend_set_macro(&s, 17, 0));
        }
        /* This fixed schedule is independent of the predicate under test. */
        for (unsigned i = 0; i < RELEASE_BLOCKS; ++i) miami_block(&s, &w);
        assert(!s.keys.committed_enable && !s.keys.effective_latch);
        assert(!s.keys.physical_count && s.keys.latched_count == 1);
        assert(!s.allocator.held_active.count);
        for (unsigned i = 0; i < ORIGINAL_MIAMI_BACKEND_VOICES; ++i)
            assert(!s.voices[i].active && !s.voices[i].awaiting_restart);
        assert(!s.output.fx.decimator.kernel.held_left && !s.output.fx.decimator.kernel.held_right);
        unsigned char before[sizeof s]; memcpy(before, &s, sizeof s);
        check_activity("Miami completed release", factory, sustain, original_miami_backend_active(&s), 0);
        check_activity("Miami idle", factory, sustain, original_miami_backend_idle(&s), 1);
        assert(!memcmp(before, &s, sizeof s));
        hash_bytes(&key_hash, &s.keys, sizeof s.keys);
        ++cases;
    }
}

/* Full public algorithms are a host-only dedicated-history wiring fixture.
 * This test does not admit the original shared-arena FX owner. */
typedef struct {
    original_chicago_fx2_bank bank;
    original_chicago_fx2_storage storage;
    original_chicago_fx2_owner owner;
} projection_fixture;
static void *allocate(size_t n) { void *p = calloc(1, n); assert(p); return p; }
static void fixture_init(projection_fixture *f)
{
    memset(f, 0, sizeof *f);
    f->storage = (original_chicago_fx2_storage){
        {allocate(2 * CHFX_SHORT_SLOTS), CHFX_SHORT_SLOTS},
        {allocate(2 * CHFX_BPM_STEREO_SLOTS), CHFX_BPM_STEREO_SLOTS},
        {allocate(2 * CHFX_GATE_SLOTS), CHFX_GATE_SLOTS},
        {allocate(2 * CHFX_HALL_SLOTS), CHFX_HALL_SLOTS},
        {allocate(4096), 2048}, allocate(4096), allocate(4096), 1024, {NULL, NULL}};
    assert(original_chicago_fx2_bank_init(&f->bank, &f->storage));
    f->owner = (original_chicago_fx2_owner){&f->bank,
        original_chicago_fx2_bank_find, original_chicago_fx2_bank_reset_public_projection};
}
static void fixture_free(projection_fixture *f)
{
    free(f->storage.short_delay.data); free(f->storage.bpm_delay.data);
    free(f->storage.gate.data); free(f->storage.hall.data); free(f->storage.chorus.data);
    free(f->storage.ensemble_left); free(f->storage.ensemble_right);
}
static void chicago_block(original_chicago_backend *s, original_chicago_backend_workspace *w)
{
    float left[N], right[N];
    assert(original_chicago_backend_render(s, w, NULL, 0, left, right, N) == 1);
    hash_bytes(&audio_hash, left, sizeof left); hash_bytes(&audio_hash, right, sizeof right);
    frames += N;
}
static void chicago_releases(void)
{
    original_chicago_backend s;
    original_chicago_tuning tuning;
    projection_fixture f; fixture_init(&f);
    uint32_t words[2][N]; int16_t converted[2][N], scratch[4][N];
    original_chicago_backend_workspace w = {words[0], words[1], converted[0], converted[1],
        {scratch[0], scratch[1], scratch[2], scratch[3]}, N};
    assert(original_chicago_tuning_bind(&tuning, 0, 0, NULL, 0x6d70));
    for (unsigned mode = 0; mode < 5; ++mode) {
        unsigned arp = mode >= 2, sustain = mode == 1 || mode >= 3;
        assert(original_chicago_backend_init(&s, 0, 137, 0, &tuning, &f.owner) == 1);
        assert(original_chicago_backend_set(&s, 10, 0) == 1); /* Public Release=0 bounds this focused tail. */
        assert(original_chicago_backend_set(&s, 13, (float)arp) == 1);
        assert(original_chicago_backend_set(&s, 17, 0) == 1); /* Bypass selected; no FX-tail idle claim. */
        assert(original_chicago_backend_set(&s, 21, (float)sustain) == 1);
        assert(original_chicago_backend_prepare(&s) == 1);
        assert(original_chicago_backend_note(&s, 60, 110) == 1);
        if (arp && sustain) {
            /* Before the first clock tick, the latch is the only Chicago
             * activity source: no physical key, voice or pending note. */
            assert(original_chicago_backend_release(&s, 60) == 1);
            assert(!s.keys.physical_count && s.keys.latched_count == 1);
            assert(!s.voice.active && !s.pending.mask);
            check_activity("Chicago latch before tick", 0, mode, original_chicago_backend_active(&s), 1);
        }
        chicago_block(&s, &w); chicago_block(&s, &w);
        assert(original_chicago_backend_prepare(&s) == 1);
        if (!(arp && sustain)) assert(original_chicago_backend_release(&s, 60) == 1);
        assert(!s.keys.physical_count && s.keys.latched_count == 1);
        check_activity("Chicago release tail", 0, mode, original_chicago_backend_active(&s), 1);
        if (sustain) {
            for (unsigned i = 0; i < 4; ++i) chicago_block(&s, &w);
            check_activity("Chicago sustain/latch", 0, mode, original_chicago_backend_active(&s), 1);
            if (mode == 4) {
                assert(original_chicago_backend_set(&s, 13, 0) == 1);
                assert(original_chicago_backend_prepare(&s) == 1);
            } else assert(original_chicago_backend_set(&s, 21, 0) == 1);
        }
        for (unsigned i = 0; i < RELEASE_BLOCKS; ++i) chicago_block(&s, &w);
        assert(!s.keys.physical_count && !s.voice.active && !s.pending.mask);
        assert(s.keys.latched_count == (mode < 3 ? 1u : 0u));
        unsigned char before[sizeof s]; memcpy(before, &s, sizeof s);
        check_activity("Chicago completed release", 0, mode, original_chicago_backend_active(&s), 0);
        assert(!memcmp(before, &s, sizeof s));
        hash_bytes(&key_hash, &s.keys, sizeof s.keys);
        ++cases;
    }
    fixture_free(&f);
}

int main(void)
{
    miami_releases(); chicago_releases();
    printf("RESULT cases=%u failures=%u stereo_frames=%u audio=%016llx raw_keys=%016llx\n",
        cases, failures, frames, (unsigned long long)audio_hash, (unsigned long long)key_hash);
    return failures ? 1 : 0;
}
