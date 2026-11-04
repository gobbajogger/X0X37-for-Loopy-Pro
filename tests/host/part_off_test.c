/* SPDX-License-Identifier: GPL-3.0-only */
/* M7a: per-part ON/OFF (909, 808, Chicago = 303A, Miami = 303B, BREAK), the whole app on the host.
 * Storage (x0x.h PROJ_SET_TAIL): the settings object is the older 40-byte settings_t while every part is ON;
 * with any part OFF, the OFF mask (bit p = PART_p, engine.h order 909, 808, 303A, 303B, BRK) and three zero
 * reserved bytes follow it. settings_t.rsv_bright is unused again (opaque, kept).
 * The helpers below drive the stored mask and project_apply_io / project_capture_stage. Without
 * PROJ_SET_TAIL (base 4b240bd, and cf9a283's settings_t.rsv_bright 0xA0|mask) they use that older byte, so
 * the same source builds on the base, on cf9a283 (red for the storage cases) and here (green).
 * Build: tests/host/run_part_off.sh. Output: "  PASS|FAIL|NOT_RUN <case>". */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stddef.h>

int engine_bass_keys_held(void);
static int fails;
static const char *cur;
static int case_fail;
#define CHECK(c) do { if (!(c)) { case_fail++; printf("    check %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
static void begin(const char *n) { cur = n; case_fail = 0; }
static void end(void) { printf("  %s %s\n", case_fail ? "FAIL" : "PASS", cur); if (case_fail) fails++; }
static void not_run(const char *n, const char *why) { printf("  NOT_RUN %s (%s)\n", n, why); }

static void audio(void) { static int32_t b[512]; engine_render(b, 256); }
static void btn(int b, int down) { if (down) held_btn |= 1u << b; else held_btn &= ~(1u << b); run_ms(1); }
static void play(int on) { if (!!seq.playing != !!on) { btn(B_PLAY, 1); btn(B_PLAY, 0); } run_ms(20); }
static void midi3(int s, int a, int b) { min_q[mi_w++ % MQ] = (uint32_t)s >> 4 | (uint32_t)s << 8 | (uint32_t)a << 16 | (uint32_t)b << 24; }

#ifdef PROJ_SET_TAIL
static void set_off(unsigned m) { part_off_live = (uint8_t)(m & 31u); project_apply_io(); }
static void set_byte(uint8_t v) { proj.set.rsv_bright = v; part_off_live = 0; project_apply_io(); }
static uint8_t captured(void) { project_capture_stage(); return part_off_staged; }
#else
static void set_off(unsigned m) { proj.set.rsv_bright = (uint8_t)(0xA0u | (m & 31u)); project_apply_io(); }
static void set_byte(uint8_t v) { proj.set.rsv_bright = v; project_apply_io(); }
static uint8_t captured(void)
{
    project_capture_stage();
    return (project_stage.set.rsv_bright & 0xE0u) == 0xA0u ? project_stage.set.rsv_bright & 31u : 0u;
}
#endif

/* MIDI out: note-ons per channel, through the host's midi_log */
static int ons[16];
static void log_start(void) { if (midi_log) fclose(midi_log); midi_log = tmpfile(); }
static void log_count(void)
{
    char line[96]; unsigned ms, st, d1, d2;
    memset(ons, 0, sizeof ons);
    if (!midi_log) return;
    rewind(midi_log);
    while (fgets(line, sizeof line, midi_log))
        if (sscanf(line, "%u ms: %X %X %X", &ms, &st, &d1, &d2) == 4 && (st & 0xF0u) == 0x90u && d2)
            ons[st & 15u]++;
    fclose(midi_log); midi_log = 0;
}

static void populate(void)
{
    for (int k = 0; k < NKIT; k++) {
        dpart_t *d = &proj.pat[seq.ppat[k]].drum[k];
        for (int s = 0; s < 16; s++) sm_set(d->hit[0], s);
    }
    for (int b = 0; b < NBASS; b++) {
        bpart_t *bp = &proj.pat[seq.ppat[TRK_BASS0 + b]].bass[b];
        for (int s = 0; s < 16; s++) { bp->step[s].note = 60; bp->step[s].velocity = 100; bp->step[s].flags = (s & 1) ? G_REST : G_NOTE; }
    }
    proj.pat[seq.ppat[TRK_BRK]].brk.steps = 0xFFFFu;
}
/* play ~1 s on the sequencer; ons[] per channel afterwards */
static void run_pattern(void)
{
    seq.send_notes = 1;
    log_start();
    play(1); run_ms(1000); play(0); run_ms(50);
    log_count();
}
static int part_ons(int p) { return seq.ch[p] == SEQ_CH_OFF ? 0 : ons[seq.ch[p]]; }

static void t_struct(void)
{
    begin("struct_size_unchanged");
    CHECK(sizeof(settings_t) == 40u);
    CHECK(offsetof(settings_t, rsv_bright) == 17u);
    CHECK(PROJ_FORMAT == 1u);
    CHECK(PROJECT_FILE_SIZE == 36452u);
    end();
}
static void t_legacy(void)
{
    static const uint8_t legacy[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 0x1F, 0x80, 0xC0, 0xFF};
    begin("legacy_all_on_and_byte_preserved");
    for (unsigned i = 0; i < sizeof legacy; i++) {
        set_byte(legacy[i]);
        CHECK(captured() == 0);                       /* an older byte (brightness 1-8, anything) is all ON */
        CHECK(project_stage.set.rsv_bright == legacy[i]);   /* and is saved as it was */
        for (int k = 0; k < NKIT; k++) { eng_hit[k][0] = 0; engine_drum(k, 0, 1.0f); }
        audio();
        for (int k = 0; k < NKIT; k++) CHECK(eng_hit[k][0] != 0);
    }
    set_byte(0);
    end();
}
static void t_roundtrip(void)
{
    begin("roundtrip_each_mask");
    for (unsigned m = 0; m < 32; m++) {
        set_off(m);
        uint8_t c = captured();
        CHECK(c == m);
        set_off(0); set_off(c);
        CHECK(captured() == m);
    }
    set_byte(0);
    end();
}
static void t_midi_ch(void)
{
    begin("midi_ch_untouched");
    uint8_t ch0[NPARTS];
    project_capture_stage(); memcpy(ch0, project_stage.set.midi_ch, sizeof ch0);
    for (unsigned m = 0; m < 32; m++) {
        set_off(m);
        project_capture_stage();
        CHECK(!memcmp(ch0, project_stage.set.midi_ch, sizeof ch0));
    }
    set_byte(0);
    end();
}
static void t_validator(void)
{
    begin("validator_still_rejects_midi_ch_18");
    project_capture_stage();
    project_stage.set.midi_ch[4] = 18;
    CHECK(project_validate_stage() != 0);
    end();
    begin("forward_compat_validator_accepts_marker");
    project_capture_stage();
    project_stage.set.rsv_bright = 0xBFu;
    CHECK(project_validate_stage() == 0);
    end();
}
/* Part ON/OFF and the mutes/solo are independent: OFF is not a mute, and a mute is not OFF. Chicago (303A)
 * shows both directions: its internal voice runs behind the bus mute (sequencer.c fire_bass), only OFF stops it. */
static void clear_pattern(void)
{
    for (int k = 0; k < NKIT; k++) memset(proj.pat[seq.ppat[k]].drum[k].hit, 0, sizeof proj.pat[seq.ppat[k]].drum[k].hit);
    for (int b = 0; b < NBASS; b++) {
        bpart_t *bp = &proj.pat[seq.ppat[TRK_BASS0 + b]].bass[b];
        for (int s = 0; s < NSTEPS; s++) { bp->step[s].note = 60; bp->step[s].velocity = 100; bp->step[s].flags = G_REST; }
    }
    proj.pat[seq.ppat[TRK_BRK]].brk.steps = 0;
}
/* play ~0.6 s; returns how many 10 ms samples had the 303 b's sequencer voice sounding */
static int sounding_run(int b)
{
    int n = 0;
    seq.send_notes = 1; log_start();
    play(1);
    for (int i = 0; i < 60; i++) { run_ms(10); n += seq.t[TRK_BASS0 + b].sounding != 0; }
    play(0); run_ms(50); log_count();
    return n;
}
static void t_independent(void)
{
    begin("independent_of_mute_solo");
    set_byte(0); seq.mute = 0; seq.vmute = 0; engine_smc_solo(0);
    clear_pattern();
    bpart_t *bp = &proj.pat[seq.ppat[TRK_BASS0]].bass[0];
    for (int s = 0; s < 16; s++) bp->step[s].flags = (s & 1) ? G_REST : G_NOTE;
    int ch = seq.ch[PART_303A];
    /* control: ON, unmuted: notes and a sounding voice */
    int snd = sounding_run(0); int on_ons = ons[ch];
    CHECK(snd > 0 && on_ons > 0);
    /* OFF while unmuted and unsoloed: silent (MIDI and voice), and the mutes are not written */
    set_off(1u << PART_303A);
    CHECK(seq.mute == 0 && seq.vmute == 0 && !seq_part_muted(&seq, PART_303A));
    snd = sounding_run(0);
    CHECK(snd == 0 && ons[ch] == 0);
    CHECK(seq.mute == 0 && seq.vmute == 0);
    /* back ON: the same notes again (OFF left nothing behind in the mutes) */
    set_byte(0);
    snd = sounding_run(0);
    CHECK(snd > 0 && ons[ch] == on_ons);
    /* ON while muted: the mute alone governs; MIDI out silent, the voice still runs behind the bus mute, and
     * the mute is not saved as OFF */
    seq.mute = seq_part_mask(PART_303A);
    snd = sounding_run(0);
    CHECK(snd > 0 && ons[ch] == 0);
    CHECK(captured() == 0);
    seq.mute = 0;
    /* muted AND OFF: silent (MIDI and voice); back ON with the mute kept, the mute alone governs again: MIDI
     * silent, the voice runs behind the bus mute; unmuted, the same notes as the control */
    seq.mute = seq_part_mask(PART_303A);
    set_off(1u << PART_303A);
    snd = sounding_run(0);
    CHECK(snd == 0 && ons[ch] == 0);
    CHECK(seq.mute == seq_part_mask(PART_303A) && captured() == (1u << PART_303A));
    set_byte(0);
    CHECK(seq.mute == seq_part_mask(PART_303A) && captured() == 0);
    snd = sounding_run(0);
    CHECK(snd > 0 && ons[ch] == 0);
    seq.mute = 0;
    snd = sounding_run(0);
    CHECK(snd > 0 && ons[ch] == on_ons);
    /* ON while another part is soloed (SMC solo): the sequencer still plays it; OFF is not taken from solo */
    engine_smc_solo((uint8_t)(1u << PART_909));
    snd = sounding_run(0);
    CHECK(snd > 0 && ons[ch] == on_ons && captured() == 0);
    engine_smc_solo(0);
    /* OFF mask survives mute edits, and the saved byte ignores the mutes */
    set_off(0x15u);
    seq.mute = seq_part_mask(PART_808) | seq_part_mask(PART_303B);
    CHECK(captured() == 0x15u);
    seq.mute = 0;
    CHECK(captured() == 0x15u);
    set_byte(0);
    end();
}
/* held -> part OFF -> release: every release path still ends the note */
static void t_release(void)
{
    begin("brk_pad_release_not_gated");
    set_byte(0);
    engine_brk_live(32, 1); audio(); audio();
    CHECK(engine_brk_held() != 0);
    set_off(1u << PART_BRK);
    engine_brk_live(32, 0); audio(); audio();
    CHECK(engine_brk_held() == 0);
    set_byte(0); audio(); audio();
    end();

    begin("midi_note_off_not_gated_break");
    set_byte(0);
    if (seq.ch[PART_BRK] == SEQ_CH_OFF) CHECK(!"break has no MIDI channel");
    else {
        midi3(0x90 | seq.ch[PART_BRK], 12, 100); audio(); audio();
        CHECK(engine_brk_held() != 0);
        set_off(1u << PART_BRK);
        midi3(0x80 | seq.ch[PART_BRK], 12, 0); audio(); audio();
        CHECK(engine_brk_held() == 0);
    }
    set_byte(0); audio(); audio();
    end();

    for (int b = 0; b < NBASS; b++) {
        int p = PART_303A + b;
        begin(b ? "midi_note_off_not_gated_miami" : "midi_note_off_not_gated_chicago");
        set_byte(0);
        midi3(0x90 | seq.ch[p], 50, 100); audio(); audio();
        CHECK(engine_bass_keys_held() >> b & 1);
        set_off(1u << p);
        midi3(0x80 | seq.ch[p], 50, 0); audio(); audio();
        CHECK(!(engine_bass_keys_held() >> b & 1));
        set_byte(0); end();

        begin(b ? "bass_off_note_not_gated_miami" : "bass_off_note_not_gated_chicago");
        engine_bass_on_velocity(b, 48, 0, 0, 100); audio(); audio();
        CHECK(engine_bass_keys_held() >> b & 1);
        set_off(1u << p);
        engine_bass_off_note(b, 48); audio(); audio();
        CHECK(!(engine_bass_keys_held() >> b & 1));
        set_byte(0); end();

        begin(b ? "live_303_held_off_release_miami" : "live_303_held_off_release_chicago");
        engine_bass_on_velocity(b, 52, 0, 0, 100); audio(); audio();
        CHECK(engine_bass_keys_held() >> b & 1);
        set_off(1u << p);
        engine_bass_off(b); audio(); audio();
        CHECK(!(engine_bass_keys_held() >> b & 1));
        set_byte(0); end();
    }
}
/* the sequencer 303: a long TIE'd note, the part turned OFF mid-note: released by the next step, MIDI too */
static void t_seq_bass_release(void)
{
    for (int b = 0; b < NBASS; b++) {
        int p = PART_303A + b;
        begin(b ? "seq_bass_tie_released_after_off_miami" : "seq_bass_tie_released_after_off_chicago");
        uint8_t midi_ch0 = proj.set.midi_ch[p];
        proj.set.midi_ch[p] = (uint8_t)(3 + b);          /* an enabled MIDI channel (2 + b): the MIDI checks run */
        set_byte(0); seq.mute = 0; clear_pattern();
        int ch = seq.ch[p];
        CHECK(ch == 2 + b && ch != SEQ_CH_OFF);
        bpart_t *bp = &proj.pat[seq.ppat[TRK_BASS0 + b]].bass[b];
        bp->step[0].flags = G_NOTE;
        for (int s = 1; s < NSTEPS; s++) bp->step[s].flags = G_TIE;
        seq.send_notes = 1; log_start();
        play(1); run_ms(400);
        CHECK(seq.t[TRK_BASS0 + b].sounding);          /* control: the TIE holds the note */
        CHECK(seq.bass_out_valid[b]);
        set_off(1u << p);
        run_ms(300);                                     /* > 2 steps at 120 BPM */
        int still = seq.t[TRK_BASS0 + b].sounding != 0, out = seq.bass_out_valid[b] != 0;
        CHECK(!still);
        CHECK(!out);
        /* MIDI: every Note On on the channel has its Note Off before the stop (read before stopping) */
        int on = 0, off = 0; char line[96]; unsigned ms, st, d1, d2;
        if (midi_log) {
            rewind(midi_log);
            while (fgets(line, sizeof line, midi_log))
                if (sscanf(line, "%u ms: %X %X %X", &ms, &st, &d1, &d2) == 4 && (int)(st & 15u) == ch) {
                    if ((st & 0xF0u) == 0x90u && d2) on++;
                    else if ((st & 0xF0u) == 0x80u || (st & 0xF0u) == 0x90u) off++;
                }
            fclose(midi_log); midi_log = 0;
        }
        play(0); run_ms(50);
        CHECK(on > 0 && off >= on);
        proj.set.midi_ch[p] = midi_ch0;
        set_byte(0); end();
    }
}
static void t_seq_gate(void)
{
    int base[NPARTS];
    set_byte(0); populate(); run_pattern();
    for (int p = 0; p < NPARTS; p++) base[p] = part_ons(p);
    for (int p = 0; p < NPARTS; p++) {
        static const char *const n[NPARTS] = {"seq_gate_909", "seq_gate_808", "seq_gate_chicago", "seq_gate_miami", "seq_gate_break"};
        if (!base[p]) { not_run(n[p], "the all-ON run sent no notes for this part"); continue; }
        begin(n[p]);
        set_off(1u << p); run_pattern();
        CHECK(part_ons(p) == 0);
        end();
        begin(p == 0 ? "negative_control_others_unchanged_909" : p == 1 ? "negative_control_others_unchanged_808" :
              p == 2 ? "negative_control_others_unchanged_chicago" : p == 3 ? "negative_control_others_unchanged_miami" :
              "negative_control_others_unchanged_break");
        for (int q = 0; q < NPARTS; q++) if (q != p) CHECK(part_ons(q) == base[q]);
        end();
    }
    set_byte(0);
}
static void t_midi_in(void)
{
    static const char *const n[NPARTS] = {"midi_in_gate_909", "midi_in_gate_808", "midi_in_gate_chicago", "midi_in_gate_miami", "midi_in_gate_break"};
    for (int p = 0; p < NPARTS; p++) {
        int ch = seq.ch[p], note = p < NKIT ? GM_DRUM[p][0] : p == PART_BRK ? 12 : 48;
        if (ch == SEQ_CH_OFF) { not_run(n[p], "part has no MIDI channel"); continue; }
        begin(n[p]);
        for (int on = 0; on < 2; on++) {               /* on = 0: all ON (control), 1: this part OFF */
            if (on) set_off(1u << p); else set_byte(0);
            for (int k = 0; k < NKIT; k++) for (int v = 0; v < NDRUM; v++) eng_hit[k][v] = 0;
            midi3(0x90 | ch, note, 100); audio(); audio();
            int drum = 0; for (int k = 0; k < NKIT; k++) for (int v = 0; v < NDRUM; v++) drum |= eng_hit[k][v] != 0;
            int bass = engine_bass_keys_held(), brk = (int)engine_brk_held();
            if (!on) {
                if (p < NKIT) CHECK(drum); else if (p == PART_BRK) CHECK(brk); else CHECK(bass >> (p - PART_303A) & 1);
            } else
                CHECK(!drum && !bass && !brk);          /* swallowed, and nothing else plays it */
            midi3(0x80 | ch, note, 0); audio(); audio();
            CHECK(!engine_bass_keys_held() && !engine_brk_held());
        }
        end();
    }
    begin("midi_in_off_releases_held_note");
    set_byte(0);
    midi3(0x90 | seq.ch[PART_303A], 50, 100); audio(); audio();
    CHECK(engine_bass_keys_held() & 1);
    set_off(1u << PART_303A);
    midi3(0x80 | seq.ch[PART_303A], 50, 0); audio(); audio();
    CHECK(!(engine_bass_keys_held() & 1));             /* the note-off still arrives: no stuck note */
    end();
    set_byte(0);
}
static void t_live(void)
{
    static const char *const n[NPARTS] = {"live_key_gate_909", "live_key_gate_808", "live_key_gate_chicago", "live_key_gate_miami", "live_key_gate_break"};
    for (int p = 0; p < NPARTS; p++) {
        begin(n[p]);
        for (int on = 0; on < 2; on++) {
            if (on) set_off(1u << p); else set_byte(0);
            int got;
            if (p < NKIT) { eng_hit[p][0] = 0; engine_drum(p, 0, 1.0f); audio(); got = eng_hit[p][0] != 0; }
            else if (p == PART_BRK) { engine_brk_live(32, 1); audio(); audio(); got = engine_brk_held() != 0; engine_brk_live(32, 0); audio(); audio(); }
            else { engine_bass_on_velocity(p - PART_303A, 48, 0, 0, 100); audio(); audio(); got = engine_bass_keys_held() >> (p - PART_303A) & 1; engine_bass_off(p - PART_303A); audio(); audio(); }
            CHECK(on ? !got : got);
        }
        CHECK(!engine_bass_keys_held() && !engine_brk_held());
        end();
    }
    set_byte(0);
}
/* Storage (in-process, the host flash emulation): the settings object's two sizes, the round trip of every
 * mask, the tail's validation, the Bluetooth toggle's patch and the .x0x file. The cross-firmware cases
 * (base 4b240bd reading these bytes) are tests/host/part_off_store.c via run_part_off.sh. */
#define SET_N (4 + (int)sizeof(settings_t))
#ifdef SEQ_PART_OFF
static unsigned live_mask(void) { return seq.part_off; }
#else
static unsigned live_mask(void) { return 0; }        /* the base: no part ON/OFF */
#endif
static int save_reload(unsigned m)
{
    set_off(m);
    if (project_save()) return -100;
    set_off(m ^ 31u);                                 /* the runtime differs until the load */
    int rc = project_load();
    project_apply_io();
    return rc;
}
static void t_store(void)
{
    static uint8_t keep[PLAT_STORE_MAX], prefix[PLAT_STORE_MAX];
    begin("store_all_on_is_legacy_40");
    CHECK(save_reload(0) == 0);
    CHECK(store_len[OBJ_SET] == SET_N && live_mask() == 0);
    CHECK(get32(store[OBJ_SET]) == PROJ_FORMAT && !memcmp(store[OBJ_SET] + 4, &proj.set, sizeof proj.set));
    memcpy(prefix, store[OBJ_SET], (size_t)SET_N);
    end();
    begin("store_off_long_form");
    CHECK(save_reload(0x05u) == 0);
    CHECK(store_len[OBJ_SET] == SET_N + 4);
    CHECK(!memcmp(store[OBJ_SET], prefix, (size_t)SET_N));      /* the same 40 bytes, then the tail */
    CHECK(store[OBJ_SET][SET_N] == 0x05u && !store[OBJ_SET][SET_N + 1] && !store[OBJ_SET][SET_N + 2] &&
          !store[OBJ_SET][SET_N + 3]);
    CHECK(live_mask() == 0x05u && !project_storage_unsupported());
    end();
    begin("store_roundtrip_all_masks");
    for (unsigned m = 0; m < 32; m++) {
        int rc = save_reload(m);
        CHECK(rc == 0 && live_mask() == m && store_len[OBJ_SET] == (m ? SET_N + 4 : SET_N));
        if (rc || live_mask() != m) printf("    mask 0x%02x: rc %d loaded 0x%02x len %d\n", m, rc, live_mask(), store_len[OBJ_SET]);
    }
    end();
    begin("store_40_loads_all_on");
    CHECK(save_reload(0) == 0);
    set_off(0x1Fu);
    CHECK(project_load() == 0);
    project_apply_io();
    CHECK(live_mask() == 0 && !project_storage_unsupported());
    end();
    /* a long object the validator refuses: unsupported, and not one byte rewritten by a save */
    static const struct { const char *name; int at; uint8_t v; } bad[] = {
        {"store_rejects_mask_gt31", 0, 0x20u}, {"store_rejects_mask_gt31", 0, 0xFFu},
        {"store_rejects_reserved", 1, 1u}, {"store_rejects_reserved", 2, 0x80u}, {"store_rejects_reserved", 3, 1u},
    };
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        if (!i || strcmp(bad[i].name, bad[i - 1].name)) begin(bad[i].name);
        CHECK(save_reload(0x01u) == 0);
        store_len[OBJ_SET] = SET_N + 4;
        store[OBJ_SET][SET_N + bad[i].at] = bad[i].v;
        memcpy(keep, store[OBJ_SET], (size_t)(SET_N + 4));
        CHECK(project_load() == -9 && project_storage_unsupported());
        uint32_t w = store_writes;
        CHECK(project_save() == -9 && store_writes == w);
        CHECK(store_len[OBJ_SET] == SET_N + 4 && !memcmp(keep, store[OBJ_SET], (size_t)(SET_N + 4)));
        store[OBJ_SET][SET_N + bad[i].at] = bad[i].at ? 0u : 0x01u;    /* repaired: loads again */
        CHECK(project_load() == 0 && !project_storage_unsupported());
        project_apply_io();
        if (i + 1 == sizeof bad / sizeof bad[0] || strcmp(bad[i].name, bad[i + 1].name)) end();
    }
    begin("store_bt_toggle_keeps_tail");
    CHECK(save_reload(0x05u) == 0);
    memcpy(keep, store[OBJ_SET], (size_t)(SET_N + 4));
    uint8_t bt = proj.set.bt;
    project_device_bt_set(!(bt & 1u));
    CHECK(project_device_bt_save() == 0);
    CHECK(store_len[OBJ_SET] == SET_N + 4 && !memcmp(keep + SET_N, store[OBJ_SET] + SET_N, 4));
    CHECK((store[OBJ_SET][4 + offsetof(settings_t, bt)] & 1u) == (uint8_t)!(bt & 1u));
    project_device_bt_set(bt & 1u);
    CHECK(project_device_bt_save() == 0 && !memcmp(keep, store[OBJ_SET], (size_t)(SET_N + 4)));
    end();
    begin("file_import_all_on");
    {   /* the .x0x file keeps its size and has no part ON/OFF: an import plays every part */
        uint8_t h[PROJECT_FILE_HDR];
        set_off(0x05u);
        project_capture_stage();
        project_file_header(h);
        CHECK(get32(h + 8) == sizeof(project_t) && PROJECT_FILE_SIZE == 36452u);
        CHECK(project_file_validate(h) == 0);
        project_commit_stage();
        project_apply_io();
        CHECK(live_mask() == 0);
    }
    end();
    set_byte(0);
}

/* all ON: audio + MIDI CRC of a fixed pattern run, compared with the untouched base's */
static uint32_t render_crc(void)
{
    static int32_t b[512];
    uint32_t crc = 0;
    set_byte(0); populate(); seq.send_notes = 1;
    log_start();
    play(1);
    for (int i = 0; i < 400; i++) { engine_render(b, 256); crc = st_crc32((const uint8_t *)b, sizeof b) ^ (crc * 31u); }
    play(0);
    char line[96];
    if (midi_log) { rewind(midi_log); while (fgets(line, sizeof line, midi_log)) crc = st_crc32((const uint8_t *)line, (uint32_t)strlen(line)) ^ (crc * 31u); fclose(midi_log); midi_log = 0; }
    return crc;
}

int main(void)
{
    boot();
    run_ms(300);
    seq.bpm = 120.0f;
    const char *ref_out = getenv("PART_OFF_REF_OUT"), *ref_in = getenv("PART_OFF_REF_IN");
    if (ref_out) {                                       /* reference mode: only the all-ON render */
        FILE *f = fopen(ref_out, "w"); if (!f) return 2;
        fprintf(f, "%08x\n", render_crc()); fclose(f); return 0;
    }
    /* first: the bit-exact render from a fresh boot, as the reference was made */
    if (ref_in) {
        unsigned want = 0; FILE *f = fopen(ref_in, "r");
        begin("all_on_bit_exact_vs_base");
        CHECK(f && fscanf(f, "%x", &want) == 1);
        if (f) fclose(f);
        unsigned got = render_crc();
        printf("    crc base %08x here %08x\n", want, got);
        CHECK(got == want);
        end();
    } else not_run("all_on_bit_exact_vs_base", "PART_OFF_REF_IN not set");
    t_struct();
    t_legacy();
    t_roundtrip();
    t_midi_ch();
    t_validator();
    t_independent();
    t_release();
    t_seq_bass_release();
    t_live();
    t_midi_in();
    t_seq_gate();
    t_store();
    printf("part_off: %s (%d failing cases)\n", fails ? "FAIL" : "ok", fails);
    return fails != 0;
}
