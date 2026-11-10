/* SPDX-License-Identifier: GPL-3.0-only */
/* Host check: host_midi_take_seq skips 0 when the 16-bit accept counter wraps,
 * then issues the next stamps one at a time.
 * A stamp of 0 is not a real accept (the engine treats it as a legacy packet).
 * panic-order still passes if that skip is removed, because the following take
 * and the panic sample both turn a stored 0 into 1 before a note is ordered.
 * Adding 2 instead of 1 still stores 1 after 0xFFFF, because 0xFFFF+2 wraps to 1.
 * The second take after that wrap must be 2, and later takes must step by 1.
 * Device and native builds are not this test. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stdio.h>

static int fails;

static void check_u16(const char *what, unsigned got, unsigned want)
{
    if (got != want || got == 0u) {
        fails++;
        fprintf(stderr, "FAIL %s: got %u want %u\n", what, got, want);
    }
}

static void check_step(const char *what, unsigned prev, unsigned got)
{
    unsigned want = (unsigned)(uint16_t)(prev + 1u);

    if (want == 0u)
        want = 1u;
    check_u16(what, got, want);
}

int main(void)
{
    uint16_t first, pending, second, third, prev, cur;
    unsigned i;

    /* Counter sits on the last legal stamp. The take returns it and the
     * next stamp must be 1, never 0. The following take must issue that 1.
     * The take after that is the second stamp after the wrap and must be 2. */
    mi_seq_next = 0xFFFFu;
    first = host_midi_take_seq();
    pending = mi_seq_next;
    second = host_midi_take_seq();
    third = host_midi_take_seq();
    check_u16("stamp at 0xFFFF", first, 0xFFFFu);
    check_u16("stamp stored after wrap", pending, 1u);
    check_u16("stamp taken after wrap", second, 1u);
    check_u16("second take after wrap", third, 2u);
    prev = third;
    for (i = 0; i < 4u; i++) {
        cur = host_midi_take_seq();
        check_step("consecutive take", prev, cur);
        prev = cur;
    }

    if (fails) {
        fprintf(stderr, "midi seq skip0: %d failures\n", fails);
        return 1;
    }
    printf("midi seq skip0: 0 failures\n");
    return 0;
}
