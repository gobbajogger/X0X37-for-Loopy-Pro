/* SPDX-License-Identifier: GPL-3.0-only */
/* Real project/UI/engine path with simulated flash. An adopted project's first
 * pattern must recall its sound even when the previous project cached the same
 * pattern and preset byte. This is independent of the lifecycle supplier repro. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/original_sound.h"

static unsigned checks, cases;
static sound_t base;
static const int recalled[NBASS] = {5, 8};
static const char *phase = "boot";
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "FAIL %s line %d: %s\n", phase, __LINE__, #x); exit(1); } } while (0)

static void block(void)
{
    int32_t out[512];
    engine_render(out, 256);
}
static void settle(void) { for (int i = 0; i < 12; ++i) block(); }
static void wait_applied(void)
{
    for (unsigned i = 0; i < 4096; ++i) {
        int pending = 0;
        for (int b = 0; b < NBASS; ++b) pending |= engine_bass_preset_pending(b);
        if (!pending) return;
        block();
    }
    CHECK(0);
}
static void choose(int preset)
{
    for (int b = 0; b < NBASS; ++b)
        CHECK(engine_bass_select_preset(b, ORIGINAL_SOUND_CHICAGO, preset, &proj.sound) == 0);
    wait_applied();
    settle();
    for (int b = 0; b < NBASS; ++b) CHECK(engine_bass_preset_index(b) == preset);
}
static void set_recall(void)
{
    for (int b = 0; b < NBASS; ++b) {
        seq.ppat[TRK_BASS0 + b] = 0;
        seq.pcue[TRK_BASS0 + b] = 0xFF;
        proj.pat[0].rsv[b] = (uint8_t)bass_pattern_preset_encode(ORIGINAL_SOUND_CHICAGO, recalled[b] + 1);
    }
}
static void check_recall_once(void)
{
    uint32_t serial = engine_bass_choice_serial();
    pattern_sound_tick();
    if (engine_bass_choice_serial() != serial + NBASS)
        fprintf(stderr, "%s request serial: before %u, after %u, expected +%u\n", phase,
            (unsigned)serial, (unsigned)engine_bass_choice_serial(), (unsigned)NBASS);
    CHECK(engine_bass_choice_serial() == serial + NBASS);
    for (int b = 0; b < NBASS; ++b) {
        CHECK(engine_bass_requested_bank(b) == ORIGINAL_SOUND_CHICAGO);
        CHECK(engine_bass_requested_preset(b) == recalled[b]);
        CHECK(engine_bass_preset_gate(b) == 0);
        CHECK(engine_bass_preset_pending(b) != 0);
    }
    serial = engine_bass_choice_serial();
    pattern_sound_tick();
    CHECK(engine_bass_choice_serial() == serial); /* no duplicate while pending */
    wait_applied();
    for (int b = 0; b < NBASS; ++b) CHECK(engine_bass_preset_index(b) == recalled[b]);
    pattern_sound_tick();
    CHECK(engine_bass_choice_serial() == serial); /* no duplicate after apply */
    settle();
}
static void loaded_base(void)
{
    for (int b = 0; b < NBASS; ++b) {
        CHECK(seq.ppat[TRK_BASS0 + b] == 0);
        CHECK(engine_bass_preset_index(b) == 0);
        CHECK(proj.pat[0].rsv[b] == bass_pattern_preset_encode(ORIGINAL_SOUND_CHICAGO, recalled[b] + 1));
    }
}

static void saved_project_load(void)
{
    ++cases;
    choose(0);
    base = proj.sound;
    set_recall();
    CHECK(project_save() == 0); /* saved base 0 differs from the pattern's sound */
    phase = "prime cache before load";
    check_recall_once();
    uint32_t writes = store_writes;
    CHECK(ui_project_load() == 0);
    CHECK(store_writes == writes && !ui.dirty && !ui_project_busy());
    loaded_base();
    phase = "same-key saved project load";
    check_recall_once(); /* fails on 94197fc8: both cache keys still match */
    CHECK(store_writes == writes);
    puts("saved load: same pattern/preset recalls for both bass parts");
}

static void successful_adoption(unsigned count)
{
    ++cases;
    phase = count == 1 ? "single staged adoption" : "256 staged adoptions";
    uint32_t writes = store_writes;
    for (unsigned i = 0; i < count; ++i) {
        CHECK(ui_project_begin() == 0);
        project_capture_stage();
        project_stage.sound = base;
        CHECK(project_validate_stage() == 0);
        ui_project_end(1);
        CHECK(!ui_project_busy());
    }
    loaded_base();
    check_recall_once();
    CHECK(store_writes == writes);
    printf("staged apply: %u successful replacements, no intervening tick\n", count);
}

static void failed_load_and_abort(void)
{
    ++cases;
    phase = "failed load and aborted adoption";
    choose(2); /* a musician's manual sound override stays until pattern changes */
    uint32_t serial = engine_bass_choice_serial(), writes = store_writes;
    uint8_t saved = store[OBJ_PAT0][16];
    store[OBJ_PAT0][16] ^= 0x55;
    CHECK(ui_project_load() != 0);
    store[OBJ_PAT0][16] = saved;
    pattern_sound_tick();
    CHECK(engine_bass_choice_serial() == serial);
    for (int b = 0; b < NBASS; ++b) CHECK(engine_bass_preset_index(b) == 2);
    CHECK(ui_project_begin() == 0);
    project_capture_stage();
    project_stage.sound = base;
    ui_project_end(0);
    pattern_sound_tick();
    CHECK(engine_bass_choice_serial() == serial && store_writes == writes);
    for (int b = 0; b < NBASS; ++b) CHECK(engine_bass_preset_index(b) == 2);
    puts("failed load and cancelled adoption: manual override/cache preserved");
}

static void cue_and_reset(void)
{
    ++cases;
    phase = "cue and reset";
    /* An applied project still uses the existing cue gate and cancellation. */
    CHECK(ui_project_load() == 0);
    loaded_base();
    check_recall_once();
    proj.pat[1].rsv[0] = (uint8_t)bass_pattern_preset_encode(ORIGINAL_SOUND_CHICAGO, 3);
    seq.pcue[TRK_BASS0] = 1;
    pattern_sound_tick();
    CHECK(engine_bass_preset_gate(0) == 1 && engine_bass_preset_pending(0));
    CHECK(engine_bass_preset_index(0) == recalled[0]);
    seq.pcue[TRK_BASS0] = 0xFF;
    pattern_sound_tick();
    CHECK(!engine_bass_preset_pending(0));
    settle();
    CHECK(engine_bass_preset_index(0) == recalled[0]);
    uint32_t writes = store_writes;
    CHECK(ui_project_reset() == 0);
    uint32_t serial = engine_bass_choice_serial();
    for (int b = 0; b < NBASS; ++b) CHECK(proj.pat[0].rsv[b] == 0);
    pattern_sound_tick();
    CHECK(engine_bass_choice_serial() == serial && store_writes == writes);
    puts("cue gate/cancellation retained; reset unset patterns request nothing");
}

int main(void)
{
    load_force = 0;
    boot();
    settle();
    saved_project_load();
    successful_adoption(1);
    successful_adoption(256); /* an 8-bit revision-only cache key would alias */
    failed_load_and_abort();
    cue_and_reset();
    printf("pattern-sound-project-adoption: %u checks across %u cases passed\n", checks, cases);
    return 0;
}
