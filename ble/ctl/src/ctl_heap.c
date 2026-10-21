// SPDX-License-Identifier: MPL-2.0
// Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
/* Small first-fit allocator with neighbour merge, plus the lbuf API. */
#include "ctl_port.h"
#include "ctl_host_scope.h"


extern u8 __ctl_heap_start[], __ctl_heap_end[];

struct blk { u32 size; struct blk *next; }; /* size includes header; bit0 = used */
static struct blk *heap_first;
#if CTL_VENDOR_CPUPOOL_TEST
/* 4096 of the reviewed high region, leaving room for BSS and metadata.
 * Only the four audited CPU init allocations may enter this pool. */
static u8 cpu_pool[4096] CTL_HIGHCPU_BSS __attribute__((aligned(8)));
static struct blk *cpu_first CTL_HIGHCPU_BSS;
#endif
u32 ctl_heap_stat(void) { return 0; } /* image space is reserved for both LE roles */

void ctl_heap_init(void)
{
    heap_first = (struct blk *)(((uintptr_t)__ctl_heap_start + 7u) & ~7u);
    heap_first->size = ((u32)(__ctl_heap_end - (u8 *)heap_first)) & ~7u;
    heap_first->next = NULL;
#if CTL_VENDOR_CPUPOOL_TEST
    cpu_first = (struct blk *)cpu_pool;
    cpu_first->size = sizeof cpu_pool;
    cpu_first->next = NULL;
#endif
}

#if CTL_VENDOR_CPUPOOL_TEST
static void *heap_alloc(size_t n, int force_low)
#else
void *malloc(size_t n)
#endif
{
#if CTL_VENDOR_CPUPOOL_TEST
    int high = !force_low && ctl_scope_allocate(n);
#endif
#if CTL_VENDOR_HOST_TEST
    if (n > UINT32_MAX - sizeof(struct blk) - 7u) return NULL;
#endif
    u32 need = ((u32)n + sizeof(struct blk) + 7u) & ~7u;
    u32 f = ctl_port_irq_save();
#if CTL_VENDOR_CPUPOOL_TEST
    struct blk *first = high ? cpu_first : heap_first;
#endif
#if CTL_VENDOR_CPUPOOL_TEST
    for (struct blk *b = first; b; b = b->next) {
#else
    for (struct blk *b = heap_first; b; b = b->next) {
#endif
        if (b->size & 1u) continue;
        while (b->next && !(b->next->size & 1u)) {          /* merge free neighbours */
            b->size += b->next->size; b->next = b->next->next;
        }
        if (b->size < need) continue;
        if (b->size - need >= sizeof(struct blk) + 8u) {     /* split */
            struct blk *r = (struct blk *)((u8 *)b + need);
            r->size = b->size - need; r->next = b->next;
            b->size = need; b->next = r;
        }
        b->size |= 1u;
        ctl_port_irq_restore(f);
        return b + 1;
    }
    ctl_port_irq_restore(f);
#if CTL_VENDOR_CPUPOOL_TEST
    if (high) ctl_scope_oom(n);
#endif
    return NULL;
}
#if CTL_VENDOR_CPUPOOL_TEST
void *malloc(size_t n) { return heap_alloc(n, 0); }
#endif

void free(void *p)
{
    if (!p) return;
    u32 f = ctl_port_irq_save();
#if CTL_VENDOR_CPUPOOL_TEST
    uintptr_t address = (uintptr_t)p;
    struct blk *first = (address >= (uintptr_t)cpu_pool &&
                         address < (uintptr_t)cpu_pool + sizeof cpu_pool) ? cpu_first : heap_first;
    /* Validate exact allocation boundaries, not merely membership in RAM. */
    struct blk *match = NULL;
    for (struct blk *b = first; b; b = b->next)
        if ((void *)(b + 1) == p && (b->size & 1u)) { match = b; break; }
    if (!match) { ctl_port_irq_restore(f); ctl_scope_oom(0); return; }
    match->size &= ~1u;
#else
    ((struct blk *)p - 1)->size &= ~1u;
#endif
    ctl_port_irq_restore(f);
}

void *zalloc(size_t n) { void *p = malloc(n); if (p) memset(p, 0, n); return p; }
#if CTL_VENDOR_CPUPOOL_TEST
void *ram_malloc(unsigned long n) { return heap_alloc(n, 1); }
#else
void *ram_malloc(unsigned long n) { return malloc(n); }
#endif
void ram_free(void *p) { free(p); }

/* lbuf: the library passes its own lbuff_head. This image keeps every
 * lbuf packet in the common heap and stores the length in front. */
struct lbuff_head;
void *lbuf_alloc(struct lbuff_head *head, u32 len)
{
    (void)head;
#if CTL_VENDOR_HOST_TEST
    if (len > UINT32_MAX - 4u) return NULL;
#endif
#if CTL_VENDOR_CPUPOOL_TEST
    u32 *p = heap_alloc(len + 4u, 1);
#else
    u32 *p = malloc(len + 4u);
#endif
    if (!p) return NULL;
    p[0] = len;
    return p + 1;
}
int  lbuf_free(void *lbuf) { if (lbuf) free((u32 *)lbuf - 1); return 0; }
int  lbuf_real_size(void *lbuf) { return lbuf ? (int)((u32 *)lbuf)[-1] : 0; }
void lbuf_free_check(void *lbuf, u32 rets) { (void)lbuf; (void)rets; }
void lbuf_dump(struct lbuff_head *head) { (void)head; }

#if CTL_VENDOR_HOST_TEST
/* Exact SDK lbuf_realloc is shrink-only, keeps the address, and returns NULL
 * without freeing on an equal-size request. Adapt that contract to this shim's
 * own allocation header; never link the SDK realloc against our different lbuf
 * layout. A genuine split releases the tail; no caller-size/zeroing deception. */
void *lbuf_realloc(void *packet, u32 len)
{
    if (!packet) return NULL;
    u32 *size = (u32 *)packet - 1;
    u32 f = ctl_port_irq_save();
    if (len >= *size) { ctl_port_irq_restore(f); return NULL; }
    struct blk *b = (struct blk *)size - 1;
    u32 need = (len + 4u + sizeof(struct blk) + 7u) & ~7u;
    u32 old = b->size & ~1u;
    if (old - need >= sizeof(struct blk) + 8u) {
        struct blk *tail = (struct blk *)((u8 *)b + need);
        tail->size = old - need;
        tail->next = b->next;
        b->next = tail;
        b->size = need | 1u;
    }
    *size = len;
    ctl_port_irq_restore(f);
    return packet;
}
#endif
