/* SPDX-License-Identifier: GPL-3.0-only */
#include "original_sound.h"
#include <string.h>

static const char *const original_sound_fx_names[ORIGINAL_CHICAGO_FX_CHOICES] = {
    "OFF", "CHO/FLG", "PHASER", "RING MOD", "COMPRESSOR", "DECIMATOR", "EQ",
    "ENSEM", "SH DLY", "BPM DL", "HALL", "GT REV"
};
static const x0x_param_t original_sound_chicago_params[22] = {
    {"Wave",127,0,0},{"Glide",127,0,0},{"Cutoff",127,0,0},{"Peak",127,0,0},
    {"EnvAmt",127,0,0},{"Bite",127,0,0},{"Gnaw",127,0,0},{"Attack",127,0,0},
    {"Decay",127,0,0},{"Sust",127,0,0},{"Rel",127,0,0},{"AmpMod",127,0,0},
    {"Level",127,0,0},{"ArpOn",127,0,0},{"ArpMod",127,0,0},{"ArpRng",127,0,0},
    {"ArpSpd",127,0,0},{"FxType",ORIGINAL_CHICAGO_FX_CHOICES - 1u,0,original_sound_fx_names},{"FxEd1",127,0,0},{"FxEd2",127,0,0},
    {"PBend",127,0,0},{"Damper",127,0,0}
};

/* Visible Chicago factory order -> original program ID. Positions 0..54 keep
 * the stored indexes of the 55 compact-policy programs (saved schema-2 cells
 * stay the same sound). Positions 55..80 are the 26 programs restored with the
 * wip Ensemble, Short Delay, BPM Delay, Hall and Gate Reverb inserts.
 * Program data and untouched floats remain in their original immutable table. */
static const uint8_t original_sound_chicago_factories[ORIGINAL_CHICAGO_FACTORIES] = {
    0,1,2,3,4,5,6,11,12,13,14,15,17,18,19,21,22,29,30,31,32,34,35,36,37,38,39,40,
    41,42,43,46,47,49,52,53,54,55,57,58,59,60,62,63,67,69,70,71,72,73,74,76,78,79,80,
    7,8,9,10,16,20,23,24,25,26,27,28,33,44,45,48,50,51,56,61,64,65,66,68,75,77
};
static const uint8_t original_sound_fx_types[ORIGINAL_CHICAGO_FX_CHOICES] = {0,4,5,10,18,22,24,3,12,13,15,16};
static const uint8_t original_sound_fx_macros[ORIGINAL_CHICAGO_FX_CHOICES] = {0,5,7,4,2,3,1,6,8,9,10,11};
/* Drone factories are q127, unlike the binary32 legacy synth factories. */
static const x0x_param_t original_sound_juno_params[18] = {
 {"Wave",127,0,0},{"Sub",127,0,0},{"Cutoff",127,0,0},{"Res",127,0,0},
 {"EnvAmt",127,0,0},{"Attack",127,0,0},{"Decay",127,0,0},{"Sustain",127,0,0},
 {"Release",127,0,0},{"HPF",127,0,0},{"LFORate",127,0,0},{"LFODep",127,0,0},
 {"Width",127,0,0},{"ChoRate",127,0,0},{"Chorus",127,0,0},{"Level",127,0,0},
 {"PBend",127,0,0},{"Damper",127,0,0}
};
#include "juno_stock_bank.h"
_Static_assert(sizeof original_sound_juno_factories / sizeof original_sound_juno_factories[0] == ORIGINAL_JUNO_FACTORIES,
               "Juno factory count must match stored selector range");
_Static_assert(sizeof original_sound_juno_names / sizeof original_sound_juno_names[0] == ORIGINAL_JUNO_FACTORIES,
               "Juno factory names must match selector range");
static const x0x_param_t original_sound_drone_params[ORIGINAL_DRONE_NPARAMS] = {
    {"Register",127,57,0},{"Brightness",127,66,0},
    {"Flutter",127,51,0},{"Attack",127,23,0},
    {"Release",127,83,0},{"Level",127,95,0},
    {"PBend",127,64,0},{"Damper",127,0,0}
};
static const uint8_t original_sound_drone_factories[ORIGINAL_DRONE_FACTORIES][6] = {
    {57,66,51,23,83,95},   /* Cathedral */
    {35,39,38,58,108,105}, /* Cloud */
    {94,97,70,17,72,83},   /* Aurora */
    {73,75,12,5,55,94}     /* Organ */
};
static const char *const original_sound_drone_names[ORIGINAL_DRONE_FACTORIES] = {
    "Cathedral", "Cloud", "Aurora", "Organ"
};
static const x0x_param_t original_sound_miami_params[18] = {
    {"CrWave",127,0,0},{"CrLvl",127,0,0},{"Sub",127,0,0},{"CrPit",127,0,0},
    {"Glide",127,0,0},{"XmLvl",127,0,0},{"XmWave",127,0,0},{"XmTune",127,0,0},
    {"XmDep",127,0,0},{"Cutoff",127,0,0},{"Crush",127,0,0},{"WobShp",127,0,0},
    {"WobDep",127,0,0},{"WobRat",127,0,0},{"Output",127,0,0},{"Stereo",127,0,0},
    {"PBend",127,0,0},{"Damper",127,0,0}
};

/* Only these original bipolar panel controls need an exact editable midpoint.
 * The persisted byte still spans0..127; untouched factory floats bypass this
 * editor conversion. Keep the two rounded upper-half operations explicit. */
static int original_sound_centered(unsigned kind, unsigned parameter)
{
    /* Pitch bend is centred too: control 64 is exactly 0.5 (no bend), as the
     * saved rest value in original_sound_value. */
    return (kind == ORIGINAL_SOUND_CHICAGO && (parameter == 4u || parameter == ORIGINAL_CHICAGO_PITCH_BEND)) ||
           (kind == ORIGINAL_SOUND_MIAMI && (parameter == 7u || parameter == ORIGINAL_MIAMI_PITCH_BEND)) ||
           (kind == ORIGINAL_SOUND_JUNO && parameter == 16u) ||
           (kind == ORIGINAL_SOUND_DRONE && parameter == 6u);
}
static uint8_t original_sound_display_byte(unsigned kind, unsigned parameter, float value)
{
    float position = original_sound_centered(kind, parameter) ?
        (value < 0.5f ? 128.0f * value : 64.0f + 126.0f * (value - 0.5f)) :
        127.0f * value;
    return (uint8_t)(position + 0.5f);
}

unsigned original_sound_count(unsigned kind)
{ return kind == ORIGINAL_SOUND_CHICAGO ? 22u : kind == ORIGINAL_SOUND_MIAMI ? 18u :
    kind == ORIGINAL_SOUND_JUNO ? ORIGINAL_JUNO_NPARAMS : kind == ORIGINAL_SOUND_DRONE ? ORIGINAL_DRONE_NPARAMS : 0u; }
unsigned original_sound_factories(unsigned kind)
{ return kind == ORIGINAL_SOUND_CHICAGO ? ORIGINAL_CHICAGO_FACTORIES : kind == ORIGINAL_SOUND_MIAMI ? 30u :
    kind == ORIGINAL_SOUND_JUNO ? ORIGINAL_JUNO_FACTORIES : kind == ORIGINAL_SOUND_DRONE ? ORIGINAL_DRONE_FACTORIES : 0u; }
int original_sound_factory_source(unsigned kind, unsigned factory)
{
    if (factory >= original_sound_factories(kind)) return -1;
    return kind == ORIGINAL_SOUND_CHICAGO ? original_sound_chicago_factories[factory] : (int)factory;
}
const char *original_sound_fx_name(unsigned choice)
{ return choice < ORIGINAL_CHICAGO_FX_CHOICES ? original_sound_fx_names[choice] : 0; }
static const float original_sound_unit_value[128] = {
    0x0p+0f,     0x1.020408p-7f,     0x1.020408p-6f,     0x1.83060cp-6f,
    0x1.020408p-5f,     0x1.42850ap-5f,     0x1.83060cp-5f,     0x1.c3870ep-5f,
    0x1.020408p-4f,     0x1.22448ap-4f,     0x1.42850ap-4f,     0x1.62c58cp-4f,
    0x1.83060cp-4f,     0x1.a3468ep-4f,     0x1.c3870ep-4f,     0x1.e3c79p-4f,
    0x1.020408p-3f,     0x1.122448p-3f,     0x1.22448ap-3f,     0x1.3264cap-3f,
    0x1.42850ap-3f,     0x1.52a54ap-3f,     0x1.62c58cp-3f,     0x1.72e5ccp-3f,
    0x1.83060cp-3f,     0x1.93264cp-3f,     0x1.a3468ep-3f,     0x1.b366cep-3f,
    0x1.c3870ep-3f,     0x1.d3a74ep-3f,     0x1.e3c79p-3f,     0x1.f3e7dp-3f,
    0x1.020408p-2f,     0x1.0a1428p-2f,     0x1.122448p-2f,     0x1.1a3468p-2f,
    0x1.22448ap-2f,     0x1.2a54aap-2f,     0x1.3264cap-2f,     0x1.3a74eap-2f,
    0x1.42850ap-2f,     0x1.4a952ap-2f,     0x1.52a54ap-2f,     0x1.5ab56ap-2f,
    0x1.62c58cp-2f,     0x1.6ad5acp-2f,     0x1.72e5ccp-2f,     0x1.7af5ecp-2f,
    0x1.83060cp-2f,     0x1.8b162cp-2f,     0x1.93264cp-2f,     0x1.9b366cp-2f,
    0x1.a3468ep-2f,     0x1.ab56aep-2f,     0x1.b366cep-2f,     0x1.bb76eep-2f,
    0x1.c3870ep-2f,     0x1.cb972ep-2f,     0x1.d3a74ep-2f,     0x1.dbb76ep-2f,
    0x1.e3c79p-2f,     0x1.ebd7bp-2f,     0x1.f3e7dp-2f,     0x1.fbf7fp-2f,
    0x1.020408p-1f,     0x1.060c18p-1f,     0x1.0a1428p-1f,     0x1.0e1c38p-1f,
    0x1.122448p-1f,     0x1.162c58p-1f,     0x1.1a3468p-1f,     0x1.1e3c78p-1f,
    0x1.22448ap-1f,     0x1.264c9ap-1f,     0x1.2a54aap-1f,     0x1.2e5cbap-1f,
    0x1.3264cap-1f,     0x1.366cdap-1f,     0x1.3a74eap-1f,     0x1.3e7cfap-1f,
    0x1.42850ap-1f,     0x1.468d1ap-1f,     0x1.4a952ap-1f,     0x1.4e9d3ap-1f,
    0x1.52a54ap-1f,     0x1.56ad5ap-1f,     0x1.5ab56ap-1f,     0x1.5ebd7ap-1f,
    0x1.62c58cp-1f,     0x1.66cd9cp-1f,     0x1.6ad5acp-1f,     0x1.6eddbcp-1f,
    0x1.72e5ccp-1f,     0x1.76eddcp-1f,     0x1.7af5ecp-1f,     0x1.7efdfcp-1f,
    0x1.83060cp-1f,     0x1.870e1cp-1f,     0x1.8b162cp-1f,     0x1.8f1e3cp-1f,
    0x1.93264cp-1f,     0x1.972e5cp-1f,     0x1.9b366cp-1f,     0x1.9f3e7cp-1f,
    0x1.a3468ep-1f,     0x1.a74e9ep-1f,     0x1.ab56aep-1f,     0x1.af5ebep-1f,
    0x1.b366cep-1f,     0x1.b76edep-1f,     0x1.bb76eep-1f,     0x1.bf7efep-1f,
    0x1.c3870ep-1f,     0x1.c78f1ep-1f,     0x1.cb972ep-1f,     0x1.cf9f3ep-1f,
    0x1.d3a74ep-1f,     0x1.d7af5ep-1f,     0x1.dbb76ep-1f,     0x1.dfbf7ep-1f,
    0x1.e3c79p-1f,     0x1.e7cfap-1f,     0x1.ebd7bp-1f,     0x1.efdfcp-1f,
    0x1.f3e7dp-1f,     0x1.f7efep-1f,     0x1.fbf7fp-1f,     0x1p+0f,

};
static const float original_sound_upper_value[64] = {
    0x1p-1f,     0x1.041042p-1f,     0x1.082082p-1f,     0x1.0c30c4p-1f,
    0x1.104104p-1f,     0x1.145146p-1f,     0x1.186186p-1f,     0x1.1c71c8p-1f,
    0x1.208208p-1f,     0x1.24924ap-1f,     0x1.28a28ap-1f,     0x1.2cb2ccp-1f,
    0x1.30c30cp-1f,     0x1.34d34ep-1f,     0x1.38e38ep-1f,     0x1.3cf3dp-1f,
    0x1.41041p-1f,     0x1.451452p-1f,     0x1.492492p-1f,     0x1.4d34d4p-1f,
    0x1.514514p-1f,     0x1.555556p-1f,     0x1.596596p-1f,     0x1.5d75d8p-1f,
    0x1.618618p-1f,     0x1.65965ap-1f,     0x1.69a69ap-1f,     0x1.6db6dcp-1f,
    0x1.71c71cp-1f,     0x1.75d75ep-1f,     0x1.79e79ep-1f,     0x1.7df7ep-1f,
    0x1.82082p-1f,     0x1.861862p-1f,     0x1.8a28a4p-1f,     0x1.8e38e4p-1f,
    0x1.924924p-1f,     0x1.965966p-1f,     0x1.9a69a8p-1f,     0x1.9e79e8p-1f,
    0x1.a28a28p-1f,     0x1.a69a6ap-1f,     0x1.aaaaacp-1f,     0x1.aebaecp-1f,
    0x1.b2cb2cp-1f,     0x1.b6db6ep-1f,     0x1.baebbp-1f,     0x1.befbfp-1f,
    0x1.c30c3p-1f,     0x1.c71c72p-1f,     0x1.cb2cb4p-1f,     0x1.cf3cf4p-1f,
    0x1.d34d34p-1f,     0x1.d75d76p-1f,     0x1.db6db8p-1f,     0x1.df7df8p-1f,
    0x1.e38e38p-1f,     0x1.e79e7ap-1f,     0x1.ebaebcp-1f,     0x1.efbefcp-1f,
    0x1.f3cf3cp-1f,     0x1.f7df7ep-1f,     0x1.fbefcp-1f,     0x1p+0f,

};
static const float original_sound_macro_value[12] = {
    0x0p+0f,     0x1.745d18p-4f,     0x1.745d18p-3f,     0x1.1745d2p-2f,
    0x1.745d18p-2f,     0x1.d1745ep-2f,     0x1.1745d2p-1f,     0x1.45d174p-1f,
    0x1.745d18p-1f,     0x1.a2e8bap-1f,     0x1.d1745ep-1f,     0x1p+0f,

};
int original_sound_control_value(unsigned kind, unsigned parameter, unsigned control, float *value)
{
    const x0x_param_t *p = original_sound_param(kind, parameter);
    if (!value || !p || control > p->max) return 0;
    if (kind == ORIGINAL_SOUND_CHICAGO && parameter == ORIGINAL_CHICAGO_FX_TYPE)
        *value = original_sound_macro_value[original_sound_fx_macros[control]];
    else if (original_sound_centered(kind, parameter))
        *value = control < 64u ? (float)control / 128.0f :
            original_sound_upper_value[control - 64u];
    else *value = original_sound_unit_value[control];
    return 1;
}
uint32_t original_sound_engine_id(unsigned kind)
{ return kind == ORIGINAL_SOUND_CHICAGO ? ORIGINAL_SOUND_CHICAGO_ID : kind == ORIGINAL_SOUND_MIAMI ? ORIGINAL_SOUND_MIAMI_ID :
    kind == ORIGINAL_SOUND_JUNO ? ORIGINAL_SOUND_JUNO_ID : kind == ORIGINAL_SOUND_DRONE ? ORIGINAL_SOUND_DRONE_ID : 0u; }
const char *original_sound_name(unsigned kind, unsigned factory)
{
    int source = original_sound_factory_source(kind, factory);
    if (source < 0) return 0;
    return kind == ORIGINAL_SOUND_CHICAGO ? original_chicago_program_name((unsigned)source) :
    kind == ORIGINAL_SOUND_MIAMI ? original_miami_program_name((unsigned)source) :
    kind == ORIGINAL_SOUND_JUNO ? original_sound_juno_names[factory] : original_sound_drone_names[factory];
}
const x0x_param_t *original_sound_param(unsigned kind, unsigned parameter)
{
    if (parameter >= original_sound_count(kind)) return 0;
    return kind == ORIGINAL_SOUND_CHICAGO ? original_sound_chicago_params + parameter :
    kind == ORIGINAL_SOUND_MIAMI ? original_sound_miami_params + parameter : kind == ORIGINAL_SOUND_JUNO ? original_sound_juno_params + parameter : original_sound_drone_params + parameter;
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
        for (unsigned i = 0; i < 22; ++i) next[i] = original_sound_display_byte(kind, i, p.macro[i]);
        unsigned choice = 0;
        while (choice < ORIGINAL_CHICAGO_FX_CHOICES && original_sound_fx_types[choice] != routing.fx[1][0]) ++choice;
        if (choice == ORIGINAL_CHICAGO_FX_CHOICES) return 0;
        next[ORIGINAL_CHICAGO_FX_TYPE] = (uint8_t)choice;
    } else if (kind == ORIGINAL_SOUND_MIAMI) {
        original_miami_program p;
        if (!original_miami_program_init(&p, (unsigned)source)) return 0;
        for (unsigned i = 0; i < 18; ++i) next[i] = original_sound_display_byte(kind, i, p.macro[i]);
    } else if (kind == ORIGINAL_SOUND_JUNO) {
        memcpy(next, original_sound_juno_factories[factory], 16u);
    } else if (kind == ORIGINAL_SOUND_DRONE) {
        memcpy(next, original_sound_drone_factories[factory], 6u);
    } else return 0;
    next[26] = 'O'; next[27] = 'S'; next[28] = ORIGINAL_SOUND_SCHEMA;
    next[29] = (uint8_t)kind; next[30] = (uint8_t)factory;
    memcpy(cell, next, sizeof next);
    return 1;
}
int original_sound_set(uint8_t cell[ORIGINAL_SOUND_BYTES], unsigned parameter, unsigned value)
{
    if (!original_sound_valid(cell) || parameter >= original_sound_count(cell[29]) ||
        original_sound_is_performance(cell[29], parameter) ||
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
    for (unsigned i = 0; i < ORIGINAL_MIAMI_PITCH_BEND; ++i) if (mask & (UINT32_C(1) << i)) {
        float value;
        if (!original_sound_control_value(cell[29], i, cell[i], &value) ||
            !original_miami_program_set(&next, i, value)) return 0;
    }
    *out = next;
    return 1;
}
int original_sound_restore_chicago(original_chicago_program *out, const uint8_t cell[ORIGINAL_SOUND_BYTES])
{
    if (!out || !original_sound_valid(cell) || cell[29] != ORIGINAL_SOUND_CHICAGO) return 0;
    original_chicago_program next;
    if (!original_chicago_program_init(&next, (unsigned)original_sound_factory_source(cell[29], cell[30]))) return 0;
    uint32_t mask = original_sound_edit_mask(cell);
    for (unsigned i = 0; i < ORIGINAL_CHICAGO_PITCH_BEND; ++i) if (mask & (UINT32_C(1) << i)) {
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
    if (original_sound_is_performance(cell[29], parameter)) {
        *value = parameter == original_sound_count(cell[29]) - 2u ? 0.5f : 0.0f;
        return 1;
    }
    float result;
    if (original_sound_edit_mask(cell) & (UINT32_C(1) << parameter)) {
        if (!original_sound_control_value(cell[29], parameter, cell[parameter], &result)) return 0;
    }
    else if (cell[29] == ORIGINAL_SOUND_JUNO)
        result = original_sound_unit_value[original_sound_juno_factories[cell[30]][parameter]];
    else if (cell[29] == ORIGINAL_SOUND_DRONE)
        result = original_sound_unit_value[original_sound_drone_factories[cell[30]][parameter]];
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

