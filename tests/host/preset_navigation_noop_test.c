/* SPDX-License-Identifier: GPL-3.0-only */
/* Real input/audio paths: an unchanged navigation target must not reload edits. */
#define CHICAGO_PRESENTATION_NO_MAIN
#include "chicago_presentation_test.c"
static void detent(int role,int direction)
{now_ms+=1000;enc_acc[role]=direction;ui_input_only();}
static void await_ready(int v)
{
    for(int i=0;i<5000 && engine_bass_preset_pending(v);i++)audio();
    CHECK(!engine_bass_preset_pending(v));
}
static void selected(int v,int bank,int preset)
{
    fixture(v,bank);CHECK(!engine_bass_select_preset(v,bank,preset,&proj.sound));await_ready(v);
    CHECK(!engine_bass_preset_pending(v));CHECK(engine_bass_preset_index(v)==preset);
    int param=bank?1:ORIGINAL_CHICAGO_CUTOFF;
    pref_t ref=PR(R_ENG,T_303,v,param);int value=pref_get(ref);
    pref_set(ref,value==127?126:value+1);CHECK(engine_bass_preset_edited(v));
    audio(); /* Drain the accepted edit before isolating wheel-deferred requests. */
    ui.dirty=save_wait=0;
}
static void unchanged_detent(int v,int role,int direction,int pending)
{
    sound_t before=proj.sound;unsigned calls=engine_bass_choice_serial();uint32_t rev=song_structure_revision;
    int preset=engine_bass_requested_preset(v),bank=engine_bass_requested_bank(v),page=cur_page();
    detent(role,direction);
    CHECK(engine_bass_choice_serial()==calls);CHECK(engine_bass_preset_pending(v)==pending);
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
        int target=last?(int)original_sound_factories(bank)-1:0,d=last?1:-1;
        for(int role=0;role<2;role++) {
            int encoder=role?EN_K2:EN_PRESET;
            selected(v,bank,target);sound_t before=proj.sound;
            for(int i=0;i<3;i++)audio();
            CHECK(!memcmp(&before,&proj.sound,sizeof before));
            unchanged_detent(v,encoder,d,0);
            /* A subsequent real adjacent target must still request and apply. */
            unsigned calls=engine_bass_choice_serial();detent(encoder,-d);CHECK(engine_bass_choice_serial()==calls+1);
            CHECK(engine_bass_requested_preset(v)==target-d);await_ready(v);
            CHECK(!engine_bass_preset_pending(v) && engine_bass_preset_index(v)==target-d);
            CHECK(!engine_bass_preset_edited(v));
            /* Preserve a pending endpoint without republishing or changing its
             * old edited cell; the existing request applies when wheels return. */
            selected(v,bank,target-d);engine_bass_wheel_owners(1u<<v);
            CHECK(!engine_bass_select_preset(v,bank,target,&proj.sound));CHECK(engine_bass_preset_pending(v)==3);
            unchanged_detent(v,encoder,d,3);engine_bass_wheel_owners(0);await_ready(v);
            CHECK(!engine_bass_preset_pending(v) && engine_bass_preset_index(v)==target);
            CHECK(!engine_bass_preset_edited(v));
        }
        /* ENGINE is another clamped navigation target, not reload preset zero. */
        selected(v,bank,target);unchanged_detent(v,EN_K1,bank?1:-1,0);
        unsigned calls=engine_bass_choice_serial();detent(EN_K1,bank?-1:1);CHECK(engine_bass_choice_serial()==calls+1);
        CHECK(engine_bass_requested_bank(v)==(bank^1));await_ready(v);CHECK(engine_bass_bank(v)==(bank^1));
        /* A no-op engine detent must preserve the requested bank, not compare
         * against the still-active bank while a wheel defers application. */
        selected(v,bank,target);engine_bass_wheel_owners(1u<<v);
        CHECK(!engine_bass_select_preset(v,bank^1,1,&proj.sound));CHECK(engine_bass_preset_pending(v)==3);
        unchanged_detent(v,EN_K1,(bank^1)?1:-1,3);engine_bass_wheel_owners(0);await_ready(v);
        CHECK(engine_bass_bank(v)==(bank^1) && engine_bass_preset_index(v)==1);
        /* Explicit API same-preset reload remains an intentional factory reset. */
        selected(v,bank,target);uint8_t expected[NPARAMS_MAX];
        CHECK(original_sound_init(expected,bank,target));calls=engine_bass_choice_serial();
        CHECK(!engine_bass_select_preset(v,bank,target,&proj.sound));CHECK(engine_bass_choice_serial()==calls+1);await_ready(v);
        CHECK(!engine_bass_preset_edited(v));CHECK(!memcmp(expected,proj.sound.v[T_303][v],sizeof expected));
    }
    /* USER (ffff) and out-of-range factory identities are not valid OS cells.
     * Invalid API navigation must not publish or overwrite an edited sound. */
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++) {
        selected(v,bank,0);sound_t before=proj.sound;
        unsigned serial=engine_bass_choice_serial();
        CHECK(engine_bass_select_preset(v,bank,-1,&proj.sound)==-1);
        CHECK(engine_bass_select_preset(v,bank,original_sound_factories(bank),&proj.sound)==-1);
        CHECK(engine_bass_choice_serial()==serial && !engine_bass_preset_pending(v));
        CHECK(!memcmp(&before,&proj.sound,sizeof before));
    }
    printf("PASS %u preset navigation no-op checks: both voices/banks/edges, K4/K6/K5, edits, pending, adjacent requests, invalid targets and explicit API reload\n",checks);return 0;
}
