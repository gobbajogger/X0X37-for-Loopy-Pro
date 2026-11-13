/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../../firmware/src/app/ui_waveform.h"
/* Reference evaluator of the Miami value-cell shapes (removed wobble DSP). */
#include "fixtures/miami_waveform_ref.h"
int main(void)
{
 unsigned checks=0;float maxerr=0;
 for(int pulse=0;pulse<2;pulse++) for(int selected=0;selected<2;selected++)
  for(int i=0;i<8192;i++) {
   float p=(float)i/8192.0f;
   float actual=pulse?mod_waveform(p,UI_WAVE_REFERENCE_STEP,selected):waveform(p,UI_WAVE_REFERENCE_STEP,selected);
   float preview=ui_miami_wave_value(pulse,selected,p),err=fabsf(actual-preview);
   if(err>maxerr)maxerr=err;
   assert(err<=1e-6f);checks++;
  }
 assert(ui_miami_wave_value(1,1,.05f)>.99f);
 assert(fabsf(ui_miami_wave_value(1,1,.5f)+1.0f/9.0f)<1e-6f);
 assert(ui_miami_wave_value(0,0,0)==1 && ui_miami_wave_value(0,0,.5f)==-1);
 /* Fourier square is not an ideal two-level box. Preserve its actual slope. */
 assert(fabsf(ui_miami_wave_value(0,1,0))<1e-6f);
 assert(fabsf(ui_miami_wave_value(0,1,.1f)-ui_miami_wave_value(0,1,.25f))>.001f);
 printf("PASS %u exact evaluator samples, max error %.9g; positive10%% pulse, negativeDC low, triangle, Fourier shape\n",checks,maxerr);
 return 0;
}
