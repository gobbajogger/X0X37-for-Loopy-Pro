/* SPDX-License-Identifier: GPL-3.0-only */
/* Included after undo.c. See sound_reset.h. */
#include "sound_reset.h"
#include <string.h>

#ifndef SOUND_RESET_MUTANT_CLEAR_PAN
#define SOUND_RESET_MUTANT_CLEAR_PAN 0
#endif

typedef char sound_reset_drum_plane[(NVOICES_MAX >= 12) ? 1 : -1];
typedef char sound_reset_bass_pair[(NBASS == 2) ? 1 : -1];

static int sound_reset_span(int part, int *target, int *voice, int *nvoice, int *tag)
{
    *voice = 0;
    *nvoice = 1;
    *tag = -1;
    switch (part) {
    case PART_909:
        *target = T_909;
        *nvoice = NVOICES_MAX;
        return 0;
    case PART_808:
        *target = T_808;
        *nvoice = NVOICES_MAX;
        return 0;
    case PART_303A:
        *target = T_303;
        *tag = 0;
        return 0;
    case PART_303B:
        *target = T_303;
        *voice = 1;
        *tag = 1;
        return 0;
    case PART_BRK:
        *target = T_BRK;
        return 0;
    default:
        return -1;
    }
}

static int sound_reset_empty(const sound_t *s, int target, int voice, int nvoice, int tag)
{
    int v, i;
    for (v = 0; v < nvoice; v++)
        for (i = 0; i < NPARAMS_MAX; i++)
            if (s->v[target][voice + v][i] != 0)
                return 0;
    if (tag >= 0)
        for (i = 0; i < (int)sizeof s->bass[tag].bytes; i++)
            if (s->bass[tag].bytes[i] != 0)
                return 0;
    return 1;
}

static void sound_reset_copy(sound_t *dst, const sound_t *src, int target, int voice, int nvoice, int tag)
{
    int v;
    for (v = 0; v < nvoice; v++)
        memcpy(dst->v[target][voice + v], src->v[target][voice + v], NPARAMS_MAX);
    if (tag >= 0)
        memcpy(dst->bass[tag].bytes, src->bass[tag].bytes, sizeof dst->bass[tag].bytes);
}

static int sound_reset_part(sound_t *sound, const sound_t *factory, int part,
                            uint8_t *mixer_mute, undo_t *undo)
{
    int target, voice, nvoice, tag, committed;
    if (!sound || !factory || !mixer_mute || !undo || factory == sound)
        return SOUND_RESET_BAD;
    if ((uint8_t *)sound != undo->region || undo->len < sizeof *sound)
        return SOUND_RESET_BAD;
    if (sound_reset_span(part, &target, &voice, &nvoice, &tag) != 0)
        return SOUND_RESET_BAD;
    if (sound_reset_empty(sound, target, voice, nvoice, tag))
        return SOUND_RESET_NO_SAVED;
    sound_reset_copy(sound, factory, target, voice, nvoice, tag);
#if SOUND_RESET_MUTANT_CLEAR_PAN
    sound->v[T_MIX][part][MX_PAN] = 0;
#endif
    committed = undo_commit(undo);
    if (committed < 0)
        return SOUND_RESET_BAD;
    return SOUND_RESET_OK;
}
