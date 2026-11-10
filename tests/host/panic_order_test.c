/* SPDX-License-Identifier: GPL-3.0-only */
/* Panic keeps a Note On accepted after the request when overflow, compaction
 * or a counter wrap moves that note to an earlier queue slot.
 * Device and native builds are not this test. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stdio.h>
int engine_part_sounding(void);
int engine_bass_keys_held(void);
void engine_host_cq_sync(void);
void engine_host_cq_at(uint32_t cursor);
void engine_host_note_seq(uint32_t seq);

static int fails;
static const char *order_case;
#define CHECK(c) do { \
    if (!(c)) { \
        fails++; \
        fprintf(stderr, "FAIL %s:%d: %s [%s]\n", __FILE__, __LINE__, #c, order_case); \
    } \
} while (0)

static void render_block(void)
{
    int32_t blk[512];
    engine_render(blk, 256);
}

static void settle(void)
{
    int i;
    for (i = 0; i < 24; i++)
        render_block();
}

static void reset_audio(void)
{
    boot();
    render_block();
    engine_host_cq_sync();
    mi_r = mi_w = 0;
}

static void push_harmless_cq(unsigned n)
{
    while (n--)
        engine_set(NTARGETS, 0, 0, 0);
}

static void own_bass(int part)
{
    int t;
    for (t = 0; t < NTRACKS; t++)
        if (seq.ch[t] == 0)
            seq.ch[t] = SEQ_CH_OFF;
    seq.ch[TRK_BASS0 + part] = 0;
}

static int bass_on(int part)
{
    return (engine_part_sounding() & (2 << part)) && (engine_bass_keys_held() & (1 << part));
}

static void expect_silent(int part)
{
    CHECK(!bass_on(part));
    CHECK((engine_part_sounding() & (2 << part)) == 0);
}

static void expect_held(int part)
{
    CHECK(bass_on(part));
}

static uint32_t note_pkt(uint8_t status, uint8_t d1, uint8_t d2)
{
    uint32_t cin = status >= 0xF0u ? 0x0Fu : (uint32_t)(status >> 4);
    return cin | ((uint32_t)status << 8) | ((uint32_t)d1 << 16) | ((uint32_t)d2 << 24);
}

static void cq_note(const char *name, unsigned prefix, int part, int wrap, uint32_t seq, int kept)
{
    order_case = name;
    reset_audio();
    if (wrap)
        engine_host_cq_at(0xFFFFFC00u);
    if (seq)
        engine_host_note_seq(seq);
    push_harmless_cq(prefix);
    engine_request_notes_off();
    engine_bass_on(part, 52, 1, 0);
    settle();
    if (kept)
        expect_held(part);
    else
        expect_silent(part);
}

static void cq_pre(void)
{
    order_case = "cq pre";
    reset_audio();
    engine_bass_on(0, 40, 1, 0);
    engine_request_notes_off();
    settle();
    expect_silent(0);
}

static void live_key(const char *name, int after)
{
    uint32_t generation, context;
    int arp;
    order_case = name;
    reset_audio();
    engine_live_key_state(0, &generation, &arp, &context);
    if (after)
        engine_request_notes_off();
    CHECK(engine_live_key(0, generation, 1, 48, 100, 0, 0));
    if (!after)
        engine_request_notes_off();
    settle();
    if (after)
        expect_held(0);
    else
        expect_silent(0);
}

static void midi_prefix(const char *name, unsigned prefix, int before, int compact,
                         int wrap, uint32_t seq, int kept)
{
    unsigned i;
    order_case = name;
    reset_audio();
    own_bass(0);
    if (wrap)
        mi_r = mi_w = 0xFFFFFFF0u;
    if (seq)
        mi_seq_next = seq;
    if (before)
        engine_request_notes_off();
    for (i = 0; i < prefix; i++)
        host_midi_put(note_pkt(0xB0, 1, 10));
    if (!before)
        engine_request_notes_off();
    host_midi_put(note_pkt(0x90, 52, 100));
    if (compact)
        CHECK(host_midi_compact(note_pkt(0x80, 40, 0)));
    settle();
    if (kept)
        expect_held(0);
    else
        expect_silent(0);
}

/* 128 earlier accepts fill the panic drain. The next accept is 0xFFFFFFFE,
 * then take_seq skips 0xFFFFFFFF, so the request limit is 1 and the following
 * note is post-request. Unsigned ord < limit keeps the old note. */
static void cq_seq_wrap_across(void)
{
    order_case = "cq seq across ffffffff";
    reset_audio();
    engine_host_note_seq(0xFFFFFF7Eu);
    push_harmless_cq(128);
    engine_bass_on(0, 40, 1, 0);
    engine_request_notes_off();
    engine_bass_on(1, 52, 1, 0);
    settle();
    expect_silent(0);
    expect_held(1);
}

/* 64 earlier packets fill the panic drain. The next accept is 0xFFFFFFFF,
 * midi_seq_take then skips 0, and the request limit is 1. The following note
 * is post-request. Unsigned seq < limit keeps the old note. */
static void midi_seq_wrap_across(void)
{
    unsigned i;
    int t;
    order_case = "midi seq across ffffffff";
    reset_audio();
    for (t = 0; t < NTRACKS; t++)
        if (seq.ch[t] == 0 || seq.ch[t] == 1)
            seq.ch[t] = SEQ_CH_OFF;
    seq.ch[TRK_BASS0] = 0;
    seq.ch[TRK_BASS0 + 1] = 1;
    mi_seq_next = 0xFFFFFFFFu - 64u;
    for (i = 0; i < 64u; i++)
        host_midi_put(note_pkt(0xB0, 1, 10));
    host_midi_put(note_pkt(0x90, 40, 100));
    engine_request_notes_off();
    host_midi_put(note_pkt(0x91, 52, 100));
    settle();
    expect_silent(0);
    expect_held(1);
}

static void midi_pre_tail(void)
{
    unsigned i;
    order_case = "midi pre tail";
    reset_audio();
    own_bass(0);
    for (i = 0; i < 70; i++)
        host_midi_put(note_pkt(0x90, (uint8_t)(40 + (i & 7)), 100));
    engine_request_notes_off();
    settle();
    expect_silent(0);
}

int main(void)
{
    cq_pre();
    cq_note("cq post 0", 0, 0, 0, 0, 0);
    cq_note("cq post 127", 127, 0, 0, 0, 0);
    cq_note("cq post 128", 128, 0, 0, 0, 1);
    cq_note("cq post 1023", 1023, 0, 0, 0, 1);
    cq_note("cq post 1024", 1024, 0, 0, 0, 1);
    cq_note("cq post 1024 part B", 1024, 1, 0, 0, 1);
    cq_note("cq wrap 1024", 1024, 0, 1, 0, 1);
    cq_note("cq wrap append", 8, 0, 1, 0, 0);
    cq_note("cq seq wrap 1024", 1024, 0, 0, 0xFFFFFFF0u, 1);
    live_key("live pre", 0);
    live_key("live post", 1);
    midi_prefix("midi 0", 0, 1, 0, 0, 0, 0);
    midi_prefix("midi 63", 63, 1, 0, 0, 0, 0);
    midi_prefix("midi 64", 64, 1, 0, 0, 0, 1);
    midi_prefix("midi 65", 65, 1, 0, 0, 0, 1);
    midi_prefix("midi 64 compact", 64, 0, 1, 0, 0, 1);
    midi_prefix("midi 70 compact", 70, 0, 1, 0, 0, 1);
    midi_prefix("midi wrap 65", 65, 1, 0, 1, 0, 1);
    midi_prefix("midi wrap compact", 64, 0, 1, 1, 0, 1);
    midi_prefix("midi seq wrap compact", 64, 0, 1, 0, 0xFFFFFFF0u, 1);
    cq_seq_wrap_across();
    midi_seq_wrap_across();
    midi_pre_tail();
    if (fails) {
        fprintf(stderr, "panic order: %d failures\n", fails);
        return 1;
    }
    printf("panic order: 0 failures\n");
    return 0;
}
