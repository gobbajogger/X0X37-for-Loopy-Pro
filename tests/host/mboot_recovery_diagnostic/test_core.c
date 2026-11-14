/* SPDX-License-Identifier: GPL-3.0-only */
#include "recovery_diagnostic_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t trim[MBRD_TRIM_SIZE], backup[MBRD_BACKUP_SIZE];
static const uint8_t marker[8] = {'t','r','i','m','d','a','t',0};
static unsigned cases;

/* Independent polynomial-division fixture oracle. Production uses bit feedback.
 * CRC-16/XMODEM: polynomial 0x1021, init 0, no reflection or final XOR. */
static uint16_t fixture_crc(const uint8_t *bytes, size_t n)
{
    uint32_t remainder = 0;
    size_t i;
    for (i = 0; i < n; ++i) {
        int shift;
        remainder = (remainder << 8) ^ ((uint32_t)bytes[i] << 16);
        for (shift = 7; shift >= 0; --shift)
            if (remainder & (UINT32_C(1) << (unsigned)(16 + shift)))
                remainder ^= UINT32_C(0x11021) << (unsigned)shift;
    }
    return (uint16_t)remainder;
}

static void set_crc(void)
{
    uint16_t crc = fixture_crc(backup + 16, MBRD_TRIM_SIZE);
    backup[14] = (uint8_t)crc;
    backup[15] = (uint8_t)(crc >> 8);
}

static struct mbrd_input fixture(void)
{
    struct mbrd_input in = {
        {trim, sizeof trim, MBRD_READ_EXACT},
        {backup, sizeof backup, MBRD_READ_EXACT},
        MBRD_CAPACITY, MBRD_SECTOR_SIZE, MBRD_TRIM_ADDRESS, MBRD_BACKUP_ADDRESS
    };
    size_t i;
    memset(trim, 0, sizeof trim);
    memset(backup, 0, sizeof backup);
    memcpy(backup, "TRIMCOPY", 8);
    backup[8] = 0; backup[9] = 0x10; backup[10] = 0x0c; backup[11] = 0;
    backup[12] = 0x2c; backup[13] = 0x02;
    for (i = 0; i < MBRD_TRIM_SIZE; ++i)
        backup[16 + i] = (uint8_t)(i * 37u + 19u);
    memcpy(backup + 16 + MBRD_MARKER_OFFSET, marker, sizeof marker);
    set_crc();
    return in;
}

static void unknown_identity(struct mbrd_result r)
{
    assert(r.physical_model == MBRD_IDENTITY_UNKNOWN);
    assert(r.unit == MBRD_IDENTITY_UNKNOWN);
    assert(!mbrd_platform_supported());
}

static void fault(const struct mbrd_input *in)
{
    uint8_t trim_before[sizeof trim], backup_before[sizeof backup];
    unsigned char input_before[sizeof(struct mbrd_input)];
    struct mbrd_result r;
    memcpy(trim_before, trim, sizeof trim);
    memcpy(backup_before, backup, sizeof backup);
    if (in)
        memcpy(input_before, in, sizeof *in);
    r = mbrd_diagnose(in);
    assert(r.status == MBRD_FAULT);
    assert(r.current_marker == MBRD_MARKER_UNKNOWN);
    assert(r.backup == MBRD_BACKUP_UNKNOWN);
    assert(!r.backup_signature_matches && !r.backup_address_matches);
    assert(!r.backup_length_matches && !r.backup_marker_present && !r.backup_crc_matches);
    unknown_identity(r);
    assert(!memcmp(trim_before, trim, sizeof trim));
    assert(!memcmp(backup_before, backup, sizeof backup));
    if (in)
        assert(!memcmp(input_before, in, sizeof *in));
    ++cases;
}

static struct mbrd_result diagnosed(struct mbrd_input *in, enum mbrd_backup expected,
                                    enum mbrd_marker expected_marker)
{
    uint8_t trim_before[sizeof trim], backup_before[sizeof backup];
    unsigned char input_before[sizeof *in];
    struct mbrd_result a, b;
    memcpy(trim_before, trim, sizeof trim);
    memcpy(backup_before, backup, sizeof backup);
    memcpy(input_before, in, sizeof *in);
    a = mbrd_diagnose(in);
    b = mbrd_diagnose(in);
    assert(a.status == MBRD_DIAGNOSED && b.status == a.status);
    assert(a.backup == expected && b.backup == a.backup);
    assert(a.current_marker == expected_marker && b.current_marker == a.current_marker);
    assert(a.backup_signature_matches == b.backup_signature_matches);
    assert(a.backup_address_matches == b.backup_address_matches);
    assert(a.backup_length_matches == b.backup_length_matches);
    assert(a.backup_marker_present == b.backup_marker_present);
    assert(a.backup_crc_matches == b.backup_crc_matches);
    unknown_identity(a); unknown_identity(b);
    assert(!memcmp(trim_before, trim, sizeof trim));
    assert(!memcmp(backup_before, backup, sizeof backup));
    assert(!memcmp(input_before, in, sizeof *in));
    ++cases;
    return a;
}

static void state_cases(void)
{
    struct mbrd_input in = fixture();
    struct mbrd_result r;
    size_t i;
    r = diagnosed(&in, MBRD_BACKUP_FORMAT_VALID, MBRD_MARKER_ABSENT);
    assert(r.backup_signature_matches && r.backup_address_matches && r.backup_length_matches);
    assert(r.backup_marker_present && r.backup_crc_matches);
    memcpy(trim + MBRD_MARKER_OFFSET, marker, sizeof marker);
    diagnosed(&in, MBRD_BACKUP_FORMAT_VALID, MBRD_MARKER_PRESENT);
    /* Seven visible letters without the required zero are not a marker. */
    trim[MBRD_MARKER_OFFSET + 7] = 1;
    diagnosed(&in, MBRD_BACKUP_FORMAT_VALID, MBRD_MARKER_ABSENT);

    in = fixture(); memset(backup, 0xff, sizeof backup);
    diagnosed(&in, MBRD_BACKUP_EMPTY, MBRD_MARKER_ABSENT);
    for (i = 0; i < sizeof backup; ++i) {
        backup[i] = 0;
        diagnosed(&in, MBRD_BACKUP_OCCUPIED, MBRD_MARKER_ABSENT);
        backup[i] = 0xff;
    }
    /* Wrong signature outranks otherwise valid header/body/CRC. */
    in = fixture(); backup[0] ^= 1;
    r = diagnosed(&in, MBRD_BACKUP_OCCUPIED, MBRD_MARKER_ABSENT);
    assert(!r.backup_signature_matches && r.backup_crc_matches);
    /* Interrupted-looking signature is OCCUPIED, never a partial-read result. */
    memset(backup, 0xff, sizeof backup); memcpy(backup, "TRIM", 4);
    diagnosed(&in, MBRD_BACKUP_OCCUPIED, MBRD_MARKER_ABSENT);

    in = fixture(); backup[8] ^= 1;
    r = diagnosed(&in, MBRD_BACKUP_INVALID, MBRD_MARKER_ABSENT);
    assert(!r.backup_address_matches && r.backup_crc_matches);
    in = fixture(); backup[12] ^= 1;
    r = diagnosed(&in, MBRD_BACKUP_INVALID, MBRD_MARKER_ABSENT);
    assert(!r.backup_length_matches && r.backup_crc_matches);
    in = fixture(); backup[16 + MBRD_MARKER_OFFSET + 7] = 1; set_crc();
    r = diagnosed(&in, MBRD_BACKUP_INVALID, MBRD_MARKER_ABSENT);
    assert(!r.backup_marker_present && r.backup_crc_matches);
    in = fixture(); backup[14] ^= 1;
    memcpy(trim + MBRD_MARKER_OFFSET, marker, sizeof marker);
    r = diagnosed(&in, MBRD_BACKUP_INVALID, MBRD_MARKER_PRESENT);
    assert(!r.backup_crc_matches && r.backup_marker_present);
    /* Each header-field byte matters, including high address/CRC bytes. */
    for (i = 8; i < 16; ++i) {
        in = fixture(); backup[i] ^= 1;
        diagnosed(&in, MBRD_BACKUP_INVALID, MBRD_MARKER_ABSENT);
    }
    for (i = 0; i < 8; ++i) {
        in = fixture(); backup[i] ^= 1;
        diagnosed(&in, MBRD_BACKUP_OCCUPIED, MBRD_MARKER_ABSENT);
        in = fixture(); memcpy(trim + MBRD_MARKER_OFFSET, marker, sizeof marker);
        trim[MBRD_MARKER_OFFSET + i] ^= 1;
        diagnosed(&in, MBRD_BACKUP_FORMAT_VALID, MBRD_MARKER_ABSENT);
    }
    /* Every single-byte body corruption must fail its stored CRC. */
    for (i = 0; i < MBRD_TRIM_SIZE; ++i) {
        in = fixture(); backup[16 + i] ^= 1;
        r = diagnosed(&in, MBRD_BACKUP_INVALID, MBRD_MARKER_ABSENT);
        assert(!r.backup_crc_matches);
    }
}

static void view_fault_cases(void)
{
    static const enum mbrd_read_status bad_status[] = {
        MBRD_READ_UNKNOWN, MBRD_READ_SHORT, MBRD_READ_ERROR,
        (enum mbrd_read_status)-1, (enum mbrd_read_status)99
    };
    static const size_t trim_lengths[] = {0, 1, MBRD_TRIM_SIZE - 1, MBRD_TRIM_SIZE + 1, SIZE_MAX};
    static const size_t backup_lengths[] = {0, 1, MBRD_BACKUP_SIZE - 1, MBRD_BACKUP_SIZE + 1, SIZE_MAX};
    struct mbrd_input in;
    size_t i;
    fault(NULL);
    in = fixture(); in.trim.bytes = NULL; fault(&in);
    in = fixture(); in.backup.bytes = NULL; fault(&in);
    for (i = 0; i < sizeof bad_status / sizeof bad_status[0]; ++i) {
        in = fixture(); in.trim.read_status = bad_status[i];
        in.trim.bytes = NULL; in.backup.bytes = NULL; fault(&in);
        in = fixture(); in.backup.read_status = bad_status[i];
        in.trim.bytes = NULL; in.backup.bytes = NULL; fault(&in);
        /* Fully available, otherwise valid bytes cannot override read failure. */
        in = fixture(); in.trim.read_status = bad_status[i]; fault(&in);
        in = fixture(); in.backup.read_status = bad_status[i]; fault(&in);
    }
    for (i = 0; i < sizeof trim_lengths / sizeof trim_lengths[0]; ++i) {
        in = fixture(); in.trim.length = trim_lengths[i]; fault(&in);
        in = fixture(); in.backup.length = backup_lengths[i]; fault(&in);
    }
    /* A non-null inaccessible sentinel is never read on metadata rejection. */
    in = fixture(); in.trim.bytes = (const uint8_t *)(uintptr_t)1;
    in.backup.bytes = (const uint8_t *)(uintptr_t)1;
    in.trim.read_status = MBRD_READ_SHORT; fault(&in);
    in.trim.read_status = MBRD_READ_EXACT; in.backup.length = 0; fault(&in);
}

static void geometry_fault_cases(void)
{
    static const uint32_t bad[] = {0, 1, UINT32_C(0xfff), UINT32_C(0x200000), UINT32_MAX};
    struct mbrd_input in;
    size_t i;
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        in = fixture(); in.capacity = bad[i]; fault(&in);
        in = fixture(); in.sector_size = bad[i]; fault(&in);
        in = fixture(); in.trim_address = bad[i]; fault(&in);
        in = fixture(); in.backup_address = bad[i]; fault(&in);
    }
    in = fixture(); in.backup_address = MBRD_TRIM_ADDRESS; fault(&in);
    in = fixture(); in.capacity = MBRD_SECTOR_SIZE - 1; fault(&in);
    in = fixture(); in.backup_address = MBRD_BACKUP_ADDRESS - 1; fault(&in);
    in = fixture(); in.trim.bytes = (const uint8_t *)(uintptr_t)1;
    in.backup.bytes = (const uint8_t *)(uintptr_t)1;
    in.capacity = UINT32_MAX; fault(&in);
}

int main(void)
{
    /* Known independent oracle vector guards a broken fixture generator. */
    assert(fixture_crc((const uint8_t *)"123456789", 9) == UINT16_C(0x31c3));
    assert(fixture_crc((const uint8_t *)"", 0) == 0);
    assert(!mbrd_platform_supported());
    state_cases();
    view_fault_cases();
    geometry_fault_cases();
    printf("synthetic recovery diagnostic: %u cases passed\n", cases);
    return 0;
}
