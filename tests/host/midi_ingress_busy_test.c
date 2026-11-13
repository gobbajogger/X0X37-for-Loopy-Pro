/* SPDX-License-Identifier: GPL-3.0-only */
/* Removing either ingress route's shared-queue call must strand the release. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BOARD_SMK37 1
#define RING_PUBLISH() ((void)0)
#define FM1_IRQ_TIMER5 63u
static unsigned guarded;
static uint32_t fm1_irq_mask_save(unsigned source)
{ assert(source == FM1_IRQ_TIMER5 && !guarded); guarded = 1; return 0x80u; }
static void fm1_irq_mask_restore(unsigned source, uint32_t state)
{ assert(source == FM1_IRQ_TIMER5 && guarded && state == 0x80u); guarded = 0; }
enum { S_INDEX, S_RXCSR1, S_RXCSR2, S_RXCOUNT1, S_RXCOUNT2 };
static uint8_t ep1rx[64];
static struct { unsigned rx_pkts; } usb;
static uint32_t sie_rd(unsigned reg)
{ return reg == S_RXCSR1 ? 1u : reg == S_RXCOUNT1 ? 4u : 0u; }
static void sie_wr(unsigned reg, uint32_t value) { (void)reg; (void)value; }
static void fm1_usb_rx_sync(void) {}
static void smk_inlog(unsigned src, uint32_t pkt) { (void)src; (void)pkt; }
static int smk_apc_usb_event(uint32_t pkt) { (void)pkt; return 0; }
static void sysex_byte(uint8_t byte) { (void)byte; assert(!"unexpected SysEx"); }
#include "midi_ingress_production.h"

static uint32_t packet(unsigned cin, unsigned status, unsigned d1, unsigned d2)
{ return cin | status << 8 | d1 << 16 | d2 << 24; }
static int ingress(unsigned source, uint32_t pkt)
{
    if (source == 2u)
        return um_put(pkt);
    uint32_t wire = pkt | 0x70u; /* USB host cable bits must become input source zero. */
    ep1rx[0] = (uint8_t)wire;
    ep1rx[1] = (uint8_t)(wire >> 8);
    ep1rx[2] = (uint8_t)(wire >> 16);
    ep1rx[3] = (uint8_t)(wire >> 24);
    ep1_rx();
    return 1; /* USB receive has no enqueue-return interface. Assert queue below. */
}
static void release_after_cc_flood(unsigned source, int velocity_zero, uint32_t start, int marker)
{
    uint32_t on = packet(9u, 0x92u, 60u, 100u);
    uint32_t off = packet(velocity_zero ? 9u : 8u, velocity_zero ? 0x92u : 0x82u, 60u, 0u);
    uint32_t cable = source << 4;
    unsigned prefix = marker ? 2u : 1u;
    mi_r = mi_w = start;
    if (marker) assert(midi_in_put(cable)); /* existing input link-end marker */
    assert(ingress(source, on));
    for (unsigned i = 0; i < MQ - prefix; i++)
        assert(ingress(source, packet(0xbu, 0xb2u, 1u, i)));
    assert(mi_w - mi_r == MQ);
    assert(ingress(source, off));
    assert(mi_r == start && mi_w == start + MQ);
    if (marker) assert(midi_in_q[mi_r % MQ] == cable);
    assert(midi_in_q[(mi_r + prefix - 1u) % MQ] == (on | cable));
    for (unsigned i = prefix; i < MQ - 1u; i++)
        assert(midi_in_q[(mi_r + i) % MQ] == (packet(0xbu, 0xb2u, 1u, i - prefix + 1u) | cable));
    assert(midi_in_q[(mi_w - 1u) % MQ] == (off | cable));
    uint32_t before[MQ];
    memcpy(before, midi_in_q, sizeof before);
    int accepted = ingress(source, packet(0xbu, 0xb2u, 7u, 0x55u));
    if (source == 2u) assert(!accepted);
    assert(mi_w == start + MQ && mi_r == start);
    assert(!memcmp(before, midi_in_q, sizeof before) && !guarded);
}
int main(void)
{
    const uint32_t starts[] = {0u, MQ - 3u, UINT32_MAX - 31u};
    for (unsigned s = 0; s < 2; s++)
        for (unsigned zero = 0; zero < 2; zero++)
            for (unsigned i = 0; i < sizeof starts / sizeof starts[0]; i++)
                for (unsigned marker = 0; marker < 2u; marker++)
                    release_after_cc_flood(s ? 2u : 0u, (int)zero, starts[i], (int)marker);
    puts("PASS USB/TRS busy ingress: Note Off, velocity-zero, source bits, order and CC refusal");
}
