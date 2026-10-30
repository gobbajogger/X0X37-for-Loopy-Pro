/* SPDX-License-Identifier: GPL-3.0-only */
/* Host extractors for smc_mixer.c: momentary SELECT FX calls sit in
 * smc_poll / smc_release / smc_project_reset; smc_knob toggles accel_smc around
 * turn_ref. Queue, Solo-overflow, link, BLE-reconnect-epoch and clip-visibility
 * harnesses do not exercise that stack — stub the entry points (and the panel
 * accel flag) so the real queue/link/clip regressions can compile and run. */
#pragma once
#include "app/smc_fx_map.h"
#include "app/smc_fx_map_tables.c" /* one map copy for single-file smc_mixer harnesses */

/* Solo / BLE-reconnect extractors omit smc_knob; queue/link/buttons still write it. */
static uint8_t __attribute__((unused)) accel_smc; /* production lives in ui.c */
static void __attribute__((unused)) smc_fx_main_tick(void) {}
static void __attribute__((unused)) smc_fx_clear(void) {}
static void __attribute__((unused)) smc_fx_release_note(uint8_t note) { (void)note; }
static void __attribute__((unused)) smc_fx_load_page(int page) { (void)page; }

/* These source-extracted queue fixtures do not exercise the lock gesture.
 * The production integration smc-lock-beat test owns its timer/loss coverage. */
static void __attribute__((unused)) smc_lock_tick(void) {}

/* smc_state_t sizes its lock/page arrays with ui.c and engine.h counts. The
 * single-file extractors do not include ui.c, so give the same values here. */
#ifndef MAXPAGES
#define MAXPAGES 14 /* ui.c */
#endif
#ifndef NVIEWS
#define NVIEWS 9 /* ui.c view enum */
#endif
#ifndef NPARTS
#define NPARTS 5 /* engine.h part enum */
#endif
