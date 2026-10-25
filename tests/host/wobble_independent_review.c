#include "dsp/wobble_backend.h"
#include "app/synth_state.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void){
 wobble_t a,d,b; bass303_t old; synth_slot_t s; float l[258],r[258],dl[256],dr[256],m[258]; uint8_t bank[16],saved[16],ticket;
 assert(synth_init(&s,synth303_binding(&old))); wobble_foundation_init(&a); wobble_foundation_init(&b);
 for(unsigned i=0;i<16;i++)bank[i]=(uint8_t)((i*29)%128);
 assert(wobble_bank_load(&a,WOBBLE_EXPERIMENT_ENGINE,1,bank,16)); wobble_bank_save(&a,saved); assert(!memcmp(bank,saved,16));
 for(unsigned bad=0;bad<16;bad++){uint8_t corrupt[16];memcpy(corrupt,bank,16);corrupt[bad]=128; wobble_t before=a;assert(!wobble_bank_load(&a,WOBBLE_EXPERIMENT_ENGINE,1,corrupt,16));assert(!memcmp(&a,&before,sizeof a));}
 assert(!wobble_bank_load(&a,1,1,bank,16));assert(!wobble_bank_load(&a,WOBBLE_EXPERIMENT_ENGINE,2,bank,16));assert(!wobble_bank_load(&a,WOBBLE_EXPERIMENT_ENGINE,1,bank,15));
 assert(synth_request(&s,wobble_foundation_binding(&a),&ticket)); assert(!synth_request(&s,wobble_foundation_binding(&b),0)); assert(synth_boundary(&s));assert(s.acknowledged==ticket); assert(!synth_boundary(&s));
 synth_param_key_t k={WOBBLE_EXPERIMENT_ENGINE,1,W_STEREO};assert(synth_set(&s,k,127));k.schema=2;assert(!synth_set(&s,k,0));k.schema=1;k.param=16;assert(!synth_set(&s,k,0));k.param=0;k.engine=1;assert(!synth_set(&s,k,0));
 synth_note(&s,48,1,1,80); d=a;
 for(int n=1;n<=256;n++){ l[0]=r[0]=l[n+1]=r[n+1]=123;assert(wobble_backend_stereo(&s,l+1,r+1,n));wobble_render(&d,dl,dr,(size_t)n);assert(!memcmp(l+1,dl,n*sizeof(float)));assert(!memcmp(r+1,dr,n*sizeof(float)));assert(l[0]==123&&r[0]==123&&l[n+1]==123&&r[n+1]==123); }
 assert(!wobble_backend_stereo(&s,l,l,1));assert(!wobble_backend_stereo(&s,l,r,0));assert(!wobble_backend_stereo(&s,l,r,257));
 a=d; for(int n=1;n<=256;n++){m[0]=m[n+1]=123;synth_render(&s,m+1,n);wobble_render(&d,dl,dr,n);for(int i=0;i<n;i++)assert(m[i+1]==.5f*(dl[i]+dr[i]));assert(m[0]==123&&m[n+1]==123);}
 synth_off(&s);for(int i=0;i<10;i++)assert(wobble_backend_stereo(&s,l,r,256));assert(synth_idle(&s));for(int i=0;i<256;i++)assert(l[i]==0&&r[i]==0);
 synth_note(&s,60,0,0,99);assert(synth_active(&s));synth_transport(&s,0);assert(synth_idle(&s));wobble_bank_save(&a,saved);assert(saved[W_STEREO]==127);
 synth_note(&s,60,0,0,99);assert(synth_request(&s,synth303_binding(&old),&ticket));assert(synth_boundary(&s));assert(!a.active);assert(!wobble_backend_stereo(&s,l,r,256));
 uint8_t raw[20]={0},bytes[512];size_t size; synth_state_view_t v,v2; assert(!synth_state_migrate_legacy(raw,20,20,&v));v.part[0].selected_engine_id=WOBBLE_EXPERIMENT_ENGINE;v.part[0].engine_id=WOBBLE_EXPERIMENT_ENGINE;v.part[0].schema=1;v.part[0].size=16;v.part[0].bytes=bank;assert(!synth_state_encode(bytes,sizeof bytes,&v,&size));assert(!synth_state_decode(bytes,size,20,&v2));assert(!memcmp(v2.part[0].bytes,bank,16));assert(v2.part[0].selected_engine_id==WOBBLE_EXPERIMENT_ENGINE);assert(wobble_bank_load(&b,v2.part[0].engine_id,v2.part[0].schema,v2.part[0].bytes,v2.part[0].size));wobble_bank_save(&b,saved);assert(!memcmp(saved,bank,16));
 puts("Independent QA PASS: all fragments stereo exact and mono exact, guards, lifecycle ACK, stale keys, bank atomics/roundtrip and LXSE boundary");
}
