/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_selected_voice.h"
#include "binary32_classify.h"
#include <math.h>
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#include "miami_interpolation.h"
#define ref_fma original_rne_fmaf
#define table_fma original_miami_interpolate_rne
#else
#define ref_fma fmaf
#define table_fma fmaf
#endif
#define TABLE_READY 0x4d565431u
#define CONTROL_READY 0x4d564331u
#define VOICE_READY 0x71u
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static unsigned quantize(float x,unsigned n){unsigned q=(unsigned)ref_fma(x,(float)(n-1),.5f);return q<n?q:n-1;}
static float bipolar(float x){return ref_fma(x,2,-1);}
static float native(const original_miami_program *p,unsigned id){float x=0;(void)original_miami_program_native(p,id,&x);return x;}
static int valid(const original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,float bpm){
 return s&&c&&t&&s->ready==VOICE_READY&&c->ready==CONTROL_READY&&t->ready==TABLE_READY&&original_binary32_isfinite(bpm)&&bpm>=20&&bpm<=400;
}
static int tables_valid(const original_ms20_tables *in,const int32_t pitch[128]){
 if(!in||!in->a||!in->b||!in->r||!in->cap||!pitch)return 0;
 for(unsigned i=0;i<129;i++)if(!original_binary32_isfinite(in->a[i])||!original_binary32_isfinite(in->b[i])||!original_binary32_isfinite(in->r[i])||!original_binary32_isfinite(in->cap[i]))return 0;
 for(unsigned i=0;i<128;i++)if(pitch[i]<=0||(i&&pitch[i]<=pitch[i-1]))return 0;
 return 1;
}
int original_miami_voice_tables_bind_immutable(original_miami_voice_tables *out,const original_ms20_tables *in,const int32_t pitch[128]){
 if(!out||!tables_valid(in,pitch))return 0;
 original_miami_voice_tables t;memset(&t,0,sizeof t);t.filter=*in;t.pitch=pitch;t.ready=TABLE_READY;*out=t;return 1;
}
int original_miami_voice_tables_init(original_miami_voice_tables *out,original_miami_voice_table_storage *storage,const original_ms20_tables *in,const int32_t pitch[128]){
 if(!out||!storage||!tables_valid(in,pitch))return 0;
 memcpy(storage->a,in->a,sizeof storage->a);memcpy(storage->b,in->b,sizeof storage->b);memcpy(storage->r,in->r,sizeof storage->r);memcpy(storage->cap,in->cap,sizeof storage->cap);memcpy(storage->pitch,pitch,sizeof storage->pitch);
 original_miami_voice_tables t;memset(&t,0,sizeof t);t.filter=(original_ms20_tables){storage->a,storage->b,storage->r,storage->cap};t.pitch=storage->pitch;t.ready=TABLE_READY;*out=t;return 1;
}
int original_miami_voice_compile(original_miami_voice_controls *out,const original_miami_program *p){
 if(!out||!p)return 0;
 for(unsigned i=0;i<18;i++)if(!original_binary32_isfinite(p->macro[i])||p->macro[i]<0||p->macro[i]>1)return 0;
 for(unsigned i=0;i<24;i++)if(!original_binary32_isfinite(p->target[i])||p->target[i]<0||p->target[i]>1)return 0;
 original_miami_voice_controls c;memset(&c,0,sizeof c);
 c.pitch.master_tune=bipolar(native(p,0));
 c.pitch.unison_detune=native(p,90);c.pitch.bend_range=24.0f*native(p,55);
 c.pitch.transpose=(int)quantize(native(p,18),49)-24;
 c.pitch.detune=native(p,17);c.pitch.portamento=native(p,16);c.pitch.modulation_on=1;
 for(unsigned i=0;i<4;i++){
  unsigned n=i*4;c.shape[i]=(uint8_t)quantize(native(p,n+1),5);
  if(c.shape[i]>3)return 0; /* PCM is outside the selected18 macro profile. */
  c.pitch.semitone[i]=(int)quantize(native(p,n+2),49)-24;
  c.pitch.fine_tune[i]=i?bipolar(native(p,n)):0;
  c.gain.level[i]=native(p,n+3);
 }
 c.gain.output=native(p,94);c.gain.output=c.gain.output*c.gain.output;
 c.gain.pan=bipolar(native(p,93));c.gain.spread=native(p,91);
 c.gain.curve=0;c.gain.ms20=1;c.gain.filter_gain=1;
 c.cutoff=native(p,24);c.resonance=native(p,25);c.env_depth=bipolar(native(p,26));c.noise=native(p,23);
 c.pw_intensity=native(p,21);c.pw_width=native(p,22);
 for(unsigned i=0;i<4;i++)
  if(!original_pw_width(c.shape[i],c.pw_intensity,0,c.pw_width,0,0,&c.wave_width[i]))return 0;
 if(!original_xmod_coefficient(native(p,39),0,&c.xmod))return 0;
 c.vco_gain=ref_fma(native(p,102),3,1);
 c.patch_slave=bipolar(native(p,80));c.patch_cutoff=bipolar(native(p,81));c.patch_gain=bipolar(native(p,87));
 static const float base[10]={96,48,36,24,16,12,8,6,4,3};
 for(unsigned i=0;i<2;i++){
  c.mg_shape[i]=(uint8_t)quantize(native(p,i?49:43),5);if(c.mg_shape[i]>3)return 0;
  c.mg_base[i]=base[quantize(native(p,i?53:47),10)];
  c.mg_times[i]=(float)(16-quantize(native(p,i?54:48),16));
 }
 c.bend_normalized=native(p,105);c.damper=(uint8_t)(quantize(native(p,108),128)>=64);
 c.master_volume=native(p,96);c.master_volume=c.master_volume*c.master_volume;
 c.decimator_normalized[0]=native(p,110);c.decimator_normalized[1]=native(p,111);
 c.revision=p->revision;c.ready=CONTROL_READY;*out=c;return 1;
}
static int mg_output(original_miami_voice_mg *m,unsigned shape){
 if(!original_miami_mg_shape(shape,m->phase,&m->bipolar,&m->unipolar))return 0;
 m->wrapped=0;return 1;
}
static void mg_advance(original_miami_voice_mg *m,float base,float times,float bpm,uint32_t D){
 float numerator=(bpm*.800000011920929f)*(float)D;
 float denominator=(44100.0f*base)*times;
 float delta=numerator/denominator;if(delta>1)delta=1;
 m->phase=m->phase+delta;if(m->phase>=1){m->phase=m->phase-2.0f;m->wrapped=1;}
}
/* VCF EG selected controls A0/D.5/S1/R0; distinct native release threshold.
 * Sustain subtraction for its bipolar output happens after both EG ticks. */
static void vcf_tick(original_vca *e,uint32_t D){
 float q=e->level;
 if(e->state==0){q=q+((float)D*(1.2999999523162842f-q))/(44100.0f*fl(0x3a83126fu));if(q>=1){q=1;e->state=1;}}
 else if(e->state==1){if(q<1)q=1;else {q=q+((float)D*(1.0f-q))/(fl(0x3fc820c5u)*44100.0f);q=(q+1.0f)-1.0f;}}
 else if(e->state==3){if(q>=fl(0x3727c5ac)){float den=fl(0x3b03126fu)*44100.0f;q=q*(1.0f-(float)D/den);q=(q+1.0f)-1.0f;}else {q=0;e->state=6;}}
 e->level=q;
}
#ifndef MIAMI_VOICE_CHECKED_TABLE_PREP
static float lookup(const float t[129],float q){if(q<=fl(0x3727c5ac))return t[0];if(q>=128)return t[128];unsigned j=(unsigned)q;return table_fma(t[j+1]-t[j],q-(float)j,t[j]);}
static void stage(original_ramp *r,float target,float reciprocal){r->target=target;r->delta=(target-r->current)*reciprocal;}
#endif
/* Identical selected MS20 preparation arithmetic to the checked dependency.
 * Only immutable-table scanning is hoisted into tables_init(). No state sample,
 * nonlinear equation, interpolation or precision is changed. */
static void ms20_prepare(original_miami_ms20 *s,const original_miami_voice_tables *t,const original_miami_voice_controls *c,float env,float mc,uint32_t D){
#ifdef MIAMI_VOICE_CHECKED_TABLE_PREP
 original_ms20_tables tables=t->filter;
 (void)original_miami_ms20_prepare(s,&tables,c->cutoff,c->resonance,c->env_depth,env,0,mc,0,c->noise,0,D);
#else
 float d=clamp(c->env_depth+0.0f,-1,1),z=ref_fma(env,d,c->cutoff),q=ref_fma(mc,128,z*128.0f);
 float a=lookup(t->filter.a,q),b=lookup(t->filter.b,q),cap=lookup(t->filter.cap,q),r=c->resonance+0.0f,R;
 if(r==1)R=t->filter.r[128];else {float rq=r*128.0f;unsigned j=(unsigned)rq;R=table_fma(t->filter.r[j+1]-t->filter.r[j],rq-(float)j,t->filter.r[j]);}
 float target=R<=fl(0x3727c5ac)?fl(0x3727c5ac):(R<cap?R:cap);
 float ng=clamp(c->noise+0.0f,0,1);ng=ng*ng+fl(0x3727c5ac);float reciprocal=1.0f/(float)D;
 stage(&s->a,a,reciprocal);stage(&s->resonance,target,reciprocal);stage(&s->noise_gain,ng,reciprocal);s->b=b;s->cap=cap;s->prepared=1;
#endif
}
int original_miami_selected_voice_init(original_miami_selected_voice *s,const original_miami_voice_tables *t,unsigned index,unsigned last){
 if(!s||!t||t->ready!=TABLE_READY||index>=128||last>127)return 0;
 original_miami_selected_voice n;memset(&n,0,sizeof n);
 if(!original_miami_pitch_init(&n.pitch,last)||!original_miami_ms20_init(&n.filter,index,t->filter.a[0],t->filter.b[0],t->filter.cap[0]))return 0;
 /* Native B0738 reset, B0990..B09C0: both MG phases -1, cached outputs/
  * hold zero, S&H LFSR seed6d70 and pending-wrap flag1. Full-note inputs
  * still replace these with the owner-resolved ordered-head MG snapshot. */
 for(unsigned i=0;i<2;i++){n.mg[i].phase=-1;n.mg[i].noise=0x6d70u;n.mg[i].wrapped=1;}
 original_voice_gain_init(&n.gain);n.vca.state=n.vcf.state=6;n.ready=VOICE_READY;*s=n;return 1;
}
static void release(original_miami_selected_voice *s){s->key_down=0;s->damper_deferred=0;if(s->active){s->vcf.state=3;s->vca.state=3;}}
int original_miami_selected_voice_note_off(original_miami_selected_voice *s,const original_miami_voice_controls *c){
 if(!s||!c||s->ready!=VOICE_READY||c->ready!=CONTROL_READY||s->awaiting_restart)return 0;
 if(c->damper){s->key_down=0;s->damper_deferred=(uint8_t)(s->active!=0);}else release(s);return 1;
}
int original_miami_selected_voice_pedal(original_miami_selected_voice *s,const original_miami_voice_controls *c){
 if(!s||!c||s->ready!=VOICE_READY||c->ready!=CONTROL_READY||s->awaiting_restart)return 0;
 if(!c->damper&&s->damper_deferred)release(s);
 return 1;
}
static int preparation_tail(original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,uint32_t D){
 s->vcf_bipolar=s->vcf.level-1.0f;s->vca_bipolar=s->vca.level-1.0f;
 original_miami_pitch_controls pc=c->pitch;pc.unison_coordinate=s->unison_coordinate;pc.bend=bipolar(c->bend_normalized);pc.mod_slave_pitch=s->mod_slave;
 original_osc_controls oc;memset(&oc,0,sizeof oc);
 if(!original_miami_pitch_prepare(&s->pitch,&pc,t->pitch,44100,D,oc.increment))return 0;
 for(unsigned i=0;i<4;i++){
  oc.shape[i]=c->shape[i];
  oc.width[i]=c->wave_width[i];
 }
 oc.xmod=c->xmod;
 if(!s->prepared.ready||memcmp(&s->prepared.controls,&oc,sizeof oc)){
  if(!original_oscillator_prepare(&s->prepared,&oc))return 0;
  s->wave_preparations++;
 }
 original_voice_gain_controls gc=c->gain;gc.envelope=s->vca.level;gc.mod_gain=s->mod_gain;gc.velocity=s->velocity;gc.unison_flag=s->unison_flag;
 if(!original_voice_gain_prepare(&s->gain,&gc,s->activation,D))return 0;
 ms20_prepare(&s->filter,t,c,s->vcf.level,s->mod_cutoff,D);
 for(unsigned i=0;i<4;i++)if(s->activation[i]>0)s->activation[i]--;
 s->last_duration=D;s->control_updates++;return 1;
}
int original_miami_selected_voice_block_valid(const original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,float bpm){
 return valid(s,c,t,bpm)&&!s->awaiting_restart;
}
int original_miami_selected_voice_control_prevalidated(original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,float bpm,unsigned *action){
 if(s->awaiting_restart)return 0;
 *action=0;if(!s->active||s->vca.state==6)return 1;
 uint32_t D=0;int due=original_control_tick(&s->counter,44100,&D);if(due<0)return 0;if(!due)return 1;
 s->previous_vca=s->vca.level;s->previous_mg1=s->mg[0].bipolar;
 /* Three nonzero selected destinations, each a single ordered contribution.
  * This is exactly the eight-route graph specialization, not current feedback. */
 s->mod_slave=clamp(s->previous_vca*c->patch_slave+0.0f,-1,1);
 s->mod_cutoff=clamp(s->previous_mg1*c->patch_cutoff+0.0f,-1,1);
 s->mod_gain=clamp(s->previous_vca*c->patch_gain+0.0f,-1,1);
 for(unsigned i=0;i<2;i++){
  if(!mg_output(&s->mg[i],c->mg_shape[i]))return 0;
  mg_advance(&s->mg[i],c->mg_base[i],c->mg_times[i],bpm,D);
 }
 vcf_tick(&s->vcf,D);
 if(!original_vca_tick(&s->vca,44100,D,0,1,1,0,action))return 0;
 if(*action){s->active=0;s->vca.level=0;s->vcf.level=0;s->vcf.state=6;
  if(*action==2){s->pending_tail_duration=D;s->awaiting_restart=1;return 1;}
  for(unsigned i=0;i<4;i++)s->activation[i]=0;
 }
 /* Native ordinary retirement B3D94->CF08->B3EC8 reaches B3ECC: despite
  * cleared levels/states, recompute E-S and continue the preparation tail.
  * State5 returns earlier and resumes this tail only after D8F0 parts. */
 return preparation_tail(s,c,t,D);
}
int original_miami_selected_voice_control(original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,float bpm,unsigned *action){
 if(!action||!valid(s,c,t,bpm))return 0;
 return original_miami_selected_voice_control_prevalidated(s,c,t,bpm,action);
}
static int note_values(unsigned key,unsigned velocity,float pitch){return key<=127&&velocity>=1&&velocity<=127&&original_binary32_isfinite(pitch)&&pitch>=0&&pitch<=127;}
static int full_mask(original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,const original_miami_voice_note *note,unsigned mask,float bpm){
 original_miami_selected_voice n=*s;
 for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){n.pitch.target[i]=note->tuned_pitch;n.pitch.current[i]=(float)note->last_note;n.activation[i]=-1;}
 n.note_key=(uint8_t)note->note_key;n.raw_velocity=note->velocity;n.velocity=(float)note->velocity/127.0f;n.unison_coordinate=note->unison_coordinate;n.unison_flag=(uint8_t)note->unison_flag;
 for(unsigned i=0;i<2;i++){n.mg[i]=note->mg[i];if(!mg_output(&n.mg[i],c->mg_shape[i]))return 0;}
 if(!original_miami_ms20_note_reset(&n.filter,t->filter.a[0]))return 0;
 original_voice_gain_miami_full_note(&n.gain);n.vca=(original_vca){0,0};n.vcf=(original_vca){0,0};n.vca_bipolar=n.vcf_bipolar=0;
 n.key_down=1;n.damper_deferred=0;n.active=1;n.counter=44100;
 unsigned action;if(!original_miami_selected_voice_control(&n,c,t,bpm,&action))return 0;
 *s=n;return 1;
}
int original_miami_selected_voice_full_note(original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,const original_miami_voice_note *note,float bpm){
 if(!valid(s,c,t,bpm)||!note||s->active||s->vca.state!=6||s->awaiting_restart||!note_values(note->note_key,note->velocity,note->tuned_pitch)||note->last_note>127||!original_binary32_isfinite(note->unison_coordinate)||(note->unison_flag!=0&&note->unison_flag!=2))return 0;
 for(unsigned i=0;i<2;i++)if(!original_binary32_isfinite(note->mg[i].phase)||note->mg[i].phase< -1||note->mg[i].phase>1||!original_binary32_isfinite(note->mg[i].hold))return 0;
 return full_mask(s,c,t,note,15,bpm);
}
int original_miami_selected_voice_partial_note(original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,unsigned key,unsigned velocity,float pitch,unsigned mask,float bpm){
 if(!valid(s,c,t,bpm)||!s->active||s->awaiting_restart||!mask||mask>15||!note_values(key,velocity,pitch))return 0;
 original_miami_selected_voice n=*s;
 if(!original_miami_pitch_note(&n.pitch,key,0,mask,0))return 0;
 for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){n.pitch.target[i]=pitch;n.activation[i]=-1;}
 n.note_key=(uint8_t)key;n.raw_velocity=velocity;n.velocity=(float)velocity/127.0f;n.key_down=1;n.damper_deferred=0;
 for(unsigned i=0;i<2;i++)if(!mg_output(&n.mg[i],c->mg_shape[i]))return 0;
 unsigned action;if(!original_miami_selected_voice_control(&n,c,t,bpm,&action))return 0;
 *s=n;return 1;
}
int original_miami_selected_voice_sample(original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,float bpm,float *left,float *right,unsigned *action){
 if(!left||!right||left==right||!action||!valid(s,c,t,bpm))return 0;
 if(!original_miami_selected_voice_control(s,c,t,bpm,action))return 0;
 return original_miami_selected_voice_render_frame(s,c,left,right);
}
int original_miami_selected_voice_render_frame(original_miami_selected_voice *s,const original_miami_voice_controls *c,float *left,float *right){
 if(!s||!c||!left||!right||left==right||s->ready!=VOICE_READY||c->ready!=CONTROL_READY||s->awaiting_restart)return 0;
 return original_miami_selected_voice_render_prevalidated(s,c,left,right);
}
int original_miami_selected_voice_render_prevalidated(original_miami_selected_voice *s,const original_miami_voice_controls *c,float *left,float *right){
 if(s->awaiting_restart)return 0;
 if(!s->active){*left=0;*right=0;return 1;}
 float mix,filtered,L,R;
 if(!original_voice_gain_oscillator(&s->gain,&s->pitch.oscillator,&s->prepared,0,&mix)||!original_miami_ms20_sample(&s->filter,mix,c->vco_gain,&filtered)||!original_voice_gain_stereo(&s->gain,filtered,&L,&R))return 0;
 *left=L;*right=R;return 1;
}
unsigned original_miami_selected_voice_request_kind(const original_miami_selected_voice *s,unsigned full){
 if(!s||s->ready!=VOICE_READY||s->awaiting_restart||full>1||s->vca.state>6)return 0;
 return full?(s->vca.state==6?ORIGINAL_MIAMI_REQUEST_FULL:ORIGINAL_MIAMI_REQUEST_STAGE):ORIGINAL_MIAMI_REQUEST_PARTIAL;
}
int original_miami_selected_voice_stage_raw(original_miami_selected_voice *s){
 if(!s||s->ready!=VOICE_READY||s->awaiting_restart||s->vca.state>=6)return 0;
 s->vca.state=5;return 1;
}
int original_miami_selected_voice_stop_raw(original_miami_selected_voice *s){
 if(!s||s->ready!=VOICE_READY||s->awaiting_restart||s->vca.state>=6)return 0;
 s->key_down=0;s->vca.state=4;return 1;
}
static int raw_note_valid(const original_miami_voice_note *n,unsigned full){
 if(!n||n->note_key>127||n->velocity>127||n->unison_flag>2||!original_binary32_isfinite(n->tuned_pitch)||n->tuned_pitch<0||n->tuned_pitch>127||!original_binary32_isfinite(n->unison_coordinate))return 0;
 if(full){
  if(n->last_note>127)return 0;
  for(unsigned i=0;i<2;i++)if(!original_binary32_isfinite(n->mg[i].phase)||n->mg[i].phase< -1||n->mg[i].phase>1||!original_binary32_isfinite(n->mg[i].hold))return 0;
 }
 return 1;
}
int original_miami_selected_voice_immediate_raw(original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,const original_miami_voice_note *note,unsigned full,unsigned mask,float bpm,unsigned *action){
 if(!action||!valid(s,c,t,bpm)||s->awaiting_restart||full>1||!mask||mask>15||!raw_note_valid(note,full))return 0;
 if(full){if(s->vca.state!=6)return 0;int result=full_mask(s,c,t,note,mask,bpm);if(result)*action=0;return result;}
 original_miami_selected_voice n=*s;
 for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){n.pitch.target[i]=note->tuned_pitch;n.activation[i]=-1;}
 n.note_key=(uint8_t)note->note_key;n.raw_velocity=note->velocity;n.velocity=(float)note->velocity/127.0f;n.unison_flag=(uint8_t)note->unison_flag;n.unison_coordinate=note->unison_coordinate;n.active=1;n.key_down=1;n.damper_deferred=0;
 for(unsigned i=0;i<2;i++)if(!mg_output(&n.mg[i],c->mg_shape[i]))return 0;
 unsigned result;if(!original_miami_selected_voice_control(&n,c,t,bpm,&result))return 0;*s=n;*action=result;return 1;
}
int original_miami_selected_voice_resume_part(original_miami_selected_voice *s,const original_miami_voice_controls *c,const original_miami_voice_tables *t,const original_miami_voice_note *note,unsigned mask,unsigned final_part,float bpm){
 if(!valid(s,c,t,bpm)||!s->awaiting_restart||!raw_note_valid(note,1)||final_part>1||!mask||mask>15||((mask&(mask-1))&&mask!=15)||(mask==15&&!final_part))return 0;
 original_miami_selected_voice n=*s;uint32_t D=n.pending_tail_duration;n.awaiting_restart=0;
 if(!full_mask(&n,c,t,note,mask,bpm))return 0;
 if(final_part){if(!preparation_tail(&n,c,t,D))return 0;n.pending_tail_duration=0;}
 else n.awaiting_restart=1;
 *s=n;return 1;
}
int original_miami_selected_voice_core_reset(original_miami_selected_voice *s,const original_miami_voice_tables *t,unsigned index){
 if(!s||s->ready!=VOICE_READY||!t||t->ready!=TABLE_READY||index>=128)return 0;
 original_miami_selected_voice n;if(!original_miami_selected_voice_init(&n,t,index,60))return 0;
 n.pitch.oscillator=s->pitch.oscillator;for(unsigned i=0;i<4;i++)n.pitch.target[i]=0;
 n.velocity=s->velocity;n.unison_flag=1;
 n.filter.a.current=n.filter.a.target=fl(0x3a98fbd4u);n.filter.noise_gain.target=0;
 *s=n;return 1;
}
