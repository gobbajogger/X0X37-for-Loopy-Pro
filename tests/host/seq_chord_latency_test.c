/* SPDX-License-Identifier: GPL-3.0-only */
/* HOST evidence — not a device test.
 *
 * Fast note sequences (1 ms and 2 ms apart: 44 and 88 samples at FS=44100)
 * across SEQ screen enter/exit and a chord-label redraw. The event log is
 * sample-accurate: now_ms is set from the cumulative sample count before
 * each engine_render, and every plat_midi_out packet is recorded at that
 * musical time. Compared to the unfixed 94197fc8 baseline (same driver,
 * same script): the event log and the per-block audio hash must be
 * identical — no dropped or late note events.
 *
 * The chord namer and the held-line redraw are not on this path. They
 * run only in the UI task: UI_DRAW draw_303 (held/WRITE) and UI_DRAW
 * draw_knobs (NOTE cell), from ui_frame() at ~60 frames/s in the main
 * loop (main_fm1.c). They are not called from engine_render (audio ISR
 * / render_block), midi_in, or the TIMER5 10 kHz key scan. This file
 * asserts that, on a build that has the namer, seq_chord_namer_calls
 * does not increase inside engine_render.
 *
 * Also reports worst-case namer cost (host CPU nanoseconds per call)
 * and SEQ draw time (chicago-mono, juno-held-triad) for a fair
 * before/after comparison. Labelled HOST throughout.
 */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
#include <time.h>

static FILE *lat;
static uint32_t ev_samples;
static unsigned ev_n, blk_n;
static uint32_t ev_note_ons;

static void ev_tap(uint32_t pkt)
{
    uint8_t st = (uint8_t)(pkt >> 8), d1 = (uint8_t)(pkt >> 16), d2 = (uint8_t)(pkt >> 24);
    ev_n++;
    if (lat)
        fprintf(lat, "EV samples=%u ms=%u st=%02X d1=%02X d2=%02X\n",
                ev_samples, now_ms, st, d1, d2);
}

static double cpu_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1000000.0;
}

static void paint(void)
{
    ui.help = 0;
    ui.msg_until = 0;
    ui.touched = -1;
    draw_header();
    draw_main();
    draw_knobs();
}

static void clear_bass(int b)
{
    bpart_t *bp = &cur_pat()->bass[b];
    int s;
    for (s = 0; s < NSTEPS; s++) {
        (void)held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, s, 0, 0);
        bp->step[s].flags = G_REST;
        bp->step[s].velocity = 0;
    }
}

static void set_chord(int b, int step, const uint8_t *notes, int n, uint8_t flags)
{
    bpart_t *bp = &cur_pat()->bass[b];
    uint8_t got[HELD_CHORD_NOTES];
    int ng, i;
    CHECK(n >= 1 && n <= HELD_CHORD_NOTES);
    for (i = 1; i < n; i++)
        CHECK(notes[i] > notes[i - 1]);
    bp->step[step].note = notes[0];
    bp->step[step].flags = flags;
    bp->step[step].velocity = BASS_VELOCITY_DEFAULT;
    if (n > 1)
        CHECK(held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, step, notes, n) == 1);
    else
        (void)held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, step, 0, 0);
    ng = held_chord_get(proj.arr.lane, seq.ppat[ui.part], b, step, notes[0], got);
    CHECK(ng == n);
}

static void choose_kind(int kind)
{
    int voice = bidx();
    int32_t audio[512];
    int n;
    engine_bass_off(voice);
    CHECK(engine_bass_select_preset(voice, kind, 0, &proj.sound) == 0);
    for (n = 0; n < 1024 && engine_bass_preset_pending(voice); n++)
        engine_render(audio, 256);
    CHECK(!engine_bass_preset_pending(voice));
    CHECK(engine_bass_sound_bank(&proj.sound, voice) == kind);
}

static void seq_ready_part(int part, int kind, int len)
{
    bpart_t *bp;
    setup(V_SEQ, part);
    choose_kind(kind);
    ui.view = V_SEQ;
    ui.part = part;
    ui.overlay = O_NONE;
    ui.held_step = -1;
    ui.rec = 0;
    ui.spage = 0;
    seq.playing = 0;
    seq.send_notes = 1;
    seq.send_clock = 0;
    bp = &cur_pat()->bass[bidx()];
    bp->len = (uint8_t)len;
    bp->transpose = 24;
    clear_bass(bidx());
}

static uint32_t midi_pkt(int on, int note, int vel)
{
    uint32_t st = (uint32_t)((on ? 0x90u : 0x80u) | (seq.ch[TRK_BASS0] & 0x0Fu));
    return (st >> 4) | (st << 8) | ((uint32_t)note << 16) | ((uint32_t)vel << 24);
}

static uint32_t fnv(const int32_t *a, int n)
{
    uint32_t h = 2166136261u;
    int i;
    for (i = 0; i < n; i++) {
        uint32_t u = (uint32_t)a[i];
        h ^= u & 0xFFu; h *= 16777619u;
        h ^= (u >> 8) & 0xFFu; h *= 16777619u;
        h ^= (u >> 16) & 0xFFu; h *= 16777619u;
        h ^= (u >> 24) & 0xFFu; h *= 16777619u;
    }
    return h;
}

static void render_block(int n)
{
    int32_t audio[256];
    uint32_t h, ons0, held;
#ifdef SEQ_CHORD_LABEL_TIER1
    unsigned calls0 = seq_chord_namer_calls;
#endif
    memset(audio, 0, sizeof audio);
    now_ms = (uint32_t)((uint64_t)ev_samples * 1000u / (uint32_t)FS);
    ons0 = note_ons;
    engine_render(audio, (uint32_t)n);
#ifdef SEQ_CHORD_LABEL_TIER1
    /* The namer is not on the audio path. */
    CHECK(seq_chord_namer_calls == calls0);
#endif
    ev_samples += (uint32_t)n;
    ev_note_ons = note_ons;
    held = (uint32_t)engine_midi_held();
    h = fnv(audio, n * 2); /* stereo */
    blk_n++;
    if (lat)
        fprintf(lat, "BLK samples=%u n=%d held=%u ons=%u dons=%u hash=%08x\n",
                ev_samples, n, held, ev_note_ons, ev_note_ons - ons0, h);
}

static void put_note_midi(int on, int note, int vel)
{
    host_midi_put(midi_pkt(on, note, vel));
}

static void time_paint(const char *tag, int n)
{
    int i;
    double t, full;
    paint();
    t = cpu_ms();
    for (i = 0; i < n; i++) {
        memset(blit_hash, 0, sizeof blit_hash);
        paint();
    }
    full = (cpu_ms() - t) / n;
    printf("HOST_DRAW_TIME %s iters=%d full_ms=%.6f\n", tag, n, full);
    if (lat)
        fprintf(lat, "HOST_DRAW_TIME %s iters=%d full_ms=%.6f\n", tag, n, full);
}

static void run_burst(void)
{
    /* 32 notes, alternating 1 ms (44 samples) and 2 ms (88 samples) gaps,
     * with SEQ exit / enter / chord-label redraw at scripted slots. */
    const int N = 32;
    int i;
    uint8_t triad[3] = {48, 52, 55};

    ev_samples = 0;
    ev_n = 0;
    blk_n = 0;
    now_ms = 0;
    /* Hook plat_midi_out via midi_log AND a parallel EV line from a
     * wrapping fprintf: we assign a FILE and also write EV from a
     * temporary override — plat_midi_out already fprintf's to midi_log
     * with now_ms. We write those and our BLK lines to the same file
     * by using lat for BLK/EV; midi_log gets the raw host lines too
     * (compared as MIDI_LOG). */
    midi_log = lat;

    seq.send_notes = 1;
    seq.clk_src = SEQ_CLK_INT;

    for (i = 0; i < N; i++) {
        int note = 48 + (i % 12);
        int gap = (i & 1) ? 88 : 44; /* 2 ms, 1 ms */
        /* Scripted UI work BETWEEN audio blocks, never inside render:
         * exit SEQ, re-enter, then a held-chord label redraw. */
        if (i == 8) {
            ui.view = V_HOME;
            ui.held_step = -1;
            paint();
            if (lat) fprintf(lat, "UI seq-exit i=%d samples=%u\n", i, ev_samples);
        }
        if (i == 12) {
            ui.view = V_SEQ;
            ui.held_step = -1;
            paint();
            if (lat) fprintf(lat, "UI seq-enter i=%d samples=%u\n", i, ev_samples);
        }
        if (i == 16) {
            int b = bidx();
            set_chord(b, 0, triad, 3, G_NOTE);
            ui.view = V_SEQ;
            ui.held_step = 0;
            ui.held_step_part = ui.part;
            ui.held_step_pat = seq.ppat[ui.part];
            paint(); /* chord-label redraw (namer, UI task only) */
            if (lat) fprintf(lat, "UI chord-redraw i=%d samples=%u\n", i, ev_samples);
        }
        put_note_midi(1, note, 100);
        render_block(gap);
        put_note_midi(0, note, 0);
        render_block(gap);
    }
    if (lat) {
        fprintf(lat, "BURST notes=%d ev=%u blk=%u samples=%u ons=%u\n",
                N, ev_n, blk_n, ev_samples, ev_note_ons);
    }
    printf("HOST_BURST notes=%d ev=%u blk=%u samples=%u ons=%u\n",
           N, ev_n, blk_n, ev_samples, ev_note_ons);
    CHECK(ev_samples == (uint32_t)(N * (44 + 88))); /* 32 * (44+88) wait: each note is gap+gap, gap alternates */
    /* Each iteration: two renders of `gap`, gap is 44 or 88 alternating.
     * Total = 16*44*2 + 16*88*2 = 32*44 + 32*88 = 32*132 = 4224. */
    CHECK(ev_samples == 32u * 132u);
    midi_log = 0;
}

#ifdef SEQ_CHORD_LABEL_TIER1
static void namer_cost(void)
{
    static const uint8_t longn[] = {44, 46, 49, 52};
    static const uint8_t hexa[] = {48, 52, 55, 59, 62, 65};
    static const uint8_t clamp[] = {127, 127, 127};
    char dst[16];
    const int N = 100000;
    int i;
    double t, ns_long, ns_hex, ns_clamp;
    t = cpu_ms();
    for (i = 0; i < N; i++)
        (void)seq_chord_symbol(dst, longn, 4);
    ns_long = (cpu_ms() - t) * 1e6 / N;
    t = cpu_ms();
    for (i = 0; i < N; i++)
        (void)seq_chord_symbol(dst, hexa, 6);
    ns_hex = (cpu_ms() - t) * 1e6 / N;
    t = cpu_ms();
    for (i = 0; i < N; i++)
        (void)seq_chord_symbol(dst, clamp, 3);
    ns_clamp = (cpu_ms() - t) * 1e6 / N;
    printf("HOST_NAMER_COST iters=%d long_ns=%.1f hex_ns=%.1f clamp_ns=%.1f (host CPU, not device)\n",
           N, ns_long, ns_hex, ns_clamp);
    if (lat)
        fprintf(lat, "HOST_NAMER_COST iters=%d long_ns=%.1f hex_ns=%.1f clamp_ns=%.1f\n",
                N, ns_long, ns_hex, ns_clamp);
    fprintf(lat ? lat : stdout,
            "HOST_NAMER_CONTEXT seq_chord_symbol / chord_label_cached: "
            "UI_DRAW draw_303 (ui.c held/WRITE) and UI_DRAW draw_knobs "
            "(NOTE cell), called from ui_frame() in the main loop "
            "(main_fm1.c, ~60 fps). Not engine_render, not midi_in, "
            "not TIMER5 key scan.\n");
}
#endif

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "evidence/host-latency/after-log.txt";
    uint8_t mono[1] = {48};
    uint8_t triad[3] = {48, 52, 55};
    int b;

    lat = fopen(path, "w");
    CHECK(lat);
    fprintf(lat, "HOST evidence — not a device test.\n");
    fprintf(lat, "seq_chord_latency_test: fast 1-2 ms notes across SEQ enter/exit and a chord-label redraw.\n");
#ifdef SEQ_CHORD_LABEL_TIER1
    fprintf(lat, "build=after TIER1=%d TIER2=%d namer=seq_chord_symbol\n",
            SEQ_CHORD_LABEL_TIER1, SEQ_CHORD_LABEL_TIER2);
#else
    fprintf(lat, "build=baseline-or-no-namer (94197fc8 ui.c or TIER1 undefined)\n");
#endif

    /* Juno part, a triad in the store so a later chord-label redraw has work. */
    seq_ready_part(PART_303B, ORIGINAL_SOUND_JUNO, 16);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    ui.view = V_SEQ;
    ui.held_step = -1;
    paint(); /* warm the first frame, not in the log window */

    run_burst();

    /* Fair SEQ draw times (HOST). chicago-mono and a held-triad redraw. */
    seq_ready_part(PART_303A, ORIGINAL_SOUND_CHICAGO, 16);
    b = bidx();
    set_chord(b, 0, mono, 1, G_NOTE);
    ui.held_step = -1;
    time_paint("chicago-mono", 80);

    seq_ready_part(PART_303B, ORIGINAL_SOUND_JUNO, 16);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    ui.held_step = 0;
    ui.held_step_part = ui.part;
    ui.held_step_pat = seq.ppat[ui.part];
    time_paint("juno-held-triad", 80);

#ifdef SEQ_CHORD_LABEL_TIER1
    namer_cost();
#endif

    printf("HOST latency test PASS ev=%u blk=%u (log %s)\n", ev_n, blk_n, path);
    fclose(lat);
    lat = 0;
    return 0;
}
