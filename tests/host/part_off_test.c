/* SPDX-License-Identifier: GPL-3.0-only */
/* M7a: per-part ON/OFF (909, 808, Chicago = 303A, Miami = 303B, BREAK), the whole app on the host.
 * Storage: settings_t.rsv_bright (offset 17). 0xA0 | mask (bits 7-5 = 101) holds the OFF mask, bit p =
 * PART_p (engine.h order 909, 808, 303A, 303B, BRK). Any other value (0, legacy brightness 1-8) = all ON.
 * The test drives only the stored byte and project_apply_io / project_capture_stage, so the same source
 * builds on the untouched base (red) and on the implementation (green).
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

static void set_off(unsigned m) { proj.set.rsv_bright = (uint8_t)(0xA0u | (m & 31u)); project_apply_io(); }
static void set_byte(uint8_t v) { proj.set.rsv_bright = v; project_apply_io(); }
static uint8_t captured(void) { project_capture_stage(); return project_stage.set.rsv_bright; }

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
    end();
}
static void t_legacy(void)
{
    static const uint8_t legacy[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 0x1F, 0x80, 0xC0, 0xFF};
    begin("legacy_all_on_and_byte_preserved");
    for (unsigned i = 0; i < sizeof legacy; i++) {
        set_byte(legacy[i]);
        CHECK(captured() == legacy[i]);               /* an all-ON save leaves a legacy byte as it was */
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
        proj.set.rsv_bright = 0;                       /* the runtime state, not the stale byte, is saved */
        uint8_t c = captured();
        CHECK(c == (m ? (uint8_t)(0xA0u | m) : 0u));
        proj.set.rsv_bright = c; project_apply_io();
        CHECK(captured() == c);
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
static void t_independent(void)
{
    begin("independent_of_mute_solo");
    seq.mute = 0; seq.vmute = 0;
    set_off(0x15u);
    CHECK(seq.mute == 0 && seq.vmute == 0);
    seq.mute = seq_part_mask(PART_808) | seq_part_mask(PART_303B);
    CHECK(captured() == 0xB5u);
    seq.mute = 0;
    CHECK(captured() == 0xB5u);
    set_byte(0);
    end();
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
    t_live();
    t_midi_in();
    t_seq_gate();
    printf("part_off: %s (%d failing cases)\n", fails ? "FAIL" : "ok", fails);
    return fails != 0;
}
