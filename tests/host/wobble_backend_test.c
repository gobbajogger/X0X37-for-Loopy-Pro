/* SPDX-License-Identifier: GPL-3.0-only */
#include "dsp/wobble_backend.h"
#include "app/synth_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
 wobble_t a, direct, other; bass303_t native;
 synth_slot_t slot; float l[256],r[256],dl[256],dr[256],mono[256];
 uint8_t bank[16],saved[16],blob[256],ticket; size_t used;
 assert(synth_init(&slot,synth303_binding(&native)));
 wobble_foundation_init(&a);
 for(unsigned i=0;i<16;++i) bank[i]=(uint8_t)(i*8);
 assert(wobble_bank_load(&a,WOBBLE_EXPERIMENT_ENGINE,1,bank,16));
 wobble_bank_save(&a,saved); assert(!memcmp(bank,saved,16));
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
 bad=key; bad.param=W_DECAY; assert(synth_set(&slot,bad,64));
 bad.param=19; assert(!synth_set(&slot,bad,0));
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
 synth_off(&slot); for(int i=0;i<16;++i)synth_render(&slot,mono,256);
 assert(synth_idle(&slot)); for(int i=0;i<256;++i)assert(mono[i]==0);
 wobble_bank_save(&a,saved); synth_note(&slot,55,0,0,100); synth_transport(&slot,0);
 assert(synth_idle(&slot)); wobble_bank_save(&a,bank);assert(!memcmp(bank,saved,16));
 synth_state_view_t view,decoded; const uint8_t legacy[4]={1,3,0,3};
 assert(!synth_state_migrate_legacy(legacy,4,4,&view));
 view.part[0].selected_engine_id=WOBBLE_EXPERIMENT_ENGINE;
 view.part[0].engine_id=WOBBLE_EXPERIMENT_ENGINE;view.part[0].schema=1;
 view.part[0].bytes=bank;view.part[0].size=16;
 assert(!synth_state_encode(blob,sizeof blob,&view,&used));
 assert(!synth_state_decode(blob,used,4,&decoded));
 assert(decoded.part[0].selected_engine_id==WOBBLE_EXPERIMENT_ENGINE);
 wobble_foundation_init(&other);
 assert(wobble_bank_load(&other,decoded.part[0].engine_id,decoded.part[0].schema,decoded.part[0].bytes,decoded.part[0].size));
 wobble_bank_save(&other,saved);assert(!memcmp(bank,saved,16));
 assert(!memcmp(decoded.legacy,legacy,4));
 assert(synth_request(&slot,synth303_binding(&native),&ticket));assert(synth_boundary(&slot));
 assert(!a.active && !a.gate && slot.acknowledged==ticket);
 puts("PASS wobble stereo/mono fragments1..256, lifecycle/ACK, stale keys, transactional bank and LXSE roundtrip");
 return 0;
}
