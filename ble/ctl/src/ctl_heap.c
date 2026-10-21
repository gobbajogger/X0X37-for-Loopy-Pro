// SPDX-License-Identifier: MPL-2.0
// Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
/* Small first-fit allocator with neighbour merge, plus the lbuf API. */
#include "ctl_port.h"


extern u8 __ctl_heap_start[], __ctl_heap_end[];

struct blk { u32 size; struct blk *next; }; /* size includes header; bit0 = used */
static struct blk *heap_first;
u32 ctl_heap_stat(void) { return 0; } /* image space is reserved for both LE roles */

void ctl_heap_init(void)
{
    heap_first = (struct blk *)(((uintptr_t)__ctl_heap_start + 7u) & ~7u);
    heap_first->size = ((u32)(__ctl_heap_end - (u8 *)heap_first)) & ~7u;
    heap_first->next = NULL;
}

void *malloc(size_t n)
{
#if CTL_VENDOR_HOST_TEST
    if (n > UINT32_MAX - sizeof(struct blk) - 7u) return NULL;
#endif
    u32 need = ((u32)n + sizeof(struct blk) + 7u) & ~7u;
    u32 f = ctl_port_irq_save();
    for (struct blk *b = heap_first; b; b = b->next) {
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
    return NULL;
}

void free(void *p)
{
    if (!p) return;
    u32 f = ctl_port_irq_save();
    ((struct blk *)p - 1)->size &= ~1u;
    ctl_port_irq_restore(f);
}

void *zalloc(size_t n) { void *p = malloc(n); if (p) memset(p, 0, n); return p; }
void *ram_malloc(unsigned long n) { return malloc(n); }
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
    u32 *p = malloc(len + 4u);
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
