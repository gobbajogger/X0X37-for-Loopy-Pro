/* SPDX-License-Identifier: GPL-3.0-only */
/* M1 audio/MIDI clear on the host app. Transport stays running.
 * Device and native builds are not this test. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stdio.h>
#include <string.h>
int engine_bass_attacked(int part);
int engine_part_sounding(void);

static int fails;
static const char *rec_case;
#define CHECK(c) do { \
    if (!(c)) { \
        fails++; \
        if (rec_case) \
            fprintf(stderr, "FAIL %s:%d: %s [%s]\n", __FILE__, __LINE__, #c, rec_case); \
        else \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
    } \
} while (0)

static int control(const char *name)
{
    for (int i = 0; i < engine_nparams(T_303, 0); ++i)
        if (!strcmp(engine_param(T_303, 0, i)->name, name)) return i;
    CHECK(0 && "required Chicago control missing");
    return 0;
}

static void silence_drums_and_break(pattern_t *pat)
{
    int kit, drum, word;
    pat->brk.steps = 0;
    for (kit = 0; kit < NKIT; kit++)
        for (drum = 0; drum < NDRUM; drum++)
            for (word = 0; word < NSW; word++)
                pat->drum[kit].hit[drum][word] = 0;
}

static void hold_bass(pattern_t *pat)
{
    bpart_t *bp = &pat->bass[0];
    int i;
    bp->len = 8;
    bp->rate = 0;
    bp->dir = DIR_FWD;
    for (i = 0; i < 8; i++)
        bp->step[i] = (bstep_t){99, G_TIE, 100};
    bp->step[0] = (bstep_t){36, G_NOTE, 110};
    bp->step[4] = (bstep_t){48, G_NOTE, 110};
    pat->bass[1].len = 8;
    for (i = 0; i < 8; i++)
        pat->bass[1].step[i] = (bstep_t){0, G_REST, 0};
}

static void play_until_gate(void)
{
    int i;
    seq.send_notes = 1;
    seq.send_clock = 1;
    seq.bpm = 150.0f;
    seq_start(&seq);
    for (i = 0; i < 400 && !(engine_part_sounding() & 2); i++)
        run_ms(1);
    CHECK(engine_part_sounding() & 2);
    CHECK(seq.playing == 1);
}

static void queued_note_does_not_sound(void)
{
    boot();
    CHECK(engine_bass_attacked(0) == 0);
    engine_bass_on(0, 40, 1, 0);
    engine_request_notes_off();
    run_ms(20);
    CHECK(engine_bass_attacked(0) == 0);
    CHECK((engine_part_sounding() & 2) == 0);
    CHECK(seq.playing == 0);
}

static void queued_physical_key_does_not_sound(void)
{
    boot();
    uint32_t generation, context; int arp;
    engine_live_key_state(0, &generation, &arp, &context);
    CHECK(engine_live_key(0, generation, 1, 48, 100, 0, 0));
    engine_request_notes_off();
    run_ms(20);
    CHECK((engine_part_sounding() & 2) == 0);
}

static void damper_stays_and_gate_clears(void)
{
    boot();
#if X0X_DEV_WOBBLE
    if (engine_bass_is_wobble(0)) {
        /* Miami live release does not use the Chicago damper pot. Panic still
         * clears the sounding note, and a later press can sound again. */
        engine_bass_on(0, 45, 0, 0);
        run_ms(30);
        CHECK(engine_part_sounding() & 2);
        engine_request_notes_off();
        run_ms(30);
        CHECK((engine_part_sounding() & 2) == 0);
        engine_bass_on(0, 47, 0, 0);
        run_ms(20);
        CHECK(engine_part_sounding() & 2);
        return;
    }
#endif
    engine_set(T_303, 0, control("Damper"), 127);
    run_ms(30);
    engine_bass_on(0, 45, 0, 0);
    run_ms(30);
    CHECK(engine_part_sounding() & 2);
    engine_bass_off(0);
    run_ms(30);
    CHECK(engine_part_sounding() & 2);
    engine_request_notes_off();
    run_ms(30);
    CHECK((engine_part_sounding() & 2) == 0);
    engine_bass_on(0, 47, 0, 0);
    run_ms(20);
    engine_bass_off(0);
    run_ms(20);
    CHECK(engine_part_sounding() & 2);
}

static void arp_run_stops(void)
{
    int i, returned = 0;
    boot();
    CHECK(engine_bass_select_preset(0, FACTORY_BANK_CHICAGO, 0, &proj.sound) >= 0);
    run_ms(20);
    engine_set(T_303, 0, control("ArpOn"), 127);
    run_ms(20);
    engine_bass_on(0, 60, 0, 0);
    run_ms(40);
    CHECK(engine_part_sounding() & 2);
    engine_request_notes_off();
    for (i = 0; i < 80; i++)
        run_ms(1);
    CHECK((engine_part_sounding() & 2) == 0);
    engine_bass_on(0, 64, 0, 0);
    for (i = 0; i < 40; i++) {
        run_ms(1);
        if (engine_part_sounding() & 2)
            returned = 1;
    }
    CHECK(returned);
}

static void drums_and_break_clear(void)
{
    boot();
    engine_drum(0, 0, 1.0f);
    run_ms(30);
    CHECK(engine_part_sounding() & 1);
    engine_request_notes_off();
    run_ms(20);
    CHECK((engine_part_sounding() & 1) == 0);
    engine_brk_live(3, 1);
    run_ms(20);
    CHECK(engine_midi_held() & (1 << (8 + 3)));
    engine_request_notes_off();
    run_ms(20);
    CHECK((engine_midi_held() & (1 << (8 + 3))) == 0);
    engine_brk_live(2, 1);
    engine_request_notes_off();
    run_ms(20);
    CHECK((engine_midi_held() & (1 << (8 + 2))) == 0);
}

static void transport_and_future_note(void)
{
    pattern_t *pat;
    uint8_t sound_cell, pos;
    uint32_t clocks, transport;
    int i, returned = 0;
    char midi_text[8192];
    size_t nread;
    boot();
    pat = &proj.pat[0];
    silence_drums_and_break(pat);
    hold_bass(pat);
    seq.ppat[PART_303A] = 0;
    seq.ppat[PART_303B] = 0;
    sound_cell = proj.sound.v[T_303][0][0];
    play_until_gate();
    pos = eng_step[TRK_BASS0];
    clocks = clock_out_count;
    transport = transport_out_count;
    midi_log = tmpfile();
    CHECK(midi_log != 0);
    engine_request_notes_off();
    run_ms(30);
    CHECK(seq.playing == 1);
    CHECK(seq.req_stop == 0);
    CHECK(seq.bpm == 150.0f);
    CHECK(proj.sound.v[T_303][0][0] == sound_cell);
    CHECK(pat->bass[0].step[4].note == 48);
    CHECK((engine_part_sounding() & 2) == 0);
    CHECK(transport_out_count == transport);
    CHECK(clock_out_count > clocks);
    rewind(midi_log);
    nread = fread(midi_text, 1, sizeof midi_text - 1, midi_log);
    midi_text[nread] = 0;
    {
        char off[8], stop[8];
        sprintf(off, " %02X ", 0x80 | seq.ch[TRK_BASS0]);
        sprintf(stop, " %02X ", 0xFCu);
        CHECK(strstr(midi_text, stop) == 0);
        CHECK(strstr(midi_text, off) != 0);
    }
    (void)pos;
    for (i = 0; i < 800; i++) {
        run_ms(1);
        if (engine_part_sounding() & 2) {
            returned = 1;
            break;
        }
    }
    CHECK(returned);
    CHECK(seq.playing == 1);
    CHECK(seq.t[TRK_BASS0].pos == 4);
    CHECK(seq.req_stop == 0);
}

/* A no-op command word. Type NTARGETS is not a sound target: drain hands it to
 * apply_param, which ignores it. The word only occupies CQ budget. */
static void push_harmless_cq(unsigned n)
{
    while (n--)
        engine_set(NTARGETS, 0, 0, 0);
}

static void push_midi(uint8_t status, uint8_t d1, uint8_t d2, uint8_t src)
{
    uint32_t cin = status >= 0xF0u ? 0x0Fu : (uint32_t)(status >> 4);
    host_midi_put(cin | ((uint32_t)src << 4) | ((uint32_t)status << 8) |
        ((uint32_t)d1 << 16) | ((uint32_t)d2 << 24));   /* stamped with its arrival epoch, as usb.c does */
}

static int spare_midi_channel(void)
{
    int used[16] = {0}, ch, t;
    for (t = 0; t < NTRACKS; t++)
        if (seq.ch[t] < 16)
            used[seq.ch[t]] = 1;
    for (ch = 0; ch < 16; ch++)
        if (!used[ch])
            return ch;
    return -1;
}

static void render_block(void)
{
    int32_t blk[512];
    engine_render(blk, 256);
}

static void check_original_bass(const bpart_t *before)
{
    CHECK(memcmp(before, &proj.pat[0].bass[0], sizeof *before) == 0);
    CHECK(before->step[0].note == 36 && before->step[0].flags == G_NOTE && before->step[0].velocity == 110);
    CHECK(before->step[4].note == 48 && before->step[4].flags == G_NOTE && before->step[4].velocity == 110);
    CHECK(before->step[1].note == 99 && before->step[1].flags == G_TIE);
    CHECK(before->dir == DIR_FWD);
}

static void check_other_steps(const bpart_t *before)
{
    const bpart_t *now = &proj.pat[0].bass[0];
    int i;
    CHECK(now->len == before->len && now->rate == before->rate && now->dir == before->dir);
    for (i = 1; i < 8; i++)
        CHECK(memcmp(&before->step[i], &now->step[i], sizeof now->step[i]) == 0);
}

static void check_cancelled(uint8_t gen)
{
    CHECK(!seq.rec_bass[0].active);
    CHECK(!seq.rec_bass[0].own_ok);
    CHECK(seq.rec_bass[0].gen == (uint8_t)(gen + 1u));
    CHECK(!seq.rec_bass[1].active);
    CHECK(seq.rec_mask == 1);
    CHECK(seq.rec_ext_mask == 0);
    CHECK(seq.playing == 1);
    CHECK(seq.req_stop == 0);
    CHECK(seq.t[TRK_BASS0].pos == 0);
    CHECK((engine_part_sounding() & 2) == 0);
    CHECK((engine_bass_keys_held() & 1) == 0);
}

static void check_recorded(int src)
{
    const bstep_t *st = &proj.pat[0].bass[0].step[0];
    CHECK(st->note == 42);
    CHECK(st->velocity == 80);
    CHECK(st->flags == (uint8_t)(G_NOTE | 0xF0u));
    CHECK(seq.rec_bass[0].active);
    CHECK(seq.rec_bass[0].note == 42);
    CHECK(seq.rec_bass[0].src == (uint8_t)src);
    CHECK(seq.rec_bass[0].own_ok);
    CHECK(seq.rec_bass[0].own_gen == seq.rec_bass[0].gen);
    CHECK(seq.rec_bass[0].own_src == (uint8_t)src);
    CHECK(seq.rec_bass[0].own_note == 42);
    CHECK(!seq.rec_bass[1].active);
    CHECK(seq.rec_mask == 1);
    CHECK(seq.rec_ext_mask == 0);
    CHECK(seq.playing == 1);
    CHECK(seq.req_stop == 0);
    CHECK(seq.t[TRK_BASS0].pos == 0);
    CHECK(engine_part_sounding() & 2);
    CHECK(engine_bass_keys_held() & 1);
}

/* Forward bass, REC armed. One 256-frame render at a time.
 * prefix 0: the onset is in the panic drain; it plays, so it records (0.0.25 design).
 * prefix 128 CQ / 64 MIDI: that drain is full, so the onset is the next press. */
static void rec_panic_window(int midi, unsigned prefix)
{
    pattern_t *pat;
    bpart_t bass_before, other_before;
    uint8_t gen;
    unsigned queued = prefix;
    char name[64];

    snprintf(name, sizeof name, "%s prefix %u", midi ? "midi" : "cq", prefix);
    rec_case = name;
    boot();
    mi_r = mi_w = 0;
    pat = &proj.pat[0];
    silence_drums_and_break(pat);
    hold_bass(pat);
    seq.ppat[PART_303A] = 0;
    seq.ppat[PART_303B] = 0;
    play_until_gate();
    render_block(); /* empty the CQ so the prefix is this drain's whole budget */
    seq.rec_mask = 1;
    CHECK(seq.t[TRK_BASS0].pos == 0);
    CHECK(seq_rec_step(&seq, TRK_BASS0) == 0);
    bass_before = pat->bass[0];
    other_before = pat->bass[1];
    gen = seq.rec_bass[0].gen;
    check_original_bass(&bass_before);
    CHECK(!seq.rec_bass[0].active && !seq.rec_bass[0].own_ok);
    CHECK(engine_part_sounding() & 2);

    engine_request_notes_off();
    if (midi) {
        int ch = spare_midi_channel();
        CHECK(ch >= 0);
        while (prefix--)
            push_midi((uint8_t)(0x90u | (uint8_t)ch), 1, 10, 0);
        push_midi((uint8_t)(0x90u | seq.ch[TRK_BASS0]), 42, 80, 0);
    } else {
        push_harmless_cq(prefix);
        engine_bass_record(0, 42, 80, 0);
    }
    check_original_bass(&bass_before);
    CHECK(memcmp(&other_before, &pat->bass[1], sizeof other_before) == 0);
    CHECK(!seq.rec_bass[0].active && !seq.rec_bass[0].own_ok);
    CHECK(seq.rec_bass[0].gen == gen);
    CHECK(engine_part_sounding() & 2);
    CHECK(seq.rec_mask == 1 && seq.playing == 1);

    render_block();
    if (queued == 0) {
        /* 0.0.25 PANIC (4e036296): notes-off runs first, so an onset accepted after
         * the request plays in this drain and records like any played note. No press
         * is both silenced and written (the d3510b82 case cannot occur). */
        check_other_steps(&bass_before);
        CHECK(memcmp(&other_before, &pat->bass[1], sizeof other_before) == 0);
        check_recorded(midi ? 0 : 3);
        CHECK(engine_part_sounding() & 2);   /* the same press sounds */
        return;
    }
    check_original_bass(&bass_before);
    CHECK(memcmp(&other_before, &pat->bass[1], sizeof other_before) == 0);
    check_cancelled(gen);
    if (midi)
        CHECK(plat_midi_pending());
    render_block();
    check_other_steps(&bass_before);
    CHECK(memcmp(&other_before, &pat->bass[1], sizeof other_before) == 0);
    check_recorded(midi ? 0 : 3);
}

static void rec_after_panic_still_records(void)
{
    pattern_t *pat;
    bpart_t bass_before, other_before;
    uint8_t gen;

    rec_case = "after cleanup";
    boot();
    mi_r = mi_w = 0;
    pat = &proj.pat[0];
    silence_drums_and_break(pat);
    hold_bass(pat);
    seq.ppat[PART_303A] = 0;
    seq.ppat[PART_303B] = 0;
    play_until_gate();
    render_block();
    seq.rec_mask = 1;
    CHECK(seq.t[TRK_BASS0].pos == 0);
    CHECK(seq_rec_step(&seq, TRK_BASS0) == 0);
    bass_before = pat->bass[0];
    other_before = pat->bass[1];
    gen = seq.rec_bass[0].gen;
    engine_request_notes_off();
    render_block();
    check_original_bass(&bass_before);
    CHECK(memcmp(&other_before, &pat->bass[1], sizeof other_before) == 0);
    check_cancelled(gen);
    push_midi((uint8_t)(0x90u | seq.ch[TRK_BASS0]), 42, 80, 0);
    check_original_bass(&bass_before);
    CHECK(!seq.rec_bass[0].active);
    CHECK(seq.rec_bass[0].gen == (uint8_t)(gen + 1u));
    render_block();
    check_other_steps(&bass_before);
    CHECK(memcmp(&other_before, &pat->bass[1], sizeof other_before) == 0);
    check_recorded(0);
    render_block();
    check_other_steps(&bass_before);
    CHECK(memcmp(&other_before, &pat->bass[1], sizeof other_before) == 0);
    check_recorded(0);
}

int main(void)
{
    queued_note_does_not_sound();
    queued_physical_key_does_not_sound();
    damper_stays_and_gate_clears();
    arp_run_stops();
    drums_and_break_clear();
    transport_and_future_note();
    rec_panic_window(0, 0);
    rec_panic_window(0, 128);
    rec_panic_window(1, 0);
    rec_panic_window(1, 64);
    rec_after_panic_still_records();
    rec_case = 0;
    if (fails) {
        fprintf(stderr, "panic clear: %d failures\n", fails);
        return 1;
    }
    printf("panic clear: 0 failures\n");
    return 0;
}
