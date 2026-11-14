/* SPDX-License-Identifier: GPL-3.0-only */
/* Profile-specific runtime and archival contracts; no engine/UI/device fixture. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/src/app/original_runtime.h"
#include "../../firmware/src/app/bass_identity.h"
#include "../../firmware/src/app/bass_pattern_preset.h"
#include "../../firmware/src/app/preset_record.h"
#include "../../experiments/original_synth/chicago_fx2_compact.h"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static const bass_identity_entry_t registry[] = {
    {ORIGINAL_SOUND_CHICAGO_ID, ORIGINAL_SOUND_SCHEMA, 22, 1},
    {ORIGINAL_SOUND_MIAMI_ID, ORIGINAL_SOUND_SCHEMA, 18, 1},
    {ORIGINAL_SOUND_DRONE_ID, ORIGINAL_SOUND_SCHEMA, 8, X0X_DRONE_RUNTIME_ENABLED},
    {ORIGINAL_SOUND_JUNO_ID, ORIGINAL_SOUND_SCHEMA, 18, 1}
};
static const uint8_t factories[4][6] = {
    {57,66,51,23,83,95}, {35,39,38,58,108,105},
    {94,97,70,17,72,83}, {73,75,12,5,55,94}
};
static int identity_status(void)
{ return X0X_DRONE_RUNTIME_ENABLED ? BASS_IDENTITY_READY : BASS_IDENTITY_UNAVAILABLE; }
static void make_tag(bass_sound_tag_t *tag, const uint8_t *cell)
{
    bass_sound_info_t info = {{ORIGINAL_SOUND_DRONE_ID, ORIGINAL_SOUND_SCHEMA,
        ORIGINAL_DRONE_NPARAMS, BASS_IDENTITY_KNOWN}, cell[30],
        original_sound_edit_mask(cell) ? BASS_SOUND_EDITED : 0};
    CHECK(bass_sound_tag_encode(tag, &info, registry, 4) == identity_status());
}
static void rejected(const uint8_t *cell, const bass_sound_tag_t *tag)
{
    int bank = 73;
    CHECK(bass_sound_cell_validate(cell, tag, registry, 4, &bank) == -9);
    CHECK(bank == 73);
}
static void archive_case(unsigned factory, int edited)
{
    uint8_t cell[32], before[32], tag_before[12];
    bass_sound_tag_t tag, encoded;
    bass_sound_info_t info;
    uint8_t canonical[32] = {0};
    memcpy(canonical, factories[factory], 6);
    canonical[26] = 'O'; canonical[27] = 'S'; canonical[28] = 2;
    canonical[29] = 2; canonical[30] = (uint8_t)factory;
    CHECK(original_sound_init(cell, ORIGINAL_SOUND_DRONE, factory));
    CHECK(!memcmp(cell, canonical, 32));
    if (edited) {
        CHECK(original_sound_set(cell, 0, 127));
        CHECK(original_sound_set(cell, 4, 0));
        /* Historical performance values/bits remain bytes, never live inputs. */
        cell[6] = 113; cell[7] = 91; cell[22] |= 0xC0u;
        cell[2] = 126; /* Valid archived display byte with its edit bit unset. */
    }
    CHECK(original_sound_valid(cell));
    memcpy(before, cell, sizeof cell);
    make_tag(&tag, cell); memcpy(tag_before, tag.bytes, sizeof tag_before);
    for (unsigned placement = 0; placement < 2; ++placement) {
        uint8_t cells[2][32]; bass_sound_tag_t tags[2];
        CHECK(original_sound_init(cells[1-placement], ORIGINAL_SOUND_JUNO, 7));
        memcpy(cells[placement], cell, 32); tags[placement] = tag;
        int bank = -1;
        CHECK(bass_sound_cell_validate(cells[placement], &tags[placement], registry, 4, &bank) == 0);
        CHECK(bank == 2 && !memcmp(cells[placement], before, 32));
        CHECK(!memcmp(tags[placement].bytes, tag_before, 12));
    }
    CHECK(bass_sound_tag_decode(&info, &tag, registry, 4) == identity_status());
    CHECK(bass_sound_tag_encode(&encoded, &info, registry, 4) == identity_status());
    CHECK(!memcmp(encoded.bytes, tag_before, 12));
    float bend = -1, damper = -1;
    CHECK(original_sound_value(cell, 6, &bend) && bend == 0.5f);
    CHECK(original_sound_value(cell, 7, &damper) && damper == 0.0f);
    CHECK(!memcmp(cell, before, 32));
    unsigned kind = 99, preset = 99;
    CHECK(bass_pattern_preset_encode(2, factory + 1) == 0xC1u + factory);
    CHECK(bass_pattern_preset_decode(0xC1u + factory, &kind, &preset));
    CHECK(kind == 2 && preset == factory + 1);

    preset_record_t record, decoded;
    preset_record_clear(&record);
    record.engine_id = PRESET_ENGINE_DRONE;
    record.original_engine = PRESET_ENGINE_DRONE;
    record.schema_version = PRESET_SCHEMA_V1;
    record.origin_tag = edited ? PRESET_ORIGIN_USER : PRESET_ORIGIN_FACTORY;
    record.factory_index = (uint16_t)(factory + 1);
    record.nparams = 8;
    record.date = 20261114;
    record.plugin_format = PRESET_PLUGIN_FORMAT; record.plugin_version = 19;
    CHECK(preset_record_set_source(&record, "Archive source") == PRESET_OK);
    CHECK(preset_record_set_name(&record, "Archived Drone") == PRESET_OK);
    memcpy(record.params, cell, 8);
    uint8_t wire[PRESET_MAX_BYTES], second[PRESET_MAX_BYTES];
    uint32_t n = 0, n2 = 0;
    CHECK(preset_record_encode(wire, sizeof wire, &n, &record) == PRESET_OK);
    CHECK(preset_record_decode(&decoded, wire, n, PRESET_ENGINE_DRONE) == PRESET_OK);
    CHECK(preset_record_equal(&record, &decoded));
    CHECK(!memcmp(decoded.params, cell, 8));
    CHECK(preset_record_encode(second, sizeof second, &n2, &decoded) == PRESET_OK);
    CHECK(n == n2 && !memcmp(wire, second, n));
    memset(second, 0xA6, sizeof second); n2 = 19;
    CHECK(preset_record_export(second, sizeof second, &n2, &record) == PRESET_ERR_EXPORT_FACTORY);
    CHECK(n2 == 19 && second[0] == 0xA6);
    record.origin_tag = PRESET_ORIGIN_USER; record.factory_index = 0;
    CHECK(preset_record_export(wire, sizeof wire, &n, &record) == PRESET_OK);
    CHECK(preset_record_import(&decoded, wire, n, PRESET_ENGINE_DRONE) == PRESET_OK);
    CHECK(preset_record_equal(&record, &decoded));
    preset_record_t unchanged = decoded;
    CHECK(preset_record_import(&decoded, wire, n, PRESET_ENGINE_JUNO) == PRESET_ERR_ENGINE);
    CHECK(preset_record_equal(&unchanged, &decoded));
    /* The DNPR schema's opaque byte payload is wider than a live q127 cell. */
    record.params[0] = 255; record.params[7] = 128;
    CHECK(preset_record_export(wire, sizeof wire, &n, &record) == PRESET_OK);
    CHECK(preset_record_import(&decoded, wire, n, PRESET_ENGINE_DRONE) == PRESET_OK);
    CHECK(preset_record_equal(&record, &decoded));
    CHECK(preset_record_export(second, sizeof second, &n2, &decoded) == PRESET_OK);
    CHECK(n == n2 && !memcmp(wire, second, n));
    record.nparams = 7;
    CHECK(preset_record_encode(wire, sizeof wire, &n, &record) == PRESET_ERR_PAYLOAD);
    record.nparams = 8; record.schema_version = 2;
    CHECK(preset_record_encode(wire, sizeof wire, &n, &record) == PRESET_ERR_SCHEMA);
    record.schema_version = 1; record.factory_index = 5;
    CHECK(preset_record_encode(wire, sizeof wire, &n, &record) == PRESET_ERR_ORIGIN);

    /* Exact archive validity must survive the unavailable profile unchanged. */
    uint8_t bad[32]; bass_sound_tag_t bad_tag;
    const unsigned offsets[] = {26,27,28,29,30,31,8,25};
    const uint8_t values[] = {'X','X',3,4,4,1,1,1};
    for (unsigned i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        memcpy(bad, cell, 32); bad[offsets[i]] = values[i]; rejected(bad, &tag);
    }
    memcpy(bad, cell, 32); bad[0] = 128; rejected(bad, &tag);
    const unsigned tag_offsets[] = {0,4,6,7,8,9,10,11};
    const uint8_t tag_values[] = {0,3,7,3,4,1,2,1};
    for (unsigned i = 0; i < sizeof tag_offsets / sizeof tag_offsets[0]; ++i) {
        bad_tag = tag; bad_tag.bytes[tag_offsets[i]] = tag_values[i]; rejected(cell, &bad_tag);
    }
    bad_tag = tag; bad_tag.bytes[8] = (uint8_t)((factory + 1) % 4); rejected(cell, &bad_tag);
    bad_tag = tag; bad_tag.bytes[10] ^= BASS_SOUND_EDITED; rejected(cell, &bad_tag);
    bass_sound_info_t other_info = {{ORIGINAL_SOUND_JUNO_ID, 2, 18, BASS_IDENTITY_KNOWN}, 0, 0};
    CHECK(bass_sound_tag_encode(&bad_tag, &other_info, registry, 4) == BASS_IDENTITY_READY);
    rejected(cell, &bad_tag);
    rejected(NULL, &tag); rejected(cell, NULL);
    CHECK(bass_sound_cell_validate(cell, &tag, registry, 4, NULL) == -9);
    bass_identity_entry_t conflicting[2] = {registry[2], registry[2]};
    conflicting[1].available ^= 1;
    int bank = 55;
    CHECK(bass_sound_cell_validate(cell, &tag, conflicting, 2, &bank) == -9 && bank == 55);
    conflicting[1] = conflicting[0];
    CHECK(bass_sound_cell_validate(cell, &tag, conflicting, 2, &bank) == 0 && bank == 2);
    bank = 55; conflicting[0].available = 2;
    CHECK(bass_sound_cell_validate(cell, &tag, conflicting, 1, &bank) == -9 && bank == 55);
    CHECK(bass_sound_cell_validate(cell, &tag, registry, 2, &bank) == -9 && bank == 55);
}

static void runtime_case(unsigned factory)
{
    static original_runtime_part part, expected;
    static original_runtime_workspace workspace;
    uint8_t cell[32], saved[32];
    float left[258], right[258];
    CHECK(original_sound_init(cell, ORIGINAL_SOUND_DRONE, factory));
    CHECK(original_sound_set(cell, 1, 127));
    cell[6] = 127; cell[7] = 127; cell[22] |= 0xC0;
    memcpy(saved, cell, 32);
    memset(&part, 0xA5, sizeof part);
    int status = X0X_DRONE_RUNTIME_ENABLED ? ORIGINAL_RUNTIME_OK : ORIGINAL_RUNTIME_UNAVAILABLE;
    CHECK(original_runtime_init(&part, cell, 120, NULL, NULL) == status);
    CHECK(part.kind == 2 && part.status == status && !memcmp(cell, saved, 32));
#if !X0X_DRONE_RUNTIME_ENABLED
    memset(&expected, 0, sizeof expected); expected.kind = 2; expected.status = -3;
    CHECK(!memcmp(&part, &expected, sizeof part));
    CHECK(original_runtime_prepare(&part, 130) == -3);
    CHECK(original_runtime_param(&part, 0, 0.2f) == -3);
    for (unsigned source = 0; source < ORIGINAL_INPUT_COUNT; ++source) {
        CHECK(original_runtime_key(&part, source, 60, 100) == -3);
        CHECK(original_runtime_key(&part, source, 60, 0) == -3);
        CHECK(original_runtime_release_source(&part, source) == -3);
        CHECK(original_runtime_cut_source(&part, source) == -3);
    }
    CHECK(!memcmp(&part, &expected, sizeof part));
    CHECK(!original_runtime_active(&part) && !original_runtime_owned(&part));
    float level = 9, mod = 9, values[4] = {9,9,9,9}; unsigned state = 9, down = 9;
    CHECK(!original_runtime_voice_view(&part, &level, &mod, &state, &down));
    CHECK(level == 0 && mod == 0 && state == 6 && down == 0);
    level = mod = 9;
    CHECK(!original_runtime_filter_view(&part, &level, &mod) && level == 0 && mod == 0);
    CHECK(!original_runtime_adsr_view(&part, values, &down));
    CHECK(values[0] == 0 && values[1] == 0 && values[2] == 0 && values[3] == 0 && down == 0);
    float phase = 9, bipolar = 9, current = 9, target = 9; int active = 9, env_state = 9;
    CHECK(!original_runtime_motion_view(&part, &phase, &bipolar, &current, &target, &active));
    CHECK(phase == 0 && bipolar == 0 && current == 0 && target == 0 && active == 0);
    level = 9; values[0] = 9;
    CHECK(!original_runtime_env_view(&part, values, &level, &env_state));
    CHECK(values[0] == 0 && values[1] == 0 && values[2] == 0 && values[3] == 0 && level == 0 && env_state == 6);
    for (unsigned frames = 0; frames <= 256; ++frames) {
        for (unsigned i = 0; i < 258; ++i) left[i] = right[i] = 77;
        CHECK(original_runtime_render(&part, NULL, left + 1, right + 1, frames) == -3);
        CHECK(left[0] == 77 && right[0] == 77);
        for (unsigned i = 1; i <= frames; ++i) CHECK(left[i] == 0 && right[i] == 0);
        for (unsigned i = frames + 1; i < 258; ++i) CHECK(left[i] == 77 && right[i] == 77);
    }
    left[0] = right[0] = 33;
    CHECK(original_runtime_render(&part, &workspace, left, right, 257) == 0);
    CHECK(left[0] == 33 && right[0] == 33);
    CHECK(original_runtime_render(&part, NULL, NULL, right, 1) == 0 && right[0] == 33);
    CHECK(original_runtime_render(&part, NULL, left, left, 1) == -3 && left[0] == 0);
    memset(&part.dsp, 0xA5, sizeof part.dsp); part.keys[0][60] = 3;
    part.stale[0][60] = 5; part.stale_timers = 1;
    CHECK(original_runtime_all_off(&part) == -3);
    CHECK(!memcmp(&part, &expected, sizeof part));
    CHECK(original_runtime_all_off(&part) == -3);
    CHECK(original_runtime_retire(&part) == 1 && !memcmp(&part, &expected, sizeof part));
    CHECK(original_runtime_retire(&part) == 1 && !memcmp(&part, &expected, sizeof part));
#else
    (void)expected;
    CHECK(part.dsp.drone.macro[6] == 0.5f && part.dsp.drone.macro[7] == 0);
    CHECK(original_runtime_prepare(&part, 130) == 1);
    CHECK(original_runtime_key(&part, ORIGINAL_INPUT_KEYS, 60, 100) == 1);
    CHECK(original_runtime_owned(&part) && original_runtime_active(&part));
    double energy = 0;
    for (unsigned b = 0; b < 4; ++b) {
        CHECK(original_runtime_render(&part, &workspace, left, right, 256) == 1);
        for (unsigned i = 0; i < 256; ++i) {
            CHECK(isfinite(left[i]) && isfinite(right[i])); energy += fabsf(left[i]) + fabsf(right[i]);
        }
    }
    CHECK(energy > 0);
    CHECK(original_runtime_release_source(&part, ORIGINAL_INPUT_KEYS) == 1);
    CHECK(original_runtime_all_off(&part) == 1 && !original_runtime_owned(&part));
    CHECK(original_runtime_retire(&part) == 1);
#endif
    /* Available replacement remains Juno kind 3; no renumbering/fallback. */
    CHECK(original_sound_init(cell, ORIGINAL_SOUND_JUNO, 0));
    CHECK(original_runtime_init(&part, cell, 120, NULL, NULL) == 1 && part.kind == 3);
    CHECK(original_runtime_key(&part, ORIGINAL_INPUT_KEYS, 60, 100) == 1);
    CHECK(original_runtime_owned(&part));
    CHECK(original_runtime_render(&part, &workspace, left, right, 256) == 1);
    CHECK(original_runtime_all_off(&part) == 1);
    CHECK(original_runtime_retire(&part) == 1);
    memset(&expected, 0xA5, sizeof expected); part = expected;
    saved[29] = 4;
    CHECK(original_runtime_init(&part, saved, 120, NULL, NULL) == 0);
    CHECK(!memcmp(&part, &expected, sizeof part));
}

/* Baseline red builds deliberately model the previous retire+cold-init path.
 * They compile against49f without inventing a missing-symbol test failure. */
static int hard_replace(original_runtime_part *p, const uint8_t cell[32])
{
#ifdef DRONE_REPLACEMENT_REPRO_BASE
    int rc = original_runtime_retire(p);
    return rc == ORIGINAL_RUNTIME_OK ? original_runtime_init(p, cell, 120, NULL, NULL) : rc;
#else
    return original_runtime_replace(p, cell, 120, NULL, NULL);
#endif
}
static original_runtime_part replacement_part;
static original_runtime_workspace replacement_workspace;
static void init_replacement_juno(void)
{
    uint8_t cell[32];
    CHECK(original_sound_init(cell, 3, 0));
    CHECK(original_runtime_init(&replacement_part, cell, 120, NULL, NULL) == 1);
}
static void replace_kind(unsigned kind)
{
    uint8_t cell[32], before[32];
    CHECK(original_sound_init(cell, kind, 0)); memcpy(before, cell, 32);
    CHECK(hard_replace(&replacement_part, cell) ==
          (kind == 2 && !X0X_DRONE_RUNTIME_ENABLED ? -3 : 1));
    CHECK(!memcmp(cell, before, 32));
    CHECK(replacement_part.kind == kind);
}
static void replacement_render(unsigned frames)
{
    float left[256], right[256];
    for (unsigned i = 0; i < 256; ++i) left[i] = right[i] = 91;
    int unavailable = replacement_part.status == ORIGINAL_RUNTIME_UNAVAILABLE;
    CHECK(original_runtime_render(&replacement_part,
          unavailable ? NULL : &replacement_workspace, left, right, frames) ==
          (unavailable ? -3 : 1));
    for (unsigned i = 0; i < frames; ++i) {
        CHECK(isfinite(left[i]) && isfinite(right[i]));
        if (unavailable) CHECK(left[i] == 0 && right[i] == 0);
    }
    for (unsigned i = frames; i < 256; ++i) CHECK(left[i] == 91 && right[i] == 91);
}

static original_chicago_fx2_compact replacement_fx;
static original_chicago_tuning replacement_tuning;
static struct {
    original_chicago_fx2_cold_owner borrowed;
    unsigned discards;
} replacement_fx_owner;
static int tracked_supports(void *context, unsigned type)
{
    (void)context;
    return replacement_fx_owner.borrowed.supports(replacement_fx_owner.borrowed.context, type);
}
static const original_chicago_fx2_kernel *tracked_select(void *context, unsigned type)
{
    (void)context;
    return replacement_fx_owner.borrowed.select_cold(replacement_fx_owner.borrowed.context, type);
}
static void tracked_discard(void *context)
{
    (void)context;
    ++replacement_fx_owner.discards;
    replacement_fx_owner.borrowed.discard(replacement_fx_owner.borrowed.context);
}
static void replacement_retirement_cases(void)
{
    uint8_t cell[32], next[32];
    CHECK(original_chicago_fx2_compact_init(&replacement_fx));
    CHECK(original_chicago_tuning_bind(&replacement_tuning, 0, 0, NULL, 0x6d70));
    replacement_fx_owner.borrowed = original_chicago_fx2_compact_owner(&replacement_fx);
    replacement_fx_owner.discards = 0;
    original_chicago_fx2_cold_owner owner = {NULL, tracked_supports, tracked_select, tracked_discard, NULL};
    CHECK(original_sound_init(cell, 0, 0));
    CHECK(original_runtime_init(&replacement_part, cell, 120, &replacement_tuning, &owner) == 1);
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 90) == 1);
    unsigned discards = replacement_fx_owner.discards;
    original_chicago_backend_workspace workspace = {
        replacement_workspace.chicago.left, replacement_workspace.chicago.right,
        replacement_workspace.chicago.converted[0], replacement_workspace.chicago.converted[1],
        {replacement_workspace.chicago.scratch[0], replacement_workspace.chicago.scratch[1],
         replacement_workspace.chicago.scratch[2], replacement_workspace.chicago.scratch[3]}, 256};
    CHECK(original_chicago_backend_begin(&replacement_part.dsp.chicago, &workspace, NULL, 0, 1) == 1);
    static original_runtime_part snapshot;
    snapshot = replacement_part;
    CHECK(original_sound_init(next, 2, 0));
    CHECK(hard_replace(&replacement_part, next) == 0);
    CHECK(!memcmp(&snapshot, &replacement_part, sizeof snapshot));
    CHECK(replacement_fx_owner.discards == discards);
    CHECK(original_chicago_backend_frame(&replacement_part.dsp.chicago) == 1);
    float left[256], right[256];
    CHECK(original_chicago_backend_end(&replacement_part.dsp.chicago, left, right) == 1);
    discards = replacement_fx_owner.discards;
    replace_kind(2);
    CHECK(replacement_fx_owner.discards == discards + 1);
    CHECK(replacement_part.stale[ORIGINAL_INPUT_USB][60] == 1);
    replace_kind(2); replace_kind(3);
    CHECK(replacement_fx_owner.discards == discards + 1);

#ifndef DRONE_REPLACEMENT_REPRO_BASE
    /* A valid cell whose backend lacks resources ends the old hard-adopted
     * voice, reports the real refusal and keeps its external release debt. */
    init_replacement_juno();
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 90) == 1);
    CHECK(original_runtime_replace(&replacement_part, cell, 120, &replacement_tuning, NULL) == -1);
    CHECK(replacement_part.kind == 0 && replacement_part.status == -1);
    CHECK(!replacement_part.keys[0][60] && replacement_part.stale[0][60] == 1);
    CHECK(!original_runtime_owned(&replacement_part));
#endif
}
#if !X0X_DRONE_RUNTIME_ENABLED
static void check_unavailable_owners(void)
{
    CHECK(replacement_part.kind == 2 && replacement_part.status == -3);
    CHECK(!original_runtime_active(&replacement_part) && !original_runtime_owned(&replacement_part));
    for (unsigned s = 0; s < ORIGINAL_INPUT_COUNT; ++s)
        for (unsigned key = 0; key < 128; ++key) CHECK(!replacement_part.keys[s][key]);
    const uint8_t *dsp = (const uint8_t *)&replacement_part.dsp;
    for (unsigned i = 0; i < sizeof replacement_part.dsp; ++i) CHECK(dsp[i] == 0);
}

static void source_disconnect_red(void)
{
    init_replacement_juno(); replace_kind(2);
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 90) == -3);
    CHECK(replacement_part.stale[ORIGINAL_INPUT_USB][60] == 1);
    CHECK(original_runtime_release_source(&replacement_part, ORIGINAL_INPUT_USB) == -3);
    CHECK(replacement_part.stale[ORIGINAL_INPUT_USB][60] == 0);
    replace_kind(3);
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 100) == 1);
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 0) == 1);
    CHECK(replacement_part.keys[ORIGINAL_INPUT_USB][60] == 0);
}
static void unavailable_disconnect_cases(void)
{
    original_runtime_part *p = &replacement_part;
    for (unsigned source = 0; source < ORIGINAL_INPUT_KEYS; ++source) {
        unsigned other = (source + 1) % ORIGINAL_INPUT_KEYS;
        init_replacement_juno(); replace_kind(2);
        CHECK(original_runtime_key(p, source, 60, 90) == -3);
        CHECK(original_runtime_key(p, other, 61, 90) == -3);
        CHECK(p->stale[source][60] == 1 && p->stale[other][61] == 1);
        CHECK(original_runtime_release_source(p, source) == -3);
        CHECK(!p->stale[source][60] && p->stale[other][61] == 1);
        CHECK(original_runtime_release_source(p, source) == -3);
        CHECK(!p->stale[source][60] && p->stale[other][61] == 1 && !p->stale_timers);
        check_unavailable_owners();
        replace_kind(3);
        CHECK(original_runtime_key(p, source, 60, 100) == 1);
        CHECK(original_runtime_key(p, source, 60, 0) == 1);
        CHECK(!p->keys[source][60] && !p->stale_ttl[source][60] && !p->stale_timers);
        CHECK(p->stale[other][61] == 1);
    }

    /* Create real no-held deadlines, two pitches in each MIDI source. A
     * disconnect removes only its own three timers and preserves the exact
     * other-source state and shared255-frame phase. The third target timer,
     * held count and pending bit defensively exercise silent cleanup of a
     * partially populated source without any backend call. */
    for (unsigned source = 0; source < ORIGINAL_INPUT_KEYS; ++source) {
        init_replacement_juno();
        for (unsigned input = 0; input < ORIGINAL_INPUT_KEYS; ++input) {
            for (unsigned key = 60; key <= 61; ++key) {
                CHECK(original_runtime_key(p, input, key, 90) == 1);
                CHECK(original_runtime_cut_source(p, input) == 1);
                CHECK(original_runtime_key(p, input, key, 100) == 1);
                CHECK(original_runtime_key(p, input, key, 0) == 1);
                CHECK(original_runtime_key(p, input, key, 0) == 1);
            }
        }
        replacement_render(255); replace_kind(2);
        CHECK(p->stale_timers == 6 && p->stale_tick_frames == 255);
        for (unsigned input = 0; input < ORIGINAL_INPUT_KEYS; ++input)
            CHECK(original_runtime_key(p, input, 64, 90) == -3);
        p->keys[source][12] = 2;
        p->stale[source][12] = 3; p->stale_ttl[source][12] = 1;
        p->stale_pending[source][0] = UINT32_C(1) << 12;
        ++p->stale_timers;
        static original_runtime_part before;
        before = *p;
        CHECK(original_runtime_release_source(p, source) == -3);
        CHECK(p->kind == 2 && p->status == -3 && p->stale_timers == 4 && p->stale_tick_frames == 255);
        CHECK(!memcmp(&p->dsp, &before.dsp, sizeof p->dsp));
        for (unsigned key = 0; key < 128; ++key)
            CHECK(!p->keys[source][key] && !p->stale[source][key] && !p->stale_ttl[source][key]);
        for (unsigned lane = 0; lane < 4; ++lane) CHECK(!p->stale_pending[source][lane]);
        for (unsigned input = 0; input < ORIGINAL_INPUT_COUNT; ++input) if (input != source) {
            CHECK(!memcmp(p->keys[input], before.keys[input], sizeof p->keys[input]));
            if (input < ORIGINAL_INPUT_KEYS) {
                CHECK(!memcmp(p->stale[input], before.stale[input], sizeof p->stale[input]));
                CHECK(!memcmp(p->stale_ttl[input], before.stale_ttl[input], sizeof p->stale_ttl[input]));
                CHECK(!memcmp(p->stale_pending[input], before.stale_pending[input], sizeof p->stale_pending[input]));
            }
        }
        before = *p;
        CHECK(original_runtime_release_source(p, source) == -3);
        CHECK(!memcmp(p, &before, sizeof before));
        CHECK(original_runtime_release_source(p, ORIGINAL_INPUT_COUNT) == 0);
        CHECK(!memcmp(p, &before, sizeof before));
        replacement_render(0);
        CHECK(p->stale_tick_frames == 255 && p->stale_timers == 4);
        replacement_render(1);
        CHECK(!p->stale_tick_frames && p->stale_timers == 4);
        for (unsigned input = 0; input < ORIGINAL_INPUT_KEYS; ++input) if (input != source)
            CHECK(p->stale_ttl[input][60] == 15 && p->stale_ttl[input][61] == 15);
        CHECK(original_runtime_release_source(p, ORIGINAL_INPUT_KEYS) == -3);
        CHECK(p->stale_timers == 4);
        check_unavailable_owners();
    }
}
static void replacement_red_adoption(void)
{
    init_replacement_juno();
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 90) == 1);
    replace_kind(2);
    CHECK(replacement_part.stale[ORIGINAL_INPUT_USB][60] == 1);
}
static void replacement_red_unheard(void)
{
    init_replacement_juno(); replace_kind(2);
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_BLE, 61, 90) == -3);
    CHECK(replacement_part.stale[ORIGINAL_INPUT_BLE][61] == 1);
}
static void replacement_cases(void)
{
    original_runtime_part *p = &replacement_part;
    for (unsigned source = 0; source < ORIGINAL_INPUT_KEYS; ++source) {
        unsigned other = (source + 1) % ORIGINAL_INPUT_KEYS;
        /* The original held press ends at the actual hard-adoption boundary.
         * Its delayed off after escape consumes debt, not a fresh Juno press. */
        init_replacement_juno();
        CHECK(original_runtime_key(p, source, 60, 90) == 1);
        CHECK(original_runtime_key(p, ORIGINAL_INPUT_KEYS, 64, 90) == 1);
        CHECK(original_runtime_key(p, ORIGINAL_INPUT_SEQUENCE, 67, 90) == 1);
        replace_kind(2); check_unavailable_owners();
        CHECK(p->stale[source][60] == 1 && p->stale_timers == 0);
        replacement_render(256); /* Waiting alone never starts retrigger grace. */
        CHECK(p->stale[source][60] == 1 && p->stale_timers == 0);
        replace_kind(3);
        CHECK(original_runtime_key(p, source, 60, 100) == 1);
        CHECK(original_runtime_key(p, other, 60, 100) == 1);
        CHECK(p->stale_ttl[source][60] == 16 && p->stale_timers == 1);
        CHECK(original_runtime_key(p, source, 60, 0) == 1);
        CHECK(p->keys[source][60] == 1 && p->keys[other][60] == 1);
        CHECK(!p->stale[source][60] && (p->stale_pending[source][1] & (UINT32_C(1) << 28)));
        CHECK(original_runtime_key(p, source, 60, 0) == 1);
        CHECK(!p->keys[source][60] && p->keys[other][60] == 1);
        CHECK(original_runtime_owned(p));
        CHECK(original_runtime_key(p, other, 60, 0) == 1 && !p->keys[other][60]);

        /* If the old off never arrives, the one real current off is ambiguous.
         * Exactly the existing16*256-frame grace recovers that held count. */
        init_replacement_juno();
        CHECK(original_runtime_key(p, source, 60, 90) == 1);
        replace_kind(2); replace_kind(3);
        CHECK(original_runtime_key(p, source, 60, 100) == 1);
        CHECK(original_runtime_key(p, source, 60, 0) == 1);
        for (unsigned i = 0; i < 15; ++i) replacement_render(256);
        replacement_render(255);
        CHECK(p->keys[source][60] == 1 && p->stale_ttl[source][60] == 1 && p->stale_tick_frames == 255);
        replacement_render(1);
        CHECK(!p->keys[source][60] && !p->stale_ttl[source][60] && !p->stale_timers);
        CHECK(!(p->stale_pending[source][1] & (UINT32_C(1) << 28)));

        /* Refused MIDI onsets also owe offs, without becoming live owners. */
        init_replacement_juno(); replace_kind(2);
        CHECK(original_runtime_key(p, source, 60, 90) == -3);
        CHECK(p->stale[source][60] == 1); check_unavailable_owners();
        CHECK(original_runtime_key(p, source, 60, 90) == -3);
        CHECK(p->stale[source][60] == 2); check_unavailable_owners();
        CHECK(original_runtime_key(p, source, 60, 0) == -3 && p->stale[source][60] == 1);
        replace_kind(2);
        CHECK(p->stale[source][60] == 1); check_unavailable_owners();
        replace_kind(3);
        CHECK(original_runtime_key(p, source, 60, 100) == 1);
        CHECK(original_runtime_key(p, source, 60, 0) == 1 && p->keys[source][60] == 1);
        CHECK(original_runtime_key(p, source, 60, 0) == 1 && !p->keys[source][60]);

        /* When old/unheard offs arrive while unavailable, no stale debt leaks
         * into the next accepted press and its current off works immediately. */
        init_replacement_juno();
        CHECK(original_runtime_key(p, source, 60, 90) == 1); replace_kind(2);
        CHECK(original_runtime_key(p, source, 60, 90) == -3 && p->stale[source][60] == 2);
        CHECK(original_runtime_key(p, source, 60, 0) == -3 && p->stale[source][60] == 1);
        CHECK(original_runtime_key(p, source, 60, 0) == -3 && !p->stale[source][60]);
        replace_kind(3);
        CHECK(original_runtime_key(p, source, 60, 100) == 1);
        CHECK(original_runtime_key(p, source, 60, 0) == 1 && !p->keys[source][60]);
    }

    /* Preserve an unrelated no-held deadline and fractional input clock.
     * Do not copy pending ambiguity into new held debt or restart timers. */
    init_replacement_juno();
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 90) == 1);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_BLE, 61, 90) == 1);
    CHECK(original_runtime_cut_source(p, ORIGINAL_INPUT_USB) == 1);
    CHECK(original_runtime_cut_source(p, ORIGINAL_INPUT_BLE) == 1);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 100) == 1);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_BLE, 61, 100) == 1);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 0) == 1);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_BLE, 61, 0) == 1);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_BLE, 61, 0) == 1);
    replacement_render(17);
    CHECK(p->stale_timers == 2 && p->stale_tick_frames == 17);
    replace_kind(2);
    CHECK(p->stale[0][60] == 1 && !p->stale_ttl[0][60] && !p->stale_pending[0][1]);
    CHECK(!p->keys[1][61] && p->stale_ttl[1][61] == 16);
    CHECK(p->stale_timers == 1 && p->stale_tick_frames == 17);
    replace_kind(2);
    CHECK(p->stale_timers == 1 && p->stale_tick_frames == 17 && p->stale_ttl[1][61] == 16);
    replacement_render(0);
    CHECK(p->stale_tick_frames == 17 && p->stale_ttl[1][61] == 16);
    float l = 73, r = 73;
    CHECK(original_runtime_render(p, NULL, &l, &r, 257) == 0);
    CHECK(l == 73 && r == 73 && p->stale_tick_frames == 17);
    replacement_render(238);
    CHECK(p->stale_tick_frames == 255 && p->stale_ttl[1][61] == 16);
    replacement_render(1);
    CHECK(!p->stale_tick_frames && p->stale_ttl[1][61] == 15);
    for (unsigned i = 0; i < 15; ++i) replacement_render(256);
    CHECK(!p->stale_timers && !p->stale_ttl[1][61] && p->stale[0][60] == 1);
    check_unavailable_owners();

    /* New unheard input follows the existing new-cut reset policy for that
     * pitch alone. Saturate counts rather than wrapping or faulting live DSP. */
    for (unsigned i = 0; i < 300; ++i) CHECK(original_runtime_key(p, ORIGINAL_INPUT_TRS, 62, 90) == -3);
    CHECK(p->stale[ORIGINAL_INPUT_TRS][62] == 255);
    for (unsigned i = 0; i < 255; ++i) CHECK(original_runtime_key(p, ORIGINAL_INPUT_TRS, 62, 0) == -3);
    CHECK(!p->stale[ORIGINAL_INPUT_TRS][62]); check_unavailable_owners();
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_KEYS, 62, 90) == -3);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_SEQUENCE, 62, 90) == -3);
    CHECK(!p->stale[ORIGINAL_INPUT_TRS][62]); check_unavailable_owners();
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 0) == -3);
    CHECK(!p->stale[ORIGINAL_INPUT_USB][60]);


    /* An unheard onset is a new cut obligation: cancel only that pitch's
     * old no-held deadline, retaining another pitch's deadline and phase. */
    init_replacement_juno();
    for (unsigned key = 60; key <= 61; ++key) {
        CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, key, 90) == 1);
        CHECK(original_runtime_cut_source(p, ORIGINAL_INPUT_USB) == 1);
        CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, key, 100) == 1);
        CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, key, 0) == 1);
        CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, key, 0) == 1);
    }
    replacement_render(33); replace_kind(2);
    CHECK(p->stale_timers == 2 && p->stale_tick_frames == 33);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 90) == -3);
    CHECK(p->stale[0][60] == 1 && !p->stale_ttl[0][60]);
    CHECK(p->stale_ttl[0][61] == 16 && p->stale_timers == 1 && p->stale_tick_frames == 33);
    unsigned debt = p->stale[0][60];
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 128, 90) == 0);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 128) == 0);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_COUNT, 60, 90) == 0);
    CHECK(p->stale[0][60] == debt && p->stale_timers == 1 && p->stale_tick_frames == 33);
    replace_kind(2);
    CHECK(p->stale[0][60] == 1 && p->stale_ttl[0][61] == 16 && p->stale_tick_frames == 33);

    /* Accepted held counts merge with old unheard debt with the same255 cap;
     * hard replacement never adds the same current press a second time. */
    CHECK(original_runtime_all_off(p) == -3);
    for (unsigned i = 0; i < 250; ++i) CHECK(original_runtime_key(p, ORIGINAL_INPUT_TRS, 62, 90) == -3);
    replace_kind(3);
    for (unsigned i = 0; i < 10; ++i) CHECK(original_runtime_key(p, ORIGINAL_INPUT_TRS, 62, 90) == 1);
    CHECK(p->keys[2][62] == 10 && p->stale[2][62] == 250 && p->stale_timers == 1);
    replace_kind(2);
    CHECK(!p->keys[2][62] && p->stale[2][62] == 255 && !p->stale_timers);
    replace_kind(2);
    CHECK(p->stale[2][62] == 255 && !p->stale_timers);
    check_unavailable_owners();

    /* Bad replacement input must preserve current DSP, held/debt and clock.
     * Copy for comparison is fixture-only; production never copies a part. */
    init_replacement_juno();
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 90) == 1);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_BLE, 61, 90) == 1);
    CHECK(original_runtime_cut_source(p, ORIGINAL_INPUT_BLE) == 1);
    static original_runtime_part before;
    before = *p;
    uint8_t bad[32]; CHECK(original_sound_init(bad, 2, 0)); bad[28] = 3;
    CHECK(hard_replace(p, bad) == 0);
    CHECK(!memcmp(p, &before, sizeof before));
    CHECK(hard_replace(p, NULL) == 0 && !memcmp(p, &before, sizeof before));

    /* Explicit panic/cold init continue to clear debt by caller intent. */
    replace_kind(2);
    CHECK(p->stale[0][60] == 1 && p->stale[1][61] == 1);
    CHECK(original_runtime_all_off(p) == -3);
    CHECK(!p->stale[0][60] && !p->stale[1][61] && !p->stale_timers);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 90) == -3 && p->stale[0][60] == 1);
    init_replacement_juno();
    CHECK(!p->stale[0][60] && !p->stale_timers);
}
#else
static void replacement_cases(void)
{
    init_replacement_juno();
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 90) == 1);
    replace_kind(2);
    CHECK(replacement_part.stale[0][60] == 1 && !replacement_part.keys[0][60]);
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 100) == 1);
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 0) == 1);
    CHECK(replacement_part.keys[0][60] == 1);
    replacement_render(256);
    CHECK(original_runtime_key(&replacement_part, ORIGINAL_INPUT_USB, 60, 0) == 1);
    CHECK(!replacement_part.keys[0][60]);
    replace_kind(3);
}
#endif


static void ready_disconnect_cases(void)
{
    original_runtime_part *p = &replacement_part;
    for (unsigned source = 0; source < ORIGINAL_INPUT_KEYS; ++source) {
        unsigned other = (source + 1) % ORIGINAL_INPUT_KEYS;
        init_replacement_juno();
        CHECK(original_runtime_key(p, source, 60, 90) == 1);
        CHECK(original_runtime_cut_source(p, source) == 1);
        CHECK(original_runtime_key(p, source, 60, 100) == 1);
        CHECK(original_runtime_key(p, other, 60, 100) == 1);
        replacement_render(255);
        CHECK(p->stale_timers == 1 && p->stale_tick_frames == 255);
        CHECK(original_runtime_release_source(p, source) == 1);
        CHECK(!p->keys[source][60] && !p->stale[source][60] && !p->stale_ttl[source][60]);
        CHECK(!p->stale_pending[source][1] && !p->stale_timers && p->stale_tick_frames == 255);
        CHECK(p->keys[other][60] == 1 && p->status == 1 && original_runtime_owned(p));
        CHECK(original_runtime_key(p, other, 60, 0) == 1 && !p->keys[other][60]);
        CHECK(original_runtime_release_source(p, source) == 1 && !p->stale_timers);
    }
    /* Preserve the established sticky-fault behavior of an available engine. */
    init_replacement_juno();
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 90) == 1);
    CHECK(original_runtime_cut_source(p, ORIGINAL_INPUT_USB) == 1);
    CHECK(original_runtime_key(p, ORIGINAL_INPUT_USB, 60, 100) == 1);
    p->status = ORIGINAL_RUNTIME_FAULT;
    static original_runtime_part before;
    before = *p;
    CHECK(original_runtime_release_source(p, ORIGINAL_INPUT_USB) == ORIGINAL_RUNTIME_FAULT);
    CHECK(!memcmp(p, &before, sizeof before));
    CHECK(original_runtime_release_source(NULL, ORIGINAL_INPUT_USB) == 0);
}

int main(int argc, char **argv)
{
#if !X0X_DRONE_RUNTIME_ENABLED
    if (argc == 2 && !strcmp(argv[1], "--red-adoption")) { replacement_red_adoption(); return 0; }
    if (argc == 2 && !strcmp(argv[1], "--red-unheard")) { replacement_red_unheard(); return 0; }
    if (argc == 2 && !strcmp(argv[1], "--red-disconnect")) { source_disconnect_red(); return 0; }
#else
    (void)argc; (void)argv;
#endif
    CHECK(ORIGINAL_SOUND_DRONE == 2 && ORIGINAL_SOUND_JUNO == 3);
    CHECK(ORIGINAL_SOUND_DRONE_ID == UINT32_C(0x4f445231));
    CHECK(ORIGINAL_SOUND_JUNO_ID == UINT32_C(0x4f4a5531));
    CHECK(ORIGINAL_SOUND_SCHEMA == 2 && ORIGINAL_SOUND_BYTES == 32);
    CHECK(ORIGINAL_DRONE_NPARAMS == 8 && ORIGINAL_DRONE_FACTORIES == 4);
    CHECK(PRESET_ENGINE_DRONE == 4 && PRESET_ENGINE_JUNO == 3);
    CHECK(PRESET_FACTORY_DRONE_N == 4 && PRESET_DRONE_NPARAMS_V1 == 8);
    CHECK(ORIGINAL_RUNTIME_OK == 1 && ORIGINAL_RUNTIME_INVALID == 0);
    CHECK(ORIGINAL_RUNTIME_NEEDS_FX == -1 && ORIGINAL_RUNTIME_FAULT == -2);
    CHECK(ORIGINAL_RUNTIME_UNAVAILABLE == -3);
    CHECK(original_runtime_kind_available(0) && original_runtime_kind_available(1));
    CHECK(original_runtime_kind_available(3));
    CHECK(original_runtime_kind_available(2) == X0X_DRONE_RUNTIME_ENABLED);
    CHECK(!original_runtime_kind_available(-1) && !original_runtime_kind_available(4));
    for (unsigned factory = 0; factory < 4; ++factory) {
        archive_case(factory, 0); archive_case(factory, 1); runtime_case(factory);
    }
    replacement_cases();
    replacement_retirement_cases();
#if !X0X_DRONE_RUNTIME_ENABLED
    unavailable_disconnect_cases();
#endif
    ready_disconnect_cases();
    CHECK(original_runtime_retire(NULL) == 0);
    printf("drone profile %s: %u checks PASS (runtime/archive only)\n",
        X0X_DRONE_RUNTIME_ENABLED ? "historical opt-in" : "shipping disabled", checks);
    return 0;
}
