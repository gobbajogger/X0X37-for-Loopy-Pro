/* SPDX-License-Identifier: GPL-3.0-only
 * The SMK-37 "<" and ">" buttons: the route and the hold repeat. Pure functions, no hardware access.
 * main_fm1.c (smk_arrows) and the host simulator use this file. tests/smk_arrows_test.c tests it. */
#pragma once
#include <stdint.h>

#ifndef ARW_FN
#ifndef X0X_MINSIZE
#if defined(__clang__)
#define X0X_MINSIZE minsize,   /* clang (the firmware toolchain): small code */
#else
#define X0X_MINSIZE            /* gcc host tests: gcc has no minsize */
#endif
#endif
#define ARW_FN __attribute__((X0X_MINSIZE noinline, unused))   /* small code: the app area is almost full */
#endif

/* Hold repeat in a menu: the first repeat 400 ms after the press, then one step each 80 ms.
 * The frame tick is 16 ms or more, so a repeat comes on the first frame at or after its time.
 * The repeat keeps the 80 ms grid. A frame that comes more than 80 ms late gives one step only, not a burst. */
#define ARW_FIRST_MS 400u
#define ARW_NEXT_MS 80u

/* where the arrows go: nothing, SELECT (one detent for each press), or the menu value (with repeat).
 * ARW_NONE in clip mode with no menu: the clip keys page takes the arrows there (ck_controls, CLP-76). */
enum { ARW_NONE, ARW_SELECT, ARW_MENU };

/* menu: a menu list that the main code owns is open (GLO or a page list, also in clip mode).
 * clip: clip mode is on. home: HOME is held (SELECT is the tempo). */
static inline uint32_t arw_route(uint32_t menu, uint32_t clip, uint32_t home)
{
    return home ? ARW_NONE : menu ? ARW_MENU : clip ? ARW_NONE : ARW_SELECT;
}

typedef struct {
    uint8_t prev;         /* the arrows at the last frame: bit 0 "<", bit 1 ">" */
    uint8_t rep;          /* the arrow that repeats (1 or 2), 0 none */
    uint16_t due;         /* the time of the next repeat (ms, the low 16 bits) */
} arw_t;

/* One frame. now: the arrows that are down (bit 0 "<", bit 1 ">"). Return the steps: -1, 0 or +1
 * (ARW_SELECT: a press of each arrow, as before; both at once give 0). Only ARW_MENU repeats.
 * A press of the other arrow while one is held stops the repeat. */
ARW_FN static int32_t arw_frame(arw_t *a, uint32_t now, uint32_t route, uint32_t ms)
{
    uint32_t pr = now & ~(uint32_t)a->prev;
    int32_t d = (int32_t)(pr >> 1 & 1u) - (int32_t)(pr & 1u);
    a->prev = (uint8_t)now;
    if (route != ARW_MENU || now != a->rep)
        a->rep = 0;
    if (route == ARW_NONE)
        return 0;
    if (route == ARW_MENU) {
        if (pr && (now == 1u || now == 2u)) {
            a->rep = (uint8_t)now;
            a->due = (uint16_t)(ms + ARW_FIRST_MS);
        } else if (a->rep && (int16_t)(uint16_t)(ms - a->due) >= 0) {
            a->due = (uint16_t)(a->due + ARW_NEXT_MS);
            if ((int16_t)(uint16_t)(ms - a->due) >= 0)
                a->due = (uint16_t)(ms + ARW_NEXT_MS);
            d = a->rep == 2u ? 1 : -1;
        }
    }
    return d;
}

/* Clip mode (CLP-82): the open menu list changed (another view or page) while the route stayed ARW_MENU.
 * Stop the repeat. An arrow that was held already in the old list stays "pressed": it does not edit the
 * new list. A fresh press in the same frame (now & ~prev) is kept and steps the new list. */
static inline void arw_rearm(arw_t *a, uint32_t now)
{
    a->rep = 0;
    a->prev = (uint8_t)(a->prev & now);
}
/* The identity of an open menu list for arw_rearm: the view and its page. */
static inline uint32_t arw_menu_id(uint32_t view, uint32_t page) { return view << 8 | (page & 0xFFu); }
