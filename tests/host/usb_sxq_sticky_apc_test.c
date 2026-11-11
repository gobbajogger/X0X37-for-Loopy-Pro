/* SPDX-License-Identifier: GPL-3.0-only */
/* Clearing a pending reply on refusal or omitting EP1's retry loses the frame. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() ((void)0)
#define MQ 64u
#define SXQ 64u
static struct { unsigned config, tx_pkts; uint8_t sysex[64]; } usb;
static uint32_t midi_out_q[MQ];
static volatile uint32_t mo_w, mo_r;
#if FELUCCA_OTA
static volatile uint8_t sx_busy;
static uint32_t sx_out_q[SXQ];
static volatile uint32_t so_w, so_r;
#endif
static uint8_t ep1tx[64], wire[2048];
static unsigned wire_size, tx_csr;
enum { S_INDEX, S_TXCSR1 };
static uint32_t sie_rd(unsigned reg) { return reg == S_TXCSR1 ? tx_csr : 0u; }
static void sie_wr(unsigned reg, uint32_t value) { if (reg == S_TXCSR1) tx_csr = value; }
static void fm1_usb_ep_send(unsigned ep, const uint8_t *data, uint32_t size)
{
    assert(ep == 1u && size <= 64u && !(size % 4u));
    assert(wire_size + size <= sizeof wire);
    memcpy(wire + wire_size, data, size);
    wire_size += size;
}
#include "usb_apc_retry_production.h"

#if FELUCCA_OTA
#define QUEUE_W so_w
#define QUEUE_R so_r
#else
#define QUEUE_W mo_w
#define QUEUE_R mo_r
#endif

static void reset(uint32_t index)
{
    memset(&usb, 0, sizeof usb);
    usb.config = 1;
    usb_sx_reply = usb_sx_ch = 0;
    mo_r = mo_w = index;
#if FELUCCA_OTA
    so_r = so_w = index;
    sx_busy = 0;
#endif
    wire_size = tx_csr = 0;
}
static void drain(void)
{
    for (unsigned i = 0; i < 16u; i++) {
        tx_csr = 0; /* host acknowledges the previous endpoint transfer */
        ep1_tx();
    }
    assert(QUEUE_R == QUEUE_W && !usb_sx_reply);
}
static void one_complete_reply(unsigned intro, uint8_t channel)
{
    static const uint8_t identity[35] = {
        0xf0, 0x7e, 0x00, 0x06, 0x02, 0x47, 0x29, 0x00, 0x19,
        0x01, 0x02, 0x00, 0x00, 0x7f,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xf7
    };
    static const uint8_t introduction[17] = {
        0xf0, 0x47, 0x7f, 0x29, 0x61, 0x00, 0x09,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0xf7
    };
    uint8_t expected[35], frame[64];
    unsigned size = intro ? sizeof introduction : sizeof identity;
    unsigned collected = 0, frames = 0, in_frame = 0;
    memcpy(expected, intro ? introduction : identity, size);
    if (!intro) expected[2] = channel;
    for (unsigned i = 0; i < wire_size; i += 4u) {
        unsigned cin = wire[i] & 15u;
        assert(cin >= 4u && cin <= 7u);
        unsigned bytes = cin == 4u || cin == 7u ? 3u : cin == 6u ? 2u : 1u;
        for (unsigned j = 0; j < bytes; j++) {
            uint8_t b = wire[i + 1u + j];
            if (!in_frame) {
                if (b == 0xf7u) continue; /* full-ring filler frames */
                assert(b == 0xf0u);
                in_frame = 1;
                collected = 0;
            }
            assert(collected < sizeof frame);
            frame[collected++] = b;
            if (b == 0xf7u) {
                assert(cin != 4u && j + 1u == bytes);
                assert(collected == size && !memcmp(frame, expected, size));
                frames++;
                in_frame = 0;
            }
        }
    }
    assert(frames == 1u && !in_frame);
}
static void full_ring_retry(unsigned intro, uint8_t channel, uint32_t index)
{
    const uint8_t filler[] = {0xf7};
    reset(index);
    for (unsigned i = 0; i < MQ; i++)
        assert(midi_send_sysex(filler, sizeof filler) == 0);
    if (intro) apc40_introduction_reply();
    else apc40_device_inquiry(channel);
    assert(usb_sx_reply == (intro ? 2u : 1u));
    uint32_t full = QUEUE_W;
    for (unsigned i = 0; i < 4u; i++) usb_sx_answer();
    assert(QUEUE_W == full && QUEUE_W - QUEUE_R == MQ && !wire_size);
    ep1_tx(); /* retry still full, then the real endpoint frees sixteen slots */
    assert(usb_sx_reply && QUEUE_R == index + 16u);
    drain(); /* a later real EP1 invocation retries and sends the whole reply */
    unsigned finished_size = wire_size;
    drain();
    assert(wire_size == finished_size); /* retries after completion never duplicate */
    one_complete_reply(intro, channel);
}
static void immediate_reply(unsigned intro)
{
    reset(0);
    if (intro) apc40_introduction_reply();
    else apc40_device_inquiry(3u);
    assert(!usb_sx_reply && QUEUE_W != QUEUE_R);
    drain();
    one_complete_reply(intro, 3u);
}
static void exact_space_retry(unsigned intro, uint32_t index)
{
    const uint8_t filler[] = {0xf7};
    /* Preserve the original identity regression's exact twelve-slot boundary;
     * the seventeen-byte introduction similarly needs exactly six packets. */
    unsigned packets = intro ? 6u : 12u;
    reset(index);
    for (unsigned i = 0; i < MQ; i++)
        assert(midi_send_sysex(filler, sizeof filler) == 0);
    if (intro) apc40_introduction_reply();
    else apc40_device_inquiry(3u);
    uint32_t full = QUEUE_W;
    QUEUE_R += packets - 1u;
    usb_sx_answer();
    assert(usb_sx_reply == (intro ? 2u : 1u) && QUEUE_W == full);
    QUEUE_R++;
    usb_sx_answer();
    assert(!usb_sx_reply && QUEUE_W == full + packets);
    assert(QUEUE_W - QUEUE_R == MQ);
    drain();
    one_complete_reply(intro, 3u);
}
#if FELUCCA_OTA
static void busy_retry(unsigned intro)
{
    reset(0);
    sx_busy = 1;
    if (intro) apc40_introduction_reply();
    else apc40_device_inquiry(0x7fu);
    for (unsigned i = 0; i < 4u; i++) ep1_tx();
    assert(usb_sx_reply == (intro ? 2u : 1u) && !wire_size && !so_w);
    sx_busy = 0;
    drain();
    one_complete_reply(intro, 0x7fu);
}
#endif
int main(void)
{
    for (unsigned intro = 0; intro < 2u; intro++) {
        full_ring_retry(intro, 0x7fu, 0);
        full_ring_retry(intro, 3u, MQ - 2u);
        full_ring_retry(intro, 0u, UINT32_MAX - 31u);
        immediate_reply(intro);
        exact_space_retry(intro, 0u);
        exact_space_retry(intro, UINT32_MAX - 31u);
#if FELUCCA_OTA
        busy_retry(intro);
#endif
    }
    printf("PASS APC EP1 retry: full/busy queue, UDI/intro framing and exact-once (OTA=%d)\n", FELUCCA_OTA);
}
