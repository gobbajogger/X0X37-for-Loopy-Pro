/* SPDX-License-Identifier: GPL-3.0-only */
/* MID-13/MID-15: selecting INTERNAL releases the old external clock owner
 * immediately, even if no external MIDI packet arrives while INTERNAL is on. */
#include <stdint.h>
#include <stdio.h>
#include "../../firmware/src/seq/sequencer.h"

static int failures;
#define CHECK(cond, description) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", description); failures++; } \
} while (0)
static int drum(void *x, int k, int v, float vel)
{ (void)x; (void)k; (void)v; (void)vel; return 1; }
static int bass_on(void *x, int p, int n, int acc, int slide, int vel)
{ (void)x; (void)p; (void)n; (void)acc; (void)slide; (void)vel;  return 1; }
static void bass_off(void *x, int p) { (void)x; (void)p; }
static void brk(void *x, int s, int b, float f, int en)
{ (void)x; (void)s; (void)b; (void)f; (void)en; }
static void brk_stop(void *x) { (void)x; }
static void midi_out(void *x, uint8_t s, uint8_t n, uint8_t v)
{ (void)x; (void)s; (void)n; (void)v; }
static const seq_sink_t SINK = {drum, bass_on, bass_off, brk, brk_stop, midi_out, 0, 0};
static pattern_t pat[NPAT];

static void test_owner_release(unsigned next_mode)
{
    seq_t s;
    int i;
    for (i=0; i<NPAT; i++) pattern_init(&pat[i], (unsigned)(i+1), (unsigned)(i+11));
    seq_init(&s,pat);
    s.send_clock=0;
    s.clk_src=SEQ_CLK_EXT;
    seq_advance(&s,0,&SINK);
    CHECK(seq_clk_take(&s,0xFAu,0u)==1,"USB Start initially accepted");
    CHECK(s.clk_lock==1u,"USB is initial clock owner");

    /* The user briefly selects INTERNAL. No incoming MIDI messages occur.
     * This switch must itself release USB (it no longer owns the clock). */
    s.clk_src=SEQ_CLK_INT;
    seq_advance(&s,0,&SINK);
    CHECK(s.clk_lock==0u,"INTERNAL must immediately clear external clock owner");
    s.clk_src=(uint8_t)next_mode;
    seq_advance(&s,0,&SINK);
    CHECK(seq_clk_take(&s,0xFAu,1u)==1,"Bluetooth Start accepted after leaving INTERNAL");
    CHECK(s.clk_lock==2u,"Bluetooth becomes the clock owner");
    CHECK(seq_clk_take(&s,0xFAu,0u)==0,"old USB Start cannot steal new owner's clock");
}

int main(void)
{
    test_owner_release(SEQ_CLK_EXT);
    test_owner_release(SEQ_CLK_AUTO);
    if (failures) {
        fprintf(stderr,"clock source owner reset: %d failures\n",failures);
        return 1;
    }
    puts("clock source owner reset: PASS (INTERNAL->EXTERNAL, INTERNAL->AUTO)");
    return 0;
}
