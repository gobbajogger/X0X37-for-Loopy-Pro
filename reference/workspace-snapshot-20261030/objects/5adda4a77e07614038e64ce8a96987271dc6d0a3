/* SPDX-License-Identifier: GPL-3.0-only */
/* MID-15 source-tag guard: the packet header has a 2-bit source field,
 * but only USB=0, BLE=1 and TRS=2 are defined (plat.h). The reserved value 3
 * must never claim external clock ownership or inject transport requests.
 * Also reject wider internal API inputs whose uint8_t source+1 would wrap and
 * collide with a legitimate owner. Non-clock MIDI (including Note Off) is unchanged.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"
static int failures, checks;
#define CHECK(x, ...) do { checks++; if (!(x)) { failures++; fprintf(stderr, "FAIL: "); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)
static seq_t s;
static void fresh(uint8_t mode)
{
    memset(&s, 0, sizeof s);
    s.clk_src = mode;
    s.ext_now = 50000u;
}
static int recv_rt(uint8_t status, uint32_t origin)
{
    int accepted = seq_clk_take(&s, status, origin);
    if (accepted)
        (void)seq_midi_rt(&s, status);
    return accepted;
}
int main(void)
{
    const uint32_t bad[] = {3u, 255u, 256u, UINT32_MAX};
    const uint8_t statuses[] = {0xF8u, 0xFAu, 0xFBu, 0xFCu, 0xF2u};
    const uint8_t modes[] = {SEQ_CLK_AUTO, SEQ_CLK_EXT};
    unsigned m, i, j;
    for (m = 0; m < sizeof modes / sizeof modes[0]; ++m) {
        for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
            for (j = 0; j < sizeof statuses / sizeof statuses[0]; ++j) {
                fresh(modes[m]);
                CHECK(!recv_rt(statuses[j], bad[i]),
                      "mode %u reserved input %u accepted MIDI 0x%02x", modes[m], bad[i], statuses[j]);
                CHECK(!s.clk_lock && !s.clk_last && !s.ext_pending && !s.req_start &&
                      !s.req_cont && !s.req_stop && !s.clk_stop,
                      "mode %u reserved input %u mutated clock or transport state", modes[m], bad[i]);
            }
        }
        for (i = 0; i < 3u; ++i) {
            fresh(modes[m]);
            CHECK(recv_rt(0xF8u, i) && s.clk_lock == i + 1u && s.ext_pending == 1,
                  "mode %u legitimate origin %u failed to own clock", modes[m], i);
            CHECK(recv_rt(0xFCu, i) && s.clk_stop && s.req_stop,
                  "mode %u legitimate origin %u STOP lost", modes[m], i);
            CHECK(recv_rt(0xFAu, i) && !s.clk_stop && s.req_start,
                  "mode %u legitimate origin %u START lost", modes[m], i);
            CHECK(recv_rt(0xF2u, i) && s.clk_lock == i + 1u,
                  "mode %u legitimate origin %u Song Position lost", modes[m], i);
            for (j = 0; j < sizeof bad / sizeof bad[0]; ++j) {
                uint32_t ticks = s.ext_pending;
                uint32_t previous = s.clk_lock_at;
                CHECK(!recv_rt(0xF8u, bad[j]) && s.ext_pending == ticks &&
                      s.clk_lock == i + 1u && s.clk_lock_at == previous,
                      "mode %u bogus origin %u injected a tick with owner %u", modes[m], bad[j], i);
            }
        }
        fresh(modes[m]);
        CHECK(!recv_rt(0xF8u, 3u), "mode %u reserved packet tag seized an idle clock", modes[m]);
        CHECK(recv_rt(0xF8u, 0u) && s.clk_lock == 1u && s.ext_pending == 1u,
              "mode %u USB clock blocked after rejected tag", modes[m]);
        CHECK(seq_clk_take(&s, 0x80u, 3u) && s.clk_lock == 1u,
              "mode %u Note-Off passthrough changed", modes[m]);
        CHECK(seq_clk_take(&s, 0xFEu, 3u) && s.clk_lock == 1u,
              "mode %u nontransport Active Sensing passthrough changed", modes[m]);
    }
    fresh(SEQ_CLK_INT);
    s.clk_lock = 2u;
    CHECK(recv_rt(0xF8u, 0u) && !s.clk_lock && !s.ext_pending,
          "INTERNAL mode must preserve accepted-but-ignored valid MIDI clock");
    printf("clock source-tag regression: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
