/* SPDX-License-Identifier: GPL-3.0-only */
/* Pure factory-AU bytes -> complete engine-owned product cell. No DSP/storage writes. */
#pragma once
#include "engine.h"
#include "factory_banks.h"
#include "chicago_env.h"
#include "chicago_arp.h"
#include "chicago_fx.h"
#include "chicago_aux.h"
#include "../dsp/wobble_config.h"

typedef const x0x_param_t *(*bass_preset_param_fn)(int bank, int param);
enum { BASS_PRESET_OK = 0, BASS_PRESET_INVALID = -1, BASS_PRESET_UNAVAILABLE = -2 };

static inline int bass_preset_decode_record(int bank, const factory_preset_t *p,
                                            bass_preset_param_fn param, uint8_t out[NPARAMS_MAX])
{
    static const uint8_t au_to_chicago[13] = {5,10,0,1,2,18,17,19,20,21,22,16,7};
    uint8_t values[NPARAMS_MAX] = {0};
    int count, required, i, name_end = 0;
    if (!p || !param || !out || (bank != FACTORY_BANK_CHICAGO && bank != FACTORY_BANK_MIAMI))
        return BASS_PRESET_INVALID;
    count = bank == FACTORY_BANK_CHICAGO ? CHI_AUX_END : WOBBLE_EXPERIMENT_NPARAMS;
    required = bank == FACTORY_BANK_CHICAGO ? FACTORY_CHICAGO_NPARAMS : FACTORY_MIAMI_NPARAMS;
    if (p->nparams != required || !p->name[0]) return BASS_PRESET_INVALID;
    for (i = 0; i < FACTORY_NAME_MAX; i++) name_end |= !p->name[i];
    if (!name_end || count > NPARAMS_MAX) return BASS_PRESET_INVALID;
    if (!param(bank,0)) return BASS_PRESET_UNAVAILABLE;
    if (param(bank,count)) return BASS_PRESET_INVALID; /* wrong backend surface */
    for (i = 0; i < count; i++) {
        const x0x_param_t *d = param(bank,i);
        if (!d || !d->name || d->def > d->max) return BASS_PRESET_INVALID;
        values[i] = d->def;
    }
    if (bank == FACTORY_BANK_MIAMI) {
        for (i = 0; i < required; i++)
            values[i] = chi_env_pot_from_byte(p->pots[i], param(bank,i)->max);
    } else {
        for (i = 0; i < 13; i++) {
            unsigned target = au_to_chicago[i];
            values[target] = chi_env_pot_from_byte(p->pots[i],param(bank,(int)target)->max);
        }
        chi_env_apply_factory_voice(values,p->pots+7);
        chi_arp_apply_factory_voice(values,p->pots+13);
        uint8_t fx[3]; chi_fx_decode_factory(p->pots+17,fx);
        for (i = 0; i < 3; i++) values[CHI_FX_TYPE+i]=fx[i];
        chi_aux_apply_factory_voice(values,p->pots+20);
    }
    for (i = 0; i < count; i++)
        if (values[i] > param(bank,i)->max) return BASS_PRESET_INVALID;
    /* Invalid/unavailable input leaves the caller's original cell unchanged. */
    for (i = 0; i < NPARAMS_MAX; i++) out[i]=values[i];
    return BASS_PRESET_OK;
}
static inline int bass_preset_decode(int bank, int index, bass_preset_param_fn param,
                                     uint8_t out[NPARAMS_MAX])
{
    return bass_preset_decode_record(bank,factory_preset(bank,index),param,out);
}
