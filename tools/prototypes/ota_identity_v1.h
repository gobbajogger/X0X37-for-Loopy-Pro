/* SPDX-License-Identifier: GPL-3.0-only
 * Portable FUTURE identity-query prototype; not included in app or loader.
 * No SPI, USB, flash, persistence, session arming or destructive operations.
 * Payloads are NOT assigned production SysEx commands. Native endpoint binding,
 * a qualified hardware UID provider and actual target integration remain gates.
 */
#ifndef X0X_OTA_IDENTITY_V1_H
#define X0X_OTA_IDENTITY_V1_H
#include <stddef.h>
#include <stdint.h>

#define OTA_UID_BYTES 16u
#define OTA_ID_NONCE_BYTES 32u
#define OTA_ID_QUERY_BYTES 37u
#define OTA_ID_REPLY_BYTES 56u
enum { OTA_UID_UNKNOWN = 0, OTA_UID_P25Q80H_V17 = 1, OTA_UID_P25Q80SH_V14 = 2 };
enum { OTA_ID_UNQUALIFIED = 0, OTA_ID_FACTORY_TAG = 1 };
enum { OTA_ID_APP = 1, OTA_ID_LOADER = 2 };
enum { OTA_ID_ELITE = 1, OTA_ID_PRO = 2 };
enum { OTA_ID_OK = 0, OTA_ID_UNAVAILABLE = 1 };

typedef struct {
    uint8_t tx[5];              /* RUID opcode plus FOUR dummy bytes */
    uint8_t tx_bytes, rx_bytes;
} ota_uid_read_plan;

/* A documented part profile must be supplied by separate physical/provenance
 * qualification. JEDEC 856014 ALONE NEVER selects a profile. EB6014 is unknown.
 * This builds a plan in RAM; it does not execute any bus command.
 */
static inline int ota_uid_plan(unsigned qualified_profile, uint32_t jedec, ota_uid_read_plan *out)
{
    unsigned i;
    if (!out) return 0;
    for (i = 0; i < 5u; i++) out->tx[i] = 0;
    out->tx_bytes = out->rx_bytes = 0;
    if ((qualified_profile != OTA_UID_P25Q80H_V17 && qualified_profile != OTA_UID_P25Q80SH_V14)
        || jedec != 0x856014u) return 0;
    out->tx[0] = 0x4Bu;
    out->tx_bytes = 5u;
    out->rx_bytes = OTA_UID_BYTES;
    return 1;
}

static inline int ota_id_nonzero(const uint8_t *p, unsigned n)
{
    unsigned i; uint8_t any = 0;
    if (!p) return 0;
    for (i = 0; i < n; i++) any |= p[i];
    return any != 0;
}

/* Sanity checks only: matching successful reads do not establish manufacture
 * uniqueness/provenance. Raw UID stays local; this function never exports it.
 */
static inline int ota_uid_reads_valid(const uint8_t *a, const uint8_t *b, size_t n,
                                      int first_status, int second_status)
{
    unsigned i; uint8_t different = 0, not_ff = 0;
    if (!a || !b || n != OTA_UID_BYTES || first_status || second_status) return 0;
    for (i = 0; i < OTA_UID_BYTES; i++) {
        different |= a[i] ^ b[i];
        not_ff |= (uint8_t)(a[i] ^ 0xFFu);
    }
    return !different && not_ff && ota_id_nonzero(a, OTA_UID_BYTES);
}

static inline int ota_id_header(const uint8_t *p, uint8_t kind)
{
    return p && p[0] == 'X' && p[1] == 'U' && p[2] == 'I' && p[3] == 1u && p[4] == kind;
}

/* Caller must generate a fresh unpredictable nonce for EACH query. This is
 * freshness, not a new physical unit ID or authorization to write anything.
 * Input nonce and output buffer must not alias.
 */
static inline int ota_id_query(const uint8_t nonce[OTA_ID_NONCE_BYTES],
                               uint8_t *out, size_t capacity)
{
    unsigned i;
    if (!out || capacity < OTA_ID_QUERY_BYTES) return -1;
    for (i = 0; i < OTA_ID_QUERY_BYTES; i++) out[i] = 0;
    if (!ota_id_nonzero(nonce, OTA_ID_NONCE_BYTES)) return -1;
    out[0] = 'X'; out[1] = 'U'; out[2] = 'I'; out[3] = 1; out[4] = 'Q';
    for (i = 0; i < OTA_ID_NONCE_BYTES; i++) out[5u + i] = nonce[i];
    return (int)OTA_ID_QUERY_BYTES;
}

/* public_tag must be the SAME domain-separated cryptographic derivation of a
 * genuinely qualified factory UID in app and loader, never raw trim/key_mac,
 * model/location/JEDEC, CRC, random-per-boot bytes or a substituted provider.
 * No derivation/provider is implemented here. Default qualification=0 returns
 * UNAVAILABLE with a zero tag. Buffers must not alias.
 */
static inline int ota_id_reply(const uint8_t *query, size_t n, unsigned qualification,
                               const uint8_t public_tag[OTA_UID_BYTES], unsigned phase,
                               unsigned model, uint8_t *out, size_t capacity)
{
    unsigned i; int available;
    if (!out || capacity < OTA_ID_REPLY_BYTES) return -1;
    for (i = 0; i < OTA_ID_REPLY_BYTES; i++) out[i] = 0;
    if (n != OTA_ID_QUERY_BYTES || !ota_id_header(query, 'Q') ||
        !ota_id_nonzero(query + 5u, OTA_ID_NONCE_BYTES) ||
        (phase != OTA_ID_APP && phase != OTA_ID_LOADER) ||
        (model != OTA_ID_ELITE && model != OTA_ID_PRO)) return -1;
    available = qualification == OTA_ID_FACTORY_TAG && ota_id_nonzero(public_tag, OTA_UID_BYTES);
    out[0] = 'X'; out[1] = 'U'; out[2] = 'I'; out[3] = 1; out[4] = 'R';
    out[5] = available ? OTA_ID_OK : OTA_ID_UNAVAILABLE;
    out[6] = (uint8_t)phase; out[7] = (uint8_t)model;
    for (i = 0; i < OTA_ID_NONCE_BYTES; i++) out[8u + i] = query[5u + i];
    if (available)
        for (i = 0; i < OTA_UID_BYTES; i++) out[40u + i] = public_tag[i];
    return (int)OTA_ID_REPLY_BYTES;
}

/* Compare a read-only reply to an ALREADY trusted public tag and fresh nonce.
 * This is not initial enrollment, transport binding, session arming or a write
 * permit. It cannot authenticate legacy installed firmware or malicious peers.
 */
static inline int ota_id_matches(const uint8_t *reply, size_t n,
                                 const uint8_t nonce[OTA_ID_NONCE_BYTES],
                                 const uint8_t trusted_tag[OTA_UID_BYTES],
                                 unsigned phase, unsigned model)
{
    unsigned i; uint8_t mismatch = 0;
    if (n != OTA_ID_REPLY_BYTES || !ota_id_header(reply, 'R') || reply[5] != OTA_ID_OK ||
        (phase != OTA_ID_APP && phase != OTA_ID_LOADER) ||
        (model != OTA_ID_ELITE && model != OTA_ID_PRO) || reply[6] != phase || reply[7] != model ||
        !ota_id_nonzero(nonce, OTA_ID_NONCE_BYTES) || !ota_id_nonzero(trusted_tag, OTA_UID_BYTES)) return 0;
    for (i = 0; i < OTA_ID_NONCE_BYTES; i++) mismatch |= reply[8u + i] ^ nonce[i];
    for (i = 0; i < OTA_UID_BYTES; i++) mismatch |= reply[40u + i] ^ trusted_tag[i];
    return mismatch == 0;
}
#endif
