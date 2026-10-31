/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual SMK renderer and original engine. Hardware/clip painter are stubs. */
#define BOARD_SMK37 1
#define ORIGINAL_UI_MAIN original_ui_main
#include "original_release_ui_test.c"
#include "../../firmware/src/app/original_runtime.h"
uint8_t smk_clip_mode_on(void) { return 0; }
uint8_t smk_clip_transport_button(uint8_t b,uint8_t p) { (void)b;(void)p;return 0; }
uint8_t smk_clip_history_button(uint8_t b,uint8_t p,uint8_t h) { (void)b;(void)p;(void)h;return 0; }
void smk_play_note_button(uint8_t p,uint8_t h) { (void)p;(void)h; }
uint8_t smk_ck_flash_busy(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *p,uint32_t f) { (void)p;(void)f;return 0; }
uint8_t smk_clip_lcd_draw(uint32_t w,uint32_t h) { (void)w;(void)h;return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_apc_transport_led_state(uint8_t n) { (void)n;return 0; }
uint8_t smk_padcal_on(void) { return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_padcal_draw(uint32_t w,uint32_t h) { (void)w;(void)h;return 0; }

int smk_ck_smc_turn(int n,int d) { (void)n;(void)d;return 0; }
int smk_ck_smc_nav(uint8_t n) { (void)n;return 0; }
int plat_ble_smc_send(const uint8_t *m,uint8_t n) { (void)m;(void)n;return 1; }
int plat_ble_smc_send_slot(unsigned s,const uint8_t *m,uint8_t n) { (void)s;(void)m;(void)n;return 1; }
#include "../../firmware/src/app/smc_mixer.c"

static double luminance(uint16_t c)
{
    double v[3]={(c>>11)/31.0,((c>>5)&63)/63.0,(c&31)/31.0};
    for(unsigned i=0;i<3;i++) v[i]=v[i]<=.04045?v[i]/12.92:pow((v[i]+.055)/1.055,2.4);
    return .2126*v[0]+.7152*v[1]+.0722*v[2];
}
static void contrast(uint16_t ink,uint16_t bg)
{
    double a=luminance(ink)+.05,b=luminance(bg)+.05;
    CHECK((a>b?a/b:b/a)>=4.5);
}
static void theme_capture(const char *name,int view,int part,int page)
{
    project_t *before=malloc(sizeof proj);CHECK(before);memcpy(before,&proj,sizeof proj);
    capture(name,view,part,page);CHECK(!memcmp(before,&proj,sizeof proj));free(before);
}
static void glyph_bounds(void)
{
    unsigned slots=0;
    const int views[]={V_HOME,V_FX,V_SEQ,V_MIX,V_SONG,V_PART};
    for(unsigned vi=0;vi<sizeof views/sizeof views[0];vi++) for(int part=0;part<NPARTS;part++) {
        screen(views[vi],part,0);int count=pg.n;
        for(int page=0;page<count;page++) {
            screen(views[vi],part,page);
            for(int k=0;k<4;k++) {
                pref_t r=pg.r[cur_page()][k];const x0x_param_t *d=pref_desc(r);if(!d)continue;slots++;
                for(int edge=0;edge<3;edge++) {
                    int v=edge==0?0:edge==1?d->max/2:d->max;
                    cv_begin(240,KNOB_H,C_BLACK);control_glyph(r,v,30,C_HI);
                    for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)
                        if(x<10 || x>50 || y<26 || y>50)CHECK(cv_px[y*240+x]==swap16(C_BLACK));
                }
            }
        }
    }
    CHECK(slots>200);
}
static void themes(void)
{
    CHECK(PROJ_SND_REV==13 && PROJ_STREAM==13 && NPALETTES==6);
    CHECK(UI_FIRST_KNOB==5 && !strcmp(UI_CAP_EDIT,"PARA"));
    CHECK(!strcmp(caps_of("About DnBOX"),"ABOUT DnBOX"));
    CHECK(FONT_B.last>='n' && FONT_B.off[glyph(&FONT_B,'n')]!=FONT_B.off[glyph(&FONT_B,'N')]);
    for(unsigned theme=0;theme<6;theme++) {
        proj.set.palette=(uint8_t)theme;palette_set(theme);
        CHECK(project_validate_for(&proj)==0);
        CHECK(project_save()==0);proj.set.palette=0;
        CHECK(project_load()==0 && proj.set.palette==theme && proj.set.snd_rev==13);
        palette_set(proj.set.palette);
        contrast(SOUND_INK,SOUND_SELECT);contrast(C_WHITE,SOUND_FACE);contrast(C_GRAY,C_BLACK);
        if(theme==5) { CHECK(luminance(C_BLACK)>.8);contrast(C_HI,C_BLACK); }
        char name[80];
        for(unsigned kind=0;kind<2;kind++) {
            choose(0,kind,0);
            for(unsigned row=0;row<(kind?6:8);row++) {
                snprintf(name,sizeof name,"theme%u-%s-row%u",theme,kind?"miami":"chicago",row);
                theme_capture(name,V_PART,PART_303A,row);
                pref_t r=PR(R_ENG,T_303,0,3);CHECK(!strcmp(matrix_name(r),kind?"PITCH":"PEAK"));
                int mix=0;CHECK(wave_shape(PR(R_ENG,T_303,0,5),64,&mix)==0);
            }
        }
        const int views[]={V_HOME,V_FX,V_SEQ,V_MIX,V_SONG};
        for(unsigned i=0;i<sizeof views/sizeof views[0];i++) {
            snprintf(name,sizeof name,"theme%u-view%d",theme,views[i]);theme_capture(name,views[i],PART_909,0);
        }
        screen(V_HOME,PART_909,0);open_global();
        for (unsigned i=0;i<ui.list_n;i++) if(list_rows[i].kind==R_PALETTE) ui.list_sel=(uint8_t)i;
        ui.list_top=ui.list_sel>3?ui.list_sel-3:0;draw_main();snprintf(name,sizeof name,"theme%u-settings",theme);shot(name);
        draw_boot_screen(0);snprintf(name,sizeof name,"theme%u-boot",theme);shot(name);
    }
    project_t *saved=malloc(sizeof proj);CHECK(saved);memcpy(saved,&proj,sizeof proj);
    proj.set.palette=6;CHECK(project_validate_for(&proj)!=0);memcpy(&proj,saved,sizeof proj);free(saved);
    uint8_t custom[BOOT_REC_MAX]={BOOT_KIND_CUSTOM,BOOT_F_PIC,5,5};
    memcpy(custom+4,"DnBOX",5);memcpy(custom+16,"STILL",5);
    boot_p16(custom+40,240);boot_p16(custom+42,96);custom[BOOT_PAY_FIXED]=0x80;
    CHECK(boot_payload_ok(custom,sizeof custom));draw_boot_screen(custom);shot("custom-still");
    CHECK(fb[20*240]==swap16(C_HI));
    char a[13],b[22];boot_lines(0,a,b);CHECK(!strcmp(a,"DnBOX") && !strcmp(b,"909 808 CHICAGO MIAMI"));
}
unsigned engine_original_tail_state(int v);
static void switch_tail(void)
{
    palette_set(1);
    /* A retained decimator sample is signal history, not a held note. Seed
     * that exact documented state independently of any chosen factory tail. */
    original_runtime_part *residual=calloc(1,sizeof *residual);uint8_t cell[32];CHECK(residual);
    CHECK(original_sound_init(cell,1,29) && original_runtime_init(residual,cell,125,0,0)==1);
    residual->dsp.miami.output.fx.decimator.kernel.held_left=1;
    CHECK(original_runtime_active(residual) && !original_runtime_owned(residual));
    CHECK(original_runtime_retire(residual)==1);free(residual);
    for(unsigned kind=0;kind<2;kind++) {
        choose(1,kind,kind?29:54);
        if(!kind) {
            pref_set(PR(R_ENG,T_303,1,13),0); /* ARP off */
            pref_set(PR(R_ENG,T_303,1,7),0);pref_set(PR(R_ENG,T_303,1,9),0);pref_set(PR(R_ENG,T_303,1,10),0);
        } else pref_set(PR(R_ENG,T_303,1,10),127); /* Real Miami decimator. */
        block();engine_bass_on_velocity(1,60,0,0,120);
        for(unsigned i=0;i<40;i++) block();
        engine_bass_off(1);CHECK(engine_bass_select_preset(1,1-kind,0,&proj.sound)==0);
        unsigned observed_tail=0,blocks=0,cancelled=0;int32_t out[512];
        for(;blocks<4000 && engine_bass_preset_pending(1);blocks++) {
            unsigned state=engine_original_tail_state(1);
            int live=(state & 0x10000u)!=0; unsigned quiet=state & 0xffffu,gain=state>>17;
            if(!live && quiet<512) {
                CHECK(engine_bass_preset_pending(1)==2);observed_tail++;
                if(!cancelled && gain>0 && gain<512) {
                    uint8_t saved[32];memcpy(saved,proj.sound.v[T_303][1],32);
                    engine_bass_cancel_preset(1);engine_render(out,1);
                    CHECK(!engine_bass_preset_pending(1));
                    CHECK(engine_original_tail_state(1)>>17==gain+1);
                    CHECK(!memcmp(saved,proj.sound.v[T_303][1],32));
                    CHECK(engine_bass_select_preset(1,1-kind,0,&proj.sound)==0);cancelled++;
                }
            }
            int ready=engine_bass_preset_pending(1)==1;
            if(ready) CHECK(quiet>=512 && !live);
            engine_render(out,256);
            if(!engine_bass_preset_pending(1)) {
                CHECK(ready);
                for(unsigned i=0;i<512;i++) CHECK(abs(out[i])<=1);
            }
        }
        CHECK(!engine_bass_preset_pending(1));
        if(!kind) CHECK(observed_tail>0 && cancelled==1);
        CHECK(engine_bass_bank(1)==(int)(1-kind) && engine_bass_preset_index(1)==0);
        printf("tail_switch kind=%u quiet_samples=512 threshold=1/65536 tail_blocks=%u total_blocks=%u cancellation=%u\n",kind,observed_tail,blocks,cancelled);
    }
}

static void history_without_factory_request(void)
{
    choose(1,0,54);
    pref_set(PR(R_ENG,T_303,1,13),0);
    pref_set(PR(R_ENG,T_303,1,7),0);pref_set(PR(R_ENG,T_303,1,9),0);pref_set(PR(R_ENG,T_303,1,10),0);block();
    uint8_t before[32];memcpy(before,proj.sound.v[T_303][1],32);undo_start();
    pref_set(PR(R_ENG,T_303,1,12),32);block();CHECK(undo_commit_stable());
    engine_bass_on_velocity(1,60,0,0,120);for(unsigned i=0;i<40;i++)block();engine_bass_off(1);
    unsigned i;for(i=0;i<4000 && (engine_original_tail_state(1)&0x10000u);i++)block();
    CHECK(i<4000 && !engine_bass_preset_pending(1));
    do_undo(0);CHECK(!undo_wait && !memcmp(before,proj.sound.v[T_303][1],32));
    /* Cold undo reset has zero quiet samples and no factory fade owner. */
    CHECK((engine_original_tail_state(1)&0xffffu)==0);
    do_undo(1);CHECK(!undo_wait && proj.sound.v[T_303][1][12]==32);
}

/* Candidate-pinned production captures for every firmware screen family.
 * Navigation, values and pixel geometry remain owned by ui.c.  This is a
 * render regression, not an invented graphical mockup or a DSP test. */
static void capture_all_screen_families(void)
{
    static const struct {
        const char *name;
        int view, part, page;
    } cases[] = {
        {"home",V_HOME,PART_909,0},
        {"sound-909",V_PART,PART_909,0},
        {"sound-808",V_PART,PART_808,0},
        {"sound-chicago",V_PART,PART_303B,0},
        {"sound-chicago-selector",V_PART,PART_303B,7},
        {"sound-miami",V_PART,PART_303A,0},
        {"sound-miami-selector",V_PART,PART_303A,5},
        {"break",V_PART,PART_BRK,0},
        {"seq-909",V_SEQ,PART_909,0},
        {"seq-808",V_SEQ,PART_808,0},
        {"seq-chicago",V_SEQ,PART_303B,0},
        {"seq-miami",V_SEQ,PART_303A,0},
        {"seq-break",V_SEQ,PART_BRK,0},
        {"generate",V_GEN,PART_303A,0},
        {"fx",V_FX,PART_909,0},
        {"mix-parts",V_MIX,PART_909,0},
        {"mix-909",V_MIX,PART_909,1},
        {"mix-808",V_MIX,PART_808,2},
        {"mix-master",V_MIX,PART_909,3},
        {"song",V_SONG,PART_909,0},
        {"global",V_GLO,PART_909,0},
        {"performance",V_PERF,PART_909,0},
    };
    choose(0, ORIGINAL_SOUND_MIAMI, 0);
    choose(1, ORIGINAL_SOUND_CHICAGO, 0);
    for (unsigned theme = 0; theme < 6; ++theme) {
        proj.set.palette = (uint8_t)theme;
        palette_set(theme);
        for (unsigned i = 0; i < sizeof cases/sizeof cases[0]; ++i) {
            char name[100];
            CHECK(snprintf(name,sizeof name,"theme%u-all-%s",theme,cases[i].name)>0);
            theme_capture(name,cases[i].view,cases[i].part,cases[i].page);
        }
    }
}

int main(int argc,char **argv)
{
    image_dir=argc==2?argv[1]:0;load_force=0;boot();settle();
    projection_cases();pages_and_edits();selector_cases();locks_and_undo();
    themes();glyph_bounds();capture_all_screen_families();switch_tail();history_without_factory_request();
    printf("PASS ui_theme checks=%u captures=%u; production SMK renderer, no hardware\n",checks,captures);
    return 0;
}
