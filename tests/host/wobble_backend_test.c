/* SPDX-License-Identifier: GPL-3.0-only */
#include "dsp/wobble_backend.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void render_frames(synth_slot_t *slot, unsigned n)
{
 float l[256],r[256];
 while(n) {
   unsigned count=n>256 ? 256 : n;
   assert(wobble_backend_stereo(slot,l,r,(int)count));
   for(unsigned i=0;i<count;++i) assert(isfinite(l[i]) && isfinite(r[i]));
   n-=count;
 }
}
static float block_rms(const float *l,const float *r,unsigned n)
{
 float sum=0;
 for(unsigned i=0;i<n;++i) sum+=.5f*(l[i]*l[i]+r[i]*r[i]);
 return sqrtf(sum/(float)n);
}
static void test_legato(void)
{
 wobble_t w,plain;
 synth_slot_t slot;
 float l[256],r[256],pl[256],pr[256];
 assert(synth_init(&slot,wobble_foundation_binding(&w)));
 assert(wobble_set(&w,W_ATTACK,.1f));
 assert(wobble_set(&w,W_DECAY,.1f));
 assert(wobble_set(&w,W_SUSTAIN,.4f));
 assert(wobble_set(&w,W_GLIDE,.5f));
 /* Exercise real attack, decay and sustain, including repeated slide pitch. */
 synth_note(&slot,48,0,1,90); /* Inactive slide must attack. */
 assert(w.envelope==0 && w.env_stage==0 && w.hz==w.target_hz);
 for(unsigned stage=0;stage<3;++stage) {
   unsigned frames=0;
   while((w.env_stage!=stage || w.envelope==0) && frames++<10000)
     render_frames(&slot,1);
   assert(w.env_stage==stage && w.envelope>0);
   float env=w.envelope,hz=w.hz,phase=w.carrier_phase;
   uint32_t age=w.active_samples;
   synth_note(&slot,55+(int)stage,0,1,100);
   assert(w.envelope==env && w.env_stage==stage);
   assert(w.hz==hz && w.target_hz>hz && w.carrier_phase==phase);
   assert(w.active_samples==age && w.note==55+stage && w.velocity==100.0f/127);
   float target=w.target_hz;
   render_frames(&slot,1);
   assert(w.hz>hz && w.hz<target); /* W_GLIDE slews rather than jumps. */
   env=w.envelope;
   synth_note(&slot,55+(int)stage,0,1,100);
   assert(w.envelope==env && w.env_stage==stage);
 }
 /* Old note-offs, including velocity-zero messages, cannot cut the latest. */
 wobble_note_off(&w,48); synth_note(&slot,56,0,1,0);
 assert(w.gate && w.note==57);
 assert(wobble_set(&w,W_GLIDE,0));
 float env=w.envelope;
 synth_note(&slot,60,0,1,100);
 assert(w.envelope==env && w.env_stage==2 && w.hz==w.target_hz);
 /* Compare audible continuation with a fresh attack from identical DSP state. */
 assert(wobble_set(&w,W_ATTACK,1));
 plain=w;
 synth_note(&slot,64,0,1,100);
 wobble_note_on(&plain,64,100);
 assert(plain.envelope==0 && plain.env_stage==0);
 assert(wobble_backend_stereo(&slot,l,r,256));
 wobble_render(&plain,pl,pr,256);
 float legato_rms=block_rms(l,r,256),plain_rms=block_rms(pl,pr,256);
 assert(legato_rms>1e-4f && legato_rms>plain_rms*10);
 printf("PASS legato audio: 256-frame RMS held=%.8f retrigger=%.8f; envelope=%.6f stage=%u\n",
        legato_rms,plain_rms,w.envelope,w.env_stage);
 /* Ordinary repeated NoteOn still retriggers; slide after NoteOff also attacks. */
 synth_note(&slot,64,0,0,100);
 assert(w.envelope==0 && w.env_stage==0);
 assert(wobble_set(&w,W_ATTACK,.1f));
 render_frames(&slot,256);
 wobble_note_off(&w,64);
 assert(w.active && !w.gate);
 synth_note(&slot,65,0,1,100); /* Even before release rendering. */
 assert(w.envelope==0 && w.env_stage==0 && w.gate);
 render_frames(&slot,256);
 wobble_note_off(&w,65); render_frames(&slot,1);
 assert(w.env_stage==3 && w.active);
 synth_note(&slot,67,0,1,100);
 assert(w.envelope==0 && w.env_stage==0);
 /* Held damper slide clears deferred old release and transfers ownership. */
 assert(wobble_set(&w,W_DAMPER,1));
 wobble_note_off(&w,67); assert(w.deferred_off && w.gate);
 render_frames(&slot,256); env=w.envelope;
 synth_note(&slot,69,0,1,100);
 assert(w.envelope==env && !w.deferred_off);
 assert(wobble_set(&w,W_DAMPER,0)); assert(w.gate);
 wobble_note_off(&w,67); assert(w.gate);
 synth_note(&slot,69,0,1,0); assert(!w.gate);
 render_frames(&slot,1024); assert(synth_idle(&slot));
 synth_note(&slot,72,0,1,100);
 assert(w.envelope==0 && w.env_stage==0 && w.hz==w.target_hz);
 render_frames(&slot,256);
 synth_off(&slot); render_frames(&slot,1024); assert(synth_idle(&slot));
 synth_note(&slot,72,0,1,100); render_frames(&slot,256);
 slot.active.ops->panic(slot.active.context);
 assert(!w.gate && !w.active && !w.envelope);
 assert(wobble_backend_stereo(&slot,l,r,256));
 for(unsigned i=0;i<256;++i) assert(l[i]==0 && r[i]==0);
 synth_note(&slot,72,0,1,100); render_frames(&slot,256);
 synth_transport(&slot,0);
 assert(!w.gate && !w.active && !w.envelope);
 assert(wobble_backend_stereo(&slot,l,r,256));
 for(unsigned i=0;i<256;++i) assert(l[i]==0 && r[i]==0);
 puts("PASS legato attack/decay/sustain, W_GLIDE, latest ownership, release/retrigger, damper, panic/stop");
}
int main(void) {
 test_legato();
 wobble_t a, direct, other; bass303_t native;
 synth_slot_t slot; float l[256],r[256],dl[256],dr[256],mono[256];
 uint8_t bank[16],saved[16],ticket;
 assert(synth_init(&slot,synth303_binding(&native)));
 wobble_foundation_init(&a);
 for(unsigned i=0;i<16;++i) bank[i]=(uint8_t)(i*8);
 assert(wobble_bank_load(&a,WOBBLE_EXPERIMENT_ENGINE,1,bank,16));
 wobble_bank_save(&a,saved); assert(!memcmp(bank,saved,16));
 assert(a.p[W_SUSTAIN] > 0.99f && a.p[W_ATTACK] < 0.01f);
 assert(!wobble_bank_load(&a,1,1,bank,16));
 assert(!wobble_bank_load(&a,WOBBLE_EXPERIMENT_ENGINE,2,bank,16));
 assert(!wobble_bank_load(&a,WOBBLE_EXPERIMENT_ENGINE,1,bank,15));
 bank[15]=255; other=a;
 assert(!wobble_bank_load(&a,WOBBLE_EXPERIMENT_ENGINE,1,bank,16));
 assert(!memcmp(&a,&other,sizeof a)); bank[15]=120;
 assert(synth_request(&slot,wobble_foundation_binding(&a),&ticket));
 assert(!synth_request(&slot,wobble_foundation_binding(&a),0));
 assert(synth_boundary(&slot) && slot.acknowledged==ticket);
 synth_param_key_t key={WOBBLE_EXPERIMENT_ENGINE,1,W_STEREO},bad=key;
 assert(synth_set(&slot,key,100));
 assert(!synth_set(&slot,synth303_key(0),0));
 bad.schema=2; assert(!synth_set(&slot,bad,0));
 bad=key; bad.param=W_RELEASE; assert(synth_set(&slot,bad,100));
 bad.param=W_PARAMETER_COUNT; assert(!synth_set(&slot,bad,0));
 bad.param=W_RELEASE; assert(synth_set(&slot,bad,0));
 synth_note(&slot,48,1,1,80); direct=a;
 for(int n=1;n<=256;++n) {
   assert(wobble_backend_stereo(&slot,l,r,n));
   wobble_render(&direct,dl,dr,(size_t)n);
   assert(!memcmp(l,dl,(size_t)n*sizeof(float)) && !memcmp(r,dr,(size_t)n*sizeof(float)));
 }
 assert(!wobble_backend_stereo(&slot,l,l,1));
 assert(!wobble_backend_stereo(&slot,l,r,0));
 assert(!wobble_backend_stereo(&slot,l,r,257));
 direct=a;
 for(int n=1;n<=256;++n) {
   synth_render(&slot,mono,n); wobble_render(&direct,dl,dr,(size_t)n);
   for(int i=0;i<n;++i) assert(mono[i]==.5f*(dl[i]+dr[i]));
 }
 synth_off(&slot);
 for(int i=0;i<64 && !synth_idle(&slot);++i) synth_render(&slot,mono,256);
 assert(synth_idle(&slot));
 for(int i=0;i<64;++i) synth_render(&slot,mono,256);
 for(int i=0;i<256;++i) assert(fabsf(mono[i])<1e-5f);
 wobble_bank_save(&a,saved); synth_note(&slot,55,0,0,100); synth_transport(&slot,0);
 wobble_bank_save(&a,bank);assert(!memcmp(bank,saved,16));
 assert(synth_request(&slot,synth303_binding(&native),&ticket));assert(synth_boundary(&slot));
 assert(slot.acknowledged==ticket);
 puts("PASS wobble stereo/mono fragments1..256, lifecycle/ACK, stale keys, transactional bank");
 return 0;
}
