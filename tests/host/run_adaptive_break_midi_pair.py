#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Execute the actual native atomic pulse function with bounded platform rings.
Synthetic queue/IRQ boundary proof, not transport or hardware validation."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'firmware/src/app/plat_fm1.c').read_text();a=s.index('int plat_midi_pulse(');z=s.index('\n#if !X0X_BLE',a);body=s[a:z]
pre=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BOARD_SMK37 1
#define X0X_BLE 1
#define MQ 8u
#define BLE_OQ 6u
#define RING_PUBLISH() do {} while(0)
static uint32_t mo_w,mo_r,midi_out_q[MQ],ble_oq[BLE_OQ],irqmask;
static struct {unsigned config;} usb;
static struct {unsigned ow,orr;struct {int h;} host;} ble_r;
static int clip,ble_central_role,smc_role,ble_want=1,ble_host_on=1,notify=1;
uint8_t smk_clip_mode_on(void){return clip;}
static int smc_ble_role_get(void){return smc_role;}
static int bleh_notifying(int*p){(void)p;return notify;}
static uint32_t irq_save(void){uint32_t old=irqmask;irqmask=1;return old;}
static void irq_restore(uint32_t old){irqmask=old;}
'''
post=r'''
static unsigned checks,failures;
#define C(x) do {checks++;if(!(x)){failures++;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
int main(void){
 for(unsigned offset=0;offset<24;offset++)for(unsigned uf=0;uf<=MQ;uf++)for(unsigned bf=0;bf<=BLE_OQ;bf++)for(unsigned mask=0;mask<2;mask++){
  mo_r=offset;mo_w=offset+MQ-uf;ble_r.orr=offset;ble_r.ow=offset+BLE_OQ-bf;usb.config=1;irqmask=mask;
  unsigned uw=mo_w,bw=ble_r.ow;int sent=plat_midi_pulse(3,43,100);
  C(irqmask==mask);C(sent==((uf>=2?1:0)|(bf>=2?2:0)));
  C(mo_w==uw+(uf>=2?2:0));C(ble_r.ow==bw+(bf>=2?2:0));
  if(uf>=2){C(((midi_out_q[uw%MQ]>>8)&255)==0x93);C(((midi_out_q[(uw+1)%MQ]>>8)&255)==0x83);C((midi_out_q[uw%MQ]&0xff0000)==(43u<<16));}
  if(bf>=2){C(((ble_oq[bw%BLE_OQ]>>8)&255)==0x93);C(((ble_oq[(bw+1)%BLE_OQ]>>8)&255)==0x83);}
 }
 mo_r=mo_w=ble_r.orr=ble_r.ow=0;clip=1;C(plat_midi_pulse(0,12,100)==0);C(mo_w==0&&ble_r.ow==0);clip=0;
 usb.config=0;notify=0;C(plat_midi_pulse(0,12,100)==0);C(mo_w==0&&ble_r.ow==0);
 printf("adaptive native pulse rings: %u checks, %u failures\n",checks,failures);return !!failures;
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(pre+body+post)
 subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
