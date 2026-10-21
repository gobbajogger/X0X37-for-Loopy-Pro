/* SPDX-License-Identifier: GPL-3.0-only */
/* NON-INSTALLABLE study wrapper. Include after existing lz.c + st_crc32.
 * No address ownership, boot sequencing, cache sync or executable admission.
 * The runtime owner must supply fixed admitted source/destination ranges and
 * exclude radio/IRQ/flash writers throughout load/verify/execute transition. */
typedef struct { uint32_t packed_size, raw_size, packed_crc, raw_crc; } xvl_meta;
typedef struct { const uint8_t *p; uint32_t pos, n; } xvl_source;
static int xvl_get(void *ctx)
{
    xvl_source *s=ctx;
    return s->pos < s->n ? s->p[s->pos++] : -1;
}
__attribute__((section(".text.vendor_lz_probe"), noinline))
int vendor_lz_probe(const xvl_meta *m, const uint8_t *src, uint32_t available,
                    uint8_t *dst, uint32_t capacity)
{
    xvl_source input;
    lz_src_t stream;
    uintptr_t a=(uintptr_t)src,b=(uintptr_t)dst;
    if (!m || !src || !dst || !m->raw_size || !m->packed_size ||
        m->packed_size != available || m->raw_size != capacity ||
        available > UINTPTR_MAX-a || capacity > UINTPTR_MAX-b ||
        (a < b+capacity && b < a+available)) return -1;
    /* Do not write even a partial output from unverified compressed bytes. */
    if(st_crc32(src,available)!=m->packed_crc) return -1;
    input.p=src; input.pos=0; input.n=available;
    stream.get=xvl_get; stream.ctx=&input;
    if(lz_decompress(&stream,dst,capacity) || input.pos != available) return -1;
    return st_crc32(dst,capacity)==m->raw_crc ? 0 : -1;
}
