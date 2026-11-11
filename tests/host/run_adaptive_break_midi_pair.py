#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual pulse + actual HAL source-mask register arithmetic, with CSYNC model hooks.
No fake irq_save/restore. Synthetic CPU0 consumer/link interleavings; not hardware proof.
CPU1-to-CPU0 callback ownership is separately exercised by the real engine control test."""
from pathlib import Path
import subprocess,tempfile,re
root=Path(__file__).resolve().parents[2]
s=(root/'firmware/src/app/plat_fm1.c').read_text();a=s.index('int plat_midi_pulse(');z=s.index('\n#if !X0X_BLE',a);body=s[a:z]
assert 'irq_save(' not in body and 'irq_restore(' not in body, 'AS-003: nonpreserving flash IRQ helpers forbidden'
hal=(root/'firmware/hal/fm1_irq.h').read_text()
def function(name):
 a=hal.index('static ',hal.index(name)-25);start=hal.index('{',a);depth=1;i=start+1
 while depth:
  depth+=(hal[i]=='{')-(hal[i]=='}');i+=1
 return hal[a:i]
helpers='\n'.join(function(n) for n in ['fm1_irq_mask(', 'fm1_irq_mask_save(', 'fm1_irq_mask_restore('])
# Keep actual register arithmetic. Only unsupported target CSYNC instruction becomes a hook.
helpers=helpers.replace('__asm__ volatile("csync" ::: "memory");','hal_sync();')
assert '__asm__' not in helpers
alnk=int(re.search(r'FM1_IRQ_ALNK0\s*=\s*(\d+)',hal)[1]);timer=int(re.search(r'FM1_IRQ_TIMER5\s*=\s*(\d+)',hal)[1])
pre=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BOARD_SMK37 1
#define X0X_BLE 1
#define MQ 8u
#define BLE_OQ 6u
static uint32_t mo_w,mo_r,midi_out_q[MQ],ble_oq[BLE_OQ],global_irq;
static uint32_t icfg[16];
#define FM1_ICFG(n) icfg[(n)>>3]
static struct {unsigned config;} usb;
static struct {unsigned ow,orr;struct {int h;} host;} ble_r;
static int clip,ble_central_role,smc_role,ble_want=1,ble_host_on=1,notify=1;
uint8_t smk_clip_mode_on(void){return clip;}
static int smc_ble_role_get(void){return smc_role;}
static int bleh_notifying(int*p){(void)p;return notify;}
static unsigned checks,failures,hook,target,pending,mode,usb_on,ble_on,observed;
#define C(x) do {checks++;if(!(x)){failures++;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
static void hal_sync(void);
'''+f'\n#define FM1_IRQ_ALNK0 {alnk}u\n#define FM1_IRQ_TIMER5 {timer}u\n'+helpers+r'''
static int timer_enabled(void){return global_irq&&(FM1_ICFG(FM1_IRQ_TIMER5)&(1u<<((FM1_IRQ_TIMER5&7)*4)));}
static void consume(uint32_t p,unsigned *on){unsigned st=(p>>8)&0xf0;if(st==0x90){C(!*on);*on=1;}if(st==0x80){C(*on);*on=0;}}
static void service(void){
 if(!pending||!timer_enabled())return;
 pending=0;observed++;
 if(mode==2){usb.config=0;notify=0;mo_r=mo_w;ble_r.orr=ble_r.ow;return;}
 while(mo_r!=mo_w)consume(midi_out_q[mo_r++%MQ],&usb_on);
 while(ble_r.orr!=ble_r.ow)consume(ble_oq[ble_r.orr++%BLE_OQ],&ble_on);
 C(!usb_on&&!ble_on); /* No consumer can observe only one half of a newly published pair. */
}
static void hal_sync(void){hook++;if(target&&hook==target)pending=1;service();}
static void fm1_data_sync(void){hal_sync();}
'''
post=r'''
static void reset(void){memset(icfg,0x55,sizeof icfg);usb.config=notify=ble_want=ble_host_on=1;clip=smc_role=ble_central_role=0;hook=target=pending=mode=usb_on=ble_on=observed=0;}
int main(void){
 for(unsigned offset=0;offset<24;offset++)for(unsigned uf=0;uf<=MQ;uf++)for(unsigned bf=0;bf<=BLE_OQ;bf++)for(unsigned enabled=0;enabled<2;enabled++){
  reset();mo_r=offset;mo_w=offset+MQ-uf;ble_r.orr=offset;ble_r.ow=offset+BLE_OQ-bf;global_irq=enabled;
  uint32_t saved[16];memcpy(saved,icfg,sizeof saved);unsigned uw=mo_w,bw=ble_r.ow;int sent=plat_midi_pulse(3,43,100);
  C(global_irq==enabled);C(!memcmp(saved,icfg,sizeof saved));C(sent==((uf>=2?1:0)|(bf>=2?2:0)));
  C(mo_w==uw+(uf>=2?2:0));C(ble_r.ow==bw+(bf>=2?2:0));
  if(uf>=2){C(((midi_out_q[uw%MQ]>>8)&255)==0x93);C(((midi_out_q[(uw+1)%MQ]>>8)&255)==0x83);C((midi_out_q[uw%MQ]&0xff0000)==(43u<<16));}
  if(bf>=2){C(((ble_oq[bw%BLE_OQ]>>8)&255)==0x93);C(((ble_oq[(bw+1)%BLE_OQ]>>8)&255)==0x83);}
 }
 // Every source mask/publication/restore boundary: consumer or link reset requested.
 for(unsigned which=1;which<=2;which++)for(unsigned at=1;at<=8;at++)for(unsigned enabled=0;enabled<2;enabled++)for(unsigned sources=0;sources<4;sources++){
  reset();mo_r=mo_w=ble_r.orr=ble_r.ow=0;global_irq=enabled;mode=which;target=at;
  if(!(sources&1))FM1_ICFG(FM1_IRQ_ALNK0)&=~(1u<<((FM1_IRQ_ALNK0&7)*4));
  if(!(sources&2))FM1_ICFG(FM1_IRQ_TIMER5)&=~(1u<<((FM1_IRQ_TIMER5&7)*4));
  uint32_t saved[16];memcpy(saved,icfg,sizeof saved);(void)plat_midi_pulse(3,43,100);
  C(global_irq==enabled);C(!memcmp(saved,icfg,sizeof saved));C(!usb_on&&!ble_on);
  // Allow a previously masked event only after checking exact state preservation.
  global_irq=1;FM1_ICFG(FM1_IRQ_TIMER5)|=1u<<((FM1_IRQ_TIMER5&7)*4);pending=1;service();C(!usb_on&&!ble_on);
 }
 reset();mo_r=mo_w=ble_r.orr=ble_r.ow=0;clip=1;C(plat_midi_pulse(0,12,100)==0);C(mo_w==0&&ble_r.ow==0);clip=0;
 usb.config=0;notify=0;C(plat_midi_pulse(0,12,100)==0);C(mo_w==0&&ble_r.ow==0);
 printf("adaptive pulse actual HAL model: %u checks, %u failures\n",checks,failures);return !!failures;
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(pre+body+post)
 subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
