/* SPDX-License-Identifier: GPL-3.0-only */
/* Production queues, parameter store, engine/motion and framebuffer. The write
 * recorder forwards every call to the real engine; no pickup/target substitutes. */
#define SMC_UI_SEND_IMPLEMENTED 1
#define engine_set pickup_engine_set
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
#undef engine_set
extern void engine_set(int t, int v, int i, int val);
static unsigned writes, feedback_count;
static int written_t, written_voice, written_param, written_value;
static uint8_t feedback[8];
int plat_ble_smc_send(const uint8_t *m, uint8_t n)
{
    if (n == 3 && m[0] >= 0xE0 && m[0] < 0xE8) {
        CHECK(m[1] == m[2]);
        feedback[m[0] - 0xE0] = m[2]; feedback_count++;
    }
    return 1;
}
void pickup_engine_set(int t, int v, int i, int val)
{
    writes++;
    written_t = t; written_voice = v; written_param = i; written_value = val;
    engine_set(t, v, i, val);
}
static unsigned peek_pickup_hints;
static void status_is(const char *status)
{
    CHECK(smc_ui_active());
    ui_frame();
    memcpy(active_fb, fb, sizeof fb);
    if (SMC_PEEK_ON) {
        /* Approved #737 peek: the status is the second header line, drawn
         * over the real page. Compare that line only. */
        cv_begin(240, HDR_H, C_BLACK);
        text_c(120, vc(&FONT_T, 10, 10), &FONT_T, status,
               status[0] == 'P' ? C_HI : C_GRAY);
        cv_commit(0, 0, 0);
        CHECK(!memcmp(active_fb + 10 * 240, fb + 10 * 240,
                      240 * (HDR_H - 10) * sizeof *fb));
        if (status[0] == 'P') peek_pickup_hints++;
        return;
    }
    cv_begin(240, KNOB_H, C_BLACK);
    text_c(120, 49, &FONT_XS, status, C_HI);
    cv_commit(3, 0, KNOB_Y);
    CHECK(!memcmp(active_fb + (KNOB_Y + 49) * 240,
                  fb + (KNOB_Y + 49) * 240, 240 * 10 * sizeof *fb));
}
static void prepare(int view, int part, int strip, int target)
{
    setup(view, part);
    pref_set(smc_target(strip), target); smc_frame(); writes = 0;
}
static void sample(int strip, int value, int user)
{
    if (user) message(0xB0, 40 + strip, value);
    else fader(strip, value);
}
static void crossing_matrix(void)
{
    for (int user = 0; user < 2; user++)
    for (int drum = 0; drum < 3; drum++)
    for (int strip = 0; strip < 8; strip++)
    for (int down = 0; down < 2; down++)
    for (int coarse = 0; coarse < 2; coarse++) {
        int start = down ? 127 : 0, target = down ? 47 : 80;
        int end = coarse ? (down ? 0 : 127) : target;
        prepare(drum ? V_SEQ : V_HOME, drum ? drum - 1 : PART_303A, strip, 64);
        if (user) proj.set.bt |= PROJ_BT_SMC_USER;
        pref_t r = smc_target(strip);
        CHECK(pref_desc(r)->max == 127);
        sample(strip, start, user);
        CHECK(!writes && !(smc.picked & (1u << strip)));
        /* Movement UI is independent of target acquisition. */
        status_is(down ? "PICKUP: MOVE DOWN TO TARGET" : "PICKUP: MOVE UP TO TARGET");
        expire(); sample(strip, start, user); CHECK(!smc_ui_active()); CHECK(!writes);
        pref_set(r, target); smc_frame(); writes = 0;
        sample(strip, start, user); CHECK(!writes && !(smc.picked & (1u << strip)));
        CHECK(!smc_ui_active());
        sample(strip, end, user);
        CHECK((smc.picked & (1u << strip)) && pref_get(r) == end && writes == 1);
        status_is("ACTIVE");
        writes = 0; sample(strip, end, user); CHECK(!writes);
        int next = down ? 1 : 126;
        sample(strip, next, user); CHECK(writes == 1 && pref_get(r) == next);
    }
    puts("PASS 192 crossings: DAW/User, main/both drum kits, eight strips, both directions, fine/coarse extrema; movement UI and writes checked separately");
}
static void routing_epochs(void)
{
    for (int down = 0; down < 2; down++) for (int epoch = 0; epoch < 4; epoch++) {
        int start = down ? 127 : 0, end = down ? 0 : 127;
        prepare(V_HOME, PART_303A, 0, 64);
        fader(0, start); expire();
        if (epoch == 0) CHECK(smc_lock_toggle());
        if (epoch == 1) { CHECK(smc_lock_toggle()); CHECK(smc_lock_toggle()); }
        if (epoch == 2) {
            ui.part = PART_808; set_view(V_SEQ); ui.sel[1] = 8; build_pages();
            pref_set(smc_target(0), 80); smc_frame();
        }
        if (epoch == 3) {
            ui.part = PART_909; set_view(V_SEQ); ui.sel[0] = 8; build_pages();
            pref_set(smc_target(0), 80); smc_frame();
        }
        writes = 0; ui_frame(); CHECK(!smc_ui_active());
        fader(0, start); CHECK(!writes && !smc_ui_active());
        fader(0, end); CHECK(writes == 1 && (smc.picked & 1)); status_is("ACTIVE");
    }
    /* No pre-session physical crossing may acquire after reconnect. */
    prepare(V_HOME, PART_303A, 0, 64); fader(0, 0);
    smc_ble_link(0); smc_poll(); smc_ble_link(1); smc_poll();
    writes = 0; fader(0, 127); CHECK(!writes && !(smc.picked & 1));
    status_is("PICKUP: MOVE DOWN TO TARGET");
    fader(0, 0); CHECK(writes == 1 && (smc.picked & 1));
    /* A parked packet at a newly equal target is not a new acceptance. */
    prepare(V_HOME, PART_303A, 0, 64); fader(0, 20); expire();
    pref_set(smc_target(0), 20); smc_frame(); writes = 0;
    fader(0, 20); CHECK(!writes && !(smc.picked & 1) && !smc_ui_active());
    fader(0, 21); CHECK(writes == 1 && (smc.picked & 1));
    puts("PASS routing epochs: lock/unlock, page/part/bank target change, parked equal target, reconnect and first-entry safety");
}
static void render_blocks(int n)
{
    int32_t block[512]; while (n--) engine_render(block, 256);
}
static void motion_acquisition(void)
{
    for (int down = 0; down < 2; down++) {
        prepare(V_HOME, PART_303A, 0, 64);
        pref_t r = smc_target(0);
        int part = engine_motion_part(r.a, r.b);
        int k = motion_alloc(proj.arr.lane, seq.ppat[part], part, r.a, r.b, r.c);
        CHECK(k >= 0);
        for (int i = 0; i < NSTEPS; i++) proj.arr.lane[k].val[i] = 20;
        engine_motion_enable(1); seq_start(&seq); render_blocks(100);
        CHECK(engine_motion_value(k) == 20);
        fader(0, down ? 127 : 0); CHECK(!writes);
        fader(0, 64);
        CHECK(writes == 1 && pref_get(r) == 64 && (smc.picked & 1));
        render_blocks(100); CHECK(engine_motion_value(k) == -1);
        status_is("ACTIVE");
        writes = 0; fader(0, 64); CHECK(!writes);
        fader(0, down ? 63 : 65); CHECK(writes == 1);
    }
    puts("PASS exact-equal acquisition emits one real engine write and holds real motion lane immediately; no second movement required");
}
/* The production decoder intentionally takes pitch-bend MSB into a 7-bit
 * parameter. Sweep every fine raw code, including 127->128 byte rollovers.
 * No smoothing, reordering or synthetic jitter removal is applied. */
static void monotonic_ramps(void)
{
    unsigned raw_packets = 0, accepted_writes = 0;
    const char *dir = getenv("SMC_UI_SHOTS"); char path[1024];
    snprintf(path, sizeof path, "%s/slow-ramp.csv", dir ? dir : ".");
    FILE *trace = fopen(path, "w"); CHECK(trace);
    fprintf(trace, "strip,direction,raw14,lsb,msb,normalized,write_count,write_value,saved,feedback\n");
    for (int strip = 0; strip < 8; strip++) for (int down = 0; down < 2; down++) {
        prepare(V_HOME, PART_303A, strip, down ? 127 : 0);
        pref_t r = smc_target(strip);
        int prev = -1;
        for (int step = 0; step < 16384; step++) {
            int raw = down ? 16383 - step : step, normalized = raw >> 7;
            writes = 0; feedback_count = 0; now_ms += 16;
            message(0xE0 + strip, raw & 127, raw >> 7);
            raw_packets++;
            CHECK(smc.last[strip] == normalized && pref_get(r) == normalized);
            CHECK(smc.picked & (1u << strip));
            CHECK(writes == (unsigned)(prev != normalized));
            if (writes) {
                accepted_writes++;
                CHECK(written_t == r.a && written_voice == r.b && written_param == r.c && written_value == normalized);
                smc_frame(); CHECK(feedback[strip] == normalized && feedback_count == 1);
                /* Rendered value uses this exact saved source, not a feedback cache. */
                smc_ui_begin(strip, 0); status_is("ACTIVE");
            }
            fprintf(trace, "%d,%s,%d,%d,%d,%d,%u,%d,%d,%u\n", strip,
                    down ? "down" : "up", raw, raw & 127, raw >> 7, normalized,
                    writes, writes ? written_value : -1, pref_get(r), feedback[strip]);
            prev = normalized;
        }
    }
    fclose(trace);
    printf("PASS slow ramps: %u raw pitch-bend packets, %u real writes; full14-bit increasing/decreasing all8 main faders, every byte boundary, feedback and framebuffer status\n", raw_packets, accepted_writes);
}
static void stateful_sequences(void)
{
    unsigned packets = 0;
    uint32_t rng = 0x5837e75u;
    for (int run = 0; run < 16; run++) {
        prepare(V_HOME, PART_303A, 0, 64);
        int history = 0, prior = 0, accepted = 0, have_target = 0, cached = 0;
        pref_t target = NONE;
        for (int step = 0; step < 256; step++) {
            rng = rng * 1664525u + 1013904223u;
            int op = (int)(rng >> 27) % 7;
            if (op == 0) {
                pref_set(smc_target(0), (rng >> 8) & 127);
            } else if (op == 1) {
                CHECK(smc_lock_toggle()); accepted = 0;
            } else if (op == 2) {
                /* Parked switch away/back; edit the destination while away. */
                uint8_t view = ui.view, part = ui.part;
                ui.part = PART_808; set_view(V_SEQ); ui.sel[1] = 8; build_pages();
                pref_t away = smc_target(0);
                if (!have_target || !smc_same(away, target) || pref_get(away) != cached) accepted = 0;
                target = away; cached = pref_get(away); have_target = 1;
                smc_frame();
                pref_set(smc_home_target(0), (rng >> 8) & 127);
                ui.part = part; set_view(view); build_pages(); smc_ui_cancel();
            } else if (op == 3) {
                ui.part = (rng >> 9) & 1; set_view(V_SEQ);
                ui.sel[ui.part] = (rng & 1) ? 8 : 0; build_pages(); smc_ui_cancel();
            } else if (op == 4) {
                smc_ble_link(0); smc_poll(); smc_ble_link(1); smc_poll();
                accepted = history = 0;
            }
            /* Vary feedback timing, including edits noticed only on input. */
            if (rng & 4) smc_frame();
            pref_t current = smc_target(0); int value = pref_get(current);
            if (!have_target || !smc_same(current, target) || value != cached) accepted = 0;
            target = current; have_target = 1;
            int input = op == 5 && history ? prior : (rng >> 16) & 127;
            int moved = !history || input != prior;
            int near = input + 2 >= value && input <= value + 2;
            int crossing = history && ((prior < value) != (input < value));
            int acquired = !accepted && moved && (near || crossing);
            int expected = (accepted || acquired) && (acquired || input != value);
            if (acquired) accepted = 1;
            writes = 0;
            if (!moved) { smc_ui_cancel(); CHECK(!smc_ui_active()); }
            fader(0, input); packets++;
            if (writes != (unsigned)expected) fprintf(stderr, "stateful run%d step%d op%d prior%d input%d target%d expected%d writes%u history%d picked%d\n", run, step, op, prior, input, value, expected, writes, history, !!(smc.picked & 1));
            CHECK(writes == (unsigned)expected);
            CHECK(!!(smc.picked & 1) == accepted);
            CHECK(pref_get(target) == (expected ? input : value));
            if (moved) CHECK(smc_ui_active());
            else CHECK(!smc_ui_active());
            if (expected) CHECK(written_t == target.a && written_voice == target.b && written_param == target.c);
            if (moved && step % 16 == 0)
                status_is(accepted ? "ACTIVE" : input < value ?
                          "PICKUP: MOVE UP TO TARGET" : "PICKUP: MOVE DOWN TO TARGET");
            history = 1; prior = input; cached = pref_get(target);
            if (!(rng & 4)) smc_frame();
        }
    }
    printf("PASS %u stateful input events: target edits, parked away/back plus edits while away, kit/bank/part, lock/unlock, reconnect, duplicates and feedback timing\n", packets);
}
static void mute_takeover(void)
{
    /* A muted level takes over on the first movement, like the keyboard. */
    for (int drum = 0; drum < 2; drum++) {
        prepare(drum ? V_SEQ : V_HOME, drum ? PART_909 : PART_303A, 0, 64);
        pref_t r = smc_target(0);
        mute_part(drum ? PART_909 : r.b); smc_frame(); CHECK(volume_muted(r));
        writes = 0; fader(0, 0); CHECK(!writes || pref_get(r) == 0);
        fader(0, 10); CHECK((smc.picked & 1) && pref_get(r) == 10 && writes);
    }
    /* Unmuted control still waits for pickup. */
    prepare(V_HOME, PART_303A, 0, 64); writes = 0;
    fader(0, 0); fader(0, 10); CHECK(!writes && !(smc.picked & 1));
    puts("PASS muted levels take over at once; unmuted levels still wait for pickup");
}
int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "stateful")) stateful_sequences();
    else if (argc > 1 && !strcmp(argv[1], "ramps")) monotonic_ramps();
    else if (argc > 1 && !strcmp(argv[1], "motion")) motion_acquisition();
    else if (argc > 1 && !strcmp(argv[1], "epochs")) routing_epochs();
    else if (argc > 1 && !strcmp(argv[1], "mute")) mute_takeover();
    else { crossing_matrix(); mute_takeover(); routing_epochs(); motion_acquisition(); monotonic_ramps(); stateful_sequences(); }
    printf("smc_pickup_acceptance: PASS (%u assertions)\n", checks);
    CHECK(peek_pickup_hints > 0); /* hint drawn on the #737 peek page */
    printf("PASS pickup hint on peek page (%u checks)\n", peek_pickup_hints);
    return 0;
}
