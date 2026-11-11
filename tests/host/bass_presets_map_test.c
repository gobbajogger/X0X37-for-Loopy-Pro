/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/factory_banks.c"
#include "../../firmware/src/app/bass_presets.h"
#include <assert.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static const x0x_param_t *actual(int bank,int i)
{
    if(bank==FACTORY_BANK_CHICAGO)return engine_param(T_303,1,i);
#if X0X_DEV_WOBBLE
    if(bank==FACTORY_BANK_MIAMI)return engine_param(T_303,0,i);
#endif
    return NULL;
}
static const x0x_param_t *wrong_surface(int bank,int i)
{
    (void)bank;return engine_param(T_303,1,i);
}
int main(void)
{
    uint8_t values[32],before[32];factory_preset_t copy;
    static const uint8_t acid[32]={42,127,87,45,64,31,64,87,0,1,35,7,51,0,4,1,1,12,74,51,45,0,4,0,1,0,8,1,13,127,64,0};
#if X0X_DEV_WOBBLE
    static const uint8_t cutz[23]={1,115,127,63,11,98,0,75,70,76,22,1,83,54,101,32,64,0,0,0,127,0,36};
#endif
    boot();
    CHECK(factory_bank_count()==2);
    CHECK(!strcmp(factory_preset_name(0,0),"Acid Lab 1"));
    CHECK(!strcmp(factory_preset_name(1,0),"16th Cutz"));
    CHECK(bass_preset_decode(0,0,actual,values)==BASS_PRESET_OK);
    CHECK(!memcmp(values,acid,sizeof acid));
    for(int bank=0;bank<2;bank++){
        const factory_bank_t *b=factory_bank(bank);
        CHECK(b && b->npresets==(bank?30:81));
        for(unsigned n=0;n<b->npresets;n++){
            const factory_preset_t *p=factory_preset(bank,(int)n);copy=*p;
            memset(values,0xa5,sizeof values);memcpy(before,values,sizeof before);
            int status=bass_preset_decode(bank,(int)n,actual,values);
#if !X0X_DEV_WOBBLE
            if(bank){CHECK(status==BASS_PRESET_UNAVAILABLE);CHECK(!memcmp(values,before,sizeof values));continue;}
#endif
            CHECK(status==BASS_PRESET_OK);CHECK(!memcmp(&copy,p,sizeof copy));
            for(int i=0;i<(bank?23:32);i++)CHECK(values[i]<=actual(bank,i)->max);
            if(bank){for(int i=18;i<23;i++)CHECK(values[i]==actual(bank,i)->def);for(int i=23;i<32;i++)CHECK(!values[i]);}
        }
    }
#if X0X_DEV_WOBBLE
    CHECK(bass_preset_decode(1,0,actual,values)==BASS_PRESET_OK);CHECK(!memcmp(values,cutz,sizeof cutz));
    copy=*factory_preset(1,0);
    for(unsigned i=0;i<4;i++){
        static const uint8_t booleans[]={0,6,11,17};
        copy.pots[booleans[i]]=127;CHECK(bass_preset_decode_record(1,&copy,actual,values)==0);CHECK(values[booleans[i]]==0);
        copy.pots[booleans[i]]=128;CHECK(bass_preset_decode_record(1,&copy,actual,values)==0);CHECK(values[booleans[i]]==1);
    }
    CHECK(bass_preset_decode(1,0,wrong_surface,values)==BASS_PRESET_INVALID);
#endif
    copy=*factory_preset(0,0);copy.pots[12]=0;
    CHECK(bass_preset_decode_record(0,&copy,actual,values)==0);CHECK(values[BASS303_VOLUME]==0); /* no offline floor */
    memset(values,0xa5,sizeof values);memcpy(before,values,sizeof before);
    copy.nparams--;CHECK(bass_preset_decode_record(0,&copy,actual,values)==BASS_PRESET_INVALID);CHECK(!memcmp(values,before,sizeof values));
    CHECK(bass_preset_decode(2,0,actual,values)==BASS_PRESET_INVALID);CHECK(!memcmp(values,before,sizeof values));
    CHECK(bass_preset_decode(0,-1,actual,values)==BASS_PRESET_INVALID);CHECK(!memcmp(values,before,sizeof values));
    CHECK(bass_preset_decode(0,81,actual,values)==BASS_PRESET_INVALID);CHECK(!memcmp(values,before,sizeof values));
    CHECK(bass_preset_decode(0,0,NULL,values)==BASS_PRESET_INVALID);CHECK(!memcmp(values,before,sizeof values));
    copy=*factory_preset(0,0);memset(copy.name,'x',sizeof copy.name);
    CHECK(bass_preset_decode_record(0,&copy,actual,values)==BASS_PRESET_INVALID);CHECK(!memcmp(values,before,sizeof values));
    printf("PASS %u complete-preset mapping checks; Chicago81 decoded, Miami30 %s; no live application claim\n",checks,X0X_DEV_WOBBLE?"decoded":"unavailable");
    return 0;
}
