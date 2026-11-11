#!/usr/bin/env python3
"""Replace the SMC page-peek block (#737) in an extracted smc_mixer.c prefix.

Input-path harnesses copy smc_mixer.c from '#define SMC_Q' onwards. Since #737
that range holds the page peek, which needs the full ui struct (ui.page,
V_PART/V_SEQ/V_GEN). These harnesses test input, not the peek, so the block
becomes an inactive peek: smc_peek->on stays 0 and the peek calls do nothing.
The peek itself is tested by smc_ui_popup_test.c.

As a filter: python3 tests/host/smc_peek_stub.py < in.inc > out.inc
"""
import sys

START = "#define SMC_PEEK_MS"
END = "static void smc_peek_locked_page(void);"
OFF_DECL = "static void smc_ui_off(void);"
STUBS = r'''
/* Test stand-in for the SMC page peek (see tests/host/smc_peek_stub.py). */
static struct { uint8_t on, peer, saved_view; } smc_peek_stub;
#define smc_peek (&smc_peek_stub)
static __attribute__((unused)) void smc_peek_cancel(void) { smc_peek_stub.on = 0; }
static __attribute__((unused)) void smc_peek_tick(void) {}
static __attribute__((unused)) void smc_peek_locked_page(void) {}
static __attribute__((unused)) void smc_peek_begin(int n) { (void)n; }
'''


def no_display(text: str) -> str:
    # Isolated input tests omit display: drop the overlay forward declarations
    # (-Werror=unused-function) and give smc_ui_cancel() an empty body.
    text = text.replace("static void smc_ui_begin(int n, int kind);", "")
    text = text.replace(OFF_DECL, "")
    return text + "\n#ifndef smc_ui_cancel\nstatic __attribute__((unused)) void smc_ui_cancel(void) {}\n#endif\n"


def strip(text: str) -> str:
    a = text.find(START)
    if a < 0:
        return no_display(text)   # wiring-only source has no page peek
    b = text.index(END, a) + len(END)
    return no_display(text[:a] + STUBS + text[b:])


if __name__ == "__main__":
    sys.stdout.write(strip(sys.stdin.read()))
