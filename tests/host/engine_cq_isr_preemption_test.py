#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Production CQ + source-mask preemption regression, adopted from PR621.

Original PR621 head6f9090cf reproduced the pre-store loss on d633. This adapts
its hook to the actual corrected synchronization, extracting target platform
lock/unlock too. No threaded/device timing claim. C_REC is reserved exactly as
in the separately reviewed REC unit, so its atomic word gets the same priority.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PRELUDE = r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t irq_cfg, global_enabled;
static unsigned armed, site, drains, pending, hooks, seen[16], protected_probes;
#define FM1_IRQ_ALNK0 11u
#define FM1_ICFG(n) irq_cfg
#define AUDIO_BIT (1u << ((FM1_IRQ_ALNK0 & 7u) * 4u))
static void schedule(unsigned at);
static void dispatch(void);
"""
HARNESS = r"""
static void consume(unsigned count)
{
    uint32_t r=cq_r;
    while(count-- && r!=cq_w) { ++seen[cq[r % CQ] >> 28]; ++r; }
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
    armed=1;cq_put(type,0,type==C_BRK?0:60,0x345);
    dispatch();consume(CQ+1u);
    /* Count the distinctive packed word, not just its type: victim can share it. */
    unsigned delivered=seen[type];
    if(hooks!=1 || delivered!=1 || cq_r!=cq_w || irq_cfg!=0xA5A5FFFFu) {
        printf("FAIL type=%u drain=%u offset=%u victim=%u site=%u delivered=%u hook=%u depth=%u\n",
               type,count,offset,victim,at,delivered,hooks,(unsigned)(cq_w-cq_r));return 1;
    }
    return 0;
}
int main(void)
{
    const unsigned types[]={C_DRUM,C_BON,C_BOFF,C_BRK,C_REC};
    const unsigned counts[]={0,1,CQ/2,CQ};
    const uint32_t starts[]={0,CQ-3u,UINT32_MAX-3u};
    const unsigned victims[]={C_PARAM_T0,C_NOP};
    unsigned cases=0;
    for(unsigned t=0;t<5;t++)for(unsigned n=0;n<4;n++)for(unsigned o=0;o<3;o++)
      for(unsigned v=0;v<2;v++)for(unsigned at=1;at<=9;at++) {
        if(one(types[t],counts[n],starts[o],victims[v],at))return 1;
        ++cases;
      }
    /* Preserve source-disabled and global-disabled caller state, nested too. */
    for(unsigned enabled=0;enabled<2;enabled++)for(unsigned global=0;global<2;global++) {
        irq_cfg=0xA5A5FFFFu & ~AUDIO_BIT;if(enabled)irq_cfg|=AUDIO_BIT;
        uint32_t prior=irq_cfg;global_enabled=global;armed=pending=0;
        uint32_t a=plat_cq_lock(),b=plat_cq_lock();
        if(irq_cfg & AUDIO_BIT)return 2;
        plat_cq_unlock(b);if(irq_cfg & AUDIO_BIT)return 3;
        plat_cq_unlock(a);
        if(irq_cfg!=prior || global_enabled!=global)return 4;
    }
    printf("PASS %u production CQ ISR scenarios +4 nested source/global-state cases\n",cases);
    return 0;
}
"""

def sources():
    data=(ROOT/'firmware/src/app/engine.c').read_text()
    start=data.index('enum { C_PARAM_T0 = 0, C_DRUM')
    end=data.index('/* ------------------------------------------------------------ params --- */',start)
    ring=data[start:end]
    anchors={'uint32_t state = plat_cq_lock();':1,'uint32_t r = cq_r;':2,
             'cq[i % CQ] = word;':1,'plat_cq_unlock(state);':1,'cq_replace(i, w, word)':2}
    for text,count in anchors.items():
        if ring.count(text)!=count:raise ValueError('queue changed: review hook '+text)
    # IRQ requested before lock, after masking, after live revalidation, before
    # replacement store and after unlock; source-disabled requests stay pending.
    ring=ring.replace('uint32_t state = plat_cq_lock();','schedule(1); uint32_t state = plat_cq_lock(); schedule(2);')
    ring=ring.replace('int live = (i - r < w - r);','int live = (i - r < w - r); schedule(3);')
    ring=ring.replace('cq[i % CQ] = word;','{ schedule(4); cq[i % CQ] = word; }')
    ring=ring.replace('plat_cq_unlock(state);','plat_cq_unlock(state); schedule(5); dispatch();')
    platform=(ROOT/'firmware/src/app/plat_fm1.c').read_text()
    start=platform.index('uint32_t plat_cq_lock(void)')
    end=platform.index('uint32_t plat_ms(void)',start)
    lock=platform[start:end]
    sync='__asm__ volatile("csync" ::: "memory");'
    if lock.count(sync)!=3:raise ValueError('platform lock changed: review MMIO model')
    # Model the hardware source-enable register; same target C and HAL mask body.
    hal=(ROOT/'firmware/hal/fm1_irq.h').read_text()
    mask=next(line for line in hal.splitlines() if line.startswith('static void fm1_irq_mask('))
    lock=lock.replace('uint32_t enabled = FM1_ICFG(FM1_IRQ_ALNK0) & bit;',
                      'uint32_t enabled = FM1_ICFG(FM1_IRQ_ALNK0) & bit; schedule(6);')
    lock=lock.replace('fm1_irq_mask(FM1_IRQ_ALNK0);',
                      'fm1_irq_mask(FM1_IRQ_ALNK0); schedule(7);')
    restore='FM1_ICFG(FM1_IRQ_ALNK0) = (FM1_ICFG(FM1_IRQ_ALNK0) & ~bit) | state;'
    if lock.count(restore)!=1:raise ValueError('platform restore changed')
    lock=lock.replace(restore,'schedule(8); '+restore+' schedule(9); dispatch();')
    lock=mask+'\n'+lock.replace(sync,'/* target CSYNC: sequential host model */')
    return ring,lock

def run(ring,lock,compiler,sanitize,mutant=0):
    if mutant==1:lock=lock.replace('fm1_irq_mask(FM1_IRQ_ALNK0);','/* MUTANT: omit source mask */')
    if mutant==2:ring=ring.replace("int live = (i - r < w - r);", "int live = 1; (void)r; (void)w;")
    source=PRELUDE+lock+ring+HARNESS
    with tempfile.TemporaryDirectory(prefix='x0x-cq-isr-') as tmp:
        c=Path(tmp)/'test.c';exe=Path(tmp)/'test';c.write_text(source)
        flags=[compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror']
        if mutant:flags+=['-Wno-unused-function']
        if sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        subprocess.run(flags+[str(c),'-o',str(exe)],check=True)
        return subprocess.run([str(exe)]).returncode

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',default='cc');p.add_argument('--sanitize',action='store_true');a=p.parse_args()
    ring,lock=sources()
    rc=run(ring,lock,a.compiler,a.sanitize)
    if rc:return rc
    rc=run(ring,lock,a.compiler,a.sanitize,1)
    if rc!=1:raise RuntimeError('unmasked mutant did not expose lost-event regression')
    print('PASS omitted-source-mask mutant rejected')
    rc=run(ring,lock,a.compiler,a.sanitize,2)
    if rc!=1:raise RuntimeError('omitted revalidation mutant did not expose lost event')
    print('PASS omitted-live-revalidation mutant rejected')
    return 0
if __name__=='__main__':raise SystemExit(main())
