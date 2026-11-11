/* SPDX-License-Identifier: GPL-3.0-only */
/* Include the real implementation; unused functions are removed at link time. */
#include "../../firmware/src/app/original_sound.c"
#include <stdio.h>
#include <stdint.h>
static uint32_t bits(float f) { uint32_t u; memcpy(&u,&f,4); return u; }
static int old_value(unsigned kind,unsigned parameter,unsigned control,float *value)
{
    const x0x_param_t *p=original_sound_param(kind,parameter);
    if(!value||!p||control>p->max)return 0;
    if(kind==ORIGINAL_SOUND_CHICAGO&&parameter==ORIGINAL_CHICAGO_FX_TYPE)
        *value=(float)original_sound_fx_macros[control]/11.0f;
    else if(original_sound_centered(kind,parameter))
        *value=control<64u?(float)control/128.0f:0.5f+(float)(control-64u)/126.0f;
    else *value=(float)control/127.0f;
    return 1;
}
int main(void)
{
    unsigned checked=0;
    for(unsigned kind=0;kind<4;kind++)
        for(unsigned p=0;p<24;p++)
            for(unsigned c=0;c<257;c++) {
                float old=-123.0f,next=old;
                int a=old_value(kind,p,c,&old),b=original_sound_control_value(kind,p,c,&next);
                if(a!=b||bits(old)!=bits(next)||original_sound_control_value(kind,p,c,0)!=0) {
                    fprintf(stderr,"FAIL kind=%u param=%u control=%u old=%08x new=%08x\n",kind,p,c,bits(old),bits(next));
                    return 1;
                }
                checked++;
            }
    printf("PASS: %u exhaustive control/parameter/kind cases, rejection outputs and null outputs; const=816 B\n",checked);
    return 0;
}
