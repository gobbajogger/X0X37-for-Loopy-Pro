/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual stock-preset/engine/UI path; all sample and panel bytes are synthetic. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/factory_banks.c"
#include "../../firmware/src/app/bass_presets.h"
#include "../../experiments/wobble/wobble.h"
#include <assert.h>
static float observed_hz;
static unsigned calls;
int miami_observe_tempo(wobble_t *w,float bpm)
{
    int rc=wobble_set_tempo(w,bpm);
    observed_hz=w->lfo_step*w->sample_rate;
    calls++;
    return rc;
}
static void block(void){int32_t audio[512];engine_render(audio,256);}
static project_t saved;
static uint8_t samples[sizeof slot_img],panel[PLAT_STORE_MAX];
int main(int argc,char **argv)
{
    static const unsigned division[8]={2,3,4,6,8,12,16,24};
    boot();engine_guard_enable(0);proj.set.autosave_off=1;seq.playing=0;
    for(unsigned i=0;i<sizeof slot_img;i++)((uint8_t *)slot_img)[i]=(uint8_t)(i*37u+11u);
    for(unsigned i=0;i<PLAT_STORE_MAX;i++)store[OBJ_PANEL][i]=(uint8_t)(i*19u+7u);
    store_len[OBJ_PANEL]=PLAT_STORE_MAX;
    memcpy(samples,slot_img,sizeof samples);memcpy(panel,store[OBJ_PANEL],sizeof panel);
    saved=proj;unsigned writes=store_writes;
    for(int preset=0;preset<FACTORY_MIAMI_NPRESETS;preset++) {
        const factory_preset_t *p=factory_preset(FACTORY_BANK_MIAMI,preset);
        unsigned bin=((unsigned)p->pots[W_WOBBLE_RATE]*7u+127u)/255u;
        assert(!engine_bass_select_preset(0,FACTORY_BANK_MIAMI,preset,&proj.sound));
        seq.ext=0;seq.bpm=120;block();assert(!engine_bass_preset_pending(0));
        assert(calls && fabsf(observed_hz-division[bin]*.5f)<1e-5f);
        assert(!engine_bass_sound_validate(&proj.sound));
    }
    assert(factory_banks_intact());
    for(unsigned bin=0;bin<8;bin++) {
        int value=(int)((bin*127u+3u)/7u);
        pref_set(PR(R_ENG,T_303,0,W_WOBBLE_RATE),value);
        seq.ext=0;seq.bpm=120;block();
        assert(fabsf(observed_hz-division[bin]*.5f)<1e-5f);
        seq.bpm=60;block();assert(fabsf(observed_hz-division[bin]*.25f)<1e-5f);
        seq.ext=1;seq.ext_bpm=180;block();assert(fabsf(observed_hz-division[bin]*.75f)<1e-5f);
        char number[24],unit[16],want[16];
        pref_value_of(PR(R_ENG,T_303,0,W_WOBBLE_RATE),value,number,unit);
        snprintf(want,sizeof want,"1/%u",division[bin]);
        assert(!strcmp(number,want) && !*unit);
        if(argc>1) {
            char path[512];ui.part=PART_303A;ui.view=V_PART;ui.overlay=O_NONE;ui.held_step=-1;
            build_pages();*page_of(V_PART)=2;ui.msg_until=0;ui.touched=-1;
            memset(blit_hash,0,sizeof blit_hash);draw_header();draw_main();draw_knobs();
            snprintf(path,sizeof path,"%s/rate-%u.png",argv[1],division[bin]);png_write(path);
        }
    }
    assert(store_writes==writes); /* Browsing does not implicitly save. */
    assert(!memcmp(saved.pat,proj.pat,sizeof saved.pat));
    assert(!memcmp(&saved.arr,&proj.arr,sizeof saved.arr));
    assert(!memcmp(samples,slot_img,sizeof samples));
    assert(!memcmp(panel,store[OBJ_PANEL],sizeof panel));
    /* Existing revision/schema and sound bytes round-trip; no migration. */
    seq.ext=0;seq.bpm=120;seq.playing=0;block();
    assert(!project_save());saved=proj;
    memset(&proj,0,sizeof proj);assert(!project_load());
    assert(!memcmp(&saved,&proj,sizeof proj));
    assert(!memcmp(samples,slot_img,sizeof samples));
    assert(!memcmp(panel,store[OBJ_PANEL],sizeof panel));
    puts("PASS 30 stock Miami presets; all8 division labels+internal/external tempo; project round-trip, patterns, samples and panel bytes preserved; no implicit save");
    return 0;
}
