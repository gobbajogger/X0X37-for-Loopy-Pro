/* SPDX-License-Identifier: GPL-3.0-only */
/* M13 BREAK rate: the generator stays on a 16th-note clock (slice[16] /
 * breaks_step). RATE_8 / RATE_4 group those 16ths. A default or foreign
 * reserved byte plays as 1/16, so old patterns stay bit-exact. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"

static pattern_t patterns[NPAT];
static seq_t S;
static unsigned checks, failed;
static unsigned brk_steps, brk_hits, last_s16, first_s16, second_at;
static unsigned now;

#define CHECK(x) do { checks++; if (!(x)) { failed++; if (failed < 20) fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); } } while (0)

static void brk(void *ctx, int s16, int bar, float spb, int en)
{
    (void)ctx; (void)bar; (void)spb;
    if (brk_steps == 0) first_s16 = (unsigned)s16;
    if (brk_steps == 1) second_at = now;
    last_s16 = (unsigned)s16;
    brk_steps++;
    if (en) brk_hits++;
}
static const seq_sink_t sink = { .brk = brk };

static void setup(void)
{
    int i;
    for (i = 0; i < NPAT; i++)
        pattern_init(&patterns[i], 1, 2);
    seq_init(&S, patterns);
    S.bpm = 150.0f;
    S.send_clock = 0;
    brk_steps = brk_hits = last_s16 = first_s16 = second_at = now = 0;
}

static void run(unsigned n)
{
    seq_advance(&S, 0, &sink);
    while (n) {
        unsigned k = seq_until_event(&S, n > 128 ? 128 : n);
        now += k;
        seq_advance(&S, k, &sink);
        n -= k;
    }
}

static void mapping(void)
{
    brkpart_t b;
    memset(&b, 0, sizeof b);
    b.steps = 0xFFFFu;
    CHECK(brk_rate_of(&b) == RATE_16);
    CHECK(brk_step_on(&b, 0) && brk_step_on(&b, 15));
    b.steps = 1u; /* only 16th 0 */
    CHECK(brk_step_on(&b, 0) && !brk_step_on(&b, 1));
    b.rate = RATE_8;
    b.steps = 1u; /* user step 0 covers 16ths 0 and 1 */
    CHECK(brk_rate_of(&b) == RATE_8);
    CHECK(brk_step_on(&b, 0) && brk_step_on(&b, 1) && !brk_step_on(&b, 2));
    b.rate = RATE_4;
    b.steps = 1u; /* user step 0 covers 16ths 0-3 */
    CHECK(brk_step_on(&b, 0) && brk_step_on(&b, 3) && !brk_step_on(&b, 4));
    b.rate = 0xA5; /* legacy reserved byte */
    b.steps = 2u;  /* only 16th 1, as 1/16 */
    CHECK(brk_rate_of(&b) == RATE_16);
    CHECK(!brk_step_on(&b, 0) && brk_step_on(&b, 1));
}

static void clock_stays_16th(void)
{
    setup();
    patterns[0].brk.steps = 0xFFFFu;
    patterns[0].brk.rate = RATE_8;
    seq_start(&S);
    run(4410 + 1);
    CHECK(brk_steps == 2);
    CHECK(first_s16 == 0);
    CHECK(second_at == 4410); /* still one 16th later, not one 8th */
    run(14u * 4410u);
    CHECK(brk_steps == 16);
    CHECK(last_s16 == 15);
}

static void hits_follow_rate(void)
{
    setup();
    patterns[0].brk.steps = 1u; /* bit 0 */
    patterns[0].brk.rate = RATE_8;
    seq_start(&S);
    run(15u * 4410u + 1);
    CHECK(brk_steps == 16);
    CHECK(brk_hits == 2); /* 16ths 0 and 1 */
}

static void default_is_exact_16th(void)
{
    setup();
    patterns[0].brk.steps = 0x5u; /* 16ths 0 and 2 */
    CHECK(patterns[0].brk.rate == RATE_16);
    seq_start(&S);
    run(15u * 4410u + 1);
    CHECK(brk_steps == 16);
    CHECK(brk_hits == 2);
}

int main(void)
{
    mapping();
    clock_stays_16th();
    hits_follow_rate();
    default_is_exact_16th();
    if (failed) {
        fprintf(stderr, "break-rate: %u failed / %u checks\n", failed, checks);
        return 1;
    }
    printf("break-rate: %u checks\n", checks);
    return 0;
}
