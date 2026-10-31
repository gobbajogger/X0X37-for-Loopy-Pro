/* SPDX-License-Identifier: GPL-3.0-only */
/* Real usb.c ingress and mixer with EP4 register/DMA storage supplied by the
 * fake HAL. The runner adds a callback after each mixed stereo frame in its
 * temporary source copy, to simulate TIMER5 preemption without target hooks.
 * This is a deterministic interrupt schedule, not an iPhone/device test. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FM1_IRQ_TIMER5 63u
#define FELUCCA_UAC 1
#define BOARD_SMK37 1
static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }
static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
static void uac_out_test_tick(void);
#include "src/usb.c"

void smk_apc_host_seen(void) {}
void smk_apc_host_lost(void) {}
uint8_t smk_apc_usb_event(uint32_t p) { (void)p; return 0; }
void smk_apc_usb_tick(void) {}
void smk_apc_usb_link(uint8_t up) { (void)up; }
void smk_apc_host_generic(uint8_t ble) { (void)ble; }
void smk_inlog(uint32_t src, uint32_t p) { (void)src; (void)p; }
void smc_mmc_host(uint8_t cmd, uint8_t src) { (void)cmd; (void)src; }

static unsigned fails, checks, tick, packet, inject;
static const unsigned packet_at[] = {1, 45, 89, 133, 177, 221};
static const unsigned packet_frames[] = {44, 44, 44, 44, 45, 44};
static uint32_t next_frame;

static void check(int ok, const char *what)
{
    checks++;
    if (!ok) {
        printf("FAIL: %s\n", what);
        fails++;
    }
}

/* Distinct left/right ramps make loss, overwrite and channel swaps visible. */
static uint32_t sample(uint32_t frame)
{
    uint16_t l = (uint16_t)(frame + 1u);
    uint16_t r = (uint16_t)(int16_t)-(int32_t)(frame + 1u);
    return (uint32_t)l | (uint32_t)r << 16;
}

static void reset(uint32_t start, unsigned fill)
{
    unsigned i;
    memset(&uo, 0, sizeof uo);
    memset(uo_ring, 0, sizeof uo_ring);
    memset(fake_reg, 0, sizeof fake_reg);
    fake_index = fake_rd_r = 0;
    uo_r = start;
    uo_w = start + fill;
    for (i = 0; i < fill; i++)
        uo_ring[(start + i) & (UO_N - 1u)] = sample(i);
    next_frame = fill;
    tick = packet = inject = 0;
}

static void receive(unsigned n)
{
    unsigned i;
    for (i = 0; i < n; i++)
        ep4rx[i] = sample(next_frame++);
    fake_reg[4][S_RXCOUNT1] = (uint8_t)(n * 4u);
    fake_reg[4][S_RXCOUNT2] = (uint8_t)((n * 4u) >> 8);
    fake_reg[4][S_RXCSR1] = 1u;
    uac_out_rx();
    check(!(fake_reg[4][S_RXCSR1] & 1u), "EP4 packet is acknowledged even when full");
}

static void uac_out_test_tick(void)
{
    tick++;
    if (inject && packet < sizeof packet_at / sizeof packet_at[0] &&
        tick == packet_at[packet]) {
        receive(packet_frames[packet]);
        packet++;
    }
}

static void prime(void)
{
    uo.alt = 1;
    uac_out_half();
    /* Isolate index publication from clock correction: one input per frame. */
    uo.pos = 1.0f;
    uo.step = 1.0f;
}

static void nested_packets(uint32_t start)
{
    int32_t control[512], out[512];
    unsigned i;
    for (i = 0; i < 512; i++)
        control[i] = out[i] = 1000 * 256;

    reset(start, 760);
    prime();
    uac_out_mix(control, 256);

    reset(start, 760);
    prime();
    inject = 1;
    uac_out_mix(out, 256);
    inject = 0;
    check(packet == 6, "all six USB packets interrupt one 256-frame mix");
    check(uo.overruns == 0, "nested receive must see consumed slots, not falsely drop a frame");
    check(uo.underruns == 0 && uo.go, "nested receive leaves playback running");
    check(uo_w - start == 1025u, "all 265 incoming frames are retained");
    check(uo_r - start == 256u, "mixer consumes exactly 256 frames");
    check(uo_w - uo_r == 769u, "remaining fill is 760 + 265 - 256");
    check(uo.pkts == 6 && uo.frames == 265, "production ingress accounts for all packets");
    check(memcmp(out, control, sizeof out) == 0, "nested ingress preserves the mixed stereo samples");
    for (i = 256; i < 1025; i++)
        check(uo_ring[(start + i) & (UO_N - 1u)] == sample(i),
              "unconsumed initial and received frames remain in order");
}

static void capacity_boundaries(uint32_t start)
{
    uint32_t before[UO_N];
    unsigned i;
    reset(start, UO_N);
    memcpy(before, uo_ring, sizeof before);
    receive(44);
    check(uo.overruns == 1 && uo_w - start == UO_N && uo_r == start,
          "a genuinely full ring rejects the packet without moving indices");
    check(memcmp(before, uo_ring, sizeof before) == 0,
          "a genuinely full ring never overwrites unread samples");

    reset(start, UO_N - 1u);
    receive(44);
    check(uo.overruns == 1 && uo_w - start == UO_N && uo_r == start,
          "one free slot accepts one frame and rejects the remainder");
    for (i = 0; i < UO_N; i++)
        check(uo_ring[(start + i) & (UO_N - 1u)] == sample(i),
              "partial packet admission preserves every unread frame");

    reset(start, 0);
    uac_out_rx();
    check(uo.pkts == 0 && uo.frames == 0 && uo_w == start && uo_r == start,
          "polling without RxPktRdy adds no packet or sample");
}

static void underrun(void)
{
    int32_t out[8] = {0};
    reset(0, 1);
    uo.go = 1;
    uo.pos = uo.step = 1.0f;
    uac_out_mix(out, 4);
    check(uo_r == 1 && uo_w == 1 && uo.underruns == 1 && !uo.go,
          "a real underrun stops at the last available frame without over-reading");
}

int main(void)
{
    nested_packets(0);
    nested_packets(UINT32_MAX - 100u);  /* both physical and unsigned index wrap */
    capacity_boundaries(0);
    capacity_boundaries(UINT32_MAX - 100u);
    underrun();
    printf("uac-out-irq: %u checks, %u failures\n", checks, fails);
    return fails != 0;
}
