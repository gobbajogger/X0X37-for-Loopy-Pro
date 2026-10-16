/* SPDX-License-Identifier: GPL-3.0-only
 * Clip colours of a velocity v from Loopy (APC40 mkII palette), for the LCD clip squares.
 * Include app/apc40_lcd565.h and app/apc40_lcd_idle.h first.
 * Idle shade: Loopy draws a clip that does not play in the darkest shade of an Akai group of 4
 * (v = 7, 11, ... 59, v % 4 == 3; the full shade is v - 2). Loopy also draws an empty slot in this
 * shade (BEHAVIOR.md LIM-03), so the lift also applies to empty slots. SMK_CLIP_IDLE_LIFT 0 turns
 * the lift off (the plain Akai colour, as before 2026-10-17). */
#pragma once
#include <stdint.h>

#ifndef SMK_CLIP_IDLE_LIFT
#define SMK_CLIP_IDLE_LIFT 1
#endif

static inline uint8_t smk_clip_idle_shade(uint32_t v)
{
    return (uint8_t)(SMK_CLIP_IDLE_LIFT && v >= 7u && v <= 59u && (v & 3u) == 3u);
}
/* CLP-05: the LCD colour (RGB565). Idle shade: half of the full shade (APC40_LCD_IDLE565). */
static inline uint16_t smk_clip_lcd565(uint32_t v)
{
    v &= 0x7Fu;
    return smk_clip_idle_shade(v) ? APC40_LCD_IDLE565[(v - 7u) >> 2] : APC40_LCD565[v];
}
