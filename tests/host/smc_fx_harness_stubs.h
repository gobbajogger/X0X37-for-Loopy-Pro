/* SPDX-License-Identifier: GPL-3.0-only */
/* Host extractors for smc_mixer.c: momentary SELECT FX calls sit in
 * smc_poll / smc_release / smc_project_reset; smc_knob toggles accel_smc around
 * turn_ref. Queue, Solo-overflow, link, BLE-reconnect-epoch and clip-visibility
 * harnesses do not exercise that stack — stub the entry points (and the panel
 * accel flag) so the real queue/link/clip regressions can compile and run. */
#pragma once
#include "app/smc_fx_map.h"
#include "app/smc_fx_map_tables.c" /* one map copy for single-file smc_mixer harnesses */

static uint8_t accel_smc; /* panel accel flag; production lives in ui.c */
static void __attribute__((unused)) smc_fx_main_tick(void) {}
static void __attribute__((unused)) smc_fx_clear(void) {}
static void __attribute__((unused)) smc_fx_release_note(uint8_t note) { (void)note; }
static void __attribute__((unused)) smc_fx_load_page(int page) { (void)page; }
