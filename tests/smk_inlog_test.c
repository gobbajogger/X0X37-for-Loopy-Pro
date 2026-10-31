/* SPDX-License-Identifier: GPL-3.0-only */
/* The inbound recorder (a76, app/smk_inlog.h, editor command 38 INLOG): the word encoding,
 * the filter (APC40 LED notes 0-98 and the Bluetooth edges only), the ring wrap and the copy
 * oldest first.
 *   cc -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src tests/smk_inlog_test.c */
#include <assert.h>
#include <stdio.h>
#include "app/smk_inlog.h"

static uint32_t pkt(uint32_t st, uint32_t d1, uint32_t d2) { return (st >> 4) | st << 8 | d1 << 16 | d2 << 24; }

int main(void)
{
    static smk_inlog_t g;
    uint32_t w[SMK_INLOG_N * 2u], i, c;

    /* the size: 32 entries (2026-10-31 RAM trim; 64 from 2026-10-16, before 16). The count goes in one 7-bit byte. */
    assert(SMK_INLOG_N == 32u && SMK_INLOG_N <= 127u && sizeof g.w == 32u * 8u);
    printf("PASS ring size %u entries\n", SMK_INLOG_N);

    /* encoding: src<<24 | status<<16 | d1<<8 | d2 */
    assert(smk_inlog_word(SMK_INLOG_BLE, pkt(0x90, 0x20, 0x45)) == 0x01902045u);
    assert(smk_inlog_word(SMK_INLOG_USB, pkt(0x8E, 0x21, 0x7F)) == 0x008E217Fu);
    assert(smk_inlog_word(SMK_INLOG_NOCELL, pkt(0x9F, 0x27, 0x50)) == 0x039F2750u);
    assert(smk_inlog_word(SMK_INLOG_EDGE, 3u << 24) == 0x02000003u);
    printf("PASS encoding\n");

    /* filter: note on/off for notes 0-98 and the edges only */
    assert(smk_inlog_wanted(SMK_INLOG_BLE, pkt(0x90, 0, 1)));
    assert(smk_inlog_wanted(SMK_INLOG_USB, pkt(0x8F, 98, 0)));
    assert(!smk_inlog_wanted(SMK_INLOG_BLE, pkt(0x90, 99, 1)));
    assert(!smk_inlog_wanted(SMK_INLOG_BLE, pkt(0xB0, 0x10, 5)));   /* knob ring CC */
    assert(!smk_inlog_wanted(SMK_INLOG_USB, pkt(0xF8, 0, 0) & ~0xFu));
    assert(smk_inlog_wanted(SMK_INLOG_EDGE, 0u));
    assert(smk_inlog_wanted(SMK_INLOG_NOCELL, pkt(0x90, 0x20, 0x31)));
    smk_inlog_put(&g, 5u, SMK_INLOG_BLE, pkt(0xB0, 0x10, 5));
    assert(g.n == 0u && g.next == 0u && smk_inlog_copy(&g, w) == 0u);
    assert(smk_inlog_wanted(SMK_INLOG_SMC, pkt(0xE0, 0, 100)));
    assert(smk_inlog_wanted(SMK_INLOG_SMC, pkt(0xBF, 40, 100)));
    assert(smk_inlog_word(SMK_INLOG_SMC, pkt(0xBF, 40, 100)) == 0x04BF2864u);
    printf("PASS filter\n");

    /* an edge goes in only when the link state changes */
    smk_inlog_put(&g, 50u, SMK_INLOG_EDGE, 0u << 24);
    assert(g.n == 0u);
    printf("PASS edge: no change, no entry\n");

    /* fewer than N: all, oldest first */
    smk_inlog_put(&g, 100u, SMK_INLOG_EDGE, 3u << 24);
    smk_inlog_put(&g, 100u, SMK_INLOG_EDGE, 3u << 24);   /* the same state again: no entry */
    smk_inlog_put(&g, 101u, SMK_INLOG_BLE, pkt(0x90, 0x20, 0x45));
    c = smk_inlog_copy(&g, w);
    assert(c == 2u && w[0] == 100u && w[1] == 0x02000003u && w[2] == 101u && w[3] == 0x01902045u);
    printf("PASS fewer than %u entries, oldest first\n", SMK_INLOG_N);

    /* full: 2 + 62 messages fill the ring exactly; nothing is lost yet */
    for (i = 0; i < SMK_INLOG_N - 2u; i++)
        smk_inlog_put(&g, 200u + i, SMK_INLOG_USB, pkt(0x90, i, 1u + i));
    c = smk_inlog_copy(&g, w);
    assert(c == SMK_INLOG_N && g.next == 0u && w[0] == 100u && w[4] == 200u);
    assert(w[2u * SMK_INLOG_N - 2u] == 200u + SMK_INLOG_N - 3u);
    printf("PASS full ring: %u entries, the first one stays\n", SMK_INLOG_N);

    /* wrap: 2 + 90 messages, only the last N stay, oldest first */
    for (i = SMK_INLOG_N - 2u; i < 90u; i++)
        smk_inlog_put(&g, 200u + i, SMK_INLOG_USB, pkt(0x90, i, 1u + i));
    c = smk_inlog_copy(&g, w);
    assert(c == SMK_INLOG_N);
    for (i = 0; i < SMK_INLOG_N; i++) {
        uint32_t k = 90u - SMK_INLOG_N + i;
        assert(w[2u * i] == 200u + k);
        assert(w[2u * i + 1u] == (0x00900000u | k << 8 | (1u + k)));
    }
    printf("PASS ring wrap keeps the last %u entries, oldest first\n", SMK_INLOG_N);

    /* many wraps (300 more messages): the order stays right */
    for (i = 0; i < 300u; i++)
        smk_inlog_put(&g, 1000u + i, SMK_INLOG_BLE, pkt(0x90, 1, i & 0x7Fu));
    c = smk_inlog_copy(&g, w);
    assert(c == SMK_INLOG_N && g.n == SMK_INLOG_N);
    for (i = 0; i < SMK_INLOG_N; i++)
        assert(w[2u * i] == 1000u + 300u - SMK_INLOG_N + i);
    printf("PASS many wraps\n");
    return 0;
}
