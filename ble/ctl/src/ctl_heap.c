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
    u32 *p = malloc(len + 4u);
    if (!p) return NULL;
    p[0] = len;
    return p + 1;
}
int  lbuf_free(void *lbuf) { if (lbuf) free((u32 *)lbuf - 1); return 0; }
int  lbuf_real_size(void *lbuf) { return lbuf ? (int)((u32 *)lbuf)[-1] : 0; }
void lbuf_free_check(void *lbuf, u32 rets) { (void)lbuf; (void)rets; }
void lbuf_dump(struct lbuff_head *head) { (void)head; }
