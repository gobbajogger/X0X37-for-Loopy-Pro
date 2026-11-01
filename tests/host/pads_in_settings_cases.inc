/* SPDX-License-Identifier: GPL-3.0-only */
/* The 16 SMK-37 pads are the 16 white step keys. With a settings list (O_LIST) or a
 * HELP card open, each pad does what it does with the settings closed, and the pad
 * LEDs show the same picture. Menu navigation stays on the buttons and knobs. */
enum { CX_NONE, CX_LIST, CX_HELP, CX_N };
static const char *const CXN[CX_N] = {"closed", "settings list", "HELP"};
static unsigned checks;
#define T(c, ...) do { checks++; CHECK(c, __VA_ARGS__); } while (0)

static void close_all(void)
{
    held_keys = 0; held_btn = 0; tick(30);
    ui.overlay = O_NONE; ui.help = 0; build_pages(); tick(30);
}
/* Open the context. HELP: one button held (FX) until its card shows, as on the device. */
static void open_cx(int cx)
{
    if (cx == CX_LIST) { open_list_of_pages(); tick(20); T(ui.overlay == O_LIST, "list open"); }
    if (cx == CX_HELP) { btn(B_FX, 1); tick(HELP_HOLD + 50); T(ui.help == B_FX + 1, "HELP open"); }
}
static void leave_cx(int cx)
{
    if (cx == CX_HELP) btn(B_FX, 0);
    tick(20);
}
static void same_picture(int cx, const char *page)
{
    uint32_t lit, dim, l2, d2;
    held_keys = 0; tick(20); lit = lit_keys; dim = dim_keys;
    open_cx(cx); tick(20); l2 = lit_keys; d2 = dim_keys;
    T(l2 == lit && d2 == dim, "%s: pad LEDs %08x/%08x with %s, %08x/%08x closed", page, l2, d2, CXN[cx], lit, dim);
    leave_cx(cx); close_all();
}
static void tap_pad(int w) { key(WHITE_KEY[w], 1); tick(15); key(WHITE_KEY[w], 0); tick(15); }

static void para_shot(const char *name)
{
    const char *dir=getenv("X0X_PADS_SHOTS");char path[1024];
    if(!dir)return;
    snprintf(path,sizeof path,"%s/%s.png",dir,name);png_write(path);
}
/* PARA still displays the drum step pattern on the physical pads. Editing
 * those pads must target that kit/voice/page, not audition a hidden action. */
static void drum_para(int cx)
{
    for(int kit=PART_909;kit<=PART_808;kit++) {
        ui.part=(uint8_t)kit;set_view(V_PART);build_pages();close_all();
        same_picture(cx,kit==PART_909?"909 PARA":"808 PARA");
        for(int w=0;w<16;w++) {
            int st=ui.spage*16+w,voice=ui.sel[kit];
            dpart_t *d=&pat_of(kit)->drum[kit];
            int before=sm_get(d->hit[voice],st);
            uint32_t shown=drum_step_lights(kit);
            T(((shown>>WHITE_KEY[w])&1u)==(unsigned)before,"PARA pad %d shows its step",w+1);
            if(kit==PART_909 && cx==CX_NONE && w==0)para_shot("drum-para-before-step");
            open_cx(cx);tap_pad(w);leave_cx(cx);
            if(kit==PART_909 && cx==CX_NONE && w==0) {
                para_shot("drum-para-after-step");
                printf("PARA interaction kit=%d voice=%d step=%d hit=%d->%d LEDs=%08x\n",kit,voice,st,before,sm_get(d->hit[voice],st),lit_keys);
            }
            T(sm_get(d->hit[voice],st)!=before,"kit %d PARA pad %d edits its displayed step with %s",kit,w+1,CXN[cx]);
            close_all();
        }
    }
}
static void drum_seq(int cx)
{
    ui.part = PART_909; set_view(V_SEQ); build_pages(); close_all();
    same_picture(cx, "909 SEQ");
    for (int w = 0; w < 16; w++) {
        uint32_t before = drum_step_lights(PART_909);
        open_cx(cx); tap_pad(w); leave_cx(cx);
        uint32_t after = drum_step_lights(PART_909);
        T(((before ^ after) >> WHITE_KEY[w]) & 1u, "909 SEQ pad %d toggles its step with %s", w + 1, CXN[cx]);
        close_all();
    }
}
static void drum_mix(int cx)
{
    ui.part = PART_909; set_view(V_MIX); build_pages();
    int p;
    for (p = 0; p < pg.n; p++) { *page_of(V_MIX) = (uint8_t)p; build_pages(); if (drum_mix_page() >= 0) break; }
    T(drum_mix_page() >= 0, "a drum MIX page exists");
    int kit = drum_mix_page();
    close_all();
    same_picture(cx, "drum MIX");
    for (int w = 0; w < 16; w++) {
        uint32_t before = drum_step_lights(kit);
        open_cx(cx); tap_pad(w); leave_cx(cx);
        T(((before ^ drum_step_lights(kit)) >> WHITE_KEY[w]) & 1u, "drum MIX pad %d toggles its step with %s", w + 1, CXN[cx]);
        close_all();
    }
}
static void bass_seq(int cx, int bank)
{
    T(!engine_bass_select_preset(0, bank, 0, &proj.sound), "bank"); tick(200);
    ui.part = PART_303A; ui.freeplay[0] = 0; set_view(V_SEQ); build_pages(); close_all();
    same_picture(cx, "synth SEQ");
    for (int w = 0; w < 16; w++) {
        bpart_t *bp = &cur_pat()->bass[0];
        int s = ui.spage * 16 + w, before = s < bp->len ? bstep_gate(&bp->step[s]) : G_REST;
        open_cx(cx); tap_pad(w); leave_cx(cx);
        int after = s < bp->len ? bstep_gate(&cur_pat()->bass[0].step[s]) : G_REST;
        T(after != before, "synth bank %d SEQ pad %d edits its step with %s (%d -> %d)", bank, w + 1, CXN[cx], before, after);
        close_all();
    }
}
static void bass_play(int cx, int bank, int view, int freeplay)
{
    T(!engine_bass_select_preset(0, bank, 0, &proj.sound), "bank"); tick(200);
    ui.part = PART_303A; ui.freeplay[0] = (uint8_t)freeplay; set_view(view); build_pages(); close_all();
    same_picture(cx, view == V_PART ? "synth EDIT" : "synth SEQ FREE PLAY");
    for (int w = 0; w < 16; w++) {
        open_cx(cx);
        key(WHITE_KEY[w], 1); tick(15);
        T(engine_bass_keys_held() > 0, "synth bank %d %s pad %d plays its note with %s", bank,
          view == V_PART ? "EDIT" : "FREE PLAY", w + 1, CXN[cx]);
        key(WHITE_KEY[w], 0); tick(15);
        T(engine_bass_keys_held() == 0, "pad %d release ends the note", w + 1);
        leave_cx(cx); close_all();
    }
}
static void brk_seq(int cx)
{
    ui.part = PART_BRK; set_view(V_SEQ); build_pages(); close_all();
    same_picture(cx, "BREAK SEQ");
    for (int w = 0; w < 16; w++) {
        uint32_t before = cur_pat()->brk.steps;
        open_cx(cx); tap_pad(w); leave_cx(cx);
        T(((before ^ cur_pat()->brk.steps) >> w) & 1u, "BREAK SEQ pad %d toggles its step with %s", w + 1, CXN[cx]);
        close_all();
    }
}
/* The same displayed bit must address its original kit, voice and step page.
 * Cover all four pages on PARA/GLOBAL/GEN, alternating first/last voice and
 * closed/list/HELP contexts; compare whole kit structs for collateral edits. */
static void drum_visible_identity(void)
{
    static const int view[] = {V_PART,V_GLO,V_GEN};
    for(int kit=PART_909;kit<=PART_808;kit++)
    for(unsigned vi=0;vi<sizeof view/sizeof view[0];vi++)
    for(int page=0;page<NSTEPS/16;page++) {
        int voice=(page&1)?NDRUM-1:0,cx=page%CX_N;
        ui.part=(uint8_t)kit;set_view(view[vi]);
        pat_of(kit)->drum[kit].len=NSTEPS;
        ui.sel[kit]=(uint8_t)voice;ui.spage=(uint8_t)page;build_pages();close_all();
        same_picture(cx,"drum displayed-step identity");
        for(int w=0;w<16;w++) {
            dpart_t before[2],expected;
            for(int k=PART_909;k<=PART_808;k++)before[k]=pat_of(k)->drum[k];
            int step=page*16+w,hit=sm_get(before[kit].hit[voice],step);
            T(((lit_keys>>WHITE_KEY[w])&1u)==(unsigned)hit,
              "kit %d view %d voice %d page %d pad %d actual LED matches displayed step",kit,view[vi],voice,page,w+1);
            open_cx(cx);tap_pad(w);leave_cx(cx);
            expected=before[kit];sm_flip(expected.hit[voice],step);
            T(!memcmp(&expected,&pat_of(kit)->drum[kit],sizeof expected),
              "kit %d view %d voice %d page %d pad %d edits only displayed step",kit,view[vi],voice,page,w+1);
            int other=1-kit;
            T(!memcmp(&before[other],&pat_of(other)->drum[other],sizeof before[other]),
              "kit %d pad %d leaves other kit unchanged",kit,w+1);
            close_all();
        }
    }
    ui.spage=0;ui.sel[PART_909]=ui.sel[PART_808]=0;
}
/* Higher-priority presentation/gesture owners and no-step views keep their
 * existing meaning; showing steps behind a modal does not grant new presses. */
static void drum_capture_negatives(void)
{
    for(int kit=PART_909;kit<=PART_808;kit++)
    for(int context=0;context<9;context++) {
        ui.part=(uint8_t)kit;set_view(V_PART);ui.spage=0;build_pages();close_all();
        switch(context) {
        case 0: ui.overlay=O_ASK;break;
        case 1: set_view(V_PERF);break;
        case 2: test_clip_mode=1;break;
        case 3: set_view(V_HOME);break;
        case 4: set_view(V_SONG);break;
        case 5: set_view(V_MIX);*page_of(V_MIX)=0;build_pages();break;
        case 6: set_view(V_FX);break;
        case 7: btn(B_HOME,1);break;
        case 8: btn(B_SAVE,1);break;
        }
        for(int w=0;w<16;w++) {
            /* Force SAVE destinations into confirmation rather than executing
             * a project copy; this case verifies the gesture takes the press. */
            if(context==8 && !all_on(ui.pbank*16+w)) {
                sm_set(proj.pat[ui.pbank*16+w].drum[kit].hit[0],0);
                ui.overlay=O_NONE;
            }
            dpart_t *target=&pat_of(kit)->drum[kit],before=*target;
            tap_pad(w);
            T(!memcmp(&before,target,sizeof before),
              "kit %d context %d pad %d must not edit a drum step",kit,context,w+1);
        }
        if(context==7)btn(B_HOME,0);
        if(context==8)btn(B_SAVE,0);
        test_clip_mode=0;close_all();
    }
    for(int kit=PART_909;kit<=PART_808;kit++) {
        ui.part=(uint8_t)kit;set_view(V_PART);ui.spage=0;build_pages();close_all();
        dpart_t *d=&pat_of(kit)->drum[kit];sm_set(d->hit[ui.sel[kit]],0);
        key(WHITE_KEY[0],1);tick(STEP_TAP_MS+20);
        T(ui.hold_step==0 && held_drum_step()==0,"PARA held step retains its p-lock target");
        key(WHITE_KEY[0],0);tick(20);
        T(sm_get(d->hit[ui.sel[kit]],0) && ui.hold_step<0,"PARA long hold releases without deleting the hit");
    }
    ui.spage=0;
}

int main(void)
{
    boot(); tick(300);
    seq_stop(&seq); tick(50);
    for (int cx = CX_NONE; cx < CX_N; cx++) {
        drum_para(cx);
        drum_seq(cx);
        drum_mix(cx);
        brk_seq(cx);
        for (int bank = FACTORY_BANK_CHICAGO; bank <= FACTORY_BANK_MIAMI; bank++) {
            bass_seq(cx, bank);
            bass_play(cx, bank, V_PART, 0);
            bass_play(cx, bank, V_SEQ, 1);
        }
    }
    drum_visible_identity();
    drum_capture_negatives();
    printf("%s %u checks: pads x {909/808 PARA/GLOBAL/GEN, SEQ, drum MIX, BREAK, Chicago/Miami SEQ/EDIT/FREE PLAY} x {closed, settings list, HELP}\n",
           fails ? "FAIL" : "PASS", checks);
    return fails != 0;
}
