/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 M. Geisler (genkigenki/makkuro) */
/* Candidate segmented interface for the explicitly selected SMC host TEST profile.
 * Not included by the default ABI-2 runtime. Admission does not prove memory fit,
 * initialise the host, grant flash-write authority, or make an image installable.
 * All addresses below are fixed format limits, not caller-controlled windows. */
#pragma once
#include <stddef.h>
#include <stdint.h>

#define XB4_MAGIC        0x53423058u /* "X0BS", deliberately unlike ABI-2 X0BC */
#define XB4_ABI          4u          /* ABI 3 was a discarded historical experiment */
#define XB4_VERSION      1u
#define XB4_CONTRACT     0x534D4302u /* revision 2: 92K primary, data-only secondaries, app UI excluded */
#define XB4_CENTRAL_TEST  1u
#define XB4_SEGMENTS      3u
#define XB4_PRIMARY_OFF   0x00084000u
#define XB4_PRIMARY_SIZE  0x00017000u
#define XB4_SECOND_OFF    0x000AD000u
#define XB4_SECOND_SIZE   0x00002000u
#define XB4_THIRD_OFF     0x000FE000u
#define XB4_THIRD_SIZE    0x00001000u
#define XB4_LOW_RAM_LO    0x01C01800u
#define XB4_LOW_RAM_HI    0x01C07F00u
#define XB4_DATA_RAM_LO   0x01C7D400u
#define XB4_DATA_RAM_HI   0x01C7F000u
#define XB4_XIP(off)      (0x02000000u + (uint32_t)(off) - 0x4000u)

/* size is the stored extent, including the manifest in segment 0. exec_size is
 * its leading executable extent; load-only data may follow. Segment 0's payload
 * CRC excludes the manifest; the manifest CRC covers every descriptor. Other
 * segment CRCs cover every stored byte. No CRC grants authenticity. */
typedef struct { uint32_t off, size, exec_size, crc32; } xb4_segment_t;
typedef struct {
    uint32_t magic;
    uint16_t version, header_size;
    uint32_t abi, model, contract, flags, image_id;
    uint32_t init, poll;
    uint32_t isr_stack_lo, isr_stack_hi;
    uint32_t ram_lo, ram_hi;              /* low RAM: includes every task stack */
    uint32_t data_ram_lo, data_ram_hi;    /* CPU-only data; never entry/ISR/stack */
    uint32_t irq_lo, irq_hi;
    xb4_segment_t segment[XB4_SEGMENTS];
    uint32_t manifest_crc32, reserved[2];
} xb4_manifest_t;
_Static_assert(sizeof(xb4_manifest_t) == 128u, "segmented manifest wire size");
_Static_assert(offsetof(xb4_manifest_t, manifest_crc32) == 116u, "manifest CRC offset");

/* read must return stable bytes for the complete requested range, or NULL.
 * Target caller must hold its radio/update exclusion across validation and start.
 * Plain XIP must be configured before reading secondaries. Tests provide bounded
 * buffers, so missing and truncated data follow the same failure path. */
typedef const uint8_t *(*xb4_read_fn)(void *ctx, uint32_t off, uint32_t size);
enum { XB4_OK = 0, XB4_E_HEADER, XB4_E_MANIFEST_CRC, XB4_E_SEGMENT,
       XB4_E_ENTRY, XB4_E_RAM, XB4_E_IRQ, XB4_E_MISSING, XB4_E_PAYLOAD_CRC };

static inline uint32_t xb4_segment_off(unsigned i)
{
    return i == 0u ? XB4_PRIMARY_OFF : i == 1u ? XB4_SECOND_OFF : XB4_THIRD_OFF;
}
static inline uint32_t xb4_segment_capacity(unsigned i)
{
    return i == 0u ? XB4_PRIMARY_SIZE : i == 1u ? XB4_SECOND_SIZE : XB4_THIRD_SIZE;
}
/* A narrow proposed helper; callers still need their own transaction/park checks.
 * Zero lengths, wrapping ranges, and crossing a segment boundary are rejected. */
static inline int xb4_range_ok(unsigned i, uint32_t off, uint32_t size)
{
    uint32_t lo, cap;
    if (i >= XB4_SEGMENTS || !size) return 0;
    lo = xb4_segment_off(i); cap = xb4_segment_capacity(i);
    return off >= lo && off - lo < cap && size <= cap - (off - lo);
}
static inline int xb4_erase_ok(unsigned i, uint32_t off, uint32_t size)
{
    return !(off & 0xFFFu) && !(size & 0xFFFu) && xb4_range_ok(i, off, size);
}
static inline int xb4_payload_write_ok(unsigned i, uint32_t off, uint32_t size)
{
    return xb4_range_ok(i, off, size) && (i != 0u || off >= XB4_PRIMARY_OFF + sizeof(xb4_manifest_t));
}

/* Foreign/unknown store ownership may erase actual object sectors and its owner
 * tag, never the former spare now occupied by segment 1. The default ABI-2 store
 * remains unchanged until the runtime owner selects the new profile explicitly. */
static inline int xb4_owner_erase_ok(uint32_t off)
{
    return !(off & 0xFFFu) && ((off >= 0x9D000u && off < 0xAB000u) || off == 0xAF000u);
}

static inline uint32_t xb4_crc_byte(uint32_t crc, uint8_t byte)
{
    unsigned bit;
    crc ^= byte;
    for (bit = 0; bit < 8u; ++bit)
        crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    return crc;
}
static inline uint32_t xb4_crc32(const uint8_t *bytes, uint32_t size)
{
    uint32_t crc = ~0u, i;
    for (i = 0; i < size; ++i) crc = xb4_crc_byte(crc, bytes[i]);
    return ~crc;
}
static inline uint32_t xb4_manifest_crc(const xb4_manifest_t *h)
{
    const uint8_t *p = (const uint8_t *)h;
    uint32_t crc = ~0u, i, pos = (uint32_t)offsetof(xb4_manifest_t, manifest_crc32);
    for (i = 0; i < sizeof *h; ++i)
        crc = xb4_crc_byte(crc, i >= pos && i < pos + 4u ? 0u : p[i]);
    return ~crc;
}
/* Use only after manifest admission. Intended also for the controller's IRQ
 * registration guard; no instruction may target load-only or high-RAM data. */
static inline int xb4_executable(const xb4_manifest_t *h, uint32_t address)
{
    uint32_t lo = XB4_XIP(XB4_PRIMARY_OFF);
    return !(address & 1u) && address >= lo + sizeof *h &&
           address - lo < h->segment[0].exec_size;
}

/* h points to a naturally aligned snapshot. Pass available before dereferencing
 * untrusted storage. expected_model comes from the app's compiled model guard. */
static inline int xb4_validate(const xb4_manifest_t *h, uint32_t available,
                               uint32_t expected_model, xb4_read_fn read, void *ctx)
{
    unsigned i;
    uint32_t primary_xip = XB4_XIP(XB4_PRIMARY_OFF);
    if (!h || available < sizeof *h || !expected_model || !read) return XB4_E_HEADER;
    if (h->magic != XB4_MAGIC || h->version != XB4_VERSION || h->header_size != sizeof *h ||
        h->abi != XB4_ABI || h->model != expected_model || h->contract != XB4_CONTRACT ||
        h->flags != XB4_CENTRAL_TEST || !h->image_id || h->reserved[0] || h->reserved[1]) return XB4_E_HEADER;
    if (xb4_manifest_crc(h) != h->manifest_crc32) return XB4_E_MANIFEST_CRC;
    for (i = 0; i < XB4_SEGMENTS; ++i) {
        const xb4_segment_t *s = &h->segment[i];
        if (s->off != xb4_segment_off(i) || !xb4_range_ok(i, s->off, s->size) ||
            (s->size & 3u) || (s->exec_size & 3u) || s->exec_size > s->size ||
            ((i != 0u && s->exec_size != 0u) || (i == 0u && s->exec_size < sizeof *h + 4u))) return XB4_E_SEGMENT;
    }
    /* Bootstrap entries stay in the verified primary executable payload. */
    if (!xb4_executable(h, h->init) || !xb4_executable(h, h->poll) ||
        h->init < primary_xip || h->poll < primary_xip ||
        h->init - primary_xip >= h->segment[0].exec_size ||
        h->poll - primary_xip >= h->segment[0].exec_size) return XB4_E_ENTRY;
    if (h->ram_lo != XB4_LOW_RAM_LO || h->ram_hi > XB4_LOW_RAM_HI ||
        h->isr_stack_lo < h->ram_lo || h->isr_stack_hi > h->ram_hi ||
        h->isr_stack_lo >= h->isr_stack_hi ||
        ((h->isr_stack_lo | h->isr_stack_hi | h->ram_hi) & 15u) ||
        h->data_ram_lo != XB4_DATA_RAM_LO || h->data_ram_hi < h->data_ram_lo ||
        h->data_ram_hi > XB4_DATA_RAM_HI || (h->data_ram_hi & 15u)) return XB4_E_RAM;
    /* Preserve every reserved IRQ in the existing ABI-2 ble_glue.h guard:
     * exception, ALNK0, SPI1, UART1, SARADC, SPI2, TIMER4 and TIMER5. */
    if ((h->irq_lo & ((1u << 1) | (1u << 11) | (1u << 16) | (1u << 20) | (1u << 24))) ||
        (h->irq_hi & ((1u << 5) | (1u << 30) | (1u << 31)))) return XB4_E_IRQ;
    for (i = 0; i < XB4_SEGMENTS; ++i) {
        const xb4_segment_t *s = &h->segment[i];
        uint32_t skip = i == 0u ? (uint32_t)sizeof *h : 0u;
        const uint8_t *p = read(ctx, s->off + skip, s->size - skip);
        if (!p) return XB4_E_MISSING;
        if (xb4_crc32(p, s->size - skip) != s->crc32) return XB4_E_PAYLOAD_CRC;
    }
    return XB4_OK;
}
