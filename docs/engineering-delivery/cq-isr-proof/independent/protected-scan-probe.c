
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t irq_cfg, global_enabled;
static unsigned armed, site, drains, pending, hooks, seen[16], protected_probes;
static uint32_t expected_word;
static unsigned exact_deliveries;
#define FM1_IRQ_ALNK0 11u
#define FM1_ICFG(n) irq_cfg
#define AUDIO_BIT (1u << ((FM1_IRQ_ALNK0 & 7u) * 4u))
static void schedule(unsigned at);
static void dispatch(void);
static void fm1_irq_mask(uint32_t n) { FM1_ICFG(n) &= ~(1u << ((n & 7u) * 4u)); }
uint32_t plat_cq_lock(void)
{
    uint32_t bit = 1u << ((FM1_IRQ_ALNK0 & 7u) * 4u);
    uint32_t enabled = FM1_ICFG(FM1_IRQ_ALNK0) & bit; schedule(6);
    fm1_irq_mask(FM1_IRQ_ALNK0); schedule(7);
    /* target CSYNC: sequential host model */
    return enabled;
}
void plat_cq_unlock(uint32_t state)
{
    uint32_t bit = 1u << ((FM1_IRQ_ALNK0 & 7u) * 4u);
    /* target CSYNC: sequential host model */
    schedule(8); FM1_ICFG(FM1_IRQ_ALNK0) = (FM1_ICFG(FM1_IRQ_ALNK0) & ~bit) | state; schedule(9); dispatch();
    /* target CSYNC: sequential host model */
}

enum { C_PARAM_T0 = 0, C_DRUM = 8, C_BON, C_BOFF, C_BRK, C_NOP, C_REC };
#define CQ 1024u
static uint32_t cq[CQ];
static volatile uint32_t cq_w, cq_r;

/* The scan may be preempted after choosing a victim. Revalidate the slot only
 * while the consumer source is masked, then publish exactly one aligned word.
 * Scanning the 1024-entry queue never runs with the audio source masked. */
static int cq_replace(uint32_t i, uint32_t w, uint32_t word)
{
    schedule(1); uint32_t state = plat_cq_lock(); schedule(2);
    uint32_t r = cq_r;
    int live = (i - r < w - r); schedule(3);
    if (live)
        { schedule(4); cq[i % CQ] = word; }
    plat_cq_unlock(state); schedule(5); dispatch();
    return live;
}

__attribute__((noinline)) static void cq_put(uint32_t type, uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t w = cq_w;
    uint32_t word = (type << 28) | ((a & 0xFFu) << 20) | ((b & 0xFFu) << 12) | (c & 0xFFFu);
retry:
    if (w - cq_r >= CQ) {
        /* Full: params may drop (mirror still has them). Never drop C_BON,
         * C_BOFF, C_BRK, or C_DRUM — missing a live drum hit or break press
         * under param flood is a silent loss; a retained release without its
         * press is rare but must not displace an already-queued release. */
        uint32_t i, keep = (type == C_BON) || (type == C_BOFF) || (type == C_BRK) || (type == C_DRUM) || (type == C_REC);
        if (!keep)
            return;
        uint32_t r = cq_r;
        for (i = r; i != w; i++) {
            uint32_t t = cq[i % CQ] >> 28;
            if (t < C_DRUM || t == C_DRUM) {
                if (cq_replace(i, w, word)) return;
                goto retry;
            }
        }
        for (i = cq_r; i != w; i++) {
            uint32_t tw = cq[i % CQ], t = tw >> 28, bb = (tw >> 12) & 0xFFu;
            if (t != C_BOFF && !(t == C_BRK && bb == 0u)) {
                if (cq_replace(i, w, word)) return;
                goto retry;
            }
        }
        schedule(10); dispatch(); if (cq_r != r) goto retry; /* consumer freed room during the scan */
        return;
    }
    cq[w % CQ] = word;
    __asm__ volatile("" ::: "memory");
    cq_w = w + 1;
}


static void consume(unsigned count)
{
    uint32_t r=cq_r;
    while(count-- && r!=cq_w) { uint32_t word=cq[r % CQ]; ++seen[word >> 28]; exact_deliveries+=(word==expected_word); ++r; }
    cq_r=r;
}
static void dispatch(void)
{
    if(pending && global_enabled && (irq_cfg & AUDIO_BIT)) {
        pending=0;consume(drains);
    }
}
static void schedule(unsigned at)
{
    if(armed && at==site) { armed=0; ++hooks; pending=1; dispatch(); }
    if(at==3 || at==4) { ++protected_probes; }
}
static int one(unsigned type,unsigned count,uint32_t offset,unsigned victim,unsigned at)
{
    cq_r=cq_w=offset; irq_cfg=0xA5A5FFFFu;global_enabled=1;
    armed=pending=hooks=protected_probes=0;drains=count;site=at;
    for(unsigned k=0;k<16;k++)seen[k]=0;
    for(unsigned k=0;k<CQ;k++)cq_put(victim,0,60,k);
    uint32_t a=type==C_REC?0xA5u:1u,b=type==C_BRK?0u:60u;
    expected_word=(type<<28)|(a<<20)|(b<<12)|0x345u;exact_deliveries=0;
    armed=1;cq_put(type,a,b,0x345);
    dispatch();consume(CQ+1u);
    /* Count the distinctive packed word, not just its type: victim can share it. */
    unsigned delivered=seen[type];
    if(hooks!=1 || delivered!=1 || exact_deliveries!=1 || cq_r!=cq_w || irq_cfg!=0xA5A5FFFFu) {
        printf("FAIL type=%u drain=%u offset=%u victim=%u site=%u delivered=%u hook=%u depth=%u\n",
               type,count,offset,victim,at,delivered,hooks,(unsigned)(cq_w-cq_r));return 1;
    }
    return 0;
}

int main(void) {
 unsigned cases=0;unsigned types[]={C_DRUM,C_BON,C_BRK,C_REC};unsigned counts[]={1,CQ/2,CQ};uint32_t offsets[]={0,CQ-3u,UINT32_MAX-3u};
 for(unsigned t=0;t<4;t++)for(unsigned n=0;n<3;n++)for(unsigned o=0;o<3;o++) {
  if(one(types[t],counts[n],offsets[o],C_BOFF,10))return 1;cases++;
 }
 printf("PASS %u independent full-release-queue scan/retry scenarios\n",cases);return 0;
}
