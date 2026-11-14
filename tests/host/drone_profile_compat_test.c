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
int main(void)
{
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
    CHECK(original_runtime_retire(NULL) == 0);
    printf("drone profile %s: %u checks PASS (runtime/archive only)\n",
        X0X_DRONE_RUNTIME_ENABLED ? "historical opt-in" : "shipping disabled", checks);
    return 0;
}
