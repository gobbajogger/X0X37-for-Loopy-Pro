/* SPDX-License-Identifier: GPL-3.0-only */
/* M1 audio/MIDI clear on the host app. Transport stays running.
 * Device and native builds are not this test. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stdio.h>
int engine_bass_attacked(int part);
int engine_part_sounding(void);

static int fails;
#define CHECK(c) do { \
    if (!(c)) { fails++; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } \
} while (0)

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

static void damper_stays_and_gate_clears(void)
{
    boot();
#if !X0X_DEV_WOBBLE
    /* The pinned base defaults bass A to Miami in either host build. */
    CHECK(engine_bass_select_preset(0, ORIGINAL_SOUND_CHICAGO, 0, &proj.sound) == 0);
    run_ms(30);
    CHECK(engine_bass_bank(0) == ORIGINAL_SOUND_CHICAGO);
#endif
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
    engine_set(T_303, 0, original_sound_count(ORIGINAL_SOUND_CHICAGO) - 1u, 127);
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
    CHECK(engine_bass_select_preset(0, ORIGINAL_SOUND_CHICAGO, 0, &proj.sound) == 0);
    run_ms(30);
    CHECK(engine_bass_bank(0) == ORIGINAL_SOUND_CHICAGO);
    engine_set(T_303, 0, 13, 127); /* Original Chicago ArpOn. */
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

static void queued_physical_note_does_not_sound(void)
{
    uint32_t generation, context;
    int arp;
    boot();
    run_ms(30);
    engine_live_key_state(0, &generation, &arp, &context);
    CHECK(engine_live_key(0, generation, 1, 60, 100, 0, 0));
    engine_request_notes_off();
    run_ms(20);
    CHECK(engine_original_keys(0, 0) == 0);
    CHECK((engine_bass_keys_held() & 1) == 0);
    CHECK((engine_part_sounding() & 2) == 0);
    engine_live_key_state(0, &generation, &arp, &context);
    CHECK(engine_live_key(0, generation, 2, 64, 100, 0, 0));
    run_ms(20);
    CHECK(engine_original_keys(0, 0) == 1);
    CHECK(engine_part_sounding() & 2);
    CHECK(engine_live_key(0, generation, 2, 64, 0, 0, 0));
    run_ms(30);
    CHECK(engine_original_keys(0, 0) == 0);
}

static void break_transport_keeps_running(void)
{
    int slice, bank, div, running;
    boot();
    proj.pat[0].brk.steps = 0xFFFFu;
    CHECK(load_slot(0, 0, "assets/factory-breaks/909FL.wav") == 0);
    CHECK(load_slot(1, 0, "assets/factory-breaks/909GR.wav") == 0);
    proj.pat[0].brk.slot_a = 0;
    proj.pat[0].brk.slot_b = 1;
    proj.pat[0].brk.set[BRK_PHRASE] = 0;
    proj.pat[0].brk.set[BRK_BCHANCE] = 100;
    proj.pat[0].brk.set[BRK_COMPLEX] = 0;
    proj.pat[0].brk.set[BRK_ROLL] = 0;
    proj.pat[0].brk.set[BRK_FILL] = 0;
    proj.pat[0].brk.set[BRK_ALEN] = 1;
    proj.pat[0].brk.set[BRK_BLEN] = 1;
    engine_brk_loops();
    seq.bpm = 300.0f;
    seq_start(&seq);
    for (int i = 0; i < 2200; i++) {
        run_ms(1);
        engine_brk_state(&slice, &bank, &div, &running);
        if (bank == 1 && slice == 2 && (seq.t[TRK_BRK].pos & 15u) == 2)
            break;
    }
    CHECK(running == 1);
    CHECK(bank == 1);
    CHECK(slice == 2);
    uint8_t pos = seq.t[TRK_BRK].pos;
    engine_request_notes_off();
    run_ms(6);
    engine_brk_state(&slice, &bank, &div, &running);
    CHECK(running == 1);
    CHECK(seq.playing == 1);
    CHECK(seq.t[TRK_BRK].pos == pos);
    CHECK(bank == 1);
    CHECK(slice == 2);
    for (int i = 0; i < 100 && seq.t[TRK_BRK].pos == pos; i++)
        run_ms(1);
    engine_brk_state(&slice, &bank, &div, &running);
    CHECK(running == 1);
    CHECK(seq.t[TRK_BRK].pos != pos);
    CHECK(bank == 1); /* A transport restart would force bank A, slice 0. */
    CHECK(slice == 3);
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
    engine_drum(1, 0, 1.0f);
    run_ms(30);
    CHECK(eng_peak[PART_808] > 0);
    eng_peak[PART_808] = 0;
    engine_request_notes_off();
    run_ms(20);
    CHECK(eng_peak[PART_808] == 0);
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

int main(void)
{
    queued_note_does_not_sound();
    queued_physical_note_does_not_sound();
    damper_stays_and_gate_clears();
    arp_run_stops();
    break_transport_keeps_running();
    drums_and_break_clear();
    transport_and_future_note();
    if (fails) {
        fprintf(stderr, "panic clear: %d failures\n", fails);
        return 1;
    }
    printf("panic clear: 0 failures\n");
    return 0;
}
