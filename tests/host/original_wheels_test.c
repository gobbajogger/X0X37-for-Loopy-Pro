/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the extracted production scanner. Engine calls are recording stubs:
 * this checks dispatch/ownership, not ADC hardware or rendered audio. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/engine.h"
#include "app/original_sound.h"

static struct { uint32_t frames; int32_t wheel[2]; } fm1_in;
static struct { sound_t sound; } proj;
static sound_t saved_sound;
static struct { int part; } ui;
static int busy, kind[2];
static unsigned hold_depth, hold_enter, hold_leave, owner_updates;
static unsigned transaction_owner_updates, scan_should_publish;
static uint8_t owners;
static float base[2][22], live[2][22];
typedef struct { int restore, target, voice, parameter, value; } command_t;
static command_t commands[64];
static unsigned command_count, scans;

static int ui_project_busy(void) { return busy; }
static int is_303(void) { return ui.part == PART_303A || ui.part == PART_303B; }
static int bidx(void) { assert(is_303()); return ui.part - PART_303A; }
static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

void engine_bass_edit_hold(int hold)
{
    if (hold) {
        assert(hold_depth == 0);
        hold_depth = 1;
        hold_enter++;
        transaction_owner_updates = owner_updates;
    } else {
        assert(hold_depth == 1);
        assert(owner_updates == transaction_owner_updates + scan_should_publish);
        hold_depth = 0;
        hold_leave++;
    }
}

int engine_bass_is_wobble(int voice)
{
    assert(hold_depth == 1 && voice >= 0 && voice < 2);
    return kind[voice] == ORIGINAL_SOUND_MIAMI;
}

void engine_bass_wheel_owners(uint8_t mask)
{
    assert(hold_depth == 1 && (mask & ~3u) == 0);
    owners = mask;
    owner_updates++;
}

static void record_command(int restore, int target, int voice, int parameter, int value)
{
    assert(hold_depth == 1);
    assert(target == T_303 && voice >= 0 && voice < 2);
    assert(parameter >= 0 && parameter < 22);
    assert(command_count < sizeof(commands) / sizeof(commands[0]));
    commands[command_count++] = (command_t){restore, target, voice, parameter, value};
}

void engine_set(int target, int voice, int parameter, int value)
{
    assert(value >= 0 && value <= 127);
    record_command(0, target, voice, parameter, value);
    live[voice][parameter] = (float)value / 127.0f;
}

void engine_set_base(int target, int voice, int parameter)
{
    record_command(1, target, voice, parameter, -1);
    live[voice][parameter] = base[voice][parameter];
}

#include "smk_wheels.inc"

enum { SCAN_NORMAL, SCAN_EARLY };
static void scan(int pitch, int mod, uint8_t expected_owners, int mode)
{
    unsigned enters = hold_enter, leaves = hold_leave, updates = owner_updates;
    fm1_in.wheel[0] = pitch;
    fm1_in.wheel[1] = mod;
    scan_should_publish = mode == SCAN_NORMAL;
    smk_wheels();
    assert(hold_depth == 0 && hold_enter == hold_leave);
    assert(hold_enter == enters + (mode != SCAN_EARLY));
    assert(hold_leave == leaves + (mode != SCAN_EARLY));
    assert(owner_updates == updates + scan_should_publish);
    assert(owners == expected_owners);
    assert(memcmp(&proj.sound, &saved_sound, sizeof(saved_sound)) == 0);
    scans++;
}

static void expect_command(unsigned at, int restore, int voice, int parameter, int value)
{
    const command_t *c;
    assert(at < command_count);
    c = &commands[at];
    assert(c->restore == restore && c->target == T_303 && c->voice == voice);
    assert(c->parameter == parameter && c->value == value);
}

static void expect_one(unsigned before, int restore, int voice, int parameter, int value)
{
    assert(command_count == before + 1);
    expect_command(before, restore, voice, parameter, value);
}

int main(int argc, char **argv)
{
    unsigned before, i;
    int first, other, pitch_param, mod_param, other_mod_param;
    uint8_t first_mask, other_mask;
    assert(argc == 3);
    first = atoi(argv[1]);
    assert(first >= 0 && first < 2);
    kind[first] = atoi(argv[2]);
    assert(kind[first] == ORIGINAL_SOUND_CHICAGO || kind[first] == ORIGINAL_SOUND_MIAMI);
    other = 1 - first;
    kind[other] = 1 - kind[first];
    /* Independent expected public positions catch accidental legacy enums. */
    assert(ORIGINAL_CHICAGO_CUTOFF == 2 && ORIGINAL_CHICAGO_PITCH_BEND == 20);
    assert(ORIGINAL_MIAMI_WOBBLE_DEPTH == 12 && ORIGINAL_MIAMI_PITCH_BEND == 16);
    pitch_param = kind[first] ? 16 : 20;
    mod_param = kind[first] ? 12 : 2;
    other_mod_param = kind[other] ? 12 : 2;
    first_mask = (uint8_t)(1u << first);
    other_mask = (uint8_t)(1u << other);
    for (i = 0; i < sizeof(proj.sound); i++)
        ((uint8_t *)&proj.sound)[i] = (uint8_t)(i * 37u + 11u);
    memcpy(&saved_sound, &proj.sound, sizeof(saved_sound));
    for (i = 0; i < 22; i++) {
        base[0][i] = live[0][i] = 0.5f;
        base[1][i] = live[1][i] = 0.375f;
    }
    base[first][pitch_param] = live[first][pitch_param] = 0.5f;
    ui.part = PART_303A + first;

    /* The current scanner's readiness exits do not enter the hold. */
    fm1_in.frames = 299;
    scan(550, 615, 0, SCAN_EARLY);
    fm1_in.frames = 400;
    busy = 1;
    scan(550, 615, 0, SCAN_EARLY);
    busy = 0;
    scan(550, 615, 0, SCAN_NORMAL);
    assert(command_count == 0);

    /* Unowned gestures on a drum part must not send synth commands. */
    ui.part = PART_909;
    scan(630, 485, 0, SCAN_NORMAL);
    scan(550, 615, 0, SCAN_NORMAL);
    assert(command_count == 0);

    ui.part = PART_303A + first;
    before = command_count;
    scan(630, 615, first_mask, SCAN_NORMAL);
    expect_one(before, 0, first, pitch_param, 127);
    /* A stationary wheel must preserve a later knob/lock write, across parts. */
    live[first][pitch_param] = 0.125f;
    before = command_count;
    for (i = 0; i < 8; i++) {
        ui.part = (i & 1u) ? PART_303A + other : PART_BRK;
        scan(630, 615, first_mask, SCAN_NORMAL);
    }
    assert(command_count == before && live[first][pitch_param] == 0.125f);
    scan(470, 615, first_mask, SCAN_NORMAL);
    expect_one(before, 0, first, pitch_param, 0);
    before = command_count;
    scan(550, 615, 0, SCAN_NORMAL);
    expect_one(before, 1, first, pitch_param, -1);
    assert(live[first][pitch_param] == 0.5f); /* exact-base command, not byte 64 */
    before = command_count;
    scan(550, 615, 0, SCAN_NORMAL);
    assert(command_count == before);

    ui.part = PART_303A + first;
    scan(550, 485, first_mask, SCAN_NORMAL);
    expect_one(before, 0, first, mod_param, 127);
    live[first][mod_param] = 0.25f;
    before = command_count;
    ui.part = PART_303A + other;
    scan(550, 485, first_mask, SCAN_NORMAL);
    ui.part = PART_909;
    scan(550, 485, first_mask, SCAN_NORMAL);
    assert(command_count == before && live[first][mod_param] == 0.25f);
    scan(550, 550, first_mask, SCAN_NORMAL);
    expect_one(before, 0, first, mod_param, 57);
    before = command_count;
    scan(550, 615, 0, SCAN_NORMAL);
    expect_one(before, 0, first, mod_param, 0);

    /* Simultaneous wheels keep independent owners and release independently. */
    ui.part = PART_303A + first;
    before = command_count;
    scan(630, 615, first_mask, SCAN_NORMAL);
    expect_one(before, 0, first, pitch_param, 127);
    ui.part = PART_303A + other;
    before = command_count;
    scan(630, 485, 3, SCAN_NORMAL);
    expect_one(before, 0, other, other_mod_param, 127);
    ui.part = PART_BRK;
    before = command_count;
    scan(470, 550, 3, SCAN_NORMAL);
    assert(command_count == before + 2);
    expect_command(before, 0, first, pitch_param, 0);
    expect_command(before + 1, 0, other, other_mod_param, 57);
    before = command_count;
    scan(550, 550, other_mask, SCAN_NORMAL);
    expect_one(before, 1, first, pitch_param, -1);
    before = command_count;
    scan(550, 615, 0, SCAN_NORMAL);
    expect_one(before, 0, other, other_mod_param, 0);
    before = command_count;
    scan(550, 615, 0, SCAN_NORMAL);
    assert(command_count == before && owners == 0);
    printf("PASS part=%d kind=%s scans=%u commands=%u holds=%u/%u patch-bytes=unchanged\n",
           first, kind[first] ? "Miami" : "Chicago", scans, command_count, hold_enter, hold_leave);
    return 0;
}
