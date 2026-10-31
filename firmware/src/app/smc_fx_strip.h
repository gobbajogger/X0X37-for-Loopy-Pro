/* SPDX-License-Identifier: GPL-3.0-only */
/* Two FX pages, four independent momentary button assignments per SMC strip.
 * The physical note's button is REC, SOLO, MUTE, SELECT in groups of eight. */
#pragma once
#include "smc_fx_map.h"

enum { SMCFX_BUTTONS = 4, SMCFX_STRIP_PAGES = 2, SMCFX_EMPTY = 0xFF };
enum { SMCFX_REC = 0, SMCFX_SOLO = 1, SMCFX_MUTE = 2, SMCFX_SELECT = 3 };

typedef struct {
    uint8_t map[SMCFX_STRIP_PAGES][SMCFX_N][SMCFX_BUTTONS];
    uint8_t key_bus[32]; /* Original bus, even if the keyboard reorders strips. */
    uint8_t held_fx[SMCFX_N][SMCFX_BUTTONS];
    uint8_t order[SMCFX_N][SMCFX_BUTTONS]; /* Latest held key wins on a bus. */
    uint8_t count[SMCFX_N];
} smcfx_strip_t;

static inline int smcfx_button(unsigned note)
{
    return note < 32u ? (int)(note >> 3) : -1;
}

static inline uint8_t smcfx_strip_get(const smcfx_strip_t *s, int page, int bus, int button)
{
    if ((unsigned)page >= SMCFX_STRIP_PAGES || (unsigned)bus >= SMCFX_N ||
        (unsigned)button >= SMCFX_BUTTONS) return SMCFX_EMPTY;
    return s->map[page][bus][button];
}

static inline void smcfx_strip_init(smcfx_strip_t *s)
{
    for (int page = 0; page < SMCFX_STRIP_PAGES; page++)
        for (int bus = 0; bus < SMCFX_N; bus++)
            for (int button = 0; button < SMCFX_BUTTONS; button++) {
                int slot = page * SMCFX_BUTTONS + button;
                s->map[page][bus][button] = slot < SMCFX_SUIT_N[bus] ?
                    SMCFX_SUIT[bus][slot] : SMCFX_EMPTY;
            }
    for (int note = 0; note < 32; note++) s->key_bus[note] = SMCFX_EMPTY;
    for (int bus = 0; bus < SMCFX_N; bus++) s->count[bus] = 0;
}

static inline uint8_t smcfx_strip_cycle(smcfx_strip_t *s, int page, int bus, int button)
{
    uint8_t cur = smcfx_strip_get(s, page, bus, button);
    if (cur == SMCFX_EMPTY && (unsigned)bus >= SMCFX_N) return cur;
    uint8_t next = smcfx_cycle(bus, cur);
    if ((unsigned)page < SMCFX_STRIP_PAGES && (unsigned)bus < SMCFX_N &&
        (unsigned)button < SMCFX_BUTTONS) s->map[page][bus][button] = next;
    return next;
}

static inline int smcfx_strip_press(smcfx_strip_t *s, unsigned note, int bus, int page)
{
    int button = smcfx_button(note);
    uint8_t fx = smcfx_strip_get(s, page, bus, button);
    if (button < 0 || (unsigned)bus >= SMCFX_N || fx == SMCFX_EMPTY ||
        s->key_bus[note] != SMCFX_EMPTY || s->count[bus] >= SMCFX_BUTTONS)
        return 0;
    s->key_bus[note] = (uint8_t)bus;
    s->held_fx[bus][button] = fx; /* Pin the effect selected at key-down. */
    s->order[bus][s->count[bus]++] = (uint8_t)button;
    return 1;
}

/* Returns original bus, or -1 if no FX press owned this key. */
static inline int smcfx_strip_release(smcfx_strip_t *s, unsigned note)
{
    if (note >= 32u || s->key_bus[note] >= SMCFX_N) return -1;
    int bus = s->key_bus[note], button = smcfx_button(note);
    s->key_bus[note] = SMCFX_EMPTY;
    for (int i = 0; i < s->count[bus]; i++)
        if (s->order[bus][i] == button) {
            for (int j = i + 1; j < s->count[bus]; j++)
                s->order[bus][j - 1] = s->order[bus][j];
            s->count[bus]--;
            break;
        }
    return bus;
}

static inline uint8_t smcfx_strip_live_fx(const smcfx_strip_t *s, int bus)
{
    if ((unsigned)bus >= SMCFX_N || !s->count[bus]) return SMCFX_EMPTY;
    return s->held_fx[bus][s->order[bus][s->count[bus] - 1u]];
}

static inline void smcfx_strip_clear_holds(smcfx_strip_t *s)
{
    for (int note = 0; note < 32; note++) s->key_bus[note] = SMCFX_EMPTY;
    for (int bus = 0; bus < SMCFX_N; bus++) s->count[bus] = 0;
}
