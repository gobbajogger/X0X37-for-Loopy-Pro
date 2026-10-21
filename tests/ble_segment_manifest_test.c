/* SPDX-License-Identifier: MPL-2.0 */
/* Host-only regression model: no device access and no runtime profile selection. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/hal/x0x_ble_segment.h"

#define MODEL 0x0037E11Eu /* synthetic fixture model; app supplies its real compiled identity */
static uint8_t flash[0x100000], before[0x100000];
static uint32_t available[3], reads;
static xb4_manifest_t good, candidate;

static const uint8_t *read_piece(void *ctx, uint32_t off, uint32_t n)
{
    unsigned i;
    (void)ctx; ++reads;
    for (i = 0; i < 3u; ++i) {
        uint32_t base = xb4_segment_off(i);
        if (off >= base && off - base <= available[i] && n <= available[i] - (off - base))
            return flash + off;
    }
    return NULL;
}
static void seal(xb4_manifest_t *h) { h->manifest_crc32 = xb4_manifest_crc(h); }
static int check(const xb4_manifest_t *h) { return xb4_validate(h, sizeof *h, MODEL, read_piece, NULL); }
static void fixture(void)
{
    unsigned i, j;
    memset(flash, 0xA5, sizeof flash);
    memset(&good, 0, sizeof good);
    good.magic = XB4_MAGIC; good.version = XB4_VERSION; good.header_size = sizeof good;
    good.abi = XB4_ABI; good.model = MODEL; good.contract = XB4_CONTRACT;
    good.flags = XB4_CENTRAL_TEST; good.image_id = 1;
    good.init = XB4_XIP(XB4_PRIMARY_OFF) + sizeof good;
    good.poll = good.init + 4;
    good.ram_lo = XB4_LOW_RAM_LO; good.ram_hi = XB4_LOW_RAM_HI;
    good.isr_stack_lo = XB4_LOW_RAM_HI - 0x800; good.isr_stack_hi = XB4_LOW_RAM_HI;
    good.data_ram_lo = XB4_DATA_RAM_LO; good.data_ram_hi = XB4_DATA_RAM_HI;
    for (i = 0; i < 3; ++i) {
        uint32_t skip = i ? 0 : sizeof good;
        xb4_segment_t *s = &good.segment[i];
        s->off = xb4_segment_off(i); s->size = i ? 64 : 256;
        s->exec_size = s->size - 16;
        available[i] = s->size;
        for (j = skip; j < s->size; ++j) flash[s->off + j] = (uint8_t)(j + 17 * i);
        s->crc32 = xb4_crc32(flash + s->off + skip, s->size - skip);
    }
    seal(&good); memcpy(flash + XB4_PRIMARY_OFF, &good, sizeof good);
    candidate = good; reads = 0;
}
static int boot_from_flash(void)
{
    xb4_manifest_t snapshot;
    memcpy(&snapshot, flash + XB4_PRIMARY_OFF, sizeof snapshot);
    return check(&snapshot);
}
static void field_errors(void)
{
    uint32_t i;
    fixture(); assert(check(&good) == XB4_OK && reads == 3);
    assert(xb4_crc32((const uint8_t *)"123456789", 9) == 0xCBF43926u);
    for (i = 0; i < sizeof good; ++i)
        assert(xb4_validate(&good, i, MODEL, read_piece, NULL) == XB4_E_HEADER);
    assert(xb4_validate(NULL, sizeof good, MODEL, read_piece, NULL) == XB4_E_HEADER);
    assert(xb4_validate(&good, sizeof good, 0, read_piece, NULL) == XB4_E_HEADER);
    assert(xb4_validate(&good, sizeof good, MODEL + 1, read_piece, NULL) == XB4_E_HEADER);
    assert(xb4_validate(&good, sizeof good, MODEL, NULL, NULL) == XB4_E_HEADER);
#define BAD(field, value, error) do { candidate = good; candidate.field = (value); seal(&candidate); reads = 0; assert(check(&candidate) == (error)); assert(reads == 0); } while (0)
    BAD(magic, 0x43423058u, XB4_E_HEADER); /* legacy ABI-2 magic */
    BAD(version, 2, XB4_E_HEADER); BAD(header_size, 124, XB4_E_HEADER);
    BAD(abi, 2, XB4_E_HEADER); BAD(abi, 3, XB4_E_HEADER);
    BAD(contract, 0, XB4_E_HEADER); BAD(flags, 0, XB4_E_HEADER);
    BAD(flags, 3, XB4_E_HEADER); BAD(image_id, 0, XB4_E_HEADER);
    BAD(reserved[0], 1, XB4_E_HEADER); BAD(reserved[1], 1, XB4_E_HEADER);
    for (i = 0; i < 3; ++i) {
        BAD(segment[i].off, 0x4000, XB4_E_SEGMENT);
        BAD(segment[i].off, 0xC0000, XB4_E_SEGMENT);
        BAD(segment[i].off, 0xFF000, XB4_E_SEGMENT);
        BAD(segment[i].size, 0, XB4_E_SEGMENT);
        BAD(segment[i].size, 0xFFFFFFFFu, XB4_E_SEGMENT);
        BAD(segment[i].size, xb4_segment_capacity(i) + 4, XB4_E_SEGMENT);
        BAD(segment[i].size, 63, XB4_E_SEGMENT);
        BAD(segment[i].exec_size, good.segment[i].size + 4, XB4_E_SEGMENT);
    }
    BAD(segment[0].exec_size, sizeof good, XB4_E_SEGMENT);
    BAD(init, XB4_XIP(XB4_PRIMARY_OFF), XB4_E_ENTRY);
    BAD(init, good.init + 1, XB4_E_ENTRY);
    BAD(init, XB4_XIP(XB4_SECOND_OFF), XB4_E_ENTRY);
    BAD(poll, XB4_DATA_RAM_LO, XB4_E_ENTRY);
    BAD(poll, XB4_XIP(XB4_PRIMARY_OFF) + good.segment[0].exec_size, XB4_E_ENTRY);
    BAD(ram_lo, XB4_LOW_RAM_LO - 16, XB4_E_RAM);
    BAD(ram_hi, XB4_LOW_RAM_HI + 16, XB4_E_RAM);
    BAD(isr_stack_lo, XB4_DATA_RAM_LO, XB4_E_RAM);
    BAD(isr_stack_hi, XB4_DATA_RAM_HI, XB4_E_RAM);
    BAD(isr_stack_lo, good.isr_stack_hi, XB4_E_RAM);
    BAD(isr_stack_hi, good.isr_stack_hi - 1, XB4_E_RAM);
    BAD(data_ram_lo, XB4_DATA_RAM_LO - 16, XB4_E_RAM);
    BAD(data_ram_hi, 0x01C7FD50u, XB4_E_RAM);
    BAD(data_ram_hi, XB4_DATA_RAM_LO - 16, XB4_E_RAM);
    {
        const unsigned forbidden[] = {1, 11, 16, 20, 24, 37, 62, 63};
        for (i = 0; i < sizeof forbidden / sizeof forbidden[0]; ++i) {
            unsigned irq = forbidden[i];
            if (irq < 32) { BAD(irq_lo, 1u << irq, XB4_E_IRQ); }
            else { BAD(irq_hi, 1u << (irq - 32u), XB4_E_IRQ); }
        }
    }
#undef BAD
    candidate = good; candidate.segment[1].crc32 ^= 1;
    assert(check(&candidate) == XB4_E_MANIFEST_CRC);
    for (i = 0; i < sizeof good; ++i) {
        candidate = good; ((uint8_t *)&candidate)[i] ^= 1;
        assert(check(&candidate) != XB4_OK);
    }
    assert(xb4_executable(&good, good.init));
    assert(xb4_executable(&good, good.init + 2u)); /* pi32v2 instructions are 2-byte aligned */
    assert(!xb4_executable(&good, good.init + 1u));
    assert(xb4_executable(&good, XB4_XIP(XB4_SECOND_OFF)));
    assert(xb4_executable(&good, XB4_XIP(XB4_SECOND_OFF) + 2u));
    candidate = good; candidate.init += 2u; seal(&candidate);
    assert(check(&candidate) == XB4_OK);
    assert(!xb4_executable(&good, XB4_XIP(XB4_SECOND_OFF) + good.segment[1].exec_size));
    assert(!xb4_executable(&good, XB4_DATA_RAM_LO));
}
static void payload_errors(void)
{
    unsigned i, j;
    fixture();
    for (i = 0; i < 3; ++i) {
        uint32_t skip = i ? 0 : sizeof good;
        for (j = 0; j < good.segment[i].size; ++j) {
            available[i] = j; assert(check(&good) == XB4_E_MISSING);
        }
        available[i] = good.segment[i].size;
        for (j = skip; j < good.segment[i].size; ++j) {
            flash[good.segment[i].off + j] ^= 0x80;
            assert(check(&good) == XB4_E_PAYLOAD_CRC);
            flash[good.segment[i].off + j] ^= 0x80;
        }
    }
    /* Valid manifest of another build cannot bless stale secondary bytes. */
    candidate = good; candidate.image_id = 2; candidate.segment[1].crc32 ^= 1; seal(&candidate);
    assert(check(&candidate) == XB4_E_PAYLOAD_CRC);
}
static void range_and_store(void)
{
    uint32_t off, i;
    fixture(); memcpy(before, flash, sizeof flash);
    for (i = 0; i < 3; ++i) {
        uint32_t base = xb4_segment_off(i), cap = xb4_segment_capacity(i);
        assert(xb4_range_ok(i, base, cap)); assert(xb4_erase_ok(i, base, cap));
        assert(!xb4_range_ok(i, base, 0)); assert(!xb4_range_ok(i, base - 1, 1));
        assert(!xb4_range_ok(i, base + cap, 1)); assert(!xb4_range_ok(i, base, cap + 1));
        assert(!xb4_range_ok(i, base, 0xFFFFFFFFu)); assert(!xb4_erase_ok(i, base + 1, 0x1000));
        assert(!xb4_erase_ok(i, base, 1));
    }
    assert(!xb4_range_ok(3, XB4_THIRD_OFF, 1));
    assert(!xb4_payload_write_ok(0, XB4_PRIMARY_OFF, sizeof good));
    assert(xb4_payload_write_ok(0, XB4_PRIMARY_OFF + sizeof good, 4));
    /* Unknown/foreign owner migration model, using the actual proposed guard. */
    for (off = 0; off < sizeof flash; off += 0x1000)
        if (xb4_owner_erase_ok(off)) memset(flash + off, 0xFF, 0x1000);
    for (off = 0; off < sizeof flash; ++off)
        if (!(off >= 0x9D000 && off < 0xAB000) && !(off >= 0xAF000 && off < 0xB0000))
            assert(flash[off] == before[off]);
    assert(!xb4_owner_erase_ok(0x9D001));
    assert(boot_from_flash() == XB4_OK); /* secondaries survived migration */
}
static void transaction(void)
{
    uint32_t i, n;
    xb4_manifest_t new_manifest;
    fixture(); memcpy(before, flash, sizeof flash);
    new_manifest = good; new_manifest.image_id = 2;
    /* Create a distinct complete new set. */
    for (i = 0; i < 3; ++i) {
        uint32_t skip = i ? 0 : sizeof good;
        for (n = skip; n < good.segment[i].size; ++n)
            flash[good.segment[i].off + n] ^= 0x5A;
        new_manifest.segment[i].crc32 = xb4_crc32(flash + good.segment[i].off + skip,
                                                 good.segment[i].size - skip);
    }
    seal(&new_manifest);
    memcpy(flash + XB4_PRIMARY_OFF, &new_manifest, sizeof new_manifest);
    assert(boot_from_flash() == XB4_OK);
    /* Publication prefixes: every torn manifest is refused. */
    for (n = 0; n < sizeof new_manifest; ++n) {
        memset(flash + XB4_PRIMARY_OFF, 0xFF, sizeof new_manifest);
        memcpy(flash + XB4_PRIMARY_OFF, &new_manifest, n);
        assert(boot_from_flash() != XB4_OK);
    }
    /* After every changed secondary byte, the old manifest refuses the mixed set.
     * Before the first change the old set remains valid. */
    memcpy(flash, before, sizeof flash);
    assert(boot_from_flash() == XB4_OK);
    assert(check(&new_manifest) == XB4_E_PAYLOAD_CRC); /* publishing early cannot admit old payloads */
    for (i = 1; i < 3; ++i)
        for (n = 0; n < good.segment[i].size; ++n) {
            flash[good.segment[i].off + n] ^= 0x5A;
            assert(boot_from_flash() != XB4_OK);
            assert(check(&new_manifest) == XB4_E_PAYLOAD_CRC); /* primary still old */
        }
    /* Erased primary and partial payload still cannot start. */
    memset(flash + XB4_PRIMARY_OFF, 0xFF, 0x1000);
    assert(boot_from_flash() != XB4_OK);
    for (n = sizeof good; n < good.segment[0].size; ++n) {
        flash[XB4_PRIMARY_OFF + n] = before[XB4_PRIMARY_OFF + n] ^ 0x5A;
        assert(boot_from_flash() != XB4_OK);
    }
    memcpy(flash + XB4_PRIMARY_OFF, &new_manifest, sizeof new_manifest);
    assert(boot_from_flash() == XB4_OK);
    for (n = 0; n < sizeof flash; ++n)
        if (!xb4_range_ok(0, n, 1) && !xb4_range_ok(1, n, 1) && !xb4_range_ok(2, n, 1))
            assert(flash[n] == before[n]); /* samples, calibration, keys and rollback data unchanged */
    /* Old owner erasure after rollback destroys segment 1. A new app must reject
     * leftovers; rollback must restore the matching old app/controller pair. */
    memset(flash + XB4_SECOND_OFF, 0xFF, XB4_SECOND_SIZE);
    assert(boot_from_flash() == XB4_E_PAYLOAD_CRC);
}
int main(void)
{
    field_errors(); payload_errors(); range_and_store(); transaction();
    puts("segmented manifest: header/range/CRC/entry/RAM/store/transaction checks PASS");
    return 0;
}
