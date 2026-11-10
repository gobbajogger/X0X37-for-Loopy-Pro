/* SPDX-License-Identifier: GPL-3.0-only */
/* A protected replace in a full command queue must not leave a false full
 * after the drain consumes 128 commands. Device and native builds are not
 * this test. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stdio.h>
void engine_host_cq_sync(void);
void engine_host_cq_at(uint32_t cursor);
int engine_part_sounding(void);
int engine_bass_keys_held(void);

static int fails;
static const char *cq_case;
#define CHECK(c) do { \
    if (!(c)) { \
        fails++; \
        fprintf(stderr, "FAIL %s:%d: %s [%s]\n", __FILE__, __LINE__, #c, cq_case); \
    } \
} while (0)

static void render_block(void)
{
    int32_t blk[512];
    engine_render(blk, 256);
}

static void reset_audio(void)
{
    boot();
    render_block();
    engine_host_cq_sync();
}

static void fill_params(unsigned n)
{
    while (n--)
        engine_set(NTARGETS, 0, 0, 0);
}

static void drain_rest(void)
{
    int i;
    for (i = 0; i < 16; i++)
        render_block();
}

static int bass_held(int part)
{
    return (engine_part_sounding() & (2 << part)) && (engine_bass_keys_held() & (1 << part));
}

/* The kept note is published at the tail. One block drains 128 commands and
 * must not reach it. Replacing the head in place, or appending without
 * advancing the read cursor, plays that note inside the block. */
static void note_stays_past_budget(const char *name, int wrap)
{
    cq_case = name;
    reset_audio();
    if (wrap)
        engine_host_cq_at(0xFFFFFC00u);
    fill_params(1024);
    engine_bass_on(0, 40, 1, 0);
    render_block();
    CHECK(!bass_held(0));
    drain_rest();
    CHECK(bass_held(0));
}

static void fresh_after_replace(const char *name, int wrap)
{
    int mp;
    cq_case = name;
    reset_audio();
    mp = engine_bass_mod_param(0);
    CHECK(mp >= 0);
    engine_set(T_303, 0, mp, 1);
    render_block();
    CHECK(engine_bass_perf(0, 2, 0) == 1);
    if (wrap)
        engine_host_cq_at(0xFFFFFC00u);
    fill_params(1024);
    engine_bass_on(0, 40, 1, 0);
    render_block();
    engine_set(T_303, 0, mp, 77);
    engine_bass_on(1, 52, 1, 0);
    drain_rest();
    CHECK(engine_bass_perf(0, 2, 0) == 77);
    CHECK(engine_part_sounding() & 4);
}

int main(void)
{
    int32_t blk[512];
    int mp;
    fails = 0;
    cq_case = "param applies when empty";
    boot();
    engine_render(blk, 256);
    engine_host_cq_sync();
    mp = engine_bass_mod_param(0);
    engine_set(T_303, 0, mp, 77);
    engine_render(blk, 256);
    CHECK(engine_bass_perf(0, 2, 0) == 77);
    note_stays_past_budget("head replace stays queued", 0);
    note_stays_past_budget("head replace stays queued wrap", 1);
    fresh_after_replace("full then drain", 0);
    fresh_after_replace("wrapped cursors", 1);
    if (fails) {
        fprintf(stderr, "cq reclaim: %d failures\n", fails);
        return 1;
    }
    printf("cq reclaim: 0 failures\n");
    return 0;
}
