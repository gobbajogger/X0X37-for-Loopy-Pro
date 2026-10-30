/* SPDX-License-Identifier: GPL-3.0-only */
/* Allocation-lifetime replay, not RF emulation or a target execution claim.
 * Sizes are the audited pinned controller's allocations. No vendor code here. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define CTL_PORT_H
#define malloc replay_malloc
#define free replay_free
typedef uint8_t u8;
typedef uint32_t u32;
static u32 ctl_port_irq_save(void) { return 0; }
static void ctl_port_irq_restore(u32 f) { (void)f; }
__asm__(".bss\n.balign 8\n.global __ctl_heap_start\n__ctl_heap_start:\n.space 16384\n.global __ctl_heap_end\n__ctl_heap_end:\n.text\n");
#include "../../ble/ctl/src/ctl_heap.c"
#undef malloc
#undef free
_Static_assert(sizeof(struct blk)==sizeof(void*)+sizeof(uintptr_t),"unsupported allocator layout");
/* Preserve target block footprints on LP64. The host's minimum split is more
 * conservative (24 vs 16); payload is never accessed by this lifetime replay. */
static void *alloc_target(unsigned n)
{ assert(n>=sizeof(struct blk)-8u); return replay_malloc(n-(sizeof(struct blk)-8u)); }
static unsigned charge(unsigned n) { return (n+8u+7u)&~7u; }
static unsigned free_bytes(void)
{ unsigned n=0;for(struct blk*b=heap_first;b;b=b->next)if(!(b->size&1u))n+=b->size;return n; }
static unsigned largest(void)
{ unsigned n=0;for(struct blk*b=heap_first;b;b=b->next)if(!(b->size&1u)&&b->size>n)n=b->size;return n; }
static void start(unsigned heap,unsigned hw,unsigned credits)
{
 assert((uintptr_t)__ctl_heap_start <= UINT32_MAX);
 ctl_heap_init();heap_first->size=heap;heap_first->next=NULL;
 /* 27-byte ACL, five RX packets: target le_hw_init allocation formula. */
 unsigned radio=36u+644u*hw+(294u*5u+35u)+(294u*credits+35u);
 const unsigned fixed[]={2048,48,2560,56,512,radio};unsigned used=0;
 for(unsigned i=0;i<6;i++){assert(alloc_target(fixed[i]));used+=charge(fixed[i]);}
 assert(free_bytes()==heap-used);
}
static int connect(unsigned heap,unsigned credits,unsigned peers,int reserve)
{
 start(heap,TEST_CTL_HW_NUMS,credits);
 unsigned before=free_bytes();
 void *scan=alloc_target(32);assert(scan); /* retained scanner handle */
 void *links[2]={0},*hardware[2]={0};
 for(unsigned i=0;i<peers;i++) {
  void *init=alloc_target(32),*init_link=alloc_target(52);
  hardware[i]=alloc_target(40);
  if(!init||!init_link||!hardware[i])return 0;
  links[i]=alloc_target(416);
  if(!links[i])return 0;
  /* Initiation structs remain alive until after link creation. */
  if(reserve){void*p=alloc_target(504);if(!p)return 0;replay_free(p);}
  replay_free(init_link);replay_free(init);
 }
 /* Per-link teardown leaves the surviving peer intact; retry after free. */
 for(unsigned i=0;i<peers;i++){replay_free(links[i]);replay_free(hardware[i]);}
 replay_free(scan);
 assert(free_bytes()==before);
 /* Force normal lazy coalescing and check reuse. */
 void*p=alloc_target(416);assert(p);replay_free(p);
 assert(largest()>=charge(416));
 return 1;
}
int main(void)
{
#if TEST_CTL_HW_NUMS == 1
 /* One-peer recovery is the shipping controller. Exercise its real heap
  * allocation and recovery rather than silently marking the test not run. */
 assert(TEST_CTL_CREDITS == 5);
 for(unsigned i=0;i<100;i++)assert(connect(TEST_CTL_HEAP,TEST_CTL_CREDITS,1,1));
 puts("controller heap: shipping one-peer lifecycle x100 with transient reserve PASS");
#else
 assert(!connect(10112,5,1,0)); /* exact B cannot establish even first peer */
 start(10112,1,5); /* A's controller has room for the one supported peer. */
 assert(alloc_target(32)&&alloc_target(32)&&alloc_target(52)&&alloc_target(40)&&alloc_target(416));
 /* Half changes are not accepted as full two-peer resource closure. */
 assert(!connect(10624,5,2,1));
 assert(!connect(10112,3,2,1));
 for(unsigned i=0;i<100;i++)assert(connect(TEST_CTL_HEAP,TEST_CTL_CREDITS,2,1));
 puts("controller heap: A first-peer PASS; old B first-peer FAIL; both partial fixes FAIL reserve; corrected two-peer lifecycle x100 with 512-byte transient reserve PASS");
#endif
 return 0;
}
