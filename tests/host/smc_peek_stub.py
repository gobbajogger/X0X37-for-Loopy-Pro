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


def strip(text: str) -> str:
    a = text.find(START)
    if a < 0:
        return text  # source before #737: nothing to replace
    b = text.index(END, a) + len(END)
    rest = text[b:]
    # Drop the forward declaration of smc_ui_off(): a harness that does not
    # copy its definition would fail -Werror=unused-function.
    rest = rest.replace(OFF_DECL, "", 1) if rest.lstrip().startswith(OFF_DECL) else rest
    return text[:a] + STUBS + rest


if __name__ == "__main__":
    sys.stdout.write(strip(sys.stdin.read()))
