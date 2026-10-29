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

/* UI ownership is covered by the production mixer fixture. Queue extractors
 * include release/poll but deliberately omit framebuffer and platform time. */
static struct { uint8_t on, peer; uint32_t until; } __attribute__((unused)) smc_ui;
#define smc_held(state) 0
#define plat_ms() 0u
#define SMC_UI_MS 1200u
static void __attribute__((unused)) smc_ui_cancel(void) { smc_ui.on = 0; }
static void __attribute__((unused)) smc_mode_cleanup(void) {} /* tested in mixer_ux_test */
