/* SPDX-License-Identifier: GPL-3.0-only */
/* Per-pattern preset recall and per-note bass sound lock (owner 2026-11-12):
 * storage (stream 13 stays byte-identical, 14 only when used, a rev-13 load
 * forces the new bytes to 0), the gated preset switch, lock capture (opt-in)
 * and lock playback at the onset. Runs the whole simulator firmware. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/original_sound.h"
#include "../../firmware/src/app/original_runtime.h"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static uint64_t hash;
static void block(unsigned frames)
{
    int32_t audio[512];
    engine_render(audio, frames);
    for (unsigned i = 0; i < frames * 2u; ++i) hash = (hash ^ (uint32_t)audio[i]) * UINT64_C(1099511628211);
}
static void settle(void) { for (unsigned i = 0; i < 12; ++i) block(256); }
static void wait_applied(int v) { for (unsigned i = 0; i < 4096 && engine_bass_preset_pending(v); i++) block(256); }
static uint32_t fbits(float x) { uint32_t b; memcpy(&b, &x, 4); return b; }
static void select_chicago(int v, int factory)
{
    CHECK(engine_bass_select_preset(v, ORIGINAL_SOUND_CHICAGO, factory, &proj.sound) == 0);
    settle(); wait_applied(v);
    CHECK(engine_bass_preset_pending(v) == 0 && engine_bass_preset_index(v) == factory);
}
static void cold(void)
{
    engine_project_hold(1);
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound);
    engine_project_release();
    settle();
}
static uint32_t stream_format_on_flash(void)
{
    uint8_t b[16];
    int n = plat_store_load(OBJ_PAT0, io_buf, sizeof io_buf);
    CHECK(n >= 16);
    memcpy(b, io_buf, 16);
    return get32(b);
}
/* the stream exactly as save_stream writes it, with a chosen format word (a rev-13 writer) */
static void save_stream_as(uint32_t format)
{
    const uint8_t *raw = (const uint8_t *)&proj + RAW_OFF;
    st_out_t o = {0, 0, 0, 0, 0, format};
    lz_sink_t sink = {st_put, &o};
    CHECK(!measure_project(&proj, &packed_len));
    o.fill = STREAM_HDR;
    put32(io_buf, format); put32(io_buf + 4, RAW_LEN); put32(io_buf + 8, crc32_of(raw, RAW_LEN));
    put32(io_buf + 12, packed_len);
    CHECK(!lz_compress(raw, RAW_LEN, &sink) && !o.err && !st_flush(&o));
    while (o.k < NSTREAM) CHECK(!st_flush(&o));
}
/* a sequenced bass line: 16 steps, notes on 0 and 4 */
static void line(int pat)
{
    bpart_t *bp = &proj.pat[pat].bass[0];
    for (int s = 0; s < NSTEPS; s++) bp->step[s].flags = G_REST;
    bp->len = 16; bp->rate = RATE_16; bp->dir = DIR_FWD; bp->transpose = 24;
    bp->step[0].flags = G_NOTE; bp->step[0].note = 36; bp->step[0].velocity = 100;
    bp->step[4].flags = G_NOTE; bp->step[4].note = 43; bp->step[4].velocity = 100;
}
static void play_to_step(int step)
{
    for (unsigned i = 0; i < 2000; i++) {
        block(256);
        if (seq.playing && seq.t[TRK_BASS0].started && eng_step[TRK_BASS0] == step) return;
    }
    CHECK(0);
}
static int lock_matches(int v, const uint8_t *cell)
{
    for (unsigned p = 0; p < original_sound_count(cell[29]); p++) {
        float value;
        if (original_sound_is_performance(cell[29], p)) continue;
        if (!original_sound_value(cell, p, &value) || engine_original_value_bits(v, p) != fbits(value)) return 0;
    }
    return 1;
}

static void storage(void)
{
    /* unset: stream 13, exactly as before */
    CHECK(project_save() == 0 && stream_format_on_flash() == 13u);
    engine_project_hold(1); project_capture_stage();
    uint8_t h[32]; project_file_header(h); CHECK(get32(h + 20) == 13u);
    engine_project_release();
    /* set: stream 14; the values come back */
    proj.pat[2].rsv[0] = (uint8_t)(ORIGINAL_SOUND_CHICAGO << 7 | 6);
    proj.pat[2].rsv[1] = (uint8_t)(ORIGINAL_SOUND_MIAMI << 7 | 30);
    CHECK(project_save() == 0 && stream_format_on_flash() == 14u);
    engine_project_hold(1);
    CHECK(project_load_stage() == 0);
    CHECK(project_stage.pat[2].rsv[0] == proj.pat[2].rsv[0] && project_stage.pat[2].rsv[1] == proj.pat[2].rsv[1]);
    project_file_header(h); CHECK(get32(h + 20) == 14u && project_file_validate(h) == 0);
    CHECK(project_stage.pat[2].rsv[1] == (ORIGINAL_SOUND_MIAMI << 7 | 30));
    /* a rev-13 file: the bytes load as 0 */
    put32(h + 20, 13u); CHECK(project_file_validate(h) == 0);
    CHECK(project_stage.pat[2].rsv[0] == 0 && project_stage.pat[2].rsv[1] == 0);
    put32(h + 20, 15u); CHECK(project_file_validate(h) != 0);
    /* out of range: Miami has 30 presets */
    project_capture_stage(); project_stage.pat[5].rsv[1] = (uint8_t)(ORIGINAL_SOUND_MIAMI << 7 | 31);
    CHECK(project_validate_stage() != 0);
    project_stage.pat[5].rsv[1] = (uint8_t)(ORIGINAL_SOUND_MIAMI << 7); CHECK(project_validate_stage() != 0);
    /* Drone uses four formerly-unused byte values, not a new stream layout. */
    project_stage.pat[5].rsv[1] = 0xC1u; CHECK(project_validate_stage() == 0);
    project_stage.pat[5].rsv[1] = 0xC4u; CHECK(project_validate_stage() == 0);
    project_stage.pat[5].rsv[1] = 0xC0u; CHECK(project_validate_stage() != 0);
    project_stage.pat[5].rsv[1] = 0xC5u; CHECK(project_validate_stage() != 0);
    engine_project_release();
    /* a rev-13 stream on flash holding stale bytes there: they load as 0 */
    save_stream_as(13u);
    CHECK(stream_format_on_flash() == 13u);
    engine_project_hold(1);
    CHECK(project_load_stage() == 0);
    CHECK(project_stage.pat[2].rsv[0] == 0 && project_stage.pat[2].rsv[1] == 0);
    engine_project_release();
    proj.pat[2].rsv[0] = proj.pat[2].rsv[1] = 0;
    CHECK(project_save() == 0 && stream_format_on_flash() == 13u);
    printf("storage: stream 13 unset, 14 set, rev-13 bytes forced to 0\n");
}

static void preset_recall(void)
{
    select_chicago(0, 0);
    seq.ppat[TRK_BASS0] = 0; seq.pcue[TRK_BASS0] = 0xFF;
    /* gated: nothing switches while another pattern plays */
    CHECK(engine_bass_select_preset_at(0, ORIGINAL_SOUND_CHICAGO, 5, &proj.sound, 1) == 0);
    CHECK(engine_bass_preset_gate(0) == 1);
    for (int i = 0; i < 64; i++) block(256);
    CHECK(engine_bass_preset_index(0) == 0 && engine_bass_preset_pending(0));
    seq_cue_part(&seq, TRK_BASS0, 1);                 /* stopped: the cue takes effect at once */
    wait_applied(0);
    CHECK(seq.ppat[TRK_BASS0] == 1 && engine_bass_preset_index(0) == 5);
    /* the UI tick: unset patterns ask for nothing */
    proj.pat[1].rsv[0] = 0; proj.pat[2].rsv[0] = (uint8_t)(ORIGINAL_SOUND_CHICAGO << 7 | 9);
    pattern_sound_tick();
    CHECK(engine_bass_preset_pending(0) == 0 && engine_bass_preset_index(0) == 5);
    /* a cue to a pattern with a preset: requested, gated to it, switched when it plays */
    seq.pcue[TRK_BASS0] = 2; pattern_sound_tick();
    CHECK(engine_bass_preset_gate(0) == 2 && engine_bass_preset_pending(0));
    CHECK(engine_bass_preset_index(0) == 5);
    wait_applied(0);
    CHECK(seq.ppat[TRK_BASS0] == 2 && engine_bass_preset_index(0) == 8);
    pattern_sound_tick(); CHECK(engine_bass_preset_pending(0) == 0);   /* asked once */
    /* a cue taken back cancels its waiting request */
    proj.pat[3].rsv[0] = (uint8_t)(ORIGINAL_SOUND_CHICAGO << 7 | 2);
    seq.pcue[TRK_BASS0] = 3; pattern_sound_tick();
    CHECK(engine_bass_preset_gate(0) == 3 && engine_bass_preset_pending(0));
    seq.pcue[TRK_BASS0] = 0xFF; pattern_sound_tick();
    CHECK(engine_bass_preset_pending(0) == 0);
    for (int i = 0; i < 64; i++) block(256);
    CHECK(engine_bass_preset_index(0) == 8 && seq.ppat[TRK_BASS0] == 2);
    proj.pat[2].rsv[0] = proj.pat[3].rsv[0] = 0;
    /* Archived Drone recall follows the same part, MIDI channel and pattern gate.
     * Shipping has a terminal unavailable runtime while preserving the base sound. */
    uint8_t base_cell[32]; memcpy(base_cell,proj.sound.v[T_303][0],sizeof base_cell);
    bass_sound_tag_t base_tag=proj.sound.bass[0];
    seq.ch[TRK_BASS0]=6;
    int channel=seq.ch[TRK_BASS0];
    int available=original_runtime_kind_available(ORIGINAL_SOUND_DRONE);
    CHECK(engine_bass_bank_available(0,ORIGINAL_SOUND_DRONE)==available);
    proj.pat[4].rsv[0] = (uint8_t)bass_pattern_preset_encode(ORIGINAL_SOUND_DRONE, 2);
    seq.pcue[TRK_BASS0] = 4; pattern_sound_tick();
    CHECK(engine_bass_requested_bank(0) == ORIGINAL_SOUND_DRONE);
    CHECK(engine_bass_requested_preset(0) == 1);
    CHECK(engine_bass_preset_gate(0) == 4 && engine_bass_preset_pending(0));
    CHECK(!memcmp(base_cell,proj.sound.v[T_303][0],sizeof base_cell));
    CHECK(!memcmp(&base_tag,&proj.sound.bass[0],sizeof base_tag));
    CHECK(seq.ch[TRK_BASS0] == channel);
    wait_applied(0);
    CHECK(!engine_bass_preset_pending(0) && seq.ppat[TRK_BASS0] == 4);
    CHECK(engine_bass_preset_gate(0) == 4 && seq.ch[TRK_BASS0] == channel);
    CHECK(engine_bass_bank(0) == ORIGINAL_SOUND_DRONE);
    CHECK(engine_bass_preset_index(0) == 1);
    if (available) {
        CHECK(engine_bass_status(0) == ORIGINAL_RUNTIME_OK);
        CHECK(engine_bass_sound_bank(&proj.sound, 0) == ORIGINAL_SOUND_DRONE);
    } else {
        CHECK(engine_bass_status(0) == ORIGINAL_RUNTIME_UNAVAILABLE);
        CHECK(engine_bass_sound_bank(&proj.sound, 0) == ORIGINAL_SOUND_CHICAGO);
        CHECK(!memcmp(base_cell,proj.sound.v[T_303][0],sizeof base_cell));
        CHECK(!memcmp(&base_tag,&proj.sound.bass[0],sizeof base_tag));
        CHECK(!(engine_bass_keys_held() & 1u));
        pattern_sound_tick(); /* terminal means no retry loop on later UI ticks */
        CHECK(!engine_bass_preset_pending(0));
        CHECK(engine_bass_status(0) == ORIGINAL_RUNTIME_UNAVAILABLE);
    }
    proj.pat[4].rsv[0] = 0;
    select_chicago(0, 0); /* restore for note-lock tests */
    CHECK(engine_bass_bank(0) == ORIGINAL_SOUND_CHICAGO);
    CHECK(engine_bass_status(0) == ORIGINAL_RUNTIME_OK && seq.ch[TRK_BASS0] == channel);
    printf("preset recall: gated to the pattern, applied when it plays, cancelled with its cue\\n");
}

static uint64_t run_line(int pat)
{
    select_chicago(0, 0);
    seq.ppat[TRK_BASS0] = (uint8_t)pat;
    for (int t = 0; t < NTRACKS; t++) if (t != TRK_BASS0) seq.mute |= 0u;
    cold();
    engine_note_lock_refresh();
    hash = UINT64_C(1469598103934665603);
    seq_start(&seq);
    for (int i = 0; i < 200; i++) block(256);
    seq_stop(&seq); settle();
    return hash;
}

static void note_lock(void)
{
    int pat = 4;
    select_chicago(0, 0);
    line(pat); line(pat + 1);
    seq.ppat[TRK_BASS0] = (uint8_t)pat;
    /* off (the default): nothing is stored */
    CHECK(!engine_note_lock_capture_on());
    CHECK(engine_bass_note_capture(proj.arr.lane, pat, 0, 0, &proj.sound) == 1);
    CHECK(bass_note_find(proj.arr.lane, pat, 0, 0) < 0);
    uint64_t plain = run_line(pat);
    /* a lock on a pattern that does not play changes nothing */
    engine_note_lock_capture(1);
    CHECK(engine_bass_note_capture(proj.arr.lane, pat + 1, 0, 0, &proj.sound) == 0);
    uint64_t other = run_line(pat);
    CHECK(other == plain);
    /* on: step 0 keeps preset 0's sound; the knobs then move to preset 12 */
    CHECK(engine_bass_note_capture(proj.arr.lane, pat, 0, 0, &proj.sound) == 0);
    int k = bass_note_find(proj.arr.lane, pat, 0, 0);
    CHECK(k >= 0 && proj.arr.lane[k].t == BASS_NOTE_TARGET && !engine_bass_note_validate(&proj.arr.lane[k]));
    uint8_t cell[32]; memcpy(cell, proj.arr.lane[k].val, 32);
    CHECK(project_save() == 0 && stream_format_on_flash() == 14u);
    engine_project_hold(1);
    CHECK(project_load_stage() == 0 && bass_note_find(project_stage.arr.lane, pat, 0, 0) >= 0);
    engine_project_release();
    select_chicago(0, 12);
    uint8_t base[32]; memcpy(base, proj.sound.v[T_303][0], 32);
    CHECK(memcmp(base, cell, 32) != 0 && lock_matches(0, base));
    engine_note_lock_refresh();
    seq.ppat[TRK_BASS0] = (uint8_t)pat;
    seq_start(&seq);
    play_to_step(0);
    CHECK(lock_matches(0, cell));                     /* the locked onset plays its own sound */
    play_to_step(4);
    CHECK(lock_matches(0, base));                     /* the next onset: the knobs again */
    play_to_step(0);
    CHECK(lock_matches(0, cell));
    seq_stop(&seq); settle();
    CHECK(lock_matches(0, base));                     /* no lock outlives playback */
    /* a locked step that becomes a rest keeps a valid project */
    proj.pat[pat].bass[0].step[0].flags = G_REST;
    engine_project_hold(1); project_capture_stage(); CHECK(project_validate_stage() == 0); engine_project_release();
    /* clearing removes it; with no lock left the project is stream 13 again */
    seq_bass_sound_clear(&seq, 0, pat, 0);
    seq_bass_sound_clear(&seq, 0, pat + 1, 0);
    CHECK(bass_note_find(proj.arr.lane, pat, 0, 0) < 0);
    engine_note_lock_capture(0);
    CHECK(project_save() == 0 && stream_format_on_flash() == 13u);
    printf("note lock: off stores nothing; a lock plays at its onset only; save/load keeps it\n");
}

int main(void)
{
    load_force = 0;
    boot(); settle();
    storage();
    preset_recall();
    note_lock();
    printf("pattern-preset-notelock: %u checks passed\n", checks);
    return 0;
}

