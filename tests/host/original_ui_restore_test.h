/* SPDX-License-Identifier: GPL-3.0-only */
/* Focused restoration proof using the real host application and renderer. */
static void restore_shot(const char *name)
{
    if(image_dir){char path[1024];CHECK(snprintf(path,sizeof path,"%s/%s.png",image_dir,name)>0);png_write(path);}
}
static void restore_tick(void){render_block();ui_frame();}
static void restore_audio_ms(unsigned ms)
{
    unsigned blocks=(ms*441u+2559u)/2560u;
    for(unsigned i=0;i<blocks;i++){render_block();now_ms+=6;}
}
static void restored_shared_cases(void)
{
    const int views[]={V_HOME,V_SEQ,V_GEN,V_FX,V_MIX,V_SONG,V_GLO,V_PART};
    unsigned slots=0,pages=0;
    for(unsigned vi=0;vi<sizeof views/sizeof views[0];vi++)for(int part=0;part<NPARTS;part++){
        ui_test_frame(views[vi],part,0);int count=pg.n;
        for(int page=0;page<count;page++){
            *page_of(views[vi])=(uint8_t)page;build_pages();
            for(int i=0;i<4;i++){
                pref_t r=pg.r[cur_page()][i];const x0x_param_t *d=pref_desc(r);if(!d)continue;slots++;
                for(int edge=0;edge<3;edge++){
                    int val=edge==0?0:edge==1?d->max/2:d->max,bad=0;
                    cv_begin(240,KNOB_H,C_BLACK);
                    if(!perform_glyph(r,val,30,C_HI))control_glyph(r,val,30,C_HI);
                    for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)
                        if(x<10 || x>50 || y<26 || y>50)bad|=cv_px[y*240+x]!=swap16(C_BLACK);
                    CHECK(!bad);
                }
            }
            project_t *before=malloc(sizeof proj);CHECK(before);memcpy(before,&proj,sizeof proj);
            ui_frame();CHECK(!memcmp(before,&proj,sizeof proj));free(before);pages++;
            if((part==PART_909 && (views[vi]==V_HOME || views[vi]==V_MIX || views[vi]==V_FX || views[vi]==V_SONG || views[vi]==V_SEQ)) ||
               (part==PART_BRK && views[vi]==V_PART) || (part==PART_303B && views[vi]==V_SEQ)){
                char name[80];snprintf(name,sizeof name,"restored-shared-view%d-part%d-page%d",views[vi],part,page);restore_shot(name);
            }
        }
    }
    /* Exact selected identities through eight; coarse eight bins above eight. */
    for(int n=0;n<=32;n++){
        if(!n)CHECK(glyph_identity_index(n,0)==-1);
        for(int v=0;v<n;v++){
            int idx=glyph_identity_index(n,v);CHECK(idx>=0 && idx<(n>8?8:n));if(n<=8)CHECK(idx==v);
        }
    }
    /* Native lowercase glyphs are independent of the explicit menu casing. */
    const felucca_font_t *fonts[]={&FONT_XS,&FONT_S,&FONT_B};
    for(unsigned f=0;f<3;f++)for(int c='a';c<='z';c++)CHECK(fonts[f]->off[glyph(fonts[f],(uint32_t)c)]!=fonts[f]->off[glyph(fonts[f],(uint32_t)(c-'a'+'A'))]);
    CHECK(tw(&FONT_T,"SCHEMATIC / CONTROL AMOUNTS (%)")<=214);
    CHECK(slots>100 && pages>30);
    printf("RESTORED shared slots=%u pages=%u bounds/project/font PASS\n",slots,pages);
}
static void restored_envelope_cases(void)
{
    /* Exhaust the finite control endpoints and middle for monotonic geometry. */
    const int edge[]={0,64,127};
    for(int a=0;a<3;a++)for(int d=0;d<3;d++)for(int s=0;s<3;s++)for(int r=0;r<3;r++){
        int v[]={edge[a],edge[d],edge[s],edge[r]},x[5],sy;original_adsr_points(v,x,&sy);
        CHECK(x[0]==13 && x[4]==227 && sy>=58 && sy<=130);
        for(int k=1;k<5;k++)CHECK(x[k]>=x[k-1]);
        if(!v[0])CHECK(x[1]==x[0]);
        if(!v[3])CHECK(x[3]==x[4]);
        if(!v[2])CHECK(sy==130);
        if(v[2]==127)CHECK(sy==58);
    }
    choose(1,0,0);capture("restored-chicago-overview",V_PART,PART_303B,0);
    choose(0,1,0);capture("restored-miami-overview",V_PART,PART_303A,0);
    for(int k=0;k<4;k++){
        ui_test_frame(V_PART,PART_303B,3);now_ms+=1000;
        int value=pref_get(pg.r[3][k]);enc_acc[EN_K1+k]=value==127?-1:1;ui_input_only();render_block();ui_frame();
        CHECK(adsr_touch_active());CHECK(matrix_touch_matches(pg.r[3][k],k));
        char name[64];snprintf(name,sizeof name,"restored-envelope-touch-%d",k);restore_shot(name);
        now_ms=ui.touch_until+1;ui_frame();CHECK(!adsr_touch_active());
    }
    ui_test_frame(V_PART,PART_303B,3);uint8_t saved[32];memcpy(saved,proj.sound.v[T_303][1],32);
    int lane=motion_alloc(proj.arr.lane,seq.ppat[PART_303B],PART_303B,T_303,1,7);CHECK(lane>=0);
    proj.arr.lane[lane].val[0]=0;ui.held_step=0;ui.held_step_part=PART_303B;ui.held_step_pat=seq.ppat[PART_303B];ui.rec=0;
    now_ms+=1000;enc_acc[EN_K1]=1;ui_input_only();render_block();ui_frame();
    CHECK(adsr_touch_active() && lock_step()==0);CHECK(matrix_value(pg.r[3][0])==1);
    CHECK(!memcmp(saved,proj.sound.v[T_303][1],32));restore_shot("restored-envelope-lock");
    bass_step_cancel();render_block();CHECK(ui.held_step<0);
    CHECK(engine_bass_select_preset(1,0,0,&proj.sound)==-4);
    engine_motion_clear(lane);render_block();CHECK(!proj.arr.lane[lane].used);
    CHECK(!memcmp(saved,proj.sound.v[T_303][1],32));
    /* Miami has no envelope row and may never acquire this overlay. */
    choose(0,1,0);
    for(int page=0;page<5;page++){
        ui_test_frame(V_PART,PART_303A,page);now_ms+=1000;enc_acc[EN_K1]=1;ui_input_only();render_block();ui_frame();CHECK(!adsr_touch_active());
    }
    printf("RESTORED Chicago touch/lock/expiry, Miami exclusion PASS\n");
}
static void restored_animation_cases(void)
{
    seq_stop(&seq);render_block();engine_fx_mute(0);
    ui_test_frame(V_FX,PART_909,1);
    pref_set(PR(R_TAPE,0,0,0),64);pref_set(PR(R_ENG,T_FX,0,FX_DL_FDBK),80);
    pref_set(PR(R_ENG,T_FX,0,FX_DL_LEVEL),100);pref_set(PR(R_ENG,T_MIX,PART_909,MX_DLY),100);render_block();
    engine_drum(0,0,1.0f);restore_audio_ms(30);ui_frame();CHECK(engine_tape_active() && !seq.playing);
    uint8_t phase=tape_glyph_phase;uint32_t at=tape_glyph_at;
    now_ms=at+79;restore_tick();CHECK(tape_glyph_phase==phase);
    now_ms=at+80;restore_tick();CHECK(tape_glyph_phase==(phase+1)%16);
    uint16_t first[240*KNOB_H];memcpy(first,fb+240*KNOB_Y,sizeof first);
    uint8_t first_phase=tape_glyph_phase;uint32_t first_sig=knobs_sig();restore_shot("restored-tape-phase-00");
    for(int i=1;i<=16;i++){
        now_ms=tape_glyph_at+80;engine_drum(0,0,1.0f);restore_tick();
        CHECK(engine_tape_active() && tape_glyph_phase==(first_phase+i)%16);
        char name[64];snprintf(name,sizeof name,"restored-tape-phase-%02d",i);restore_shot(name);
    }
    CHECK(knobs_sig()==first_sig);CHECK(!memcmp(first,fb+240*KNOB_Y,sizeof first));
    /* Inactive and off-screen animation may not churn the strip signature. */
    engine_fx_mute(2);restore_tick();CHECK(!engine_tape_active());phase=tape_glyph_phase;
    uint32_t sig=knobs_sig();memcpy(first,fb+240*KNOB_Y,sizeof first);
    now_ms+=5000;ui_frame();CHECK(tape_glyph_phase==phase && knobs_sig()==sig);CHECK(!memcmp(first,fb+240*KNOB_Y,sizeof first));
    engine_fx_mute(0);restore_tick();CHECK(engine_tape_active() && tape_glyph_phase==phase);
    now_ms+=79;restore_tick();CHECK(tape_glyph_phase==phase);now_ms++;restore_tick();CHECK(tape_glyph_phase==(phase+1)%16);
    ui_test_frame(V_FX,PART_909,0);sig=knobs_sig();memcpy(first,fb+240*KNOB_Y,sizeof first);
    for(int i=0;i<16;i++){now_ms+=80;engine_drum(0,0,1.0f);restore_tick();CHECK(knobs_sig()==sig);CHECK(!memcmp(first,fb+240*KNOB_Y,sizeof first));}
    /* Actual wet tail continues after the send is removed, then settles. */
    ui_test_frame(V_FX,PART_909,1);pref_set(PR(R_ENG,T_MIX,PART_909,MX_DLY),0);render_block();
    int tail_active=0,tail_changed=0;phase=tape_glyph_phase;
    for(int i=0;i<90;i++){
        restore_audio_ms(80);ui_frame();int active=engine_tape_active();tail_active+=active;tail_changed+=tape_glyph_phase!=phase;phase=tape_glyph_phase;
        if(i<8 || i==89){char name[64];snprintf(name,sizeof name,"restored-tape-tail-%02d",i);restore_shot(name);}
    }
    CHECK(tail_active>2 && tail_changed>2 && !engine_tape_active());
    phase=tape_glyph_phase;sig=knobs_sig();for(int i=0;i<8;i++){restore_audio_ms(80);ui_frame();CHECK(!engine_tape_active() && tape_glyph_phase==phase && knobs_sig()==sig);}
    /* The metronome follows actual sequencer audio blocks, not wall-clock art. */
    uint32_t saved_mute=seq.mute;
    for(int part=0;part<NPARTS;part++)seq.mute|=seq_part_mask(part);
    ui_test_frame(V_HOME,PART_909,0);seq_start(&seq);unsigned seen=0,captured=0;
    for(int i=0;i<180;i++){
        render_block();now_ms+=6;ui_frame();CHECK(tempo_glyph_view==seq.metro_view && seq.metro_view<=32);
        if(seq.metro_view){seen|=1u<<(seq.metro_view-1);unsigned bucket=(seq.metro_view-1)/4;
            if(!(captured&(1u<<bucket))){char name[64];snprintf(name,sizeof name,"restored-tempo-phase-%u",bucket);restore_shot(name);captured|=1u<<bucket;}}
    }
    CHECK(seen==UINT32_MAX && captured==255);seq_stop(&seq);restore_tick();CHECK(!seq.metro_view && !tempo_glyph_view);restore_shot("restored-tempo-stopped");seq.mute=saved_mute;
    /* Animation never invalidates the original native projection cache. */
    ui_test_frame(V_PART,PART_303A,0);ui_original_cache before=original_ui;
    for(int i=0;i<32;i++){now_ms+=80;ui_frame();CHECK(!memcmp(&before,&original_ui,sizeof before));}
    printf("RESTORED tape 79/80ms,16 phases,freeze/hidden/tail; real tempo32 phases; cache PASS\n");
}
static void restored_perf_cases(void)
{
    static const uint32_t raw[]={120,255,256,300,511,65535,65536,UINT32_MAX};
    for(unsigned i=0;i<sizeof raw/sizeof raw[0];i++){
        uint16_t expected=(uint16_t)(i<6?raw[i]:65535);
        CHECK(perf_percent(raw[i])==expected);
        cpu_pct=raw[i];cpu_peak=raw[i];perf.have=1;perf.frames=eng_prof_frames-1u;perf_window(1);
        CHECK(perf.load==expected && perf.peak==expected && cpu_pct==raw[i] && cpu_peak==0);
        perf.res_peak[0]=perf.peak;CHECK(perf.res_peak[0]==expected);
    }
    cpu_pct=300;cpu_peak=511;perf.have=1;perf.frames=eng_prof_frames-1u;perf_window(1);
    perf.test=PT_IDLE;capture("restored-perf-load300-peak511",V_PERF,PART_909,0);
    CHECK(perf.load==300 && perf.peak==511);cpu_pct=cpu_peak=0;
    printf("RESTORED PERF uint16 saturation and actual300/511 render PASS\n");
}
static void restored_ui_cases(void)
{
    restored_shared_cases();restored_envelope_cases();restored_animation_cases();restored_perf_cases();
}
