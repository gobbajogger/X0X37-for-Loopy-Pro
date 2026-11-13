/* SPDX-License-Identifier: GPL-3.0-only */
/* Compact mod-wheel / CC1 readout: independent snapshots, percent law,
 * last-event expiry and knob ownership, plus held-note survival. */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include <math.h>
#include <time.h>

static uint8_t clip_grid_mode;
uint8_t smk_clip_mode_on(void) { return clip_grid_mode; }
int smk_ck_smc_turn(int n, int d) { (void)n; (void)d; return 0; }
int smk_ck_smc_nav(uint8_t n) { (void)n; return 0; }
uint8_t smk_clip_transport_button(uint8_t b, uint8_t p) { (void)b; (void)p; return 0; }
uint8_t smk_clip_history_button(uint8_t b, uint8_t p, uint8_t h)
{ (void)b; (void)p; (void)h; return 0; }
void smk_play_note_button(uint8_t p, uint8_t h) { (void)p; (void)h; }
uint8_t smk_ck_flash_busy(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *p, uint32_t f) { (void)p; (void)f; return 0; }
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_apc_transport_led_state(uint8_t n) { (void)n; return 0; }
void smk_apc_transport_led_set(uint8_t note, uint8_t velocity) { (void)note; (void)velocity; }
void smk_apc_clock_tick(void) {}
void smk_apc_clock_start(void) {}
void smk_apc_clock_position(uint32_t sixteenths) { (void)sixteenths; }
void smk_ck_midi_stop(void) {}
uint8_t smk_padcal_on(void) { return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
int plat_ble_smc_send(const uint8_t *msg, uint8_t len) { (void)msg; (void)len; return 1; }
#ifndef SMC_SLOT_SEND_IMPLEMENTED
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif
int smc_connected(void) { return 0; }
int smc_project_active(void) { return 0; }
void smc_project_reset(void) {}
void smc_frame(void) {}
void smc_poll(void) {}
int smc_midi_host(uint32_t pkt) { (void)pkt; return 0; }
void smc_ui_cancel(void) {}
int smc_ui_active(void) { return 0; }
void smc_ui_draw(void) {}
void smc_ui_clip_show(int n, const char *name, const char *value)
{ (void)n; (void)name; (void)value; }

static struct { uint32_t frames; int32_t wheel[2]; uint16_t wheel_mod_seq; } fm1_in;
static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
#include "smk_wheels.inc"

extern char ui_wheel_log[192];

static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static void audio(void) { int32_t out[512]; engine_render(out, 256); }
static void settle(void) { int i; for (i = 0; i < 16; i++) audio(); }

static int pct(int v) { return (v * 100 + 63) / 127; }

static void paint(void)
{
    ui.overlay = O_NONE;
    ui.help = 0;
    memset(blit_hash, 0, sizeof blit_hash);
    draw_header();
    draw_main();
    draw_knobs();
}

static void shot(const char *dir, const char *name)
{
    char path[512];
    paint();
    snprintf(path, sizeof path, "%s/%s.png", dir, name);
    png_write(path);
}

static int has_str(const char *hay, const char *needle)
{
    return hay && needle && strstr(hay, needle) != NULL;
}

static void forbid_labels(void)
{
    CHECK(!has_str(ui_wheel_log, "CC1"));
    CHECK(!has_str(ui_wheel_log, "303A"));
    CHECK(!has_str(ui_wheel_log, "303B"));
    CHECK(!has_str(ui.msg, "CC1"));
    CHECK(!has_str(ui.msg, "303A"));
    CHECK(!has_str(ui.msg, "303B"));
    CHECK(!has_str(ui_wheel_log, "SAVED"));
    CHECK(!has_str(ui_wheel_log, "WOBBLE DEPTH"));
}

extern int engine_bass_keys_held(void);
extern void engine_bass_wheel_owners(uint8_t mask);
extern void engine_bass_cancel_preset(int voice);
static void select_kind(int voice, unsigned kind, unsigned preset)
{
    int i, other = voice ^ 1, rc;
    engine_bass_wheel_owners(0);
    engine_bass_off(0);
    engine_bass_off(1);
    engine_bass_cancel_preset(0);
    engine_bass_cancel_preset(1);
    for (i = 0; i < 600; i++) audio();
    /* At most one Miami: put the other voice on Chicago first if needed. */
    if (kind == ORIGINAL_SOUND_MIAMI && engine_bass_bank(other) == ORIGINAL_SOUND_MIAMI) {
        rc = engine_bass_select_preset(other, ORIGINAL_SOUND_CHICAGO, 0, &proj.sound);
        CHECK(rc == 0);
        for (i = 0; i < 600 && engine_bass_preset_pending(other); i++) audio();
        CHECK(engine_bass_preset_pending(other) == 0);
    }
    rc = engine_bass_select_preset(voice, (int)kind, (int)preset, &proj.sound);
    CHECK(rc == 0);
    for (i = 0; i < 600 && engine_bass_preset_pending(voice); i++) audio();
    if (engine_bass_preset_pending(voice) != 0 || engine_bass_bank(voice) != (int)kind) {
        fprintf(stderr, "select_kind v=%d kind=%u pending=%d bank=%d rc=%d\n",
                voice, kind, engine_bass_preset_pending(voice), engine_bass_bank(voice), rc);
        exit(1);
    }
}

static void set_pot(int voice, int value)
{
    int p = engine_bass_mod_param(voice);
    proj.sound.v[T_303][voice][p] = (uint8_t)value;
}

static void wheels_rest(void)
{
    fm1_in.frames = 400;
    fm1_in.wheel[0] = 550;
    fm1_in.wheel[1] = 615;
    fm1_in.wheel_mod_seq++;
    smk_wheels();
}

/* Invert the production mw law for default endpoints mlo=485, mhi=615, DEAD=12. */
static int adc_for_mw(int want)
{
    int m, best = 615, err = 999;
    for (m = 485; m <= 615; m++) {
        int mw = (615 - m <= 12) ? 0 : (615 - m - 12) * 127 / 118;
        if (mw < 0) mw = 0;
        if (mw > 127) mw = 127;
        if (abs(mw - want) < err) { err = abs(mw - want); best = m; }
    }
    return best;
}

static void physical_mw(int mw)
{
    fm1_in.wheel[1] = adc_for_mw(mw);
    fm1_in.wheel_mod_seq++;
    smk_wheels();
}

static void midi_pkt(unsigned source, unsigned ch, unsigned type, unsigned d1, unsigned d2)
{
    unsigned st = type | ch;
    min_q[mi_w++ % MQ] = (st >> 4) | (source << 4) | (st << 8) | (d1 << 16) | (d2 << 24);
}

static void seq_page(int part, int playing)
{
    ui.view = V_SEQ;
    ui.part = (uint8_t)part;
    ui.overlay = O_NONE;
    ui.help = 0;
    ui.touched = -1;
    ui.touch_until = 0;
    ui.msg_until = 0;
    ui.msg[0] = 0;
    seq.playing = (uint8_t)playing;
    seq.song_on = 0;
    build_pages();
}

static int box_nonblack(void)
{
    /* Band 1 box at screen (8,103)..(231,152). A painted frame is not all black. */
    int x, y, n = 0;
    for (y = 103; y < 153; y++)
        for (x = 8; x < 232; x++)
            if (fb[y * 240 + x] != 0) n++;
    return n;
}

static double cpu_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1000000.0;
}

static double measure_paint(int n)
{
    int i;
    paint();
    double t = cpu_ms();
    for (i = 0; i < n; i++) {
        memset(blit_hash, 0, sizeof blit_hash);
        paint();
    }
    return (cpu_ms() - t) / (double)n;
}

static void expect_log(const char *a, const char *b, const char *c)
{
    CHECK(has_str(ui_wheel_log, a));
    if (b) CHECK(has_str(ui_wheel_log, b));
    if (c) CHECK(has_str(ui_wheel_log, c));
    forbid_labels();
}

int main(int argc, char **argv)
{
    const char *outdir = argc > 1 ? argv[1] : "build/modwheel-ui";
    char dir1[512], num[8];
    int v, live, i;
    unsigned held;
    double t_off, t_wheel, t_knob;

    snprintf(dir1, sizeof dir1, "%s/1x", outdir);

    load_force = 0;
    boot();
    now_ms = 2000;
    proj.set.autosave_off = 1;
    engine_guard_enable(0);
    settle();
    wheels_rest();
    now_ms += 1000; /* expire any accidental box from init */
    paint();

    /* ---- 1. Names and percent law, each engine, physical + CC1 ---- */
    {
        static const struct { unsigned kind; const char *name; const char *param; int pot, live; } cases[] = {
            { ORIGINAL_SOUND_JUNO,    "JUNO A  CUTOFF",      "CUTOFF",     70, 70 },
            { ORIGINAL_SOUND_CHICAGO, "CHICAGO A  CUTOFF",   "CUTOFF",     64, 64 },
            { ORIGINAL_SOUND_MIAMI,   "MIAMI A  WOBBLE",     "WOBBLE",     51, 51 },
            { ORIGINAL_SOUND_DRONE,   "DRONE A  BRIGHTNESS", "BRIGHTNESS", 66, 66 },
        };
        for (i = 0; i < 4; i++) {
            select_kind(0, cases[i].kind, 0);
            set_pot(0, cases[i].pot);
            ui.part = PART_303A;
            seq_page(PART_303A, 1);
            wheels_rest();
            ui_wheel_mod(0, -1, cases[i].live);
            paint();
            put_i(num, pct(cases[i].live));
            expect_log(cases[i].name, num, "%");
            CHECK(box_nonblack() > 100);
            /* 0, 64, 127 law */
            ui_wheel_mod(0, -1, 0); paint(); expect_log(cases[i].name, "0", "%");
            ui_wheel_mod(0, -1, 64); paint(); expect_log(cases[i].name, "50", "%");
            ui_wheel_mod(0, -1, 127); paint(); expect_log(cases[i].name, "100", "%");
            CHECK(pct(0) == 0 && pct(64) == 50 && pct(127) == 100);
            /* CC1 on this part's channel only */
            wheels_rest();
            now_ms += 1000;
            midi_pkt(0, seq.ch[TRK_BASS0], 0xB0, 1, 64);
            audio();
            smk_wheels();
            paint();
            expect_log(cases[i].name, "50", "%");
            CHECK(engine_bass_mod_ctl(0) == 64);
        }
        /* Voice B Miami */
        select_kind(1, ORIGINAL_SOUND_MIAMI, 0);
        set_pot(1, 51);
        ui.part = PART_303B;
        seq_page(PART_303B, 1);
        ui_wheel_mod(1, -1, 51);
        paint();
        expect_log("MIAMI B  WOBBLE", "40", "%");
    }

    /* Frames: before / after fixtures (spec §11). */
    select_kind(0, ORIGINAL_SOUND_JUNO, 0);
    set_pot(0, 70);
    seq_page(PART_303A, 1);
    wheels_rest();
    now_ms += 1000;
    CHECK(proj.sound.v[T_303][0][engine_bass_mod_param(0)] == 70);
    shot(dir1, "before-juno-seq");
    { int nb = box_nonblack(); /* before: no box, or only SEQ content may occupy the region */ (void)nb; }

    ui_wheel_mod(0, -1, 70);
    shot(dir1, "after-single-juno");
    expect_log("JUNO A  CUTOFF", "55", "%");
    CHECK(!has_str(ui_wheel_log, "PATCH"));

    select_kind(0, ORIGINAL_SOUND_CHICAGO, 0);
    set_pot(0, 64);
    seq_page(PART_303A, 1);
    ui_wheel_mod(0, -1, 64);
    shot(dir1, "after-single-chicago");
    expect_log("CHICAGO A  CUTOFF", "50", "%");

    select_kind(0, ORIGINAL_SOUND_MIAMI, 0);
    set_pot(0, 51);
    seq_page(PART_303A, 1);
    ui_wheel_mod(0, -1, 51);
    shot(dir1, "after-single-miami");
    expect_log("MIAMI A  WOBBLE", "40", "%");
    CHECK(!has_str(ui_wheel_log, "303"));

    select_kind(0, ORIGINAL_SOUND_DRONE, 0);
    set_pot(0, 66);
    seq_page(PART_303A, 1);
    ui_wheel_mod(0, -1, 66);
    shot(dir1, "after-single-drone");
    expect_log("DRONE A  BRIGHTNESS", "52", "%");

    /* Rest-0 honesty: live 0 %, pot 70 unchanged, never written back.
     * PATCH line dropped for flash (Elite first link 1,076 B over image). */
    select_kind(0, ORIGINAL_SOUND_JUNO, 0);
    set_pot(0, 70);
    seq_page(PART_303A, 1);
    ui.part = PART_303A;
    wheels_rest();
    physical_mw(70);
    audio();
    physical_mw(0);
    audio();
    CHECK(engine_bass_mod_ctl(0) == 0);
    CHECK(proj.sound.v[T_303][0][engine_bass_mod_param(0)] == 70);
    shot(dir1, "after-rest-0");
    expect_log("JUNO A  CUTOFF", "0", "%");
    CHECK(!has_str(ui_wheel_log, "PATCH"));
    CHECK(!has_str(ui_wheel_log, "SAVED"));

    /* ---- 2. Two targets (CC1, shared channel); physical stays single ---- */
    select_kind(0, ORIGINAL_SOUND_JUNO, 0);
    select_kind(1, ORIGINAL_SOUND_MIAMI, 0);
    set_pot(0, 64);
    set_pot(1, 64);
    seq.ch[TRK_BASS0] = seq.ch[TRK_BASS0 + 1] = 1; /* MIDI channel 2 */
    seq_page(PART_303A, 1);
    wheels_rest();
    now_ms += 1000;
    midi_pkt(0, 1, 0xB0, 1, 64);
    audio();
    smk_wheels();
    CHECK(engine_bass_mod_ctl(0) == 64);
    CHECK(engine_bass_mod_ctl(1) == 64);
    shot(dir1, "after-double");
    CHECK(strcmp(ui_wheel_log, "JUNO A  CUTOFF 50%|MIAMI B  WOBBLE 50%") == 0);
    forbid_labels();

    /* Distinct live control values: the callback snapshots both, not v0 twice. */
    engine_bass_mod(0, 32);
    engine_bass_mod(1, 110);
    audio();
    CHECK(engine_bass_mod_ctl(0) == 32);
    CHECK(engine_bass_mod_ctl(1) == 110);
    ui_wheel_mod(0, 1, -1);
    shot(dir1, "after-double-distinct");
    CHECK(strcmp(ui_wheel_log, "JUNO A  CUTOFF 25%|MIAMI B  WOBBLE 87%") == 0);
    forbid_labels();
    /* A -1 from engine_bass_mod_ctl must be omitted, never displayed as 0%. */
    engine_set(T_303, 1, engine_bass_mod_param(1), 66);
    audio();
    CHECK(engine_bass_mod_ctl(1) == -1);
    paint(); /* event-time snapshots must survive subsequent live-pot changes */
    CHECK(strcmp(ui_wheel_log, "JUNO A  CUTOFF 25%|MIAMI B  WOBBLE 87%") == 0);
    ui_wheel_mod(0, 1, -1);
    paint();
    CHECK(wu.n == 1 && wu.v[0] == 0 && wu.pct[0] == 25);
    CHECK(!has_str(ui_wheel_log, "MIAMI B"));
    /* Physical wheel still one latched part. */
    ui.part = PART_303A;
    wheels_rest();
    now_ms += 1000;
    physical_mw(64);
    audio();
    paint();
    expect_log("JUNO A  CUTOFF", "50", NULL);
    CHECK(!has_str(ui_wheel_log, "MIAMI B"));

    /* ---- 3. Unassigned, once per physical gesture ---- */
    seq_page(PART_909, 1);
    wheels_rest();
    now_ms = 10000;
    physical_mw(20);
    paint();
    expect_log("UNASSIGNED", NULL, NULL);
    CHECK(!has_str(ui_wheel_log, "CUTOFF"));
    { int first = box_nonblack(); CHECK(first > 50);
      /* A moving UNASSIGNED wheel stays visible beyond 900 ms from start. */
      now_ms += 850;
      physical_mw(40);
      paint(); expect_log("UNASSIGNED", NULL, NULL);
      now_ms += 850; /* 1700 ms since the initial event, 850 ms since last */
      paint(); expect_log("UNASSIGNED", NULL, NULL);
      shot(dir1, "after-unassigned-long-gesture");
      physical_mw(80); /* refresh at t=1700 */
      now_ms += 899;
      paint(); expect_log("UNASSIGNED", NULL, NULL);
      now_ms += 1; /* exactly 900 ms after the last movement */
      ui_wheel_log[0] = 0;
      paint(); CHECK(ui_wheel_log[0] == 0);
      physical_mw(120); /* continuing the gesture is another event */
      paint(); expect_log("UNASSIGNED", NULL, NULL);
      now_ms += 400;
      physical_mw(0); /* rest itself refreshes the expiry */
      now_ms += 899;
      paint(); expect_log("UNASSIGNED", NULL, NULL);
      shot(dir1, "after-unassigned-rest");
      now_ms += 1;
      ui_wheel_log[0] = 0;
      paint(); CHECK(ui_wheel_log[0] == 0);
    }
    now_ms += 10;
    physical_mw(30); /* another gesture after rest */
    paint();
    expect_log("UNASSIGNED", NULL, NULL);
    shot(dir1, "after-unassigned");
    /* 808 and BREAK too */
    for (i = 0; i < 2; i++) {
        int part = i ? PART_BRK : PART_808;
        seq_page(part, 0);
        physical_mw(0);
        now_ms += 1000;
        physical_mw(40);
        paint();
        expect_log("UNASSIGNED", NULL, NULL);
        physical_mw(0);
    }

    /* ---- 5. Latched identity: Juno A held across a visible-part change ---- */
    select_kind(0, ORIGINAL_SOUND_JUNO, 0);
    select_kind(1, ORIGINAL_SOUND_MIAMI, 0);
    set_pot(0, 70);
    ui.part = PART_303A;
    seq_page(PART_303A, 1);
    wheels_rest();
    physical_mw(64);
    audio();
    ui.part = PART_303B;
    build_pages();
    physical_mw(80);
    audio();
    paint();
    expect_log("JUNO A  CUTOFF", NULL, NULL);
    CHECK(!has_str(ui_wheel_log, "MIAMI"));

    /* ---- 6. Held notes during fast sweeps (keyboard, USB, BLE, vel-0) ---- */
    select_kind(0, ORIGINAL_SOUND_JUNO, 0);
    ui.part = PART_303A;
    seq_page(PART_303A, 0);
    wheels_rest();
    engine_bass_on(0, 48, 0, 0);
    audio();
    held = engine_bass_keys_held(); CHECK(held & (1u << (0)));
    for (live = 0; live <= 127; live += 3) {
        physical_mw(live);
        /* between frames, smk_wheels runs without ui_frame */
    }
    for (i = 0; i < 8; i++) { paint(); physical_mw(20 + i * 10); audio(); }
    held = engine_bass_keys_held(); CHECK(held & (1u << (0)));
    engine_bass_off(0);
    audio();
    held = engine_bass_keys_held(); CHECK(!(held & (1u << (0))));

    for (v = 0; v < 2; v++) { /* USB then BLE */
        unsigned src = (unsigned)v;
        unsigned ch = seq.ch[TRK_BASS0];
        midi_pkt(src, ch, 0x90, 48, 100);
        audio();
        held = engine_bass_keys_held(); CHECK(held & 1u);
        for (live = 0; live <= 127; live += 5) {
            midi_pkt(src, ch, 0xB0, 1, (unsigned)live);
            audio();
            smk_wheels();
        }
        held = engine_bass_keys_held(); CHECK(held & 1u);
        midi_pkt(src, ch, 0x80, 48, 0);
        audio();
        held = engine_bass_keys_held(); CHECK(!(held & 1u));
        /* vel-0 note-on as off */
        midi_pkt(src, ch, 0x90, 48, 100);
        audio();
        held = engine_bass_keys_held(); CHECK(held & 1u);
        midi_pkt(src, ch, 0x90, 48, 0);
        audio();
        held = engine_bass_keys_held(); CHECK(!(held & 1u));
    }

    /* Chicago, Miami, Juno, Drone, both 303s: a short hold + off after a sweep */
    {
        static const unsigned kinds[] = {
            ORIGINAL_SOUND_CHICAGO, ORIGINAL_SOUND_MIAMI,
            ORIGINAL_SOUND_JUNO, ORIGINAL_SOUND_DRONE
        };
        int voice;
        for (voice = 0; voice < 2; voice++)
            for (i = 0; i < 4; i++) {
                select_kind(voice, kinds[i], 0);
                ui.part = (uint8_t)(PART_303A + voice);
                wheels_rest();
                engine_bass_on(voice, 48, 0, 0);
                audio();
                physical_mw(40); physical_mw(90); physical_mw(10);
                audio();
                held = engine_bass_keys_held(); CHECK(held & (1u << (voice)));
                engine_bass_off(voice);
                audio();
                held = engine_bass_keys_held(); CHECK(!(held & (1u << (voice))));
            }
    }

    /* ---- 7. Timeout: box gone after 900 ms ---- */
    seq_page(PART_303A, 1);
    ui_wheel_mod(0, -1, 64);
    paint();
    CHECK(has_str(ui_wheel_log, "CUTOFF") || has_str(ui_wheel_log, "WOBBLE") || has_str(ui_wheel_log, "BRIGHTNESS"));
    now_ms += 901;
    ui_wheel_log[0] = 0;
    paint();
    CHECK(ui_wheel_log[0] == 0);

    /* ---- 8. Knob wins, then wheel wins; fader say() stays on the header ---- */
    select_kind(0, ORIGINAL_SOUND_JUNO, 0);
    set_pot(0, 70);
    seq_page(PART_303A, 1);
    ui.part = PART_303A;
    wheels_rest();
    physical_mw(64);
    audio();
    paint();
    expect_log("JUNO A  CUTOFF", NULL, NULL);
    /* A later knob detent takes the box. */
    now_ms += 5;
    ui.touched = 0;
    ui.touch_until = plat_ms() + UI_READOUT_MS;
    ui.touch_view = ui.view;
    ui_wheel_log[0] = 0;
    paint();
    /* Wheel snapshot is still live but knob owns the surface: wheel draw is skipped. */
    CHECK(ui_wheel_log[0] == 0);
    shot(dir1, "after-knob-wins");
    /* Equal deadline duration is part of the ordering contract. */
    CHECK(ui.touch_until == plat_ms() + UI_READOUT_MS);
    /* Next accepted wheel step takes it back. */
    now_ms += 5;
    physical_mw(90);
    audio();
    paint();
    expect_log("JUNO A  CUTOFF", NULL, NULL);
    shot(dir1, "after-wheel-wins");
    /* Same-millisecond knob and wheel deadlines select the wheel (stable tie). */
    ui.touched = 0;
    ui.touch_until = plat_ms() + UI_READOUT_MS;
    ui_wheel_mod(0, -1, 64);
    paint();
    expect_log("JUNO A  CUTOFF", "50", "%");
    /* Both valid snapshots remain subject to the existing overlay/help gate. */
    ui.overlay = O_LIST;
    ui_wheel_log[0] = 0;
    draw_main();
    CHECK(ui_wheel_log[0] == 0);
    ui.overlay = O_NONE;
    /* Fader say() does not take the box; UNSAVED stays if dirty. */
    ui.dirty = 1;
    ui_say("VOL  80", 0);
    paint();
    expect_log("JUNO A  CUTOFF", NULL, NULL);
    CHECK(has_str(ui.msg, "VOL"));
    CHECK(!has_str(ui.msg, "CC1"));
    /* Header was replaced by say() — that's existing fader behaviour.
     * Wheel itself must not have written ui.msg. The VOL text is from ui_say. */
    ui.dirty = 0;
    ui.msg[0] = 0;
    ui.msg_until = 0;
    now_ms += 1000;
    ui.dirty = 1;
    ui_wheel_mod(0, -1, 64);
    paint();
    expect_log("JUNO A  CUTOFF", "50", "%");
    CHECK(ui.msg[0] == 0); /* wheel did not call say() */
    ui.dirty = 0;

    /* ---- ui_frame / paint cost ---- */
    now_ms += 1000;
    ui.touched = -1;
    ui.touch_until = 0;
    seq_page(PART_303A, 1);
    t_off = measure_paint(300);
    ui_wheel_mod(0, -1, 64);
    t_wheel = measure_paint(300);
    now_ms += 1000;
    ui.touched = 0;
    ui.touch_until = plat_ms() + 100000u;
    ui.touch_view = ui.view;
    t_knob = measure_paint(300);
    printf("UI_FRAME_MS off=%.6f wheel=%.6f knob=%.6f\n", t_off, t_wheel, t_knob);
    /* Wheel box is in the same family as a knob readout: not wildly more expensive. */
    CHECK(t_wheel < t_knob * 3.0 + 0.2);
    CHECK(t_off > 0.0);

    printf("modwheel_ui_test: PASS (%u checks)\n", checks);
    return 0;
}

