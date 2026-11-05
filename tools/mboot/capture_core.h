/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MBOOT_CAPTURE_CORE_H
#define MBOOT_CAPTURE_CORE_H
#include <stddef.h>
#include <stdint.h>

#define MB_CAPTURE_SIZE UINT32_C(0x100000)
#define MB_CAPTURE_HEAD UINT32_C(0x4000)
#define MB_CAPTURE_CHUNK UINT32_C(224)
#define MB_CAPTURE_DIGEST 32u
/* Required caller-proven facts; these are NOT implemented platform features. */
#define MB_CAPTURE_RAW UINT32_C(1)
#define MB_CAPTURE_IDENTITY UINT32_C(2)
#define MB_CAPTURE_EXCLUSION UINT32_C(4)
#define MB_CAPTURE_SESSION UINT32_C(8)
#define MB_CAPTURE_REQUIRED UINT32_C(15)

enum mb_capture_state { MB_CAPTURE_EMPTY, MB_CAPTURE_STREAMING,
    MB_CAPTURE_COMPLETE_HELD, MB_CAPTURE_REVALIDATED_HELD,
    MB_CAPTURE_CANCELLED, MB_CAPTURE_INVALID };
enum mb_capture_result { MB_CAPTURE_OK = 0, MB_CAPTURE_UNSUPPORTED = -1,
    MB_CAPTURE_ARGUMENT = -2, MB_CAPTURE_STATE = -3, MB_CAPTURE_STALE = -4,
    MB_CAPTURE_COVERAGE = -5, MB_CAPTURE_DIGEST_MISMATCH = -6,
    MB_CAPTURE_REPLAY = -7, MB_CAPTURE_FOREIGN_OWNER = -8 };

/* Models: 1=Pro, 2=Elite. Arrays are binary fixed-width values, not C strings.
 * Identity/session/challenge/selection must originate from separately reviewed
 * admission. A host-filled structure is not physical identity or attestation. */
struct mb_capture_binding {
    uint8_t unit[32], session[32], challenge[32], selection[32];
    uint64_t epoch;
    uint32_t model;
};
struct mb_capture_digests { uint8_t full[32], head[32]; };
/* Storage only: callers must not inspect/modify private hash/context fields. */
struct mb_capture_hash {
    uint32_t h[8]; uint64_t bytes; uint8_t block[64]; size_t used;
};
struct mb_capture {
    struct mb_capture_binding binding;
    struct mb_capture_hash full, head;
    struct mb_capture_digests digests;
    uint64_t last_now, deadline;
    uint32_t next;
    enum mb_capture_state state;
};

/* No currently qualified platform adapter exists: always returns 0. */
int mb_capture_platform_supported(void);
/* Core is single-owner/non-reentrant. Caller serializes operations and actually
 * holds exclusion. No wire/device callback or boot-write authority is provided.
 * Time is monotonic in one caller-defined unit; overflow/backwards time refuses.
 * Context must be zero-initialized before its first call. Start rejects an active
 * context without altering it; platform release must precede reuse of a terminal
 * context. Invalidation only clears bookkeeping, not an external lock.
 * Non-null buffers must be valid and disjoint from ctx and output as applicable. */
int mb_capture_start(struct mb_capture *ctx, const struct mb_capture_binding *binding,
                     uint32_t qualified_facts, uint64_t now, uint64_t deadline);
int mb_capture_chunk(struct mb_capture *ctx, const struct mb_capture_binding *binding,
                     uint32_t offset, const uint8_t *data, uint32_t length, uint64_t now);
int mb_capture_finish(struct mb_capture *ctx, const struct mb_capture_binding *binding,
                      const struct mb_capture_digests *target, uint64_t now,
                      struct mb_capture_digests *output);
int mb_capture_revalidate(struct mb_capture *ctx, const struct mb_capture_binding *binding,
                          const uint8_t fresh_challenge[32],
                          const struct mb_capture_digests *target, uint64_t now);
/* Eligibility is an instantaneous read-only check, never a writer token. Actual
 * device-side atomic lease transfer to a writer is absent and not authorized. */
int mb_capture_evidence_current(struct mb_capture *ctx,
                                const struct mb_capture_binding *binding, uint64_t now);
void mb_capture_invalidate(struct mb_capture *ctx);
int mb_capture_cancel(struct mb_capture *ctx, const struct mb_capture_binding *binding);
/* Pure SHA-256 helper, no target acceleration. Output only changes on success. */
int mb_capture_sha256(const uint8_t *data, size_t length, uint8_t output[32]);
#endif
