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

/* attempt 49: hci_controller_init (JieLi's btctrler.a) takes malloc(2560) and memsets all of it,
 * then hands only the first 512 B to lbuf_init_btctrler; nothing else reads the buffer (attempt 47
 * disassembly). Attempt 47 gave it 512 B up front and the memset wiped the next block headers
 * (attempt 48). So it gets the full 2560 B, and at the next malloc (ll_init's, after the memset and
 * lbuf_init_btctrler) the last 2040 B go back to the heap. Matched by size and caller only. */
#define CTL_HCI_BUF_ASKED 2560u
#define CTL_HCI_BUF_KEEP  512u
extern void hci_controller_init(void);
static struct blk *hci_buf_trim;

static void hci_buf_give_back(void)
{
    struct blk *b = hci_buf_trim;
    u32 keep = (CTL_HCI_BUF_KEEP + sizeof(struct blk) + 7u) & ~7u;
    u32 sz = b->size & ~1u;
    hci_buf_trim = NULL;
    if (!(b->size & 1u) || sz < keep + sizeof(struct blk) + 8u) return;
    struct blk *r = (struct blk *)((u8 *)b + keep);
    r->size = sz - keep; r->next = b->next;              /* free; malloc merges it with free neighbours */
    b->size = keep | 1u; b->next = r;
    heap_used -= sz - keep;
}

__attribute__((noinline)) void *malloc(size_t n)
{
    struct blk *mark = NULL;
    if (hci_buf_trim) {
        u32 f0 = ctl_port_irq_save();
        if (hci_buf_trim) hci_buf_give_back();
        ctl_port_irq_restore(f0);
    }
    if (n == CTL_HCI_BUF_ASKED) {
        uintptr_t ra = (uintptr_t)__builtin_return_address(0), fn = (uintptr_t)hci_controller_init;
        if (ra > fn && ra < fn + 0x100u) mark = (struct blk *)1;
    }
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
        if (mark) hci_buf_trim = b;
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
