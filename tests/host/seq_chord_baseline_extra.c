/* SPDX-License-Identifier: GPL-3.0-only */
/* Extra BEFORE captures, draw-time and playback hash at unfixed 94197fc8. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
#include "../../firmware/src/seq/held_chord.h"

static const char *evdir(void)
{
    const char *d = getenv("SEQ_CHORD_EVIDENCE");
    return d ? d : "evidence/before";
}
static void capture(const char *name)
{
    char path[1024];
    CHECK(snprintf(path, sizeof path, "%s/%s.png", evdir(), name) > 0);
    png_write(path);
    printf("WROTE %s\n", path);
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
    bp->step[step].note = notes[0];
    bp->step[step].flags = flags;
    bp->step[step].velocity = BASS_VELOCITY_DEFAULT;
    if (n > 1)
        CHECK(held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, step, notes, n) == 1);
    else
        (void)held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, step, 0, 0);
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
    CHECK(engine_bass_bank(voice) == kind);
}
static void seq_ready_part(int part, int kind, int len, int spage)
{
    bpart_t *bp;
    setup(V_SEQ, part);
    choose_kind(kind);
    ui.view = V_SEQ;
    ui.part = part;
    ui.overlay = O_NONE;
    ui.held_step = -1;
    ui.rec = 0;
    ui.spage = (uint8_t)spage;
    seq.playing = 0;
    bp = &cur_pat()->bass[bidx()];
    bp->len = (uint8_t)len;
    bp->transpose = 24;
    clear_bass(bidx());
}
static double cpu_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1000000.0;
}
static void time_paint(const char *tag, int n)
{
    int i;
    double t, full;
    paint(); /* warmup */
    t = cpu_ms();
    for (i = 0; i < n; i++) {
        memset(blit_hash, 0, sizeof blit_hash);
        paint();
    }
    full = (cpu_ms() - t) / n;
    printf("DRAW_TIME %s iters=%d full_ms=%.6f\n", tag, n, full);
}
static uint32_t playback_hash(void)
{
    uint32_t h = 2166136261u;
    int32_t audio[512];
    int i, j;
    seq.playing = 0;
    seq_start(&seq);
    for (i = 0; i < 128; i++) {
        engine_render(audio, 256);
        for (j = 0; j < 256; j++) {
            h ^= (uint32_t)audio[j];
            h *= 16777619u;
        }
    }
    seq_stop(&seq);
    engine_render(audio, 256);
    for (j = 0; j < 256; j++) {
        h ^= (uint32_t)audio[j];
        h *= 16777619u;
    }
    return h;
}
static void widths(void)
{
    static const char *s[] = {
        "C3", "C3x3", "C#3x6", "C", "C/E", "Cmin7", "Cmaj7", "Amin7/C",
        "Cmin7/A#", "C3 E3 G3", "C3 E3 G3 B3 D4 F4",
        "STEP 1  C3 V100",
        "STEP 1  C  C3 E3 G3 V100",
        "STEP 16  Cmin7/A#  C#-1 D#-1 F#-1 G#-1 A#-1 C#0 TIE ACC SLIDE V127",
        "WRITE 1  C3 E3 G3 V100", 0
    };
    int i;
    for (i = 0; s[i]; i++)
        printf("TW '%s' M=%d S=%d B=%d XS=%d\n", s[i],
               (int)tw(&FONT_M, s[i]), (int)tw(&FONT_S, s[i]),
               (int)tw(&FONT_B, s[i]), (int)tw(&FONT_XS, s[i]));
}
int main(void)
{
    static const uint8_t mono[] = {48};
    static const uint8_t triad[] = {48, 52, 55};
    static const uint8_t e3[] = {52};
    char path[1024];
    FILE *meas;
    int b, n = 400;
    uint32_t ph;
    uint16_t fb_chi[240 * 240], fb_mia[240 * 240];

    CHECK(snprintf(path, sizeof path, "%s/measurements-extra.txt", evdir()) > 0);
    meas = fopen(path, "w");
    CHECK(meas);

    widths();

    /* Chicago mono C3 on PART_303A. */
    seq_ready_part(PART_303A, ORIGINAL_SOUND_CHICAGO, 16, 0);
    b = bidx();
    set_chord(b, 0, mono, 1, G_NOTE);
    paint();
    memcpy(fb_chi, fb, sizeof fb);
    capture("before-chicago-mono");
    time_paint("chicago-mono", n);

    /* Miami mono C3 on PART_303B. Miami is never called 303. */
    seq_ready_part(PART_303B, ORIGINAL_SOUND_MIAMI, 16, 0);
    b = bidx();
    set_chord(b, 0, mono, 1, G_NOTE);
    paint();
    memcpy(fb_mia, fb, sizeof fb);
    capture("before-miami-mono");
    time_paint("miami-mono", n);

    /* Juno triad + slide, and WRITE, and timing, and playback. */
    seq_ready_part(PART_303B, ORIGINAL_SOUND_JUNO, 16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, (uint8_t)(G_NOTE | BS_SLIDE));
    set_chord(b, 1, e3, 1, G_NOTE);
    paint();
    capture("before-slide-triad");

    seq_ready_part(PART_303B, ORIGINAL_SOUND_JUNO, 16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    ui.rec = 1;
    ui.held_step = -1;
    ui.wpos[b] = 0;
    paint();
    capture("before-write-triad");

    /* Selected triad for NOTE cell / held line timing. */
    seq_ready_part(PART_303B, ORIGINAL_SOUND_JUNO, 16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    ui.held_step = 0;
    ui.held_step_part = ui.part;
    ui.held_step_pat = seq.ppat[ui.part];
    paint();
    capture("before-selected-triad-extra");
    time_paint("juno-held-triad", n);

    seq_ready_part(PART_303B, ORIGINAL_SOUND_JUNO, 16, 0);
    b = bidx();
    set_chord(b, 0, mono, 1, G_NOTE);
    time_paint("juno-mono", n);

    seq_ready_part(PART_303B, ORIGINAL_SOUND_JUNO, 16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    time_paint("juno-triad", n);

    /* Playback identity golden. Juno triad + ties, 16 steps. */
    seq_ready_part(PART_303B, ORIGINAL_SOUND_JUNO, 16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, (uint8_t)(G_NOTE | 0xF0u));
    {
        bpart_t *bp = &cur_pat()->bass[b];
        int s;
        for (s = 1; s <= 3; s++) {
            bp->step[s].note = 48;
            bp->step[s].flags = (uint8_t)(G_TIE | 0xF0u);
            bp->step[s].velocity = BASS_VELOCITY_DEFAULT;
            (void)held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, s, 0, 0);
        }
    }
    proj.set.bt |= PROJ_BT_CLK_IN;
    seq.ppat[TRK_BASS0 + b] = 0;
    seq.mute = 0;
    ph = playback_hash();
    printf("PLAYBACK_HASH 0x%08x\n", ph);
    fprintf(meas, "PLAYBACK_HASH 0x%08x\n", ph);
    fprintf(meas, "chicago-mono and miami-mono BEFORE frames written\n");
    fclose(meas);
    printf("seq_chord_baseline_extra: PASS (%u checks)\n", checks);
    return 0;
}
