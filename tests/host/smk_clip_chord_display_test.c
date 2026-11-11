/* SPDX-License-Identifier: GPL-3.0-only */
/* Reuse the real note/MIDI lifecycle harness without duplicating its hardware model. */
#define main clip_knob_regression_main
#include "smk_clip_knob_test.c"
#undef main
#include <assert.h>
typedef struct {
    uint8_t h, pad, first, last;
    const uint8_t *adv, *bw;
    const uint16_t *off;
    const uint8_t *data;
} felucca_font_t;
#include "felucca_font.h"
#include "chord_metrics.inc"
static int32_t tw(const felucca_font_t *f, const char *s) { return text_w(f,s); }
#define C_LINE 0x2222
#define C_AMB 0xffff
#define C_GRAY 0x8888
static uint16_t dim(uint16_t c,int t) { (void)t; return c; }
static void rbox(int32_t x,int32_t y,int32_t w,int32_t h,int32_t r,uint16_t c)
{ (void)x;(void)y;(void)w;(void)h;(void)r;(void)c; }
static void rframe(int32_t x,int32_t y,int32_t w,int32_t h,int32_t r,uint16_t c)
{ rbox(x,y,w,h,r,c); }
static char chord_draw[64], detail_draw[64];
static unsigned draws, checks;
static int32_t cv_text(int32_t x,int32_t y,const felucca_font_t *f,const char *s,uint16_t c)
{
    (void)c;
    if (x==114 && y==vc(f,35,15)) {
        assert(strlen(s)<sizeof chord_draw);
        strcpy(chord_draw,s); draws++;
        assert(x + tw(f,s) <= 236);
        /* Every character must exist in the chosen font (T lacks lowercase). */
        for (const unsigned char *p=(const unsigned char*)s;*p;p++) assert(*p>=f->first && *p<=f->last);
    }
    if(x==114 && y==vc(f,51,10)) { assert(strlen(s)<sizeof detail_draw); strcpy(detail_draw,s); }
    return x+tw(f,s);
}
static void text_c(int32_t x,int32_t y,const felucca_font_t *f,const char *s,uint16_t c)
{ cv_text(x-tw(f,s)/2,y,f,s,c); }
#include "chord_screen.inc"

static void fresh(void)
{
    fm1_in.keys=0; clip_grid_mode=0;run(2);
    memset(&ck,0,sizeof ck);memset(&proj.set,0,sizeof proj.set);
    ck_qn=0;ck_start_req=0;apc_clock_seen=0;seq.bpm=120;
    proj.set.ck_ch=2;proj.set.ck_gate=4;proj.set.ck_rate=8;
    fm1_in.buttons=0;fm1_in.pnotes=0;
    for(unsigned i=0;i<NE;i++){panel.enc[i]=i;panel.dir[i]=1;fm1_in.enc_steps[i]=0;}
    for(unsigned i=0;i<NB;i++)panel.btn[i]=4+i;
    for(unsigned i=0;i<40;i++)fm1_in.key_vel[i]=100;
    bt_up=bt_notify=1;clip_grid_mode=1;run(2);
    fake_tx_n=ble_tx_n=0;
}
static void hold(uint64_t keys){ fm1_in.keys=keys;run(1); }
static void show(const char *want)
{
    uint32_t sig=0;unsigned usb_n=fake_tx_n,ble_n=ble_tx_n;
    ck_t saved=ck;unsigned pending=ck_qn;
    assert(smk_ck_panel(&sig,1));
    if(strcmp(chord_draw,want))fprintf(stderr,"display wanted [%s], got [%s]\n",want,chord_draw);
    assert(!strcmp(chord_draw,want));
    assert(!memcmp(&saved,&ck,sizeof ck) && pending==ck_qn);
    assert(fake_tx_n==usb_n && ble_tx_n==ble_n);
    assert(!smk_ck_panel(&sig,0));checks++;
}
/* Independent two-pass summary oracle: retain absolute pitches until the bass is known. */
static void verify_summary(void)
{
    uint8_t notes[CK_NKEY*CK_MAXN];unsigned n=0;
    if(ck.on & CK_M_ARP) {
        for(unsigned a=0;a<ck.nk;a++) {
            uint8_t out[CK_MAXN];unsigned k=ck_key_notes(ck.korder[a],out);
            for(unsigned j=0;j<k && n<CK_ARPN;j++)notes[n++]=out[j];
        }
    } else for(unsigned a=0;a<CK_NKEY;a++) if(CK_BIT(ck.snd,a))
        n+=ck_key_notes(a,notes+n);
    ck_chord_input_t want={0,0,255,255},got;
    for(unsigned i=0;i<n;i++)if(notes[i]<want.bass)want.bass=notes[i];
    for(unsigned i=0;i<n;i++){
        want.pcs|=1u<<(notes[i]%12);
        if(notes[i]>want.bass){want.upper_pcs|=1u<<(notes[i]%12);if(notes[i]<want.upper_bass)want.upper_bass=notes[i];}
    }
    ck_chord_state(&got);
    assert(got.pcs==want.pcs && got.upper_pcs==want.upper_pcs && got.bass==want.bass && got.upper_bass==want.upper_bass);
    checks++;
}
#define KEY(n) (UINT64_C(1)<<(n))
int main(void)
{
    fresh();show("-");
    hold(KEY(0)|KEY(4)|KEY(7));show("C");
    hold(KEY(0)|KEY(4));show("C E");hold(KEY(0));show("C");hold(0);show("-");
    hold(KEY(0)|KEY(1)|KEY(2)|KEY(3)|KEY(4));show("C C# D D# +");
    fresh();ck.on=CK_M_SCALE;hold(KEY(1)|KEY(4)|KEY(7));show("C");
    hold(KEY(0)|KEY(1)|KEY(4)|KEY(7));show("C");hold(KEY(1)|KEY(4)|KEY(7));show("C");
    proj.set.ck_snap=1;ck_all_off();hold(0);hold(KEY(1));show("-");
    fresh();ck.on=CK_M_CHORD;proj.set.ck_chord=0;proj.set.ck_inv=1;
    hold(KEY(0));show("C");
    fresh();ck.on=CK_M_CHORD;proj.set.ck_x2=15u<<4;proj.set.ck_x1=7u<<3;
    hold(KEY(0));assert(ck.nk);show("C");run(400);show("C");hold(0);show("-");
    fresh();ck.on=CK_M_ARP;proj.set.ck_ord=CK_ORD_RANDOM;proj.set.ck_oct=2;proj.set.ck_x2=1;
    hold(KEY(0)|KEY(4)|KEY(7));show("C");
    for(unsigned i=0;i<40;i++){run(25);show("C");if(fake_tx_n>180)fake_tx_n=0;if(ble_tx_n>180)ble_tx_n=0;}
    ck.fl|=CK_F_LATCH;hold(0);show("C");assert(strstr(detail_draw,"LATCH"));
    hold(KEY(2)|KEY(5)|KEY(9));show("Dm");assert(!strstr(detail_draw,"LATCH"));
    hold(0);show("Dm");ck_set(CK_E_LATCH,0);show("-");
    fresh();hold(KEY(0)|KEY(4)|KEY(7));show("C");smk_ck_midi_stop();run(1);show("-");
    hold(0);hold(KEY(0)|KEY(4)|KEY(7));proj.set.ck_ch=3;run(1);show("-");
    hold(0);hold(KEY(0)|KEY(4)|KEY(7));bt_up=0;run(1);show("-");
    fresh();plug();hold(KEY(0)|KEY(4)|KEY(7));show("C");
    host=0;fake_intrusb=0x04u;run(1);show("-");
    fresh();hold(KEY(0)|KEY(4)|KEY(7));clip_grid_mode=0;run(1);show("-");
    clip_grid_mode=1;run(1);show("-");hold(0);hold(KEY(0)|KEY(4)|KEY(7));show("C");
    /* Same pitch classes, changed bass must invalidate the screen signature. */
    fresh();hold(KEY(0)|KEY(4)|KEY(7)|KEY(9));show("C6");
    uint32_t sig=0;smk_ck_panel(&sig,1);
    hold(KEY(9)|KEY(12)|KEY(16)|KEY(19));assert(smk_ck_panel(&sig,0));show("Am7");
    /* A ninth raw key excluded by the arp capacity must not alter the recognized set. */
    fresh();ck.on=CK_M_ARP;
    hold(KEY(0)|KEY(4)|KEY(7)|KEY(12)|KEY(16)|KEY(19)|KEY(24)|KEY(28));show("C");
    hold(fm1_in.keys|KEY(30));assert(ck.nk==8);show("C");
    /* Compare the collector against the actual arp's ordered, pre-dedup 24-note cap. */
    fresh();ck.on=CK_M_ARP|CK_M_CHORD;proj.set.ck_chord=29;
    hold(KEY(0)|KEY(1)|KEY(2)|KEY(3)|KEY(4));
    ck_chord_input_t actual;ck_chord_state(&actual);
    uint16_t mask=0;uint8_t lowest=255;unsigned count=0;
    for(unsigned a=0;a<ck.nk;a++){uint8_t out[CK_MAXN];unsigned n=ck_key_notes(ck.korder[a],out);
        for(unsigned j=0;j<n && count<CK_ARPN;j++,count++){mask|=1u<<(out[j]%12);if(out[j]<lowest)lowest=out[j];}}
    assert(actual.pcs==mask && actual.bass==lowest && count==24);checks++;
    /* Every possible pitch-class fallback/chord is within the production font width. */
    for(unsigned mask12=0;mask12<4096;mask12++){
        ck.on=0;ck.nk=0;ck.arp_note=0;ck.snd[0]=mask12;ck.snd[1]=0;
        uint32_t last=0;assert(smk_ck_panel(&last,1));checks++;
    }
    /* Generated inversions reorder notes: compare collector with independent two-pass oracle.
     * Include duplicated octaves, lower roots encountered later in PLAYED order, and cap overflow. */
    for(unsigned type=0;type<CK_NCHORD;type++)for(unsigned inv=0;inv<4;inv++)for(unsigned r=0;r<12;r++){
        ck.on=CK_M_CHORD;proj.set.ck_chord=type;proj.set.ck_inv=inv;
        ck.snd[0]=(1u<<r)|(1u<<(r+12))|(1u<<((r+4)%12));ck.snd[1]=1u<<4;
        verify_summary();uint32_t last=0;smk_ck_panel(&last,1);
        ck.on|=CK_M_ARP;ck.nk=8;
        for(unsigned k=0;k<8;k++)ck.korder[k]=(uint8_t)((r+35-k*5)%37);
        verify_summary();last=0;smk_ck_panel(&last,1);
    }
    printf("clip chord display: %u checks, %u captured chord draws passed\n",checks,draws);
    return 0;
}
