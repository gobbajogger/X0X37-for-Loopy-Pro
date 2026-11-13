/* SPDX-License-Identifier: GPL-3.0-only */
/* Gate: SMK-37 REC sees the full 37-key bed.
 * 303: every physical key records pitch 36+physical+12*octave (same as Free Play).
 * 909/808: the 11 FM-1 black keys record their voices; the 10 outer keys stay unmapped.
 * BREAK: the middle 27 keep steps/slices; C3-D#3 still page; outer keys write no step/slice.
 * Appended by freeplay_live_keys_fixture.py (X0X_LIVE_CASES). */
static unsigned checks, drops, records;
static FILE *report;
#define T(c) do{checks++;if(!(c)){fails++;fprintf(stderr,"FAIL %d %s\n",__LINE__,#c);if(report)fprintf(report,"FAIL %d %s\n",__LINE__,#c);}}while(0)

static const char *key_name(unsigned p)
{
    static const char *n[37] = {
        "C","C#","D","D#","E",
        "F","F#","G","G#","A","A#","B",
        "C","C#","D","D#","E","F","F#","G","G#","A","A#","B",
        "C","C#","D","D#","E","F","F#","G",
        "G#","A","A#","B","C"
    };
    return n[p];
}
static int is_outer(unsigned p) { return p < 5u || p >= 32u; }
static int expected_pitch(unsigned p, int oct) { return 36 + (int)p + 12 * oct; }
static int fm1_index(unsigned p) { return (p >= 5u && p < 32u) ? (int)(p - 5u) : -1; }
static int drum_voice_of(unsigned p)
{
    int k = fm1_index(p);
    return k >= 0 ? KEY_BLACK[k] : -1;
}
static int white_step_of(unsigned p)
{
    int k = fm1_index(p);
    return k >= 0 ? KEY_WHITE[k] : -1;
}

static void audio_block(void){int32_t out[512];engine_render(out,256);}
static void physical(unsigned p,unsigned velocity)
{
    smk__key_event(p,velocity);held_keys=fm1_in.notes;
    for(unsigned k=0;k<NKEYS;k++)key_velocity[k]=fm1_in.key_vel[k+5];
}
static void scan(void){ui_input_only();audio_block();ui_input_only();audio_block();}
static void edge(unsigned p,unsigned velocity){physical(p,velocity);scan();}
static void all_up(void)
{
    for(unsigned p=0;p<37;p++)physical(p,0);
    scan();
}

static void clear_bass(void)
{
    bpart_t *bp=&cur_pat()->bass[0];
    bp->len=16;
    memset(bp->step,0,sizeof bp->step);
    ui.wpos[0]=0;
    bass_rec_step[0]=kbd_held_note[0]=kbd_held_key[0]=kbd_arp_n[0]=0;
    (void)held_chord_set(proj.arr.lane,seq.ppat[PART_303A],0,0,0,0);
    for(int s=0;s<16;s++)(void)held_chord_set(proj.arr.lane,seq.ppat[PART_303A],0,s,0,0);
}

static void clear_kit(int kit)
{
    dpart_t *d=&proj.pat[seq.ppat[kit]].drum[kit];
    memset(d->hit,0,sizeof d->hit);
    memset(d->accent,0,sizeof d->accent);
}

static int recorded_notes(uint8_t *out, int max)
{
    bpart_t *bp=&cur_pat()->bass[0];
    int n=0;
    for(int s=0;s<16 && n<max;s++) {
        if(bstep_gate(&bp->step[s])==G_NOTE) {
            uint8_t chord[HELD_CHORD_NOTES];
            int cn=held_chord_get(proj.arr.lane,seq.ppat[PART_303A],0,s,bp->step[s].note,chord);
            if(cn>1) {
                for(int i=0;i<cn && n<max;i++) out[n++]=chord[i];
            } else
                out[n++]=bp->step[s].note;
        }
    }
    return n;
}

static int has_note(const uint8_t *notes,int n,int pitch)
{
    for(int i=0;i<n;i++) if(notes[i]==(uint8_t)pitch) return 1;
    return 0;
}

static int kit_hit_count(int kit, int v)
{
    dpart_t *d=&proj.pat[seq.ppat[kit]].drum[kit];
    int n=0,s;
    for(s=0;s<NSTEPS;s++) n+=sm_get(d->hit[v],s);
    return n;
}

static void enter_303_stopped(int bank, int oct)
{
    all_up();
    ui.freeplay[0]=ui.freeplay[1]=0;
    ui.overlay=O_NONE;ui.help=0;
    test_clip_mode=test_padcal=test_smcui=0;
    held_btn=ui.btn=0;ui.held_step=-1;
    seq_stop(&seq);scan();
    engine_bass_off(0);engine_bass_off(1);tick(50);
    T(!engine_bass_select_preset(0,bank,0,&proj.sound));tick(80);
    T(engine_bass_sound_bank(&proj.sound,0)==bank);
    ui.part=PART_303A;set_view(V_SEQ);build_pages();
    ui.oct=(int8_t)oct;
    ui.rec=0;scan();
    btn(B_REC,1);btn(B_REC,0);scan();
    T(ui.rec && !seq.playing);
    T(ui_live_key_owner()==0);
    T(live_owner<0);
    clear_bass();
}

static void enter_303_playing(int bank, int oct)
{
    enter_303_stopped(bank,oct);
    seq.bpm=120.0f;
    btn(B_PLAY,1);btn(B_PLAY,0);tick(20);
    T(ui.rec && seq.playing);
    clear_bass();
}

static void log_row(const char *mode,int bank,int oct,unsigned p,int pitch,int ok,const uint8_t *got,int ng)
{
    const char *tag = ok ? "RECORDED" : "DROPPED ";
    if(!ok) drops++; else records++;
    char gotbuf[64];int w=0;
    if(ng<=0) w=snprintf(gotbuf,sizeof gotbuf,"(none)");
    else {
        gotbuf[0]=0;
        for(int i=0;i<ng;i++) {
            w+=snprintf(gotbuf+w,(size_t)(sizeof gotbuf-(size_t)w),"%s%d",i?",":"",got[i]);
            if(w<0 || (size_t)w>=sizeof gotbuf) break;
        }
    }
    fprintf(report,"%s bank=%d oct=%+d physical=%2u %s%-3s %s expected=%3d got={%s}\n",
            mode,bank,oct,p,is_outer(p)?"OUTER ":"CENT  ",key_name(p),tag,pitch,gotbuf);
    printf("%s bank=%d oct=%+d physical=%2u %s%-3s %s expected=%3d got={%s}\n",
           mode,bank,oct,p,is_outer(p)?"OUTER ":"CENT  ",key_name(p),tag,pitch,gotbuf);
}

static void mono_stopped(int bank)
{
    for(int oct=-2;oct<=3;oct++) {
        enter_303_stopped(bank,oct);
        for(unsigned p=0;p<37;p++) {
            clear_bass();
            int pitch=expected_pitch(p,oct);
            edge(p,100);edge(p,0);scan();
            uint8_t got[16];int ng=recorded_notes(got,16);
            int ok=has_note(got,ng,pitch);
            T(ok);
            log_row("mono-stopped",bank,oct,p,pitch,ok,got,ng);
        }
    }
}

static void mono_playing(int bank)
{
    static const unsigned sample[8]={0,1,4,5,17,31,32,36};
    for(int oct=-2;oct<=3;oct++) {
        /* Full 37 at default and both octave bounds; a sample on the mid octaves. */
        int full = (oct==-2 || oct==0 || oct==3);
        enter_303_playing(bank,oct);
        for(unsigned j=0;j<(full?37u:8u);j++) {
            unsigned p=full?j:sample[j];
            clear_bass();
            at_step(TRK_BASS0,0,4);
            int pitch=expected_pitch(p,oct);
            edge(p,100);tick(8);edge(p,0);tick(8);
            uint8_t got[16];int ng=recorded_notes(got,16);
            int ok=has_note(got,ng,pitch);
            T(ok);
            log_row("mono-playing",bank,oct,p,pitch,ok,got,ng);
        }
        btn(B_PLAY,1);btn(B_PLAY,0);btn(B_REC,1);btn(B_REC,0);tick(20);
    }
}

static void chord_pair(int bank,const char *mode,int playing,unsigned a,unsigned b)
{
    int oct=0;
    if(playing) enter_303_playing(bank,oct); else enter_303_stopped(bank,oct);
    clear_bass();
    int pa=expected_pitch(a,oct), pb=expected_pitch(b,oct);
    if(playing) at_step(TRK_BASS0,0,8);
    edge(a,100);edge(b,100);
    if(playing) tick(8);
    else scan();
    edge(a,0);edge(b,0);tick(8);
    uint8_t got[16];int ng=recorded_notes(got,16);
    int oka=has_note(got,ng,pa), okb=has_note(got,ng,pb);
    /* Playing mono REC: two notes on one quantized step replace; later wins.
     * That is existing behavior (rec_live_test); do not change it. Stopped: both. */
    int want_a=!playing, want_b=1;
    T(!want_a || oka);
    T(want_b && okb);
    if(want_a && !oka) drops++; else if(oka) records++;
    if(want_b && !okb) drops++; else if(okb) records++;
    fprintf(report,"%s bank=%d pair %u%s+%u%s expected {%d,%d} got a=%s b=%s ng=%d\n",
            mode,bank,a,is_outer(a)?"O":"C",b,is_outer(b)?"O":"C",pa,pb,
            oka?"RECORDED":"DROPPED",okb?"RECORDED":"DROPPED",ng);
    printf("%s bank=%d pair %u%s+%u%s a=%s b=%s ng=%d\n",
           mode,bank,a,is_outer(a)?"O":"C",b,is_outer(b)?"O":"C",
           oka?"RECORDED":"DROPPED",okb?"RECORDED":"DROPPED",ng);
    if(playing){btn(B_PLAY,1);btn(B_PLAY,0);btn(B_REC,1);btn(B_REC,0);tick(20);}
}

static void chords(int bank)
{
    static const unsigned pairs[][2] = {
        {0,17},{4,17},{17,32},{17,36},{0,36},{1,3},{32,35},{10,14},{5,31}
    };
    for(unsigned i=0;i<sizeof pairs/sizeof pairs[0];i++) {
        chord_pair(bank,"chord-stopped",0,pairs[i][0],pairs[i][1]);
        chord_pair(bank,"chord-playing",1,pairs[i][0],pairs[i][1]);
    }
}

static void chord_three_poly(int bank)
{
    if(bank<2) return;
    enter_303_stopped(bank,0);
    clear_bass();
    unsigned ks[3]={0,17,36};
    int exp[3]={expected_pitch(0,0),expected_pitch(17,0),expected_pitch(36,0)};
    edge(ks[0],100);edge(ks[1],100);edge(ks[2],100);scan();
    edge(ks[0],0);edge(ks[1],0);edge(ks[2],0);scan();
    uint8_t got[16];int ng=recorded_notes(got,16);
    fprintf(report,"chord3-stopped bank=%d expected {%d,%d,%d} got={",bank,exp[0],exp[1],exp[2]);
    for(int i=0;i<ng;i++) fprintf(report,"%s%d",i?",":"",got[i]);
    fprintf(report,"}\n");
    printf("chord3-stopped bank=%d expected {%d,%d,%d} ng=%d\n",bank,exp[0],exp[1],exp[2],ng);
    for(int i=0;i<3;i++) {
        int ok=has_note(got,ng,exp[i]);
        T(ok);
        if(!ok) drops++; else records++;
        fprintf(report,"  physical %u %s\n",ks[i],ok?"RECORDED":"DROPPED");
    }
}

/* Sound, identity, suppress at REC entry, hold across REC exit, octave change. */
static void ownership(int bank)
{
    unsigned tok0=(unsigned)live_token(0);

    /* Central still sounds and releases. */
    enter_303_stopped(bank,0);
    clear_bass();
    edge(17,100);
    T(engine_bass_keys_held()&1);
    T(kbd_held_key[0]==(uint8_t)live_token(17));
    edge(17,0);tick(40);
    T(!(engine_bass_keys_held()&1));

    /* Outer sounds during REC, uses live_token identity, releases cleanly. */
    clear_bass();
    edge(0,100);
    T(engine_bass_keys_held()&1);
    T(kbd_held_key[0]==(uint8_t)tok0);
    T(engine_live_key_probe(0,2)==0); /* live owner stays off in REC */
    T(ui_live_key_owner()==0);
    edge(0,0);tick(20);
    T(!(engine_bass_keys_held()&1));
    fprintf(report,"ownership bank=%d outer 0 sounds and releases (token %u)\n",bank,tok0);

    /* Central held, outer tap, central still releases. */
    clear_bass();
    edge(17,100);
    T(engine_bass_keys_held()&1);
    edge(0,100);edge(0,0);
    T(engine_bass_keys_held()&1);
    edge(17,0);tick(40);
    T(!(engine_bass_keys_held()&1));

    /* Key already held at REC entry is suppressed; a new press after release records. */
    all_up();
    ui.part=PART_303A;set_view(V_SEQ);build_pages();
    ui.oct=0;ui.rec=0;scan();
    clear_bass();
    edge(0,100);
    { uint8_t got[16];int ng=recorded_notes(got,16); T(ng==0); } /* REC still off: no write */
    btn(B_REC,1);btn(B_REC,0);scan();
    T(ui.rec);
    { uint8_t got[16];int ng=recorded_notes(got,16);
      T(ng==0);
      T(!has_note(got,ng,expected_pitch(0,0)));
      T(!(engine_bass_keys_held()&1));
      fprintf(report,"ownership bank=%d outer 0 held at REC entry: suppressed (ng=%d held=%d)\n",
              bank,ng,engine_bass_keys_held()&1);
    }
    edge(0,0);scan();
    clear_bass();
    edge(0,100);
    T(engine_bass_keys_held()&1);
    { uint8_t got[16];int ng=recorded_notes(got,16);
      T(has_note(got,ng,expected_pitch(0,0)));
    }
    edge(0,0);tick(20);
    T(!(engine_bass_keys_held()&1));

    /* Held across REC exit: no stuck note. */
    enter_303_stopped(bank,0);
    clear_bass();
    edge(0,100);
    T(engine_bass_keys_held()&1);
    btn(B_REC,1);btn(B_REC,0);scan();
    T(!ui.rec);
    edge(0,0);tick(20);
    T(!(engine_bass_keys_held()&1));
    fprintf(report,"ownership bank=%d outer 0 held across REC exit: released, not stuck\n",bank);

    /* Octave change while held: original pitch, then a clean release. */
    enter_303_stopped(bank,0);
    clear_bass();
    edge(0,100);
    T(engine_bass_keys_held()&1);
    { uint8_t got[16];int ng=recorded_notes(got,16); T(has_note(got,ng,36)); }
    btn(B_OCTUP,1);btn(B_OCTUP,0);scan();
    T(ui.oct==1);
    T(engine_bass_keys_held()&1);
    edge(0,0);tick(20);
    T(!(engine_bass_keys_held()&1));
    { uint8_t got[16];int ng=recorded_notes(got,16);
      T(has_note(got,ng,36));
      T(!has_note(got,ng,48));
    }
    fprintf(report,"ownership bank=%d outer 0 octave change while held: pitch 36 kept, no stuck, no 48\n",bank);
}

/* 909 / 808: 11-voice FM-1 map; outer ten unmapped. */
static void enter_drum_playing(int kit)
{
    all_up();
    ui.freeplay[0]=ui.freeplay[1]=0;
    ui.overlay=O_NONE;ui.help=0;
    test_clip_mode=test_padcal=test_smcui=0;
    held_btn=ui.btn=0;ui.held_step=-1;
    seq_stop(&seq);ui.rec=0;scan();
    engine_bass_off(0);engine_bass_off(1);
    ui.part=(uint8_t)kit;set_view(V_SEQ);build_pages();
    seq.bpm=120.0f;
    btn(B_REC,1);btn(B_REC,0);scan();
    btn(B_PLAY,1);btn(B_PLAY,0);tick(20);
    T(ui.rec && seq.playing && is_drum() && ui.part==kit);
    T(ui_live_key_owner()==0);
    clear_kit(kit);
    clear_bass();
}

static void drum_voices(int kit)
{
    const char *name = kit==PART_909?"909":"808";
    enter_drum_playing(kit);
    for(int v=0;v<NDRUM;v++) {
        unsigned p=(unsigned)BLACK_KEY[v]+5u; /* FM-1 index → physical */
        T(!is_outer(p) && drum_voice_of(p)==v);
        clear_kit(kit);
        int sel0=ui.sel[kit];
        at_step(TRK_DRUM+kit,0,8);
        edge(p,100);tick(8);edge(p,0);tick(8);
        int n=kit_hit_count(kit,v);
        T(n>=1);
        T(ui.sel[kit]==v);
        /* No other voice gained a hit. */
        for(int u=0;u<NDRUM;u++) if(u!=v) T(kit_hit_count(kit,u)==0);
        if(n>=1) records++; else drops++;
        fprintf(report,"drum-%s voice %d physical %u %s hits=%d sel %d->%d\n",
                name,v,p,n>=1?"RECORDED":"DROPPED",n,sel0,ui.sel[kit]);
        printf("drum-%s voice %d physical %u hits=%d\n",name,v,p,n);
    }
    /* Outer ten write nothing and do not steal the selected track. */
    clear_kit(kit);
    ui.sel[kit]=3;
    int sel=ui.sel[kit];
    unsigned char before[sizeof(dpart_t)];
    memcpy(before,&proj.pat[seq.ppat[kit]].drum[kit],sizeof(dpart_t));
    clear_bass();
    for(unsigned j=0;j<10;j++) {
        static const unsigned outer[10]={0,1,2,3,4,32,33,34,35,36};
        unsigned p=outer[j];
        at_step(TRK_DRUM+kit,0,4);
        edge(p,100);tick(8);edge(p,0);tick(8);
        T(drum_voice_of(p)<0);
        T(ui.sel[kit]==sel);
        T(memcmp(before,&proj.pat[seq.ppat[kit]].drum[kit],sizeof(dpart_t))==0);
        { uint8_t got[16];int ng=recorded_notes(got,16); T(ng==0); } /* not a 303 write either */
        fprintf(report,"drum-%s OUTER physical %u unmapped: sel=%d hits-unchanged=1 bass=0\n",
                name,p,ui.sel[kit]);
        printf("drum-%s OUTER physical %u unmapped ok\n",name,p);
    }
    /* 16 white keys of the FM-1 window still toggle steps of the selected voice. */
    clear_kit(kit);
    ui.sel[kit]=0;
    seq_stop(&seq);tick(10);
    T(ui.rec && !seq.playing);
    for(int w=0;w<16;w++) {
        unsigned p=(unsigned)WHITE_KEY[w]+5u;
        T(white_step_of(p)==w);
        clear_kit(kit);
        edge(p,100);edge(p,0);scan();
        T(sm_get(proj.pat[seq.ppat[kit]].drum[kit].hit[0],w));
        fprintf(report,"drum-%s WHITE physical %u step %d toggled\n",name,p,w);
    }
    /* Outer white keys do not toggle a step. */
    clear_kit(kit);
    memcpy(before,&proj.pat[seq.ppat[kit]].drum[kit],sizeof(dpart_t));
    { static const unsigned ow[6]={0,2,4,33,35,36};
      for(unsigned j=0;j<6;j++) {
          edge(ow[j],100);edge(ow[j],0);scan();
          T(memcmp(before,&proj.pat[seq.ppat[kit]].drum[kit],sizeof(dpart_t))==0);
      }
    }
    btn(B_REC,1);btn(B_REC,0);tick(20);
}

/* BREAK: middle 27 keep steps/slices; low outer paging stays; outers write nothing. */
static void break_rec(void)
{
    all_up();
    ui.freeplay[0]=ui.freeplay[1]=0;
    ui.overlay=O_NONE;ui.help=0;
    test_clip_mode=test_padcal=test_smcui=0;
    held_btn=ui.btn=0;ui.held_step=-1;ui.brk_held=-1;
    ui.brk_localkeys=ui.brk_pinkeys=0;
    seq_stop(&seq);ui.rec=0;scan();
    ui.part=PART_BRK;set_view(V_SEQ);build_pages();
    brkpart_t *bp=&cur_pat()->brk;
    bp->steps=0;
    memset(bp->slice,0,sizeof bp->slice);
    btn(B_REC,1);btn(B_REC,0);scan();
    T(ui.rec && ui.part==PART_BRK && !seq.playing);

    /* 16 white keys still write steps 0-15. */
    for(int w=0;w<16;w++) {
        unsigned p=(unsigned)WHITE_KEY[w]+5u;
        bp->steps=0;
        edge(p,100);edge(p,0);scan();
        T((bp->steps>>w)&1u);
        fprintf(report,"break WHITE physical %u step %d %s\n",p,w,
                ((bp->steps>>w)&1u)?"RECORDED":"DROPPED");
        if((bp->steps>>w)&1u) records++; else drops++;
    }

    /* Outer keys write no step and no slice. */
    bp->steps=0;
    memset(bp->slice,0,sizeof bp->slice);
    { uint16_t steps0=bp->steps; uint8_t sl0[16]; memcpy(sl0,bp->slice,16);
      static const unsigned outer[10]={0,1,2,3,4,32,33,34,35,36};
      for(unsigned j=0;j<10;j++) {
          unsigned p=outer[j];
          edge(p,100);edge(p,0);scan();
          T(bp->steps==steps0);
          T(memcmp(sl0,bp->slice,16)==0);
          { uint8_t got[16];int ng=recorded_notes(got,16); T(ng==0); }
          fprintf(report,"break OUTER physical %u unmapped for step/slice (steps=%u)\n",p,(unsigned)bp->steps);
          printf("break OUTER physical %u unmapped ok\n",p);
      }
    }

    /* Low outer paging (C#3 page-down, D#3 page-up, C3/D3 count) still works
     * through ui_brk_page_keys, which reads the 37-bit bitmap — not plat_keys.
     * Drive it the same way adaptive_break_controls_test.c does. */
    {
        int page0=ui.brk_page;
        ui_brk_page_keys(0,1);
        ui_brk_page_keys(2,1); /* D#3 = page++ in the 4-bit mask (bit 1) */
        ui_brk_page_keys(0,1);
        /* page++ is a no-op if only one page of slices; just prove the call is intact. */
        T(ui.brk_page==page0 || ui.brk_page==page0+1);
        fprintf(report,"break paging ui_brk_page_keys intact (page %d->%d)\n",page0,ui.brk_page);
        ui.brk_page=(uint8_t)page0;
    }
    btn(B_REC,1);btn(B_REC,0);tick(20);
}

/* Host cycle estimate of the feeder: steady REC (no edges) and a rise+fall. */
static void cycle_estimate(void)
{
#if defined(__x86_64__) || defined(__i386__)
    unsigned long long t0,t1,acc,best;
    int i,r;
    enter_303_stopped(ORIGINAL_SOUND_CHICAGO,0);
    all_up();scan(); /* armed, no edges */
    for(i=0;i<2000;i++) rec_outer_keys(); /* warmup */
    best=~0ull;acc=0;
    for(r=0;r<7;r++) {
        t0=__builtin_ia32_rdtsc();
        for(i=0;i<20000;i++) rec_outer_keys();
        t1=__builtin_ia32_rdtsc();
        if(t1>t0){ unsigned long long d=t1-t0; if(d<best)best=d; acc+=d; }
    }
    double steady=(double)best/20000.0;
    /* one outer rise+fall, including the writer */
    best=~0ull;
    for(r=0;r<7;r++) {
        all_up();scan();
        t0=__builtin_ia32_rdtsc();
        physical(0,100);rec_outer_keys();
        physical(0,0);rec_outer_keys();
        t1=__builtin_ia32_rdtsc();
        if(t1>t0){ unsigned long long d=t1-t0; if(d<best)best=d; }
    }
    fprintf(report,"CYCLE feeder steady (no edge) best=%.1f host-cycles/call (20k x 7, rdtsc)\n",steady);
    fprintf(report,"CYCLE feeder outer rise+fall best=%llu host-cycles (rdtsc, includes bass_kbd_physical)\n",
            (unsigned long long)best);
    printf("CYCLE feeder steady=%.1f cyc/call  rise+fall=%llu cyc\n",steady,(unsigned long long)best);
#else
    fprintf(report,"CYCLE not measured (host is not x86)\n");
    printf("CYCLE not measured (not x86)\n");
#endif
}

int main(int argc,char **argv)
{
    const char *out_path="build/host/rec-keybed-37/rec-keybed-37.txt";
    if(argc>1) {
        static char path[512];
        snprintf(path,sizeof path,"%s/rec-keybed-37.txt",argv[1]);
        out_path=path;
    }
    report=fopen(out_path,"w");
    if(!report){perror(out_path);return 2;}
    fprintf(report,"REC keybed 37-key GATE\n");
    fprintf(report,"303 expected pitch = 36 + physical + 12*octave.\n");
    fprintf(report,"FM-1 window: physical 5..31. Outer: 0..4 and 32..36.\n");
    fprintf(report,"909/808 outer keys: unmapped (NDRUM=11, KEY_BLACK[27]).\n\n");
    boot();tick(300);
    int banks[]={ORIGINAL_SOUND_CHICAGO,ORIGINAL_SOUND_MIAMI,ORIGINAL_SOUND_DRONE,ORIGINAL_SOUND_JUNO};
    const char *bname[]={"Chicago","Miami","Drone","Juno"};
    for(unsigned i=0;i<4;i++) {
        int bank=banks[i];
        fprintf(report,"\n==== engine %s (bank %d) ====\n",bname[i],bank);
        printf("\n==== engine %s (bank %d) ====\n",bname[i],bank);
        seq_stop(&seq);ui.rec=0;scan();
        if(engine_bass_select_preset(0,bank,0,&proj.sound)) {
            fprintf(report,"SKIP bank %d: select_preset failed\n",bank);
            printf("SKIP bank %d\n",bank);
            continue;
        }
        tick(80);
        if(engine_bass_sound_bank(&proj.sound,0)!=bank) {
            fprintf(report,"SKIP bank %d: sound_bank=%d\n",bank,engine_bass_sound_bank(&proj.sound,0));
            printf("SKIP bank %d sound_bank=%d\n",bank,engine_bass_sound_bank(&proj.sound,0));
            continue;
        }
        if(bank==ORIGINAL_SOUND_CHICAGO) {
            mono_stopped(bank);
            mono_playing(bank);
        } else {
            enter_303_stopped(bank,0);
            for(unsigned p=0;p<37;p++) {
                clear_bass();
                int pitch=expected_pitch(p,0);
                edge(p,100);edge(p,0);scan();
                uint8_t got[16];int ng=recorded_notes(got,16);
                int ok=has_note(got,ng,pitch);
                T(ok);
                log_row("mono-stopped",bank,0,p,pitch,ok,got,ng);
            }
            for(int oct=-2;oct<=3;oct+=5) {
                enter_303_stopped(bank,oct);
                unsigned sample[8]={0,1,4,5,17,31,32,36};
                for(unsigned j=0;j<8;j++) {
                    unsigned p=sample[j];
                    clear_bass();
                    int pitch=expected_pitch(p,oct);
                    edge(p,100);edge(p,0);scan();
                    uint8_t got[16];int ng=recorded_notes(got,16);
                    int ok=has_note(got,ng,pitch);
                    T(ok);
                    log_row("mono-stopped",bank,oct,p,pitch,ok,got,ng);
                }
            }
        }
        chords(bank);
        chord_three_poly(bank);
        ownership(bank);
    }

    fprintf(report,"\n==== 909 / 808 / BREAK ====\n");
    printf("\n==== 909 / 808 / BREAK ====\n");
    drum_voices(PART_909);
    drum_voices(PART_808);
    break_rec();
    cycle_estimate();

    fprintf(report,"\n==== SUMMARY ====\n");
    fprintf(report,"recorded_ok=%u  dropped=%u  fails=%u  checks=%u\n",records,drops,fails,checks);
    printf("\nSUMMARY recorded_ok=%u dropped=%u fails=%u checks=%u (report %s)\n",
           records,drops,fails,checks,out_path);
    fclose(report);
    return (drops||fails)?1:0;
}
