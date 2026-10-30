#!/usr/bin/env python3
"""Compile exact current scanner and startup-cutoff bodies with mock raw pins/IRQ."""
from pathlib import Path
import os,subprocess
root=Path(__file__).resolve().parents[2];out=root/'build/boot-input-test';out.mkdir(parents=True,exist_ok=True)
s=(root/'firmware/hal/smk_input.h').read_text();runtime=(root/'firmware/src/app/boot_runtime.h').read_text()
def function(text,name):
 at=text.index('static ',text.index(name)-30) if False else text.rfind('\nstatic ',0,text.index(name+'('))+1
 a=text.index('{',at);level=1;b=a+1
 while level:
  if text[b]=='{':level+=1
  if text[b]=='}':level-=1
  b+=1
 return text[at:b]
pre=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/boot_input.h"
#define FM1_NKEY 37
#define FM1_NBTN 18
#define FM1_NENC 8
#define SMK_NPAD 16
#define SMK_KEY_F3 5
#define SMK_KEY_REL 3
#define SMK_DEB_RELEASE 2
#define SMK_DEB_PRESS 2
#define FM1_TICKS_PER_US 24
#define SMK_PAD_ON 24
#define SMK_PAD_OFF 12
#define SMK_PAD_PEAKN 3
#define SMK_WHITE_OF_PAD(p) ((p)<8?(p)+8:(p)-8)
static struct {uint64_t keys;uint32_t knotes,pnotes,notes,notes_pressed,buttons,pressed,released;uint8_t key_vel[37],ks_stab[37],ks_rel[37],cnt[18],pad_st[16],pad_vel[16],pad_low[16],pad_press[16];uint32_t ks_t1[37];int32_t enc_steps[8];uint16_t pad_base[16],pad_raw[16],pad_peak[16];} fm1_in;
static uint8_t contacts[37],button_raw[18],SMK_KEYC[37][2],SMK_BTNC[18];
static uint32_t smk__closed(unsigned code){return code>=100?button_raw[code-100]:(contacts[code/2]>>(code%2))&1;}
static unsigned smk_vel7(unsigned x){(void)x;return 100;}static unsigned smk_curve(const uint16_t*a,unsigned x){(void)a;return x;}
static uint16_t smk_trim_key[37][5],smk_trim_pad[16][5];
static boot_input_t smk_boot_input;static uint32_t smk_boot_epoch;
static uint8_t smk_extq_r,smk_extq_w,smk_extq_overflow,smk_div_pending,smk_div_owner,smk_pad_piano,smk_pad_note[16],smk_diag_enabled;
static uint16_t smk_div_held,smk_diag_suppressed,smk_diag_seen;static uint32_t smk_diag_ack,smk_diag_epoch;
static const uint8_t SMK_WHITE_NOTE[16]={0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
static const int8_t SMK_PIANO_NOTE[16]={7,9,11,12,14,16,18,19,8,10,-1,13,15,17,-1,20};
static unsigned ext_events,irq_state=1,saves,restores;
static void smk__extq(uint32_t k,uint32_t v){(void)k;(void)v;ext_events++;}
#define FM1_IRQ_TIMER5 63u
static uint32_t icfg_mock[16];
#define FM1_ICFG(n) icfg_mock[(n)>>3]
static void fm1_irq_mask(uint32_t n){FM1_ICFG(n)&=~(1u<<((n&7u)*4u));}

'''
irq_source=(root/'firmware/hal/fm1_irq.h').read_text()
# Exact production source-mask bodies; only target csync assembly is replaced
# with a compiler memory barrier. No fake global IRQ save/restore semantics.
irq_parts=[function(irq_source,n).replace('fm1_irq_mask_restore(', 'fm1_irq_mask_restore_actual(').replace('__asm__ volatile("csync" ::: "memory");','__asm__ volatile("" ::: "memory");') for n in ['fm1_irq_mask_save','fm1_irq_mask_restore']]
parts=irq_parts+[function(s,n) for n in ['smk__key_event','smk__keys','smk__buttons','smk__padq','smk__pad']]
parts += ['''static unsigned pending_scan;
static void fm1_irq_mask_restore(uint32_t n,uint32_t state){
 fm1_irq_mask_restore_actual(n,state);
 if(pending_scan && (FM1_ICFG(n)&(1u<<((n&7u)*4u))) && irq_state){pending_scan=0;smk__keys(9000);}
}''']
parts += [function(runtime,n) for n in ['boot_raw_keys','boot_raw_buttons','boot_raw_pads','boot_drain_locked','boot_inputs_begin','boot_inputs_skip','boot_inputs_end']]
test=r'''
static unsigned checks;
#define C(x) do{checks++;if(!(x)){printf("FAIL line%d %s\n",__LINE__,#x);return 1;}}while(0)
static void reset(void){memset(&fm1_in,0,sizeof fm1_in);memset(contacts,0,sizeof contacts);memset(button_raw,0,sizeof button_raw);memset(&smk_boot_input,0,sizeof smk_boot_input);memset(smk_pad_note,0,sizeof smk_pad_note);ext_events=0;smk_div_owner=smk_div_pending=smk_div_held=0;for(unsigned k=0;k<37;k++){SMK_KEYC[k][0]=2*k;SMK_KEYC[k][1]=2*k+1;}for(unsigned k=0;k<18;k++)SMK_BTNC[k]=100+k;}
int main(void){
 for(unsigned k=0;k<37;k++){
  reset();contacts[k]=1;fm1_in.ks_stab[k]=1;boot_inputs_begin();contacts[k]=3;smk__keys(1000);C(!boot_inputs_skip()&&!fm1_in.keys&&!fm1_in.notes&&!ext_events);boot_inputs_end();contacts[k]=0;for(int i=0;i<3;i++)smk__keys(2000+i);C(!(smk_boot_input.keys&((uint64_t)1<<k)));contacts[k]=3;smk__keys(3000);C((fm1_in.keys>>k)&1);
  reset();contacts[k]=1;fm1_in.ks_stab[k]=1;boot_inputs_begin();contacts[k]=0;for(int i=0;i<3;i++)smk__keys(2000+i);contacts[k]=3;smk__keys(3000);C(boot_inputs_skip()&&!fm1_in.keys&&!fm1_in.notes&&!ext_events);
  reset();contacts[k]=1;boot_inputs_begin();contacts[k]=0;smk__keys(2000);C(!(smk_boot_input.keys&((uint64_t)1<<k)));contacts[k]=3;smk__keys(3000);C(boot_inputs_skip()&&!fm1_in.keys&&!ext_events);
 }
 for(unsigned b=0;b<18;b++){
  reset();button_raw[b]=1;boot_inputs_begin();for(int j=0;j<3;j++)smk__buttons();C(!boot_inputs_skip()&&!fm1_in.buttons);boot_inputs_end();button_raw[b]=0;for(int j=0;j<3;j++)smk__buttons();C(!smk_boot_input.buttons&&!fm1_in.released);button_raw[b]=1;for(int j=0;j<3;j++)smk__buttons();C(fm1_in.buttons==(1u<<b));
 }
 for(unsigned p=0;p<16;p++){
  reset();fm1_in.pad_st[p]=1;boot_inputs_begin();smk__pad(p,100);smk__pad(p,100);C(!boot_inputs_skip()&&!fm1_in.pnotes);boot_inputs_end();smk__pad(p,0);smk__pad(p,0);C(!smk_boot_input.pads&&!fm1_in.pnotes);smk__pad(p,100);smk__pad(p,100);smk__pad(p,100);C(fm1_in.pnotes!=0);
  reset();boot_inputs_begin();smk__pad(p,100);smk__pad(p,100);smk__pad(p,100);smk__pad(p,0);smk__pad(p,0);C(boot_inputs_skip()&&!fm1_in.pnotes&&!smk_boot_input.pads);
 }
 for(unsigned enabled=0;enabled<2;enabled++)for(unsigned global=0;global<2;global++){
  irq_state=global;icfg_mock[7]=(0xFEDCBA98u&~0x10000000u)|(enabled<<28);uint32_t old=icfg_mock[7];icfg_mock[6]=0x12345678u;
  uint32_t token=fm1_irq_mask_save(FM1_IRQ_TIMER5);C(!(icfg_mock[7]&0x10000000u)&&irq_state==global&&icfg_mock[6]==0x12345678u);fm1_irq_mask_restore(FM1_IRQ_TIMER5,token);C(icfg_mock[7]==old&&irq_state==global);
 }
 reset();irq_state=1;icfg_mock[7]|=0x10000000u;contacts[2]=3;pending_scan=1;boot_inputs_begin();C(!pending_scan&&!fm1_in.keys&&!smk_boot_input.skip); /* pending held scan remains quarantined */
 reset();boot_inputs_begin();contacts[2]=3;pending_scan=1;C(!boot_inputs_skip());C(smk_boot_input.skip&&!fm1_in.keys); /* fresh pending publication observed after restore */
 reset();for(unsigned e=0;e<8;e++)fm1_in.enc_steps[e]=3;irq_state=0;icfg_mock[7]=0xFEDCBA98u;uint32_t oldcfg=icfg_mock[7];boot_inputs_begin();C(irq_state==0&&icfg_mock[7]==oldcfg);C(!boot_inputs_skip());irq_state=1;
 for(unsigned e=0;e<8;e++){fm1_in.enc_steps[e]=1;C(boot_inputs_skip()&&fm1_in.enc_steps[e]==0);}boot_inputs_end();C(irq_state==1);for(unsigned e=0;e<8;e++)C(!fm1_in.enc_steps[e]);
 reset();boot_inputs_begin();contacts[0]=3;smk__keys(3000);C(boot_inputs_skip());boot_inputs_end();contacts[8]=3;smk__keys(4000);C(fm1_in.keys==((uint64_t)1<<8));contacts[0]=0;for(int j=0;j<3;j++)smk__keys(5000+j);C(fm1_in.keys==((uint64_t)1<<8));
 printf("Actual scanner/startup input bodies: %u checks PASS; mocked IRQ/pins, no hardware claim\n",checks);return 0;
}
'''
(out/'actual.c').write_text(pre+'\n'.join(parts)+test)
cmd=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(root/'firmware/src'),str(out/'actual.c'),'-o',str(out/'actual')];subprocess.run(cmd,check=True);subprocess.run([str(out/'actual')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
