/* SPDX-License-Identifier: GPL-3.0-only
 * The inbound recorder (a76): the last SMK_INLOG_N APC40 LED messages that came in on USB or
 * Bluetooth, the Bluetooth link and notify edges, and decoded SMC input. The editor command 38 INLOG (read only)
 * returns it, and tools/ble_autotest.py --diag-only prints it. One writer context: usb.c and
 * ble.c run in the same timer interrupt. ble.c gives the link state after each event poll; only a
 * change of it goes into the ring. Each entry has 2 words: fm1_ms, and
 * src << 24 | status << 16 | d1 << 8 | d2. */
#pragma once
#include <stdint.h>

#define SMK_INLOG_N 64u       /* at most 127: INLOG sends the count as one 7-bit byte */
#define SMK_INLOG_CELLS 40u     /* the clip cells that INLOG returns (SMK_APC40_CLIP_COUNT) */
enum { SMK_INLOG_USB = 0, SMK_INLOG_BLE = 1, SMK_INLOG_EDGE = 2, SMK_INLOG_NOCELL = 3, SMK_INLOG_SMC = 4 };
/* SMK_INLOG_EDGE: d2 = the link state, bit 0 the link is up, bit 1 notify is on. */
typedef struct {
    uint32_t w[SMK_INLOG_N * 2u];
    uint8_t next;       /* the entry that the next message writes */
    uint8_t n;          /* the entries in use (at most SMK_INLOG_N) */
    uint8_t link;       /* the last link state given (SMK_INLOG_EDGE): only a change goes into the ring */
} smk_inlog_t;

/* The entry word of a USB-MIDI event packet (cin | status << 8 | d1 << 16 | d2 << 24). */
static inline uint32_t smk_inlog_word(uint32_t src, uint32_t pkt)
{
    return (src & 0xFFu) << 24 | ((pkt >> 8) & 0xFFu) << 16 | ((pkt >> 16) & 0x7Fu) << 8 | ((pkt >> 24) & 0x7Fu);
}

/* Keep note on and note off for the APC40 LED notes 0-98 (clip, scene, track and transport LEDs),
 * all edges, and SMC input (source 4, MMC command in d1). Other APC messages (clock, knob ring CCs, notes 99-127) do not go into the ring. */
static inline uint8_t smk_inlog_wanted(uint32_t src, uint32_t pkt)
{
    uint32_t st = (pkt >> 8) & 0xFFu;
    return (uint8_t)(src == SMK_INLOG_EDGE || src == SMK_INLOG_SMC || ((st & 0xE0u) == 0x80u && ((pkt >> 16) & 0x7Fu) <= 98u));
}

static inline void smk_inlog_put(smk_inlog_t *g, uint32_t ms, uint32_t src, uint32_t pkt)
{
    uint32_t i;
    if (!smk_inlog_wanted(src, pkt))
        return;
    if (src == SMK_INLOG_EDGE) {
        if ((uint8_t)(pkt >> 24) == g->link)
            return;
        g->link = (uint8_t)(pkt >> 24);
    }
    i = (uint32_t)g->next * 2u;
    g->w[i] = ms;
    g->w[i + 1u] = smk_inlog_word(src, pkt);
    g->next = (uint8_t)((g->next + 1u) % SMK_INLOG_N);
    if (g->n < SMK_INLOG_N)
        g->n++;
}

/* The index in w[] of the first word of the oldest entry. */
static inline uint32_t smk_inlog_at_first(const smk_inlog_t *g)
{
    return ((g->next + SMK_INLOG_N - g->n) % SMK_INLOG_N) * 2u;
}

/* Word i (0 .. 2 x n - 1) of the entries, the oldest entry first. */
static inline uint32_t smk_inlog_at(const smk_inlog_t *g, uint32_t i)
{
    return g->w[(smk_inlog_at_first(g) + i) % (SMK_INLOG_N * 2u)];
}

/* Copy the entries, the oldest first, to out (2 words each). Return the number of entries. */
static inline uint32_t smk_inlog_copy(const smk_inlog_t *g, uint32_t out[SMK_INLOG_N * 2u])
{
    uint32_t i;
    for (i = 0; i < 2u * g->n; i++)
        out[i] = smk_inlog_at(g, i);
    return g->n;
}
