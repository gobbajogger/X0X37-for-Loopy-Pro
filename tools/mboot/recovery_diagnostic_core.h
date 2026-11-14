/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MBOOT_RECOVERY_DIAGNOSTIC_CORE_H
#define MBOOT_RECOVERY_DIAGNOSTIC_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Synthetic layout only. These constants establish no physical model/support. */
#define MBRD_CAPACITY UINT32_C(0x100000)
#define MBRD_SECTOR_SIZE UINT32_C(0x1000)
#define MBRD_TRIM_ADDRESS UINT32_C(0xc1000)
#define MBRD_BACKUP_ADDRESS UINT32_C(0xff000)
#define MBRD_TRIM_SIZE 0x22cu
#define MBRD_BACKUP_SIZE 0x23cu
#define MBRD_MARKER_OFFSET 0x224u

enum mbrd_read_status {
    MBRD_READ_UNKNOWN = 0, MBRD_READ_EXACT, MBRD_READ_SHORT, MBRD_READ_ERROR
};
enum mbrd_status { MBRD_FAULT = 0, MBRD_DIAGNOSED };
enum mbrd_marker { MBRD_MARKER_UNKNOWN = 0, MBRD_MARKER_ABSENT, MBRD_MARKER_PRESENT };
enum mbrd_backup {
    MBRD_BACKUP_UNKNOWN = 0, MBRD_BACKUP_EMPTY, MBRD_BACKUP_OCCUPIED,
    MBRD_BACKUP_INVALID, MBRD_BACKUP_FORMAT_VALID
};
/* No physical identity can be established by this unit. */
enum mbrd_identity { MBRD_IDENTITY_UNKNOWN = 0 };

struct mbrd_view {
    const uint8_t *bytes;
    size_t length;
    enum mbrd_read_status read_status;
};
struct mbrd_input {
    struct mbrd_view trim, backup;
    uint32_t capacity, sector_size, trim_address, backup_address;
};
struct mbrd_result {
    enum mbrd_status status;
    enum mbrd_marker current_marker;
    enum mbrd_backup backup;
    enum mbrd_identity physical_model, unit;
    bool backup_signature_matches;
    bool backup_address_matches;
    bool backup_length_matches;
    bool backup_marker_present;
    bool backup_crc_matches;
};

/* Permanently false: no platform/transport/device adapter or feature exists. */
bool mbrd_platform_supported(void);
/* Pure synthetic byte classification, returning no input pointer or raw bytes.
 * Input may be NULL, which returns FAULT. Each view must have EXACT status,
 * non-null bytes and exactly TRIM_SIZE/BACKUP_SIZE respectively, and geometry
 * must equal the fixed synthetic layout. All metadata is checked before any
 * view byte access. For an accepted view, the caller supplies readable bytes
 * for its entire length and must not mutate them concurrently. Views may alias.
 *
 * FAULT results have no valid observations (UNKNOWN states, false properties).
 * Otherwise backup precedence is EMPTY (all FF), OCCUPIED (nonempty and wrong
 * signature), INVALID (exact signature but any format predicate fails), then
 * FORMAT_VALID. Current marker presence is independent. FORMAT_VALID and marker
 * presence establish neither correct calibration, same-unit identity nor safety
 * of restoration. No result recommends an action or qualifies any hardware route.
 * No input/global data is modified or retained, and no callbacks are invoked. */
struct mbrd_result mbrd_diagnose(const struct mbrd_input *input);

#endif
