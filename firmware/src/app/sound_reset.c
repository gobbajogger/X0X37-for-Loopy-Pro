/* SPDX-License-Identifier: GPL-3.0-only */
/* Included after undo.c. See sound_reset.h. */
#include "sound_reset.h"
#include <string.h>

#ifndef SOUND_RESET_MUTANT_CLEAR_PAN
#define SOUND_RESET_MUTANT_CLEAR_PAN 0
#endif
#ifndef SOUND_RESET_MUTANT_IGNORE_BASS_TAG
#define SOUND_RESET_MUTANT_IGNORE_BASS_TAG 0
#endif
#ifndef SOUND_RESET_MUTANT_IGNORE_LAST_PARAM
#define SOUND_RESET_MUTANT_IGNORE_LAST_PARAM 0
#endif
#ifndef SOUND_RESET_MUTANT_SKIP_RESTORE
#define SOUND_RESET_MUTANT_SKIP_RESTORE 0
#endif

typedef char sound_reset_drum_plane[(NVOICES_MAX >= 12) ? 1 : -1];
typedef char sound_reset_bass_pair[(NBASS == 2) ? 1 : -1];

/* One part: every voice cell, plus one 303 tag. Stack only, 396 bytes today. */
#define SOUND_RESET_PRE_MAX (NVOICES_MAX * NPARAMS_MAX + BASS_SOUND_TAG_BYTES)

static int sound_reset_span(int part, int *target, int *voice, int *nvoice, int *tag)
{
    *target = 0;
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
        for (i = 0; i < NPARAMS_MAX; i++) {
#if SOUND_RESET_MUTANT_IGNORE_LAST_PARAM
            if (i == NPARAMS_MAX - 1)
                continue;
#endif
            if (s->v[target][voice + v][i] != 0)
                return 0;
        }
#if SOUND_RESET_MUTANT_IGNORE_BASS_TAG
    (void)tag;
#else
    if (tag >= 0)
        for (i = 0; i < (int)sizeof s->bass[tag].bytes; i++)
            if (s->bass[tag].bytes[i] != 0)
                return 0;
#endif
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

static void sound_reset_keep(uint8_t *pre, const sound_t *s, int target, int voice, int nvoice, int tag)
{
    int v;
    for (v = 0; v < nvoice; v++)
        memcpy(pre + (size_t)v * NPARAMS_MAX, s->v[target][voice + v], NPARAMS_MAX);
    if (tag >= 0)
        memcpy(pre + (size_t)nvoice * NPARAMS_MAX, s->bass[tag].bytes, sizeof s->bass[tag].bytes);
}

/* Put the preimage back into the live part and the undo baseline. */
static void sound_reset_restore(sound_t *s, uint8_t *shadow, const uint8_t *pre,
                                int target, int voice, int nvoice, int tag)
{
#if SOUND_RESET_MUTANT_SKIP_RESTORE
    (void)s;
    (void)shadow;
    (void)pre;
    (void)target;
    (void)voice;
    (void)nvoice;
    (void)tag;
#else
    int v;
    for (v = 0; v < nvoice; v++) {
        size_t off = (size_t)((const uint8_t *)s->v[target][voice + v] - (const uint8_t *)s);
        memcpy(s->v[target][voice + v], pre + (size_t)v * NPARAMS_MAX, NPARAMS_MAX);
        memcpy(shadow + off, pre + (size_t)v * NPARAMS_MAX, NPARAMS_MAX);
    }
    if (tag >= 0) {
        size_t off = (size_t)(s->bass[tag].bytes - (const uint8_t *)s);
        size_t n = sizeof s->bass[tag].bytes;
        memcpy(s->bass[tag].bytes, pre + (size_t)nvoice * NPARAMS_MAX, n);
        memcpy(shadow + off, pre + (size_t)nvoice * NPARAMS_MAX, n);
    }
#endif
}

static int sound_reset_part(sound_t *sound, const sound_t *factory, int part,
                            uint8_t *mixer_mute, undo_t *undo)
{
    int target, voice, nvoice, tag, committed;
    uint8_t pre[SOUND_RESET_PRE_MAX];
    if (!sound || !factory || !mixer_mute || !undo || factory == sound)
        return SOUND_RESET_BAD;
    if ((uint8_t *)sound != undo->region || undo->len < sizeof *sound)
        return SOUND_RESET_BAD;
    if (sound_reset_span(part, &target, &voice, &nvoice, &tag) != 0)
        return SOUND_RESET_BAD;
    if (sound_reset_empty(sound, target, voice, nvoice, tag))
        return SOUND_RESET_NO_SAVED;
    sound_reset_keep(pre, sound, target, voice, nvoice, tag);
    sound_reset_copy(sound, factory, target, voice, nvoice, tag);
#if SOUND_RESET_MUTANT_CLEAR_PAN
    sound->v[T_MIX][part][MX_PAN] = 0;
#endif
    committed = undo_commit(undo);
    if (committed < 0) {
        /* undo.c has cleared the pool and snapped the region. Put this part back. */
        sound_reset_restore(sound, undo->shadow, pre, target, voice, nvoice, tag);
        return SOUND_RESET_BAD;
    }
    return SOUND_RESET_OK;
}
