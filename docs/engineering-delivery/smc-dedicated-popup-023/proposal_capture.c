/* SPDX-License-Identifier: GPL-3.0-only */
/* Throwaway design capture. Not included in firmware. Uses the real host
 * RGB565 framebuffer, production canvas/fonts and existing SMC primitives. */
#define SMC_UI_MAIN existing_smc_regressions
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

enum popup_screen { POP_FADERS, POP_KNOBS, POP_MUTE, POP_BANK, POP_CLIPS };
typedef struct {
    uint32_t until;
    uint8_t screen, bank, selected, peer, clip_row, visible;
} popup_state_t;
static popup_state_t popup;
static const char *const popup_titles[] = {"FADERS", "KNOBS", "MUTE", "BANK", "CLIP GRID"};
static const char *const popup_labels[] = {"909", "808", "BRK", "CHI", "MIA", "RVB", "DLY", "FLT"};
static const uint8_t demo_values[8] = {98,82,70,108,92,48,62,104};
static const uint8_t demo_physical[8] = {98,82,18,108,92,48,62,104};

/* Port of smc_ui_grid's 30-pixel strip geometry and native arc/fader
 * primitives. This renderer accepts only its own state and display model;
 * it neither calls keyboard painters nor accesses keyboard navigation. */
static void popup_draw(const popup_state_t *p)
{
    char title[64], value[16];
    cv_begin(240,24,C_BLACK);
    cv_rect(3,1,234,22,C_LINE);
    put_s(put_s(title,"SMC / "),popup_titles[p->screen]);
    cv_text(8,6,&FONT_XS,title,C_HI);
    text_r(232,6,&FONT_XS,p->screen==POP_KNOBS?"PAN+FX":"MAIN B1",C_WHITE);
    cv_commit_rect(0,0,20);
    cv_begin(240,138,C_BLACK); cv_band(0,138);
    if(p->screen==POP_FADERS || p->screen==POP_KNOBS) {
        for(int n=0;n<8;n++) {
            int x=n*30, active=n==p->selected;
            uint16_t ink=active?C_HI:C_GRAY;
            if(n)cv_rect(x,0,1,138,C_LINE);
            if(active)cv_rect(x+1,2,28,15,C_HI);
            text_c(x+15,4,&FONT_T,popup_labels[n],active?C_BLACK:C_WHITE);
            if(p->screen==POP_FADERS) {
                /* Ported existing destination travel and physical pickup mark. */
                int y=108-demo_values[n]*75/127;
                cv_rect(x+14,27,2,86,C_DIM);
                cv_rect(x+14,y,2,113-y,ink);
                cv_rect(x+7,y-2,16,5,ink);
                if(demo_physical[n]!=demo_values[n])
                    cv_rect(x+7,108-demo_physical[n]*75/127,16,1,C_WHITE);
            } else {
                static const char *const knob_labels[]={"PAN","PAN","PAN","PAN","PAN","DECAY","FDBK","RESO"};
                text_c(x+15,28,&FONT_T,knob_labels[n],C_GRAY);
                cv_rect(x+14,47,2,3,C_WHITE);
                arc(x+15,65,12,(float)demo_values[n]/127.0f,C_LINE,ink,n<5);
            }
            put_i(value,demo_values[n]);text_c(x+15,119,&FONT_T,value,ink);
        }
    } else if(p->screen==POP_MUTE) {
        /* Port the existing SMC row states into an independent 8-strip screen. */
        static const char *const rows[]={"MUTE","SOLO","REC"};
        for(int n=0;n<8;n++) {
            int x=n*30;
            if(n)cv_rect(x,0,1,138,C_LINE);
            text_c(x+15,4,&FONT_T,popup_labels[n],C_WHITE);
            for(int r=0;r<3;r++) {
                int on=r==0?n==2:r==1?n==3:n==4;
                cv_rect(x+2,28+r*34,26,26,on?C_HI:C_LINE);
                text_c(x+15,36+r*34,&FONT_T,rows[r],on?C_BLACK:C_GRAY);
            }
        }
    } else if(p->screen==POP_BANK) {
        static const char *const banks[]={"MAIN / PARTS","909 / TRACKS 1-8","808 / TRACKS 1-8","FX / PAGE 1","CLIP GRID"};
        for(int i=0;i<5;i++) {
            int y=4+i*26;
            cv_rect(8,y,224,23,i==p->bank?C_HI:C_LINE);
            text_c(120,y+7,&FONT_S,banks[i],i==p->bank?C_BLACK:C_WHITE);
        }
    } else {
        /* Illustrative fixture layout/palette in an SMC-owned viewport.
         * These are illustrative clip states, not incoming Loopy feedback. */
        for(int row=0;row<2;row++)for(int n=0;n<8;n++) {
            int x=n*29+4,y=row*62+8;
            uint16_t col=(n+row)%3==0?C_DIM:(row?RGB(140,60,170):RGB(30,150,100));
            cv_rect(x,y,25,52,col);
            put_i(value,row*8+n+1);text_c(x+12,y+18,&FONT_T,value,C_WHITE);
            if(row==0 && n==p->selected) {
                cv_rect(x,y,25,2,C_WHITE);cv_rect(x,y+50,25,2,C_WHITE);
                cv_rect(x,y,2,52,C_WHITE);cv_rect(x+23,y,2,52,C_WHITE);
            }
        }
    }
    cv_commit_rect(1,0,44);
    cv_begin(240,58,C_BLACK);cv_rect(3,0,234,1,C_LINE);
    const char *detail=p->screen==POP_FADERS?"BRK LEVEL 70 / PHYSICAL 18":
        p->screen==POP_KNOBS?"PAN+FX / RELATIVE ENCODERS":
        p->screen==POP_MUTE?"BRK MUTED / CHI SOLO":
        p->screen==POP_BANK?"SMC BANK / KEYBOARD UNCHANGED":"SMC CLIPS / ROWS 1-2";
    text_c(120,8,&FONT_XS,detail,C_WHITE);
    text_c(120,26,&FONT_XS,p->screen==POP_FADERS?"PICKUP: MOVE UP TO TARGET":"AUTO-HIDE AFTER 1200 MS",C_HI);
    text_c(120,43,&FONT_T,"PROPOSAL / NOT FIRMWARE",C_GRAY);
    cv_commit_rect(3,0,182);
}

int main(void)
{
    setup(V_SEQ,PART_303A);shot("before-keyboard");
    fader(0,0);ui_frame();shot("before-smc-activity");
    setup(V_SEQ,PART_303A);
    screen_state_t saved=screen_state();
    uint16_t saved_pixels[240*240];memcpy(saved_pixels,fb,sizeof fb);
    for(int screen=0;screen<5;screen++) {
        char name[40];
        popup=(popup_state_t){.until=plat_ms()+1200,.screen=screen,.bank=0,.selected=2,.visible=1};
        popup_draw(&popup);
        screen_state_t actual=screen_state();CHECK(!memcmp(&saved,&actual,sizeof saved));
        snprintf(name,sizeof name,"after-proposal-%s",popup_titles[screen]);shot(name);
    }
    popup.visible=0; memset(blit_hash,0,sizeof blit_hash);ui_frame();
    screen_state_t restored=screen_state();CHECK(!memcmp(&saved,&restored,sizeof saved));
    CHECK(!memcmp(saved_pixels,fb,sizeof fb));shot("after-dismiss-keyboard");
    printf("PROPOSAL framebuffer PASS: five screens; keyboard state/pixels preserved; popup state %zu B (host ABI)\n",sizeof popup);
    return 0;
}
