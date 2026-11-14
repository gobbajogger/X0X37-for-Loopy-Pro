/* SPDX-License-Identifier: GPL-3.0-only */
#include "recovery_diagnostic_core.h"

/* Format reference: m-bootloader core.c at
 * 3ac7651a019fa44daba061b05654278496477b64, lines 93-102 and 564-599.
 * No upstream runtime/HAL code is linked or called. */
static bool same_bytes(const uint8_t *a, const uint8_t *b, size_t length)
{
    size_t i;
    for (i = 0; i < length; ++i)
        if (a[i] != b[i])
            return false;
    return true;
}

static bool range_fits(uint32_t capacity, uint32_t address, uint32_t length)
{
    return address <= capacity && length <= capacity - address;
}

static bool metadata_valid(const struct mbrd_input *in)
{
    uint32_t backup;
    if (!in || in->trim.read_status != MBRD_READ_EXACT ||
        in->backup.read_status != MBRD_READ_EXACT ||
        !in->trim.bytes || !in->backup.bytes ||
        in->trim.length != MBRD_TRIM_SIZE ||
        in->backup.length != MBRD_BACKUP_SIZE)
        return false;
    if (in->capacity != MBRD_CAPACITY || in->sector_size != MBRD_SECTOR_SIZE ||
        in->trim_address != MBRD_TRIM_ADDRESS || in->capacity < in->sector_size)
        return false;
    backup = in->capacity - in->sector_size;
    if (in->backup_address != backup ||
        !range_fits(in->capacity, in->trim_address, in->sector_size) ||
        !range_fits(in->capacity, backup, in->sector_size))
        return false;
    /* Overlap check without forming potentially overflowing end addresses. */
    if (backup < UINT32_C(0xc2000) &&
        (backup >= MBRD_TRIM_ADDRESS || in->sector_size > MBRD_TRIM_ADDRESS - backup))
        return false;
    return true;
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint16_t body_crc(const uint8_t *bytes, size_t length)
{
    uint16_t crc = 0;
    size_t i;
    for (i = 0; i < length; ++i) {
        unsigned bit;
        for (bit = 0; bit < 8; ++bit) {
            unsigned feedback = ((unsigned)(crc >> 15) ^
                                 ((unsigned)bytes[i] >> (7u - bit))) & 1u;
            crc = (uint16_t)(crc << 1);
            if (feedback)
                crc ^= UINT16_C(0x1021);
        }
    }
    return crc;
}

bool mbrd_platform_supported(void)
{
    return false;
}

struct mbrd_result mbrd_diagnose(const struct mbrd_input *in)
{
    static const uint8_t signature[8] = {'T','R','I','M','C','O','P','Y'};
    static const uint8_t marker[8] = {'t','r','i','m','d','a','t',0};
    struct mbrd_result result = {0};
    const uint8_t *backup;
    size_t i;
    bool empty = true;
    if (!metadata_valid(in))
        return result;
    backup = in->backup.bytes;
    result.status = MBRD_DIAGNOSED;
    result.current_marker = same_bytes(in->trim.bytes + MBRD_MARKER_OFFSET,
                                       marker, sizeof marker)
                          ? MBRD_MARKER_PRESENT : MBRD_MARKER_ABSENT;
    result.backup_signature_matches = same_bytes(backup, signature, sizeof signature);
    result.backup_address_matches = le32(backup + 8) == MBRD_TRIM_ADDRESS;
    result.backup_length_matches = le16(backup + 12) == MBRD_TRIM_SIZE;
    result.backup_marker_present = same_bytes(backup + 16 + MBRD_MARKER_OFFSET,
                                            marker, sizeof marker);
    result.backup_crc_matches = le16(backup + 14) == body_crc(backup + 16, MBRD_TRIM_SIZE);
    for (i = 0; i < MBRD_BACKUP_SIZE; ++i)
        if (backup[i] != UINT8_C(0xff)) {
            empty = false;
            break;
        }
    if (empty)
        result.backup = MBRD_BACKUP_EMPTY;
    else if (!result.backup_signature_matches)
        result.backup = MBRD_BACKUP_OCCUPIED;
    else if (!result.backup_address_matches || !result.backup_length_matches ||
             !result.backup_marker_present || !result.backup_crc_matches)
        result.backup = MBRD_BACKUP_INVALID;
    else
        result.backup = MBRD_BACKUP_FORMAT_VALID;
    /* physical_model and unit remain UNKNOWN for every classification. */
    return result;
}
