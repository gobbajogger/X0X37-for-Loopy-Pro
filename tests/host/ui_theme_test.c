/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual SMK renderer and original engine. Hardware/clip painter are stubs. */
#define BOARD_SMK37 1
#define ORIGINAL_UI_MAIN original_ui_main
#include "original_release_ui_test.c"
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
static void themes(void)
{
    CHECK(PROJ_SND_REV==13 && PROJ_STREAM==13 && NPALETTES==6);
    CHECK(UI_FIRST_KNOB==5 && !strcmp(UI_CAP_EDIT,"PARA"));
    CHECK(FONT_B.last>='n' && glyph(&FONT_B,'n')!=glyph(&FONT_B,'N'));
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
                capture(name,V_PART,PART_303A,row);
                pref_t r=PR(R_ENG,T_303,0,3);CHECK(!strcmp(matrix_name(r),kind?"PITCH":"PEAK"));
                int mix=0;CHECK(wave_shape(PR(R_ENG,T_303,0,5),64,&mix)==0);
            }
        }
        const int views[]={V_HOME,V_FX,V_SEQ,V_MIX,V_SONG};
        for(unsigned i=0;i<sizeof views/sizeof views[0];i++) {
            snprintf(name,sizeof name,"theme%u-view%d",theme,views[i]);capture(name,views[i],PART_909,0);
        }
        screen(V_HOME,PART_909,0);open_global();draw_main();snprintf(name,sizeof name,"theme%u-settings",theme);shot(name);
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
static void switch_tail(void)
{
    palette_set(1);choose(1,0,54);
    /* Existing factory FX remains selected. A short voice feeds its real tail. */
    pref_set(PR(R_ENG,T_303,1,13),0); /* ARP off */
    pref_set(PR(R_ENG,T_303,1,7),0);pref_set(PR(R_ENG,T_303,1,9),0);pref_set(PR(R_ENG,T_303,1,10),0);
    block();engine_bass_on_velocity(1,60,0,0,120);
    for(unsigned i=0;i<40;i++) block();
    engine_bass_off(1);CHECK(engine_bass_select_preset(1,1,29,&proj.sound)==0);
    unsigned observed_tail=0,blocks=0;int32_t out[512];
    for(;blocks<4000 && engine_bass_preset_pending(1);blocks++) {
        int live=original_runtime_active(&original_bass[1]);
        if(!live && original_quiet_samples[1]<512) {
            CHECK(engine_bass_preset_pending(1)==2);observed_tail++;
        }
        int ready=engine_bass_preset_pending(1)==1;
        if(ready) CHECK(original_quiet_samples[1]>=512 && !live);
        engine_render(out,256);
        if(!engine_bass_preset_pending(1)) {
            CHECK(ready); /* Adoption only after a measured sub-LSB quiet interval. */
            for(unsigned i=0;i<512;i++) CHECK(abs(out[i])<=1);
        }
    }
    CHECK(observed_tail>0 && !engine_bass_preset_pending(1));
    CHECK(engine_bass_bank(1)==1 && engine_bass_preset_index(1)==29);
    printf("tail_switch quiet_samples=512 threshold=1/65536 tail_blocks=%u total_blocks=%u\n",observed_tail,blocks);
}
int main(int argc,char **argv)
{
    image_dir=argc==2?argv[1]:0;load_force=0;boot();settle();
    projection_cases();pages_and_edits();selector_cases();locks_and_undo();
    themes();switch_tail();
    printf("PASS ui_theme checks=%u captures=%u; production SMK renderer, no hardware\n",checks,captures);
    return 0;
}
