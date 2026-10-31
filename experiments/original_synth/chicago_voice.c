/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_voice.h"
#include "binary32_classify.h"
#include <math.h>
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
/* exact: its FMA feeds a recursion; fast FMA moved the output by -78 dBFS (docs/perf) */
#define X0X_EXACT_FMA_HERE 1
#include "fused_reference.h"
/* Exact shortcut: when a*b is exact in binary32 (a zero factor, or the two
 * significands fit in 24 bits and the product is normal), the fused result
 * is the single rounding of p+c, which is the float add. Else the exact
 * software FMA. Same bits as original_rne_fmaf for all finite inputs. */
static inline uint32_t cv_bits(float x){uint32_t u;memcpy(&u,&x,4);return u;}
static inline float cv_fma(float a,float b,float c){
 uint32_t ua=cv_bits(a),ub=cv_bits(b),uc=cv_bits(c);
 uint32_t ea=(ua>>23)&255u,eb=(ub>>23)&255u;
 if(((uc>>23)&255u)!=255u&&ea!=255u&&eb!=255u){
  if(!(ua<<1)||!(ub<<1))return a*b+c;
  if(ea&&eb){
   uint32_t ma=(ua&0x7fffffu)|0x800000u,mb=(ub&0x7fffffu)|0x800000u;
   if((24u-(unsigned)__builtin_ctz(ma))+(24u-(unsigned)__builtin_ctz(mb))<=24u){
    float p=a*b;uint32_t ep=(cv_bits(p)>>23)&255u;
    if(ep&&ep!=255u)return p+c;
   }
  }
 }
 return original_rne_fmaf(a,b,c);
}
#define ref_fma cv_fma
#else
#define ref_fma fmaf
#endif
#define TABLE_READY 0x43565431u
#define CONTROL_READY 0x43564331u
#define VOICE_READY 0x73u
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static float clamp(float x,float a,float b){return x<a?a:x>b?b:x;}
static float bipolar(float x){return ref_fma(x,2,-1);}
static unsigned Q(float x,unsigned n){unsigned q=(unsigned)ref_fma(x,(float)(n-1),.5f);return q<n?q:n-1;}
static float native(const original_chicago_program *p,unsigned id){float x=0;(void)original_chicago_program_native(p,id,&x);return x;}
static int valid(const original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,float bpm){return s&&c&&t&&s->ready==VOICE_READY&&c->ready==CONTROL_READY&&t->ready==TABLE_READY&&original_binary32_isfinite(bpm)&&bpm>=20&&bpm<=400;}
int original_chicago_voice_tables_bind(original_chicago_voice_tables *out,original_mp4_reader read,const void *context,const int32_t pitch[128]){
 if(!out||!read||!pitch)return 0;
 for(unsigned i=0;i<128;i++)if(pitch[i]<=0||(i&&pitch[i]<=pitch[i-1]))return 0;
 float a,e;if(!read(context,0,&a,&e)||!original_binary32_isfinite(a)||a<0||a>1||!original_binary32_isfinite(e)||e< -1||e>1)return 0;
 original_chicago_voice_tables t;memset(&t,0,sizeof t);t.read=read;t.context=context;t.pitch=pitch;t.a0=a;t.e0=e;t.ready=TABLE_READY;*out=t;return 1;
}
int original_chicago_voice_compile(original_chicago_voice_controls *out,const original_chicago_program *p){
 if(!out||!p)return 0;
 for(unsigned i=0;i<22;i++)if(!original_binary32_isfinite(p->macro[i])||p->macro[i]<0||p->macro[i]>1)return 0;
 for(unsigned i=0;i<29;i++)if(!original_binary32_isfinite(p->target[i])||p->target[i]<0||p->target[i]>1)return 0;
 original_chicago_voice_controls c;memset(&c,0,sizeof c);
 c.pitch.master_tune=bipolar(native(p,0));c.pitch.unison_detune=native(p,90);c.pitch.bend_range=24.0f*native(p,55);c.pitch.transpose=(int)Q(native(p,18),49)-24;c.pitch.detune=native(p,17);c.pitch.portamento=native(p,16);
 for(unsigned i=0;i<4;i++){unsigned j=i*4;c.shape[i]=(uint8_t)Q(native(p,j+1),5);if(c.shape[i]>3)return 0;c.pitch.semitone[i]=(int)Q(native(p,j+2),49)-24;c.pitch.fine_tune[i]=i?bipolar(native(p,j)):0;c.gain.level[i]=native(p,j+3);c.vcf[i]=native(p,28+i);c.vca[i]=native(p,32+i);}
 if(!original_chicago_program_routing(p,&c.routing))return 0;
 c.gain.output=native(p,94);c.gain.output=c.gain.output*c.gain.output;c.gain.pan=bipolar(native(p,93));c.gain.curve=2;c.gain.ms20=0;c.gain.unison_flag=255;
 c.cutoff=native(p,24);c.resonance=native(p,25);c.env_depth=bipolar(native(p,26));c.noise=native(p,23);c.pw_intensity=native(p,21);c.pw_width=native(p,22);c.vco_gain=ref_fma(native(p,102),3,1);
 for(unsigned i=0;i<4;i++)if(!original_pw_width(c.shape[i],c.pw_intensity,0,c.pw_width,0,0,&c.wave_width[i]))return 0;
 float track=native(p,104);c.keyboard_track=(track>fl(0x3f2a3d71)&&track<fl(0x3f2ac083))?1:track*1.5f;
 if(!original_xmod_coefficient(native(p,39),0,&c.xmod))return 0;
 c.patch_cutoff=bipolar(native(p,80));c.patch_gain=bipolar(native(p,81));c.patch_slave=bipolar(native(p,82));
 static const float base[10]={96,48,36,24,16,12,8,6,4,3};
 for(unsigned i=0;i<2;i++){c.mg_shape[i]=(uint8_t)Q(native(p,i?49:43),5);if(c.mg_shape[i]>3)return 0;c.mg_frequency[i]=native(p,i?50:44);c.mg_tempo[i]=(uint8_t)Q(native(p,i?52:46),2);c.mg_base[i]=base[Q(native(p,i?53:47),10)];c.mg_times[i]=(float)(16-Q(native(p,i?54:48),16));}
 c.revision=p->revision;c.ready=CONTROL_READY;*out=c;return 1;
}
static int mg_output(original_chicago_voice_mg *m,unsigned shape){if(!original_miami_mg_shape(shape,m->phase,&m->bipolar,&m->unipolar))return 0;m->wrapped=0;return 1;}
static int mg_advance(original_chicago_voice_mg *m,const original_chicago_voice_controls *c,unsigned i,float bpm,uint32_t D){
 float delta;
 if(c->mg_tempo[i]){float numerator=(bpm*fl(0x3f4ccccd))*(float)D;float denominator=(44100.0f*c->mg_base[i])*c->mg_times[i];delta=numerator/denominator;if(delta>1)delta=1;}
 else {float f=clamp(c->mg_frequency[i]+0.0f,0,1),f2=f*f;f=f*f2;f=ref_fma(f,fl(0x4347e666),fl(0x3c23d70a));delta=((float)D*f)/44100.0f;}
 float next=m->phase+delta;if(!original_binary32_isfinite(next))return 0;if(next>=1){next=next-2;m->wrapped=1;}m->phase=next;return 1;
}
static float fourth(float x){float y=x*x;return y*y;}
static void vcf_tick(original_vca *s,const float c[4],uint32_t D){
 float e=s->level;
 if(s->state==0){float numerator=(float)D*(1.2999999523162842f-e),den=44100.0f*ref_fma(fourth(c[0]),13,fl(0x3a83126f));e=e+numerator/den;if(e>=1){e=1;s->state=1;}}
 else if(s->state==1){if(e<c[2])e=c[2];else {float numerator=(float)D*(c[2]-e),den=ref_fma(fourth(c[1]),25,fl(0x3a83126f))*44100.0f;e=e+numerator/den;e=(e+1)-1;}}
 else if(s->state==3){if(e>=fl(0x3727c5ac)){float den=ref_fma(fourth(c[3]),25,fl(0x3b03126f))*44100.0f;e=e*(1.0f-(float)D/den);e=(e+1)-1;}else{e=0;s->state=6;}}
 s->level=e;
}
static void stage(original_ramp *r,float target,float reciprocal){r->target=target;r->delta=(target-r->current)*reciprocal;}
static float advance(original_ramp *r){float next=r->current+r->delta;if((r->delta>0)!=(r->target>next))next=r->target;r->current=next;return next;}
/* Chicago uses raw voice10 in MP4 tracking, including selector-generated>127.
 * Preserve native arithmetic instead of imposing the older helper's MIDI bound.
 * The accepted immutable packed reader and sample recurrence stay unchanged. */
static int mp4_prepare(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,uint32_t D,unsigned kick){
 float depth=clamp(c->env_depth+0.0f,-1,1),z=ref_fma(s->vcf.level,depth,c->cutoff),q=z*16384.0f;
 float key=(float)s->raw_note;key=key+(-48.0f);key=key*128.0f;q=ref_fma(key,c->keyboard_track,q);q=ref_fma(s->mod_cutoff,16384.0f,q);if(!original_binary32_isfinite(q))return 0;q=clamp(q,0,16384);
 float a,e;if(!t->read(t->context,(uint32_t)q,&a,&e))return 0;
 float r=clamp(c->resonance+0.0f,0,1),inv=1.0f/(float)D;stage(&s->filter.a,a,inv);stage(&s->filter.e,e,inv);stage(&s->filter.resonance,r,inv);
 if(kick){float r2=r*r,r4=r2*r2,r8=r4*r4;s->filter.history[0]=r>.2f?r8:0;}
 s->filter.prepared=1;s->filter_postgain=clamp(ref_fma(c->resonance+0.0f,4,1),1,4);return 1;
}
static int pitch_prepare(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,uint32_t D,uint32_t increment[4]){
 const original_chicago_voice_pitch_controls *p=&c->pitch;
 float common=p->master_tune+0.0f;common=ref_fma(s->unison_coordinate,p->unison_detune,common);common=ref_fma(0,48,common);common=common+(float)p->transpose;common=ref_fma(p->bend_range,c->routing.bend_bipolar,common);
 float porta=clamp(p->portamento+0.0f,0,1),alpha=porta*porta;alpha=ref_fma(alpha,fl(0x49f42400),fl(0x3a83126f));alpha=44100.0f*alpha;alpha=(float)D/alpha;alpha=alpha*fl(0x4a8ab75d);if(alpha>1)alpha=1;
 float detune=clamp(p->detune+0.0f,0,1)*.3499999940395355f;
 for(unsigned i=0;i<4;i++){
  s->pitch.current[i]=ref_fma(s->pitch.target[i]-s->pitch.current[i],alpha,s->pitch.current[i]);
  float value=common+s->pitch.current[i];value=value+(float)p->semitone[i];value=ref_fma(0,48,value);
  if(i){value=p->fine_tune[i]+value;if(i==1)value=value-detune;else if(i==3)value=detune+value;}
  if(i==1||i==3){value=0.0f+value;value=ref_fma(s->mod_slave,48,value);}
  if(!original_binary32_isfinite(value)||!original_pitch_increment(value,t->pitch,&increment[i]))return 0;
 }
 return 1;
}
static int tail(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,uint32_t D,unsigned kick){
 s->vcf_bipolar=s->vcf.level-c->vcf[2];s->vca_bipolar=s->vca.level-(c->routing.vca_eg_mode?1.0f:c->vca[2]);
 original_osc_controls oc;memset(&oc,0,sizeof oc);if(!pitch_prepare(s,c,t,D,oc.increment))return 0;
 for(unsigned i=0;i<4;i++){oc.shape[i]=c->shape[i];oc.width[i]=c->wave_width[i];}oc.xmod=c->xmod;
 if(!s->prepared.ready||memcmp(&s->prepared.controls,&oc,sizeof oc)){if(!original_oscillator_prepare(&s->prepared,&oc))return 0;s->wave_preparations++;}
 float ng=clamp(c->noise+0.0f,0,1);ng=ng*ng+fl(0x3727c5ac);stage(&s->noise_gain,ng,1.0f/(float)D);
 if(!mp4_prepare(s,c,t,D,kick))return 0;
 original_voice_gain_controls gain;memset(&gain,0,sizeof gain);memcpy(gain.level,c->gain.level,sizeof gain.level);gain.output=c->gain.output;gain.pan=c->gain.pan;gain.curve=c->gain.curve;gain.unison_flag=c->gain.unison_flag;gain.ms20=c->gain.ms20;gain.envelope=s->vca.level;gain.mod_gain=s->mod_gain;gain.velocity=s->velocity;gain.filter_gain=s->filter_postgain;
 if(!original_voice_gain_prepare(&s->gain,&gain,s->activation,D))return 0;
 for(unsigned i=0;i<4;i++)if(s->activation[i]>0)s->activation[i]--;
 s->last_duration=D;s->control_updates++;return 1;
}
int original_chicago_voice_init(original_chicago_voice *s,const original_chicago_voice_tables *t,const original_chicago_voice_seed *seed){
 if(!s||!t||t->ready!=TABLE_READY||!seed||!original_binary32_isfinite(seed->last_pitch)||!original_binary32_isfinite(seed->noise_gain.current)||!original_binary32_isfinite(seed->noise_gain.target)||!original_binary32_isfinite(seed->noise_gain.delta))return 0;
 original_chicago_voice n;memset(&n,0,sizeof n);
 for(unsigned i=0;i<2;i++)if(!original_binary32_isfinite(seed->mg[i].phase)||seed->mg[i].phase< -1||seed->mg[i].phase>1||!original_binary32_isfinite(seed->mg[i].hold))return 0;
 if(!original_chicago_mp4_init(&n.filter,t->a0,t->e0))return 0;
 for(unsigned i=0;i<4;i++){n.pitch.oscillator.phase[i]=seed->oscillator_phase[i];n.pitch.current[i]=n.pitch.target[i]=seed->last_pitch;}
 n.noise=seed->noise;n.noise_gain=seed->noise_gain;n.mg[0]=seed->mg[0];n.mg[1]=seed->mg[1];n.vcf.state=n.vca.state=6;n.unison_flag=1;n.ready=VOICE_READY;*s=n;return 1;
}
static void release(original_chicago_voice *s){s->key_down=0;s->damper_deferred=0;if(s->active){s->vcf.state=3;s->vca.state=3;}}
int original_chicago_voice_note_off(original_chicago_voice *s,const original_chicago_voice_controls *c){if(!s||!c||s->ready!=VOICE_READY||c->ready!=CONTROL_READY||s->awaiting_restart)return 0;if(c->routing.damper>=64){s->key_down=0;s->damper_deferred=(uint8_t)(s->active!=0);}else release(s);return 1;}
int original_chicago_voice_pedal(original_chicago_voice *s,const original_chicago_voice_controls *c){if(!s||!c||s->ready!=VOICE_READY||c->ready!=CONTROL_READY||s->awaiting_restart)return 0;if(c->routing.damper<64&&s->damper_deferred)release(s);return 1;}
int original_chicago_voice_stop(original_chicago_voice *s){if(!s||s->ready!=VOICE_READY||s->awaiting_restart)return 0;if(s->active){s->key_down=0;s->vca.state=4;}return 1;}
int original_chicago_voice_defer_restart(original_chicago_voice *s){if(!s||s->ready!=VOICE_READY||s->awaiting_restart||!s->active)return 0;s->vca.state=5;return 1;}
int original_chicago_voice_block_ready(const original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,float bpm){return valid(s,c,t,bpm)&&!s->awaiting_restart;}
int original_chicago_voice_control_admitted(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,float bpm,unsigned kick,unsigned *action){
 *action=0;if(!s->active||s->vca.state==6)return 1;
 uint32_t D=0;int due=original_control_tick(&s->counter,44100,&D);if(due<0)return 0;if(!due)return 1;
 s->previous_vcf=s->vcf.level;s->previous_vca=s->vca.level;s->previous_mg[0]=s->mg[0].bipolar;s->previous_mg[1]=s->mg[1].bipolar;
 s->mod_cutoff=clamp(s->previous_vcf*c->patch_cutoff+0.0f,-1,1);s->mod_gain=clamp(s->keyboard_source*c->patch_gain+0.0f,-1,1);s->mod_slave=clamp(s->previous_vca*c->patch_slave+0.0f,-1,1);
 for(unsigned i=0;i<2;i++)if(!mg_output(&s->mg[i],c->mg_shape[i])||!mg_advance(&s->mg[i],c,i,bpm,D))return 0;
 vcf_tick(&s->vcf,c->vcf,D);
 const float gate[4]={0,0,1,0};const float *eg=c->routing.vca_eg_mode?gate:c->vca;
 if(!original_vca_tick(&s->vca,44100,D,eg[0],eg[1],eg[2],eg[3],action))return 0;
 if(*action){s->active=0;s->vca.level=0;s->vcf.level=0;s->vcf.state=6;
  if(*action==2){s->pending_tail_duration=D;s->pending_tail_kick=(uint8_t)kick;s->awaiting_restart=1;return 1;}
  for(unsigned i=0;i<4;i++)s->activation[i]=0;
 }
 return tail(s,c,t,D,kick);
}
int original_chicago_voice_control(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,float bpm,unsigned kick,unsigned *action){
 if(!action||!original_chicago_voice_block_ready(s,c,t,bpm)||kick>1)return 0;
 return original_chicago_voice_control_admitted(s,c,t,bpm,kick,action);
}
static int note_valid(const original_chicago_voice_note *n){if(!n||n->raw_note<0||n->raw_note>187||n->velocity>127||n->unison_flag>2||!original_binary32_isfinite(n->tuned_pitch)||!original_binary32_isfinite(n->last_pitch)||!original_binary32_isfinite(n->unison_coordinate))return 0;for(unsigned i=0;i<2;i++)if(!original_binary32_isfinite(n->mg[i].phase)||n->mg[i].phase< -1||n->mg[i].phase>1||!original_binary32_isfinite(n->mg[i].hold))return 0;return 1;}
static int full_note_mask(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_voice_note *note,unsigned mask,float bpm){
 original_chicago_voice n=*s;for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){n.pitch.target[i]=note->tuned_pitch;n.pitch.current[i]=note->last_pitch;n.activation[i]=-1;}
 n.raw_note=note->raw_note;n.raw_velocity=note->velocity;n.velocity=(float)note->velocity/127.0f;n.keyboard_source=((float)note->raw_note-64.0f)*.015625f;n.unison_coordinate=note->unison_coordinate;n.unison_flag=(uint8_t)note->unison_flag;
 for(unsigned i=0;i<2;i++){n.mg[i]=note->mg[i];if(!mg_output(&n.mg[i],c->mg_shape[i]))return 0;}
 if(!original_chicago_mp4_init(&n.filter,t->a0,t->e0))return 0;
 original_voice_gain_miami_full_note(&n.gain);n.vca=(original_vca){0,0};n.vcf=(original_vca){0,0};n.vcf_bipolar=n.vca_bipolar=0;n.key_down=1;n.damper_deferred=0;n.active=1;n.counter=44100;
 unsigned action;if(!original_chicago_voice_control(&n,c,t,bpm,1,&action))return 0;*s=n;return 1;
}
int original_chicago_voice_full_note(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_voice_note *note,float bpm){
 if(!valid(s,c,t,bpm)||!note_valid(note)||s->active||s->vca.state!=6||s->awaiting_restart)return 0;
 return full_note_mask(s,c,t,note,15,bpm);
}
int original_chicago_voice_partial_note(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,int32_t key,unsigned velocity,float pitch,unsigned mask,float bpm){
 if(!valid(s,c,t,bpm)||!s->active||s->awaiting_restart||s->vca.state==5||key<0||key>187||velocity>127||!original_binary32_isfinite(pitch)||!mask||mask>15)return 0;
 original_chicago_voice n=*s;for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){n.pitch.target[i]=pitch;n.activation[i]=-1;}
 n.raw_note=key;n.raw_velocity=velocity;n.velocity=(float)velocity/127.0f;n.keyboard_source=((float)key-64.0f)*.015625f;n.key_down=1;n.damper_deferred=0;
 for(unsigned i=0;i<2;i++)if(!mg_output(&n.mg[i],c->mg_shape[i]))return 0;
 unsigned action;if(!original_chicago_voice_control(&n,c,t,bpm,1,&action))return 0;*s=n;return 1;
}
int original_chicago_voice_resume_restart(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_voice_note *note,float bpm){
 return original_chicago_voice_resume_part(s,c,t,note,15,1,bpm);
}
int original_chicago_voice_resume_part(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_voice_note *note,unsigned mask,unsigned final_part,float bpm){
 if(!valid(s,c,t,bpm)||!s->awaiting_restart||!note_valid(note)||final_part>1||!mask||mask>15||((mask&(mask-1))&&mask!=15)||(mask==15&&!final_part))return 0;
 original_chicago_voice n=*s;uint32_t D=n.pending_tail_duration;unsigned kick=n.pending_tail_kick;n.awaiting_restart=0;
 if(!full_note_mask(&n,c,t,note,mask,bpm))return 0;
 if(final_part){if(!tail(&n,c,t,D,kick))return 0;n.pending_tail_duration=0;n.pending_tail_kick=0;}
 else n.awaiting_restart=1;
 *s=n;return 1;
}
int original_chicago_voice_kernel(original_voice_gain *vg,original_oscillators *ph,const original_prepared_oscillators *p,
 original_ramp *noise_gain,uint32_t *noise_state,float vco_gain,original_chicago_mp4 *f,float *left,float *right);
int original_chicago_voice_render_frame(original_chicago_voice *s,const original_chicago_voice_controls *c,float *left,float *right){
 if(!s||!c||!left||!right||left==right||s->ready!=VOICE_READY||c->ready!=CONTROL_READY||s->awaiting_restart)return 0;
 if(!s->active){*left=0;*right=0;return 1;}
 return original_chicago_voice_kernel(&s->gain,&s->pitch.oscillator,&s->prepared,&s->noise_gain,&s->noise,c->vco_gain,&s->filter,left,right);
}
int original_chicago_voice_span(original_voice_gain *vg,original_oscillators *ph,const original_prepared_oscillators *p,
 original_ramp *noise_gain,uint32_t *noise_state,float vco_gain,original_chicago_mp4 *f,uint32_t *left,uint32_t *right,uint32_t n);
/* n>=1 frames of render_frame with no control tick due inside. */
int original_chicago_voice_render_span(original_chicago_voice *s,const original_chicago_voice_controls *c,uint32_t *left,uint32_t *right,uint32_t n){
 if(!s||!c||!left||!right||left==right||!n||s->ready!=VOICE_READY||c->ready!=CONTROL_READY||s->awaiting_restart)return 0;
 if(!s->active){for(uint32_t i=0;i<n;i++)left[i]=right[i]=0;return 1;}
 return original_chicago_voice_span(&s->gain,&s->pitch.oscillator,&s->prepared,&s->noise_gain,&s->noise,c->vco_gain,&s->filter,left,right,n);
}
int original_chicago_voice_sample(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,float bpm,float *left,float *right,unsigned *action){
 if(!left||!right||left==right||!action||!valid(s,c,t,bpm)||s->awaiting_restart)return 0;
 if(!original_chicago_voice_control(s,c,t,bpm,0,action))return 0;
 if(*action==2)return 2;
 return original_chicago_voice_render_frame(s,c,left,right);
}
static int source_note_valid(const original_chicago_voice_note *n){return n&&n->raw_note>=0&&n->raw_note<=187&&n->velocity<=127&&n->unison_flag<=2&&original_binary32_isfinite(n->unison_coordinate);}
unsigned original_chicago_voice_request_kind(const original_chicago_voice *s,unsigned full){
 if(!s||s->ready!=VOICE_READY||s->vca.state>6||s->awaiting_restart||full>1)return 0;
 return full?(s->vca.state==6?ORIGINAL_CHICAGO_REQUEST_FULL:ORIGINAL_CHICAGO_REQUEST_STAGE):ORIGINAL_CHICAGO_REQUEST_PARTIAL;
}
int original_chicago_voice_stage_raw(original_chicago_voice *s){
 if(!s||s->ready!=VOICE_READY||s->vca.state>=6||s->awaiting_restart)return 0;
 s->vca.state=5;return 1;
}
int original_chicago_voice_source_values(const original_chicago_voice_note *n,original_chicago_voice_sources *out){
 if(!out||!source_note_valid(n))return 0;
 original_chicago_voice_sources v;v.velocity_unipolar=(float)n->velocity/127.0f;v.velocity_bipolar=(float)((int)n->velocity-64)/63.0f;v.key_unipolar=(float)n->raw_note*.0078125f;v.key_bipolar=(float)(n->raw_note-64)*.015625f;*out=v;return 1;
}
int original_chicago_voice_immediate_raw(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_voice_note *note,unsigned full,unsigned mask,float bpm,unsigned *action){
 if(!action||!valid(s,c,t,bpm)||s->awaiting_restart||full>1||!mask||mask>15||!source_note_valid(note)||!original_binary32_isfinite(note->tuned_pitch))return 0;
 if(full){
  if(s->vca.state!=6||!note_valid(note))return 0;
  original_chicago_voice n=*s;if(!full_note_mask(&n,c,t,note,mask,bpm))return 0;
  *s=n;*action=0;return 1;
 }
 original_chicago_voice n=*s;for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){n.pitch.target[i]=note->tuned_pitch;n.activation[i]=-1;}
 original_chicago_voice_sources sources;if(!original_chicago_voice_source_values(note,&sources))return 0;
 n.raw_note=note->raw_note;n.raw_velocity=note->velocity;n.velocity=sources.velocity_unipolar;n.keyboard_source=sources.key_bipolar;n.unison_flag=(uint8_t)note->unison_flag;n.unison_coordinate=note->unison_coordinate;n.key_down=1;n.damper_deferred=0;n.active=1;
 for(unsigned i=0;i<2;i++)if(!mg_output(&n.mg[i],c->mg_shape[i]))return 0;
 unsigned result;if(!original_chicago_voice_control(&n,c,t,bpm,1,&result))return 0;
 *s=n;*action=result;return 1;
}
