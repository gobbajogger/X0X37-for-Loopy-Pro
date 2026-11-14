/* Isolated genuine per-note DSP + LFO source probe, not a complete engine port.
 * Original S1NoteState arithmetic is extracted unchanged by the generator.
 * Omits AU/UI, event allocator, sequencer, smoothing, effects and persistence. */
#include "portable_interface.hpp"
#include "wave_frequencies.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
static double seconds(){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+1e-9*t.tv_nsec;}
int main(int argc,char**argv){
 if(argc!=4){fprintf(stderr,"usage: voice_probe public-wavetables.f32 local-preset-params.f32 frames\n");return 2;}
 unsigned frames=(unsigned)strtoul(argv[3],0,10);if(!frames||frames>441000)return 2;
 S1DSPKernel k{};sp_create(&k.sp);k.sp->sr=44100;memcpy(k.parameters,defaults,sizeof defaults);
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 for(int i=0;i<52;i++){sp_ftbl_create(k.sp,&k.ft_array[i],4096);if(fread(k.ft_array[i]->tbl,4,4096,f)!=4096)return 2;}
 fclose(f);for(int i=0;i<13;i++)k.ft_frequencyBand[i]=wave_frequencies[i];
 sp_ftbl_create(k.sp,&k.sine,4096);sp_gen_sine(k.sp,k.sine);
 sp_phasor_create(&k.lfo1Phasor);sp_phasor_create(&k.lfo2Phasor);sp_port_create(&k.lfo1Port);sp_port_create(&k.lfo2Port);
 FILE*p=fopen(argv[2],"rb");if(!p)return 2;
 unsigned programs=0,nonzero_programs=0,mono_programs=0;unsigned long long voice_calls=0;double sumsq=0,peak=0,render_seconds=0;
 S1NoteState v[6];float left,right;int notes[]={48,55,60,64,67,72};
 while(fread(k.parameters,4,150,p)==150){
  k.sp->rand=0;k.lfo1Smooth=k.lfo2Smooth=0;sp_phasor_init(k.sp,k.lfo1Phasor,0);sp_phasor_init(k.sp,k.lfo2Phasor,0);sp_port_init(k.sp,k.lfo1Port,.0053125f);sp_port_init(k.sp,k.lfo2Port,.0053125f);
  for(int i=0;i<6;i++){v[i].kernel=&k;v[i].init();v[i].startNoteHelper(notes[i],100,440.f*exp2((notes[i]-69.f)/12.f));}
  unsigned active=k.parameters[isMono]>0?1:6;mono_programs+=active==1;voice_calls+=(unsigned long long)active*frames;
  k.monoFrequencySmooth=440.f*exp2((72.f-69.f)/12.f);
  double local=0,t0=seconds();
  for(unsigned frame=0;frame<frames;frame++){
   if(frame==frames*3/4)for(int i=0;i<6;i++)v[i].internalGate=0;
   k.processLfo();left=right=0;for(unsigned i=0;i<active;i++)v[i].run(0,&left,&right);
   if(!isfinite(left)||!isfinite(right)){fprintf(stderr,"nonfinite program %u frame %u\n",programs,frame);return 1;}
   if(fabs(left)>peak)peak=fabs(left);local+=(double)left*left;
  }
  render_seconds+=seconds()-t0;sumsq+=local;if(local>0)nonzero_programs++;
  for(int i=0;i<6;i++)v[i].destroy();programs++;
 }
 fclose(p);
 printf("{\"scope\":\"up-to-six-note source voice+LFO probe; not complete engine or audio equivalence\",\"programs\":%u,\"nonzero_programs\":%u,\"frames_per_program\":%u,\"mono_programs\":%u,\"source_voice_calls\":%llu,\"nonfinite\":0,\"peak_pre_FX\":%.9g,\"sum_squares_pre_FX\":%.17g,\"render_seconds\":%.9g,\"host_ns_per_mixed_mono_poly_frame\":%.9g,\"sizeof_voice_adapter\":%zu}\n",programs,nonzero_programs,frames,mono_programs,voice_calls,peak,sumsq,render_seconds,render_seconds*1e9/(programs*frames),sizeof(S1NoteState));
 return programs==906?0:1;
}
