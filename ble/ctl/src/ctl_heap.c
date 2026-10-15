// SPDX-License-Identifier: MPL-2.0
// Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
/* Small first-fit allocator with neighbour merge, plus the lbuf API. */
#include "ctl_port.h"


extern u8 __ctl_heap_start[], __ctl_heap_end[];

struct blk { u32 size; struct blk *next; }; /* size includes header; bit0 = used */
static struct blk *heap_first;
static u32 heap_used, heap_peak, heap_fails, heap_fail_n;
/* for DIAG after a failure: peak bytes in use (bits 0-15), failed allocations (bits 16-23),
 * the last failed request in 16-byte units (bits 24-31, 255 = larger) */
u32 ctl_heap_stat(void)
{
    u32 n = heap_fail_n >> 4;
    return (heap_peak & 0xFFFFu) | (heap_fails > 255u ? 255u : heap_fails) << 16 | (n > 255u ? 255u : n) << 24;
}

/* the heap; ABI 3: and a second region the app lends (boot parameters heap2). A used 8-byte block
 * ends the first region, so a merge never joins the two. */
void ctl_heap_init(void)
{
    heap_first = (struct blk *)(((uintptr_t)__ctl_heap_start + 7u) & ~7u);
    heap_first->size = ((u32)(__ctl_heap_end - (u8 *)heap_first)) & ~7u;
    heap_first->next = NULL;
#ifdef CTL_HOST
    if (ctl_bp && ctl_bp->heap2 && ctl_bp->heap2_size >= 64u && !(ctl_bp->heap2 & 7u)) {
        struct blk *end = (struct blk *)((u8 *)heap_first + heap_first->size - 8u);
        struct blk *r2 = (struct blk *)(uintptr_t)ctl_bp->heap2;
        heap_first->size -= 8u;
        heap_first->next = end;
        end->size = 8u | 1u;              /* never free */
        end->next = r2;
        r2->size = ctl_bp->heap2_size & ~7u;
        r2->next = NULL;
    }
#endif
}

/* attempt 42 (DIAG): the free bytes in the main region (bits 0-15) and in heap2 (bits 16-31), and
 * the main region's size */
u32 ctl_heap_free2(void)
{
    u32 a = 0, b = 0, in2 = 0;
    for (struct blk *k = heap_first; k; k = k->next) {
        if (k->size == (8u | 1u)) { in2 = 1; continue; }        /* the end mark of the main region */
        if (!(k->size & 1u)) { if (in2) b += k->size; else a += k->size; }
    }
    return (a > 0xFFFFu ? 0xFFFFu : a) | (b > 0xFFFFu ? 0xFFFFu : b) << 16;
}
u32 ctl_heap_main_size(void) { return (u32)(__ctl_heap_end - __ctl_heap_start); }

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
        heap_used += b->size & ~1u;
        if (heap_used > heap_peak) heap_peak = heap_used;
        ctl_port_irq_restore(f);
        return b + 1;
    }
    heap_fails++;
    heap_fail_n = (u32)n;
    ctl_port_irq_restore(f);
    return NULL;
}

void free(void *p)
{
    if (!p) return;
    u32 f = ctl_port_irq_save();
    heap_used -= ((struct blk *)p - 1)->size & ~1u;
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
