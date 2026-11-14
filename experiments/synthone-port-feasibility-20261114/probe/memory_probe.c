/* Isolated allocation probe, not a complete Synth One port.
 * Replays Soundpipe allocations from Synth One S1DSPKernel.mm:32-96 and
 * S1NoteState.mm:33-86 at source 6466a3715c96b7ecf1dd255a218cbd571408a314.
 * Prints requested live allocation bytes, excluding libc allocator overhead,
 * Objective-C objects, C++ containers and parameter wrappers. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "soundpipe.h"
#include "soundpipeextension.h"
static size_t live_bytes, requests;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t n) { void *p=__real_malloc(n); if(!p){fputs("allocation failed\n",stderr);exit(2);}live_bytes+=n;requests++;return p;}
static void mark(const char *label, size_t *before) { printf("%s=%zu\n",label,live_bytes-*before);*before=live_bytes; }
static void note(sp_data *sp) {
 sp_adsr *adsr,*fadsr;sp_oscmorph2d *o1,*o2;sp_crossfade *c1,*c2;
 sp_osc *sub;sp_fosc *fm;sp_noise *noise;sp_moogladder *lp;sp_butbp *bp;sp_buthp *hp;
 sp_adsr_create(&adsr);sp_adsr_init(sp,adsr);sp_adsr_create(&fadsr);sp_adsr_init(sp,fadsr);
 sp_oscmorph2d_create(&o1);sp_oscmorph2d_create(&o2);
 sp_crossfade_create(&c1);sp_crossfade_init(sp,c1);sp_crossfade_create(&c2);sp_crossfade_init(sp,c2);
 sp_osc_create(&sub);sp_fosc_create(&fm);sp_noise_create(&noise);sp_noise_init(sp,noise);
 sp_moogladder_create(&lp);sp_moogladder_init(sp,lp);sp_butbp_create(&bp);sp_butbp_init(sp,bp);sp_buthp_create(&hp);sp_buthp_init(sp,hp);
}
int main(void) {
 sp_data *sp;size_t before=0;sp_create(&sp);mark("soundpipe_context",&before);
 sp_ftbl *table[53];for(int i=0;i<53;i++)sp_ftbl_create(sp,&table[i],4096);mark("53_wavetables",&before);
 sp_vdelay *vd[4];for(int i=0;i<4;i++){sp_vdelay_create(&vd[i]);sp_vdelay_init(sp,vd[i],10.0f);}mark("four_10second_vdelays",&before);
 sp_revsc *rv;sp_revsc_create(&rv);sp_revsc_init(sp,rv);mark("reverbsc",&before);
 sp_delay *wd;sp_delay_create(&wd);sp_delay_init(sp,wd,.05f);mark("50ms_widen_delay",&before);
 for(int i=0;i<7;i++)note(sp);mark("seven_notestate_DSP_allocations",&before);
 for(int i=0;i<150+1+2;i++){sp_port*p;sp_port_create(&p);sp_port_init(sp,p,.1f);}
 for(int i=0;i<2;i++){sp_phasor*p;sp_phasor_create(&p);sp_phasor_init(sp,p,0);}
 sp_phaser *ph;sp_phaser_create(&ph);sp_phaser_init(sp,ph);
 sp_osc *panosc;sp_osc_create(&panosc);sp_osc_init(sp,panosc,table[0],0);
 sp_pan2 *pan;sp_pan2_create(&pan);sp_pan2_init(sp,pan);
 for(int i=0;i<2;i++){sp_moogladder*p;sp_moogladder_create(&p);sp_moogladder_init(sp,p);}
 for(int i=0;i<4;i++){sp_crossfade*p;sp_crossfade_create(&p);sp_crossfade_init(sp,p);}
 for(int i=0;i<2;i++){sp_buthp*p;sp_buthp_create(&p);sp_buthp_init(sp,p);}
 for(int i=0;i<6;i++){sp_compressor*p;sp_compressor_create(&p);sp_compressor_init(sp,p);}
 mark("other_DSP_globals_and_compressors",&before);
 printf("requested_total_bytes=%zu\nallocation_requests=%zu\nfloat_bytes=%zu\npointer_bytes=%zu\n",live_bytes,requests,sizeof(SPFLOAT),sizeof(void*));
 return 0;
}
