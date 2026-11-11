/* SPDX-License-Identifier: GPL-3.0-only */
/* A malformed status must end collection before another frame can arrive.
 * Compile the actual parser and USB packet receiver, mocking only SIE I/O and
 * command endpoints; ota_frame_get/done remain the production handoff. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BOARD_SMK37 1
#define RING_PUBLISH() ((void)0)
enum { S_INDEX, S_RXCSR1, S_RXCSR2, S_RXCOUNT1, S_RXCOUNT2 };
static uint8_t ep1rx[64];
static unsigned rx_count, inquiries, introductions, mmcs, midi_packets;
static unsigned host_seen, host_generic;
static uint32_t sie_rd(unsigned reg)
{
    if (reg == S_RXCSR1) return 1u;
    if (reg == S_RXCOUNT1) return rx_count;
    return 0;
}
static void sie_wr(unsigned reg, uint32_t value) { (void)reg; (void)value; }
static void fm1_usb_rx_sync(void) {}
static void smk_inlog(unsigned src, uint32_t pkt) { (void)src; (void)pkt; }
static int smk_apc_usb_event(uint32_t pkt) { (void)pkt; return 0; }
static int midi_in_put(uint32_t pkt) { (void)pkt; midi_packets++; return 1; }
static void apc40_device_inquiry(uint8_t id) { assert(id == 0x7f); inquiries++; }
static void apc40_introduction_reply(void) { introductions++; }
static void smk_apc_host_seen(void) { host_seen++; }
static void smk_apc_host_generic(unsigned value) { assert(value == 0); host_generic++; }
void smc_mmc_host(uint8_t cmd, uint8_t src) { assert(cmd >= 1 && cmd <= 43 && src == 0); mmcs++; }
#include "usb_hygiene_production.h"

static void reset(void)
{
    memset(&usb, 0, sizeof usb);
    inquiries = introductions = mmcs = midi_packets = host_seen = host_generic = 0;
#if FELUCCA_OTA
    sx_ready = sx_collect = 0;
    sx_pos = sx_frame_len = 0;
#endif
}
static void bytes(const uint8_t *p, unsigned n)
{
    for (unsigned i = 0; i < n; i++) sysex_byte(p[i]);
}
static void packet(uint8_t cin, uint8_t a, uint8_t b, uint8_t c)
{
    ep1rx[0] = cin; ep1rx[1] = a; ep1rx[2] = b; ep1rx[3] = c;
    rx_count = 4;
    ep1_rx();
}
static void no_short_command(void)
{
    assert(!usb.uboot_req && !usb.ota_req);
    assert(inquiries == 0 && introductions == 0 && mmcs == 0);
    assert(host_seen == 0 && host_generic == 0);
}
static void one_valid_frame(void)
{
#if FELUCCA_OTA
    static const uint8_t frame[] = {0xf0, 1, 2, 3, 0xf7};
    static const uint8_t payload[] = {1, 2, 3};
    const uint8_t *p = NULL;
    uint32_t n = 0;
    bytes(frame, sizeof frame);
    assert(ota_frame_get(&p, &n) == 1 && n == sizeof payload);
    assert(memcmp(p, payload, sizeof payload) == 0);
    ota_frame_done();
    assert(ota_frame_get(&p, &n) == 0);
#else
    static const uint8_t frame[] = {0xf0, 0x7e, 0x7f, 6, 1, 0xf7};
    bytes(frame, sizeof frame);
    assert(inquiries == 1 && host_seen == 1);
#endif
    assert(!usb.sx_on);
}
static void status_cancels_immediately(void)
{
    static const uint8_t broken[] = {0xf0, 0x22, 0x24, 0x35};
    for (unsigned status = 0x80; status < 0xf7; status++) {
        if (status == 0xf0) continue; /* a new start is tested separately */
        reset();
        bytes(broken, sizeof broken);
        assert(usb.sx_on);
        sysex_byte((uint8_t)status);
        assert(!usb.sx_on); /* RED on old code: status was buffered as data. */
#if FELUCCA_OTA
        assert(!sx_collect && !sx_ready);
#endif
        sysex_byte(0x3c); sysex_byte(0x7f); sysex_byte(0xf7);
        no_short_command();
#if FELUCCA_OTA
        assert(!sx_ready);
#endif
        one_valid_frame();
    }
}
static void packet_stream_recovery_and_realtime(void)
{
    reset();
    packet(4, 0xf0, 0x22, 0x24);
    packet(4, 0x35, 0x90, 0x3c);
    assert(!usb.sx_on);
    packet(5, 0x7f, 0, 0);
    no_short_command();
    one_valid_frame();

    reset();
    packet(4, 0xf0, 0x7e, 0x7f);
    assert(usb.sx_on);
    for (unsigned rt = 0xf8; rt <= 0xff; rt++) {
        packet(0xf, (uint8_t)rt, 0, 0);
        assert(usb.sx_on && inquiries == 0);
    }
    packet(7, 6, 1, 0xf7);
    assert(inquiries == 1 && host_seen == 1 && midi_packets == 8);
    assert(!usb.sx_on);
}
static void restart_and_short_commands(void)
{
    static const uint8_t incomplete[] = {0xf0, 0x22, 0x24, 0x35};
    reset(); bytes(incomplete, sizeof incomplete); one_valid_frame();
    reset();
    static const uint8_t inquiry[] = {0xf0, 0x7e, 0x7f, 6, 1, 0xf7};
    bytes(inquiry, sizeof inquiry);
    assert(inquiries == 1 && host_seen == 1 && !usb.sx_on);
    reset();
    static const uint8_t intro[] = {0xf0,0x47,0x7f,0x29,0x60,0,4,0x40,1,2,3,0xf7};
    bytes(intro, sizeof intro);
    assert(introductions == 1 && host_seen == 1 && host_generic == 1);
    reset();
    static const uint8_t boot[] = {0xf0,0x22,0x24,0x35,0x7d,0xf7};
    bytes(boot, sizeof boot); assert(usb.uboot_req == 1);
    reset();
    static const uint8_t mmc[] = {0xf0,0x7f,0x7f,6,1,0xf7};
    bytes(mmc, sizeof mmc); assert(mmcs == 1);
}
static void seventy_bytes_never_match_short_command(void)
{
    static const uint8_t prefixes[][11] = {
        {0xf0,0x22,0x24,0x35,0x7d}, {0xf0,0x22,0x24,0x35,0x7f},
        {0xf0,0x7e,0x7f,6,1}, {0xf0,0x7f,0x7f,6,1},
        {0xf0,0x47,0x7f,0x29,0x60,0,4,0x40,1,2,3}
    };
    static const unsigned lengths[] = {5,5,5,5,11};
    for (unsigned i = 0; i < sizeof lengths / sizeof lengths[0]; i++) {
        uint8_t frame[70];
        reset(); memset(frame, 0x40, sizeof frame);
        memcpy(frame, prefixes[i], lengths[i]); frame[69] = 0xf7;
        bytes(frame, sizeof frame);
        assert(usb.sx_len == sizeof usb.sysex && !usb.sx_on);
        no_short_command();
        /* Long editor frames are legal; only short-command dispatch is barred. */
#if FELUCCA_OTA
        const uint8_t *p;
        uint32_t n;
        assert(ota_frame_get(&p, &n) == 1 && n == 68);
        assert(memcmp(p, frame + 1, 68) == 0);
        ota_frame_done(); assert(ota_frame_get(&p, &n) == 0);
#endif
    }
}
int main(void)
{
    status_cancels_immediately();
    packet_stream_recovery_and_realtime();
    restart_and_short_commands();
    seventy_bytes_never_match_short_command();
    printf("PASS USB SysEx cancellation, recovery, realtime, exact command lengths (OTA=%d)\n", FELUCCA_OTA);
    return 0;
}
