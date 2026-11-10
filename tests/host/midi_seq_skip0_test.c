/* SPDX-License-Identifier: GPL-3.0-only */
/* Host check: host_midi_take_seq skips 0 when the 16-bit accept counter wraps.
 * A stamp of 0 is not a real accept (the engine treats it as a legacy packet).
 * panic-order still passes if that skip is removed, because the following take
 * and the panic sample both turn a stored 0 into 1 before a note is ordered.
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

int main(void)
{
    uint16_t first, pending, second;

    /* Counter sits on the last legal stamp. The take returns it and the
     * next stamp must be 1, never 0. The following take must issue that 1. */
    mi_seq_next = 0xFFFFu;
    first = host_midi_take_seq();
    pending = mi_seq_next;
    second = host_midi_take_seq();
    check_u16("stamp at 0xFFFF", first, 0xFFFFu);
    check_u16("stamp stored after wrap", pending, 1u);
    check_u16("stamp taken after wrap", second, 1u);

    if (fails) {
        fprintf(stderr, "midi seq skip0: %d failures\n", fails);
        return 1;
    }
    printf("midi seq skip0: 0 failures\n");
    return 0;
}
