/* SPDX-License-Identifier: GPL-3.0-only */
/* Passive central diagnostics. TIMER5 is the sole writer; no protocol decisions. */
#pragma once
#include <stdint.h>

enum { BLEC_SMP_TRACE_MAGIC = 0x534d4303u, BLEC_SMP_TRACE_WORDS = 11u };
enum { BLEC_TRACE_MAGIC = 0x534d4302u, BLEC_TRACE_WORDS = 26u, BLEC_TRACE_RECENT = 3u };
enum {
    BLEC_TRACE_NONE, BLEC_TRACE_HCI_ERROR, BLEC_TRACE_CONNECT_FAIL,
    BLEC_TRACE_DISCONNECT, BLEC_TRACE_ATT_ERROR, BLEC_TRACE_TIMEOUT,
    BLEC_TRACE_TRANSPORT_BLOCKED, BLEC_TRACE_PROTOCOL_ERROR,
    BLEC_TRACE_SIGNAL_FAILURE, BLEC_TRACE_NO_PEER,
    BLEC_TRACE_START = 16, BLEC_TRACE_CREATE, BLEC_TRACE_LINK,
    BLEC_TRACE_READY, BLEC_TRACE_CANCEL, BLEC_TRACE_SMP
};
enum {
    BLEC_TRACE_WAIT_HCI = 1, BLEC_TRACE_WAIT_CREATE, BLEC_TRACE_WAIT_CANCEL,
    BLEC_TRACE_WAIT_ATT, BLEC_TRACE_WAIT_SIGNAL, BLEC_TRACE_WAIT_DISCONNECT,
    BLEC_TRACE_WAIT_SEND, BLEC_TRACE_WAIT_ATT_CONTROL
};

/* status is an HCI status/reason, ATT error, or a kind-specific reason.
 * operation: opcode in low16; connection handle in high16, except ATT_ERROR
 * uses its attribute handle. detail is an additional caller-defined raw word.
 * SMP: opcode is the received command, detail is its complete PDU length.
 * Only a well-sized (2-byte) Security Request or Pairing Failed has status
 * copied from its non-secret auth flags/reason; every other status is zero.
 * Records and counters survive automatic retry and explicit start; the owning
 * host's initialization clears them. Timestamp zero is valid. */
typedef struct {
    uint32_t ms, context, operation, detail;
} blec_trace_record_t;

typedef struct {
    blec_trace_record_t first, latest, recent[BLEC_TRACE_RECENT];
    /* Last complete SMP metadata, retained across stop/start/retry, cleared by init.
     * It may belong to an earlier connection; its timestamp/handle are retained. */
    blec_trace_record_t last_smp;
    uint32_t smp_seen; /* opcode bits 0..30; bit31 means another opcode. */
    /* Packed permitted field: handle high16, validity bit8, auth/reason low8. */
    uint32_t smp_security_ms, smp_security, smp_failure_ms, smp_failure;
    uint32_t events;
    uint16_t attempts, ready, failures, disconnects;
    uint8_t next, count;
} blec_trace_t;

static inline void blec_trace_inc(uint16_t *value)
{ if (*value != 0xffffu) ++*value; }

static inline void blec_trace_note(blec_trace_t *t, uint32_t ms,
                                  uint8_t state, uint8_t step, uint8_t kind,
                                  uint8_t status, uint16_t opcode,
                                  uint16_t handle, uint32_t detail, int failure)
{
    blec_trace_record_t r;
    r.ms = ms;
    r.context = kind | (uint32_t)state << 8 | (uint32_t)step << 16 |
                (uint32_t)status << 24;
    r.operation = opcode | (uint32_t)handle << 16;
    r.detail = detail;
    if (kind == BLEC_TRACE_CREATE) blec_trace_inc(&t->attempts);
    if (kind == BLEC_TRACE_READY) blec_trace_inc(&t->ready);
    if (kind == BLEC_TRACE_DISCONNECT) blec_trace_inc(&t->disconnects);
    if (failure) {
        if (!t->failures) t->first = r;
        t->latest = r;
        blec_trace_inc(&t->failures);
    }
    if (kind == BLEC_TRACE_SMP) {
        t->last_smp = r;
        t->smp_seen |= 1u << (opcode < 31u ? opcode : 31u);
        if (detail == 2u && opcode == 0x0bu) {
            t->smp_security_ms = ms;
            t->smp_security = (uint32_t)handle << 16 | 0x100u | status;
        } else if (detail == 2u && opcode == 5u) {
            t->smp_failure_ms = ms;
            t->smp_failure = (uint32_t)handle << 16 | 0x100u | status;
        }
    }
    t->recent[t->next] = r;
    t->next = (uint8_t)((t->next + 1u) % BLEC_TRACE_RECENT);
    if (t->count < BLEC_TRACE_RECENT) t->count++;
    t->events++;
}

static inline void blec_trace_record_words(const blec_trace_record_t *r, uint32_t *w)
{ w[0] = r->ms; w[1] = r->context; w[2] = r->operation; w[3] = r->detail; }

/* Caller supplies BLEC_TRACE_WORDS words and a coherent snapshot/short IRQ lock.
 * Insert this block after the guard, before legacy LIVE words. Keep the SMC1
 * six-word suffix last: both old and new host decoders retain their alignment. */
static inline void blec_trace_words(const blec_trace_t *t, uint32_t *w)
{
    unsigned i;
    w[0] = BLEC_TRACE_MAGIC;
    w[1] = t->events;
    w[2] = t->attempts | (uint32_t)t->ready << 16;
    w[3] = t->failures | (uint32_t)t->disconnects << 16;
    w[4] = t->next | (uint32_t)t->count << 8;
    blec_trace_record_words(&t->first, w + 5);
    blec_trace_record_words(&t->latest, w + 9);
    for (i = 0; i < BLEC_TRACE_RECENT; i++)
        blec_trace_record_words(&t->recent[i], w + 13u + 4u * i);
    w[25] = BLEC_TRACE_WORDS;
}

/* Prepend before the unchanged SMC2 block: legacy decoders take LIVE/SMC2
 * from the end and GUARD from the front, ignoring this additive middle block. */
static inline void blec_smp_trace_words(const blec_trace_t *t, uint32_t *w)
{
    w[0] = BLEC_SMP_TRACE_MAGIC;
    blec_trace_record_words(&t->last_smp, w + 1);
    w[5] = t->smp_seen;
    w[6] = t->smp_security_ms; w[7] = t->smp_security;
    w[8] = t->smp_failure_ms; w[9] = t->smp_failure;
    w[10] = BLEC_SMP_TRACE_WORDS;
}
