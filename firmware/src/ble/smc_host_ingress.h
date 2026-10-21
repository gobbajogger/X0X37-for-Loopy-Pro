/* SPDX-License-Identifier: GPL-3.0-only */
/* App-side BLE-MIDI payload parser for a separate vendor-host TEST profile.
 * Adapted from ble_midi_central.c at baseline 816e0d3; no HCI/GATT host included.
 * The caller owns a stable copied notification for this synchronous call. It
 * must serialize calls, reject old-link mailbox records, and publish link reset
 * before consuming a new connection. No borrowed packet pointer is retained.
 *
 * This callback is ONLY the dedicated SMC input queue, never APC/Loopy/common
 * MIDI ingress. It must copy the 1..3 or 6 bytes before returning: 1 accepted,
 * 0 rejected/full. Production smc_ble_midi currently returns void; its adapter
 * must report its real queue outcome, not fabricate successful enqueue counts. */
#pragma once
#include <stdint.h>

#define SMC_HI_MAX_PAYLOAD 128u

typedef int (*smc_hi_emit_fn)(void *ctx, const uint8_t *message, uint8_t size);
typedef struct {
    uint8_t running, in_sysex, sx_len, sx[8];
} smc_hi_decoder_t;
typedef struct {
    uint32_t notifications, messages, rejected, dropped;
    smc_hi_decoder_t decoder;
    uint8_t connected, reason;
} smc_host_ingress_t;
enum { SMC_HI_OK = 0, SMC_HI_LINK, SMC_HI_SIZE, SMC_HI_HEADER,
       SMC_HI_STATUS, SMC_HI_TRUNCATED, SMC_HI_DATA, SMC_HI_SYSEX };

/* Statistics are lifetime counters; initializing the whole object to zero is
 * required once. Link boundaries reset only decoder state, not evidence. */
static inline void smc_hi_link(smc_host_ingress_t *s, int connected)
{
    s->connected = (uint8_t)(connected != 0);
    s->decoder.running = s->decoder.in_sysex = s->decoder.sx_len = 0;
}
static inline void smc_hi_emit(smc_host_ingress_t *s, smc_hi_emit_fn emit, void *ctx,
                              const uint8_t *p, uint8_t n)
{
    if (!s) return; /* preflight: validate syntax/state without side effects */
    if (emit && emit(ctx, p, n)) s->messages++;
    else s->dropped++;
}

/* Keep the baseline timestamp/running-status/fragmented six-byte SysEx grammar.
 * Differences are deliberate: reject malformed/truncated messages instead of
 * dispatching high-bit data, support realtime between channel data bytes, and
 * reject unsupported system-common messages. Complete malformed notifications
 * have no callback effects because the public entrypoint preflights first. */
static inline uint8_t smc_hi_decode(smc_hi_decoder_t *d, const uint8_t *p, uint32_t n,
                                    smc_host_ingress_t *stats, smc_hi_emit_fn emit, void *ctx)
{
    uint32_t i = 1;
    while (i < n) {
        uint8_t st, m[3], len, j;
        if (d->in_sysex) {
            uint8_t b = p[i++];
            if (b == 0xF7u && i < n && (p[i] == 0xF7u || !(p[i] & 0x80u)))
                continue; /* baseline's timestamp-low 127 before data/EOX */
            if (b == 0xF7u) {
                if (d->sx_len < sizeof d->sx) d->sx[d->sx_len++] = b;
                if (d->sx_len != 6u) return SMC_HI_SYSEX;
                smc_hi_emit(stats, emit, ctx, d->sx, 6u);
                d->in_sysex = d->sx_len = 0;
            } else if (!(b & 0x80u)) {
                if (d->sx_len >= sizeof d->sx) return SMC_HI_SYSEX;
                d->sx[d->sx_len++] = b;
            } else if (b >= 0xF8u) {
                smc_hi_emit(stats, emit, ctx, &b, 1u);
            } /* other high-bit bytes inside SysEx are baseline timestamp bytes */
            continue;
        }
        if (p[i] & 0x80u) ++i; /* timestamp low, as in the existing SMC parser */
        if (i >= n) return SMC_HI_TRUNCATED;
        st = p[i];
        if (st & 0x80u) {
            ++i;
            if (st < 0xF0u) d->running = st;
            else if (st < 0xF8u) d->running = 0;
        } else {
            if (!d->running) return SMC_HI_STATUS;
            st = d->running;
        }
        if (st == 0xF0u) {
            d->in_sysex = 1; d->sx_len = 1; d->sx[0] = st;
            continue;
        }
        len = st >= 0xF8u ? 1u : st >= 0xF0u ? 0u :
              (st & 0xE0u) == 0xC0u ? 2u : 3u;
        if (!len) return SMC_HI_STATUS;
        m[0] = st; m[1] = m[2] = 0;
        for (j = 1; j < len; ++j) {
            while (i < n && p[i] >= 0xF8u) {
                smc_hi_emit(stats, emit, ctx, p + i, 1u); ++i;
            }
            if (i >= n) return SMC_HI_TRUNCATED;
            if (p[i] & 0x80u) return SMC_HI_DATA;
            m[j] = p[i++];
        }
        smc_hi_emit(stats, emit, ctx, m, len);
    }
    return SMC_HI_OK;
}

/* Returns a reason; valid notifications may still have queue drops. A malformed
 * notification clears partial decoder state and cannot emit its valid prefix.
 * Counters are uint32_t modulo counters, like the existing receive diagnostics. */
static inline uint8_t smc_hi_notification(smc_host_ingress_t *s, const uint8_t *p, uint32_t n,
                                         smc_hi_emit_fn emit, void *ctx)
{
    smc_hi_decoder_t trial;
    uint8_t reason;
    s->notifications++;
    reason = !s->connected ? SMC_HI_LINK : !p || n < 3u || n > SMC_HI_MAX_PAYLOAD ? SMC_HI_SIZE :
             (p[0] & 0xC0u) != 0x80u ? SMC_HI_HEADER : SMC_HI_OK;
    if (!reason) {
        trial = s->decoder;
        reason = smc_hi_decode(&trial, p, n, 0, 0, 0);
    }
    s->reason = reason;
    if (reason) {
        s->rejected++;
        s->decoder.running = s->decoder.in_sysex = s->decoder.sx_len = 0;
        return reason;
    }
    (void)smc_hi_decode(&s->decoder, p, n, s, emit, ctx);
    return SMC_HI_OK;
}
