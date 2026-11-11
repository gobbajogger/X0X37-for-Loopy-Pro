/* SPDX-License-Identifier: GPL-3.0-only */
/* Legacy 303-part parameter IDs. The bass303 engine is removed; Chicago and
 * Miami ship. These IDs keep their old numbers because sound_t, project files
 * (sound rev 5-7), the UI pages and the Chicago tests index by them. */
#pragma once
#include <stdint.h>
#include "x0x_param.h"

#ifndef BASS303_SR
#define BASS303_SR 44100.0f
#endif
#define BASS303_MAX_BLOCK 256
#define BASS303_CTRL 4          /* control block divisor; the UI envelope preview uses it */

enum {
    BASS303_CUTOFF, BASS303_RESO, BASS303_ENVMOD, BASS303_DECAY,
    BASS303_ACCENT, BASS303_WAVE, BASS303_TUNE, BASS303_VOLUME,
    BASS303_DRIVE, BASS303_DRVTYPE, BASS303_SLIDE, BASS303_ACCDEC,
    BASS303_FATTACK, BASS303_FSUSTAIN, BASS303_FRELEASE, BASS303_FENV,
    BASS303_AMPMODE, BASS303_GNAW, BASS303_BITE,
    BASS303_AATTACK, BASS303_ADECAY, BASS303_ASUSTAIN, BASS303_ARELEASE,
    BASS303_NPARAMS
};
enum { BASS303_DRV_OFF, BASS303_DRV_SOFT, BASS303_DRV_RAT };
enum { BASS303_AMP_ENV, BASS303_AMP_GATE };
