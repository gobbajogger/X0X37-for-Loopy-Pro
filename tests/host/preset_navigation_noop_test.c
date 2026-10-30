/* SPDX-License-Identifier: GPL-3.0-only */
/* Real input/audio paths: an unchanged navigation target must not reload edits. */
#define CHICAGO_PRESENTATION_NO_MAIN
#define bass_publish_test_hook endpoint_base_publish_hook
#include "chicago_presentation_test.c"
#undef bass_publish_test_hook
#include "../../firmware/src/app/bass_presets.h"
static unsigned publications;
void bass_publish_test_hook(int v,int stage)
{if(stage==2)publications++;endpoint_base_publish_hook(v,stage);}
static void detent(int role,int direction)
{now_ms+=1000;enc_acc[role]=direction;ui_input_only();}
static void selected(int v,int bank,int preset)
{
    fixture(v,bank);CHECK(!engine_bass_select_preset(v,bank,preset,&proj.sound));audio();
    CHECK(!engine_bass_preset_pending(v));CHECK(engine_bass_preset_index(v)==preset);
    int param=bank?W_CARRIER_LEVEL:BASS303_CUTOFF;
    pref_t ref=PR(R_ENG,T_303,v,param);int value=pref_get(ref);
    pref_set(ref,value==127?126:value+1);CHECK(engine_bass_preset_edited(v));
    audio(); /* Drain the accepted edit before isolating wheel-deferred requests. */
    ui.dirty=save_wait=0;
}
static void unchanged_detent(int v,int role,int direction,int pending)
{
    sound_t before=proj.sound;unsigned calls=publications;uint32_t rev=song_structure_revision;
    int preset=engine_bass_requested_preset(v),bank=engine_bass_requested_bank(v),page=cur_page();
    detent(role,direction);
    CHECK(publications==calls);CHECK(engine_bass_preset_pending(v)==pending);
    CHECK(engine_bass_requested_preset(v)==preset && engine_bass_requested_bank(v)==bank);
    CHECK(!memcmp(&before,&proj.sound,sizeof before));CHECK(ui.dirty==0 && save_wait==0);
    CHECK(song_structure_revision==rev && cur_page()==page);
    for(int i=0;i<3;i++)audio();
    CHECK(engine_bass_preset_pending(v)==pending);CHECK(!memcmp(&before,&proj.sound,sizeof before));
    CHECK(engine_bass_preset_edited(v));
}
int main(void)
{
    boot();run_ms(50);engine_guard_enable(0);
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++)for(int last=0;last<2;last++) {
        int target=last?factory_bank(bank)->npresets-1:0,d=last?1:-1;
        for(int role=0;role<2;role++) {
            int encoder=role?EN_K2:EN_PRESET;
            selected(v,bank,target);sound_t before=proj.sound;
            for(int i=0;i<3;i++)audio();CHECK(!memcmp(&before,&proj.sound,sizeof before));
            unchanged_detent(v,encoder,d,0);
            /* A subsequent real adjacent target must still request and apply. */
            unsigned calls=publications;detent(encoder,-d);CHECK(publications==calls+1);
            CHECK(engine_bass_requested_preset(v)==target-d);audio();
            CHECK(!engine_bass_preset_pending(v) && engine_bass_preset_index(v)==target-d);
            CHECK(!engine_bass_preset_edited(v));
            /* Preserve a pending endpoint without republishing or changing its
             * old edited cell; the existing request applies when wheels return. */
            selected(v,bank,target-d);engine_bass_wheel_owners(1u<<v);
            CHECK(!engine_bass_select_preset(v,bank,target,&proj.sound));CHECK(engine_bass_preset_pending(v)==3);
            unchanged_detent(v,encoder,d,3);engine_bass_wheel_owners(0);audio();
            CHECK(!engine_bass_preset_pending(v) && engine_bass_preset_index(v)==target);
            CHECK(!engine_bass_preset_edited(v));
        }
        /* ENGINE is another clamped navigation target, not reload preset zero. */
        selected(v,bank,target);unchanged_detent(v,EN_K1,bank?1:-1,0);
        unsigned calls=publications;detent(EN_K1,bank?-1:1);CHECK(publications==calls+1);
        CHECK(engine_bass_requested_bank(v)==(bank^1));audio();CHECK(engine_bass_bank(v)==(bank^1));
        /* A no-op engine detent must preserve the requested bank, not compare
         * against the still-active bank while a wheel defers application. */
        selected(v,bank,target);engine_bass_wheel_owners(1u<<v);
        CHECK(!engine_bass_select_preset(v,bank^1,1,&proj.sound));CHECK(engine_bass_preset_pending(v)==3);
        unchanged_detent(v,EN_K1,(bank^1)?1:-1,3);engine_bass_wheel_owners(0);audio();
        CHECK(engine_bass_bank(v)==(bank^1) && engine_bass_preset_index(v)==1);
        /* Explicit API same-preset reload remains an intentional factory reset. */
        selected(v,bank,target);uint8_t expected[NPARAMS_MAX];
        CHECK(!bass_preset_decode(bank,target,engine_bass_bank_param,expected));calls=publications;
        CHECK(!engine_bass_select_preset(v,bank,target,&proj.sound));CHECK(publications==calls+1);audio();
        CHECK(!engine_bass_preset_edited(v));CHECK(!memcmp(expected,proj.sound.v[T_303][v],sizeof expected));
    }
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++)for(int role=0;role<2;role++)for(int d=-1;d<=1;d+=2) {
        selected(v,bank,0);proj.sound.bass[v].bytes[8]=proj.sound.bass[v].bytes[9]=0xff;
        proj.sound.bass[v].bytes[10]=BASS_SOUND_EDITED;CHECK(!engine_bass_sound_validate(&proj.sound));
        engine_bass_edit_hold(1);engine_bass_base_restore(1u<<v);engine_bass_edit_hold(0);audio();
        CHECK(engine_bass_preset_index(v)==-1);unsigned calls=publications;
        detent(role?EN_K2:EN_PRESET,d);CHECK(publications==calls+1);CHECK(engine_bass_requested_preset(v)==0);
        audio();CHECK(engine_bass_preset_index(v)==0 && !engine_bass_preset_edited(v));
    }
    printf("PASS %u preset navigation no-op checks: both voices/banks/edges, K4/K6/K5, edits, pending, adjacent requests, USER and explicit API reload\n",checks);return 0;
}
