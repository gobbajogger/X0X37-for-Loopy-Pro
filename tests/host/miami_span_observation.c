/* SPDX-License-Identifier: GPL-3.0-only */
/* Reuse the unchanged PR #830 workload and kernel comparator. No timers. */
#include "original_miami_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int observation_render_words(original_miami_backend *, original_miami_fx_workspace *,
    uint32_t *, uint32_t *, uint32_t);
#define main unchanged_main
#define fixture_dispatch unchanged_dispatch
#define original_miami_backend_render_words observation_render_words
#include "miami_zero_xmod_assessment.c"
#undef original_miami_backend_render_words
#undef fixture_dispatch
#undef main

extern int observed_miami_backend_render_words(original_miami_backend *,
    original_miami_fx_workspace *, uint32_t *, uint32_t *, uint32_t);

enum { BLOCKS = 128, MAX_CALLS = BLOCKS * 256 * ORIGINAL_MIAMI_BACKEND_VOICES };
typedef struct { unsigned block, voice, frames; } call_record;
static call_record trace[MAX_CALLS];
static original_miami_backend *states, *current_backend, *baseline_address;
static original_miami_backend initial_state;
static unsigned observed, blocks, trace_calls;
static unsigned spans, voice_calls, eligible_spans, eligible_calls, span_is_eligible;
static unsigned span_hist[65], voice_hist[65], eligible_hist[65];
static unsigned voices[ORIGINAL_MIAMI_BACKEND_VOICES];
static unsigned eligible_voices[ORIGINAL_MIAMI_BACKEND_VOICES];
static unsigned long long span_frames, voice_frames;
static unsigned pending_voice, pending_frames, pending_call;

static void require(int ok, const char *message)
{
    if (!ok) { fprintf(stderr, "OBSERVATION_MISMATCH %s\n", message); exit(31); }
}

static unsigned eligible(unsigned ordinal) { return ordinal != 0 && ordinal % 64 == 0; }

/* Called only by the temporary backend, once per leaf span after splitting. */
void span_observe_begin(uint32_t n)
{
    require(observed && n >= 1 && n <= 64, "span domain");
    ++span_hist[n];
    span_frames += n;
    span_is_eligible = eligible(++spans);
    eligible_spans += span_is_eligible;
}

/* Called at the active-voice dispatch point; it never decides whether to render. */
void span_observe_voice(unsigned id, uint32_t n)
{
    require(observed && id < ORIGINAL_MIAMI_BACKEND_VOICES && n >= 1 && n <= 64,
        "voice domain");
    ++voice_calls;
    require(!pending_call, "one observation per kernel call");
    pending_call = 1;
    pending_voice = id;
    pending_frames = n;
    ++voice_hist[n];
    ++voices[id];
    voice_frames += n;
    if (span_is_eligible) {
        ++eligible_calls;
        ++eligible_hist[n];
        ++eligible_voices[id];
    }
}

int fixture_dispatch(original_miami_ms20 *f, original_voice_gain *g, original_oscillators *p,
    const original_prepared_oscillators *c, float vco, float *l, float *r, uint32_t n)
{
    unsigned id;
    for (id = 0; id < ORIGINAL_MIAMI_BACKEND_VOICES; ++id)
        if (f == &current_backend->voices[id].filter) break;
    require(id < ORIGINAL_MIAMI_BACKEND_VOICES && trace_calls < MAX_CALLS, "trace bounds");
    if (observed) {
        require(pending_call && pending_voice == id && pending_frames == n,
            "observation agrees with actual kernel call");
        pending_call = 0;
    }
    call_record row = {blocks, id, n};
    if (!observed) trace[trace_calls] = row;
    else require(trace[trace_calls].block == row.block &&
        trace[trace_calls].voice == row.voice && trace[trace_calls].frames == row.frames,
        "kernel call order/voice/frames");
    ++trace_calls;
    return unchanged_dispatch(f, g, p, c, vco, l, r, n);
}

static int observation_render_words(original_miami_backend *s, original_miami_fx_workspace *w,
    uint32_t *l, uint32_t *r, uint32_t n)
{
    require(blocks < BLOCKS && n == 256, "unchanged block shape");
    current_backend = s;
    if (!blocks) {
        if (!observed) { baseline_address = s; initial_state = *s; }
        else require(s == baseline_address && !memcmp(&initial_state, s, sizeof *s),
            "initial complete state/address");
    }
    int ok = observed ? observed_miami_backend_render_words(s, w, l, r, n) :
        original_miami_backend_render_words(s, w, l, r, n);
    if (!observed) states[blocks] = *s;
    else require(!memcmp(&states[blocks], s, sizeof *s), "complete backend state after block");
    ++blocks;
    return ok;
}

static void histogram(const char *name, const unsigned *hist)
{
    printf(",\"%s\":{", name);
    unsigned first = 1;
    for (unsigned n = 1; n <= 64; ++n) if (hist[n]) {
        printf("%s\"%u\":%u", first ? "" : ",", n, hist[n]);
        first = 0;
    }
    putchar('}');
}

int main(int argc, char **argv)
{
    if (argc != 4) return 2;
    /* Boundary assertions include the unsigned rollover value. The workload's
     * ordinal is bounded by MAX_CALLS and cannot wrap. */
    require(!eligible(0) && !eligible(1) && !eligible(63) && eligible(64) &&
        !eligible(65) && !eligible(127) && eligible(128) && !eligible(UINT32_MAX),
        "eligibility boundaries");
    states = calloc(BLOCKS, sizeof *states);
    require(states != NULL, "state allocation");
    char *base_args[] = {argv[0], argv[1], argv[2], NULL};
    int result = unchanged_main(3, base_args);
    require(result == 0 && blocks == BLOCKS && calls == trace_calls, "baseline fixture");
    unsigned baseline_calls = trace_calls;
    blocks = trace_calls = calls = 0;
    observed = 1;
    char *observed_args[] = {argv[0], argv[1], argv[3], NULL};
    result = unchanged_main(3, observed_args);
    require(result == 0 && blocks == BLOCKS && trace_calls == baseline_calls &&
        calls == trace_calls, "observed fixture");
    unsigned reference_spans = 0, reference_eligible_calls = 0;
    unsigned reference_hist[65] = {0}, reference_voice_hist[65] = {0};
    unsigned reference_eligible_hist[65] = {0};
    for (unsigned i = 0; i < baseline_calls; ++i) {
        const call_record *row = &trace[i];
        if (row->voice == 0) { ++reference_spans; ++reference_hist[row->frames]; }
        ++reference_voice_hist[row->frames];
        /* Independent selection expression derived from the unchanged trace. */
        if (reference_spans % 64 == 0) {
            ++reference_eligible_calls;
            ++reference_eligible_hist[row->frames];
        }
    }
    require(!pending_call && reference_spans == spans &&
        reference_eligible_calls == eligible_calls &&
        !memcmp(reference_hist, span_hist, sizeof span_hist) &&
        !memcmp(reference_voice_hist, voice_hist, sizeof voice_hist) &&
        !memcmp(reference_eligible_hist, eligible_hist, sizeof eligible_hist),
        "histograms/eligibility agree with unchanged call trace");
    require(spans > 0 && span_frames == 32768 && voice_calls == trace_calls &&
        voice_frames == 65536 && eligible_spans == spans / 64 &&
        eligible_calls == eligible_spans * ORIGINAL_MIAMI_BACKEND_VOICES,
        "span/voice counters");
    printf("{\"spans\":%u,\"span_frames\":%llu,\"active_voice_calls\":%u,"
        "\"active_voice_frames\":%llu,\"eligible_spans\":%u,\"eligible_voice_calls\":%u,"
        "\"voice_calls\":[%u,%u],\"eligible_voice_ids\":[%u,%u],"
        "\"call_order_frames_state_match\":true,\"complete_state_blocks_compared\":%u,"
        "\"backend_state_bytes_per_block\":%zu,\"eligibility_boundary_checks\":8",
        spans, span_frames, voice_calls, voice_frames, eligible_spans, eligible_calls,
        voices[0], voices[1], eligible_voices[0], eligible_voices[1], blocks, sizeof *states);
    histogram("span_length_histogram", span_hist);
    histogram("active_voice_length_histogram", voice_hist);
    histogram("eligible_voice_length_histogram", eligible_hist);
    puts("}");
    free(states);
    return 0;
}
