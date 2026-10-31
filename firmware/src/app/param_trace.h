/* SPDX-License-Identifier: GPL-3.0-only */
/* B4 host diagnostics: source-labelled synth write intent, not audio-ISR I/O.
 * Off by default; no arguments, strings or storage survive in target builds. */
#pragma once

enum {
    PARAM_WHEEL_PITCH, PARAM_WHEEL_MOD, PARAM_PANEL_KNOB, PARAM_SMC_KNOB,
    PARAM_SMC_PICKUP, PARAM_MOTION, PARAM_MOTION_RESTORE, PARAM_PRESET, PARAM_MIDI
};
#ifndef X0X_PARAM_TRACE
#define X0X_PARAM_TRACE 0
#endif
#if X0X_PARAM_TRACE
#ifndef X0X_HOST
#error X0X_PARAM_TRACE requires the host diagnostic sink
#endif
void x0x_param_trace(int source, int target, int voice, int parameter, int value);
#define PARAM_TRACE(source, target, voice, parameter, value) \
    x0x_param_trace(source, target, voice, parameter, value)
#else
#define PARAM_TRACE(source, target, voice, parameter, value) ((void)0)
#endif
