/* SPDX-License-Identifier: GPL-3.0-only */
#include "original_sound.h"
#include <string.h>

static const char *const original_sound_fx_names[ORIGINAL_CHICAGO_FX_CHOICES] = {
    "Bypass", "Chorus", "Phaser", "Ring", "Comp", "Decim", "EQ"
};
static const x0x_param_t original_sound_chicago_params[22] = {
    {"Wave",127,0,0},{"Glide",127,0,0},{"Cutoff",127,0,0},{"Peak",127,0,0},
    {"EnvAmt",127,0,0},{"Bite",127,0,0},{"Gnaw",127,0,0},{"Attack",127,0,0},
    {"Decay",127,0,0},{"Sust",127,0,0},{"Rel",127,0,0},{"AmpMod",127,0,0},
    {"Level",127,0,0},{"ArpOn",127,0,0},{"ArpMod",127,0,0},{"ArpRng",127,0,0},
    {"ArpSpd",127,0,0},{"FxType",6,0,original_sound_fx_names},{"FxEd1",127,0,0},{"FxEd2",127,0,0},
    {"PBend",127,0,0},{"Damper",127,0,0}
};

/* Original program IDs whose selected FX is admitted by the compact policy.
 * Program data and untouched floats remain in their original immutable table. */
static const uint8_t original_sound_chicago_factories[ORIGINAL_CHICAGO_FACTORIES] = {
    0,1,2,3,4,5,6,11,12,13,14,15,17,18,19,21,22,29,30,31,32,34,35,36,37,38,39,40,
    41,42,43,46,47,49,52,53,54,55,57,58,59,60,62,63,67,69,70,71,72,73,74,76,78,79,80
};
static const uint8_t original_sound_fx_types[ORIGINAL_CHICAGO_FX_CHOICES] = {0,4,5,10,18,22,24};
static const uint8_t original_sound_fx_macros[ORIGINAL_CHICAGO_FX_CHOICES] = {0,5,7,4,2,3,1};
static const x0x_param_t original_sound_miami_params[18] = {
    {"CrWave",127,0,0},{"CrLvl",127,0,0},{"Sub",127,0,0},{"CrPit",127,0,0},
    {"Glide",127,0,0},{"XmLvl",127,0,0},{"XmWave",127,0,0},{"XmTune",127,0,0},
    {"XmDep",127,0,0},{"Cutoff",127,0,0},{"Crush",127,0,0},{"WobShp",127,0,0},
    {"WobDep",127,0,0},{"WobRat",127,0,0},{"Output",127,0,0},{"Stereo",127,0,0},
    {"PBend",127,0,0},{"Damper",127,0,0}
};

unsigned original_sound_count(unsigned kind)
{ return kind == ORIGINAL_SOUND_CHICAGO ? 22u : kind == ORIGINAL_SOUND_MIAMI ? 18u : 0u; }
unsigned original_sound_factories(unsigned kind)
{ return kind == ORIGINAL_SOUND_CHICAGO ? ORIGINAL_CHICAGO_FACTORIES : kind == ORIGINAL_SOUND_MIAMI ? 30u : 0u; }
int original_sound_factory_source(unsigned kind, unsigned factory)
{
    if (factory >= original_sound_factories(kind)) return -1;
    return kind == ORIGINAL_SOUND_CHICAGO ? original_sound_chicago_factories[factory] : (int)factory;
}
const char *original_sound_fx_name(unsigned choice)
{ return choice < ORIGINAL_CHICAGO_FX_CHOICES ? original_sound_fx_names[choice] : 0; }
int original_sound_control_value(unsigned kind, unsigned parameter, unsigned control, float *value)
{
    const x0x_param_t *p = original_sound_param(kind, parameter);
    if (!value || !p || control > p->max) return 0;
    *value = kind == ORIGINAL_SOUND_CHICAGO && parameter == ORIGINAL_CHICAGO_FX_TYPE ?
        (float)original_sound_fx_macros[control] / 11.0f : (float)control / 127.0f;
    return 1;
}
uint32_t original_sound_engine_id(unsigned kind)
{ return kind == ORIGINAL_SOUND_CHICAGO ? ORIGINAL_SOUND_CHICAGO_ID : kind == ORIGINAL_SOUND_MIAMI ? ORIGINAL_SOUND_MIAMI_ID : 0u; }
const char *original_sound_name(unsigned kind, unsigned factory)
{
    int source = original_sound_factory_source(kind, factory);
    if (source < 0) return 0;
    return kind == ORIGINAL_SOUND_CHICAGO ? original_chicago_program_name((unsigned)source) : original_miami_program_name((unsigned)source);
}
const x0x_param_t *original_sound_param(unsigned kind, unsigned parameter)
{
    if (parameter >= original_sound_count(kind)) return 0;
    return kind == ORIGINAL_SOUND_CHICAGO ? original_sound_chicago_params + parameter : original_sound_miami_params + parameter;
}
uint32_t original_sound_edit_mask(const uint8_t cell[ORIGINAL_SOUND_BYTES])
{
    if (!cell) return 0;
    return (uint32_t)cell[22] | (uint32_t)cell[23] << 8 | (uint32_t)cell[24] << 16 | (uint32_t)cell[25] << 24;
}
static void original_sound_put_mask(uint8_t cell[ORIGINAL_SOUND_BYTES], uint32_t mask)
{
    for (unsigned i = 0; i < 4; ++i) cell[22 + i] = (uint8_t)(mask >> (8 * i));
}
int original_sound_valid(const uint8_t cell[ORIGINAL_SOUND_BYTES])
{
    if (!cell || cell[26] != 'O' || cell[27] != 'S' || cell[28] != ORIGINAL_SOUND_SCHEMA || cell[31]) return 0;
    unsigned count = original_sound_count(cell[29]);
    if (!count || cell[30] >= original_sound_factories(cell[29])) return 0;
    if (original_sound_edit_mask(cell) >> count) return 0;
    for (unsigned i = 0; i < count; ++i) if (cell[i] > original_sound_param(cell[29], i)->max) return 0;
    for (unsigned i = count; i < 22; ++i) if (cell[i]) return 0;
    return 1;
}
int original_sound_init(uint8_t cell[ORIGINAL_SOUND_BYTES], unsigned kind, unsigned factory)
{
    int source = original_sound_factory_source(kind, factory);
    if (!cell || source < 0) return 0;
    uint8_t next[ORIGINAL_SOUND_BYTES] = {0};
    if (kind == ORIGINAL_SOUND_CHICAGO) {
        original_chicago_program p;
        original_chicago_routing routing;
        if (!original_chicago_program_init(&p, (unsigned)source) || !original_chicago_program_routing(&p, &routing)) return 0;
        for (unsigned i = 0; i < 22; ++i) next[i] = (uint8_t)(p.macro[i] * 127.0f + 0.5f);
        unsigned choice = 0;
        while (choice < ORIGINAL_CHICAGO_FX_CHOICES && original_sound_fx_types[choice] != routing.fx[1][0]) ++choice;
        if (choice == ORIGINAL_CHICAGO_FX_CHOICES) return 0;
        next[ORIGINAL_CHICAGO_FX_TYPE] = (uint8_t)choice;
    } else {
        original_miami_program p;
        if (!original_miami_program_init(&p, (unsigned)source)) return 0;
        for (unsigned i = 0; i < 18; ++i) next[i] = (uint8_t)(p.macro[i] * 127.0f + 0.5f);
    }
    next[26] = 'O'; next[27] = 'S'; next[28] = ORIGINAL_SOUND_SCHEMA;
    next[29] = (uint8_t)kind; next[30] = (uint8_t)factory;
    memcpy(cell, next, sizeof next);
    return 1;
}
int original_sound_set(uint8_t cell[ORIGINAL_SOUND_BYTES], unsigned parameter, unsigned value)
{
    if (!original_sound_valid(cell) || parameter >= original_sound_count(cell[29]) ||
        value > original_sound_param(cell[29], parameter)->max) return 0;
    uint32_t mask = original_sound_edit_mask(cell) | (UINT32_C(1) << parameter);
    cell[parameter] = (uint8_t)value;
    original_sound_put_mask(cell, mask);
    return 1;
}
int original_sound_restore_miami(original_miami_program *out, const uint8_t cell[ORIGINAL_SOUND_BYTES])
{
    if (!out || !original_sound_valid(cell) || cell[29] != ORIGINAL_SOUND_MIAMI) return 0;
    original_miami_program next;
    if (!original_miami_program_init(&next, (unsigned)original_sound_factory_source(cell[29], cell[30]))) return 0;
    uint32_t mask = original_sound_edit_mask(cell);
    for (unsigned i = 0; i < 18; ++i)
        if (mask & (UINT32_C(1) << i)) (void)original_miami_program_set(&next, i, (float)cell[i] / 127.0f);
    *out = next;
    return 1;
}
int original_sound_restore_chicago(original_chicago_program *out, const uint8_t cell[ORIGINAL_SOUND_BYTES])
{
    if (!out || !original_sound_valid(cell) || cell[29] != ORIGINAL_SOUND_CHICAGO) return 0;
    original_chicago_program next;
    if (!original_chicago_program_init(&next, (unsigned)original_sound_factory_source(cell[29], cell[30]))) return 0;
    uint32_t mask = original_sound_edit_mask(cell);
    for (unsigned i = 0; i < 22; ++i) if (mask & (UINT32_C(1) << i)) {
        float value;
        if (!original_sound_control_value(cell[29], i, cell[i], &value) ||
            !original_chicago_program_set(&next, i, value)) return 0;
    }
    *out = next;
    return 1;
}
int original_sound_value(const uint8_t cell[ORIGINAL_SOUND_BYTES], unsigned parameter, float *value)
{
    if (!value || !original_sound_valid(cell) || parameter >= original_sound_count(cell[29])) return 0;
    float result;
    if (original_sound_edit_mask(cell) & (UINT32_C(1) << parameter)) {
        if (!original_sound_control_value(cell[29], parameter, cell[parameter], &result)) return 0;
    }
    else if (cell[29] == ORIGINAL_SOUND_CHICAGO) {
        original_chicago_program p;
        if (!original_chicago_program_init(&p, (unsigned)original_sound_factory_source(cell[29], cell[30]))) return 0;
        result = p.macro[parameter];
    } else {
        original_miami_program p;
        if (!original_miami_program_init(&p, cell[30])) return 0;
        result = p.macro[parameter];
    }
    *value = result;
    return 1;
}
