/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen d8086cc rendering oracle: only function names, commit destination and
 * out-of-scope normal pattern badge painting differ. Compare x<130 and transport,
 * or full toast/sound headers, plus full list bands. */
static const char *donor_action_key_cue(void)
{
#if BOARD_SMK37
    extern uint8_t smk_clip_mode_on(void);
    if(smk_clip_mode_on())return ""; /* Physical SCALE is masked; SMC dispatch stays available. */
#endif
    return UI_CAP_SEL;
}
static int donor_release_label_box(const char *label, int selected, int32_t *x, int32_t *w)
{
    int32_t name_w, lw, lx;
    if (!label || !label[0])
        return 0;
    name_w = tw(selected ? &FONT_B : &FONT_S, "ABOUT DnBOX");
    lw = tw(&FONT_XS, label);
    lx = 226 - tw(&FONT_XS, donor_action_key_cue()) - 8 - lw;
    if (lx < 8 + name_w + 4 || lx + lw > 226 - tw(&FONT_XS, donor_action_key_cue()) - 8)
        return 0;
    *x = lx;
    *w = lw;
    return 1;
}
static UI_DRAW void donor_draw_header(void)
{
#if BOARD_SMK37
    extern uint8_t smk_clip_mode_on(void);
#endif
    char b[24];
    int32_t x;
    int ss = save_state();
    int pickup = ui.msg[0] == 'P' && ui.msg[1] == 'I' && ui.msg[2] == 'C' &&
                 ui.msg[3] == 'K' && ui.msg[4] == ' ';
    cv_begin(240, HDR_H, C_BLACK);
    night_panel(0,0,240,HDR_H,0);
    if (ui.msg_until && (int32_t)(ui.msg_until - plat_ms()) > 0 && !pickup) {   /* what a button just did */
        box(0, 0, 240, HDR_H - 1, part_col());
        if (!memcmp(ui.msg, "NEXT: PAT ", 10)) {
            int pattern=0, n=10;
            while (n<(int)sizeof ui.msg-1 && ui.msg[n]>='0' && ui.msg[n]<='9' && pattern<=NPAT)
                pattern=pattern*10+ui.msg[n++]-'0';
            put_s(put_s(b, "> "), ui.msg+10);
            x=(240-tw(&FONT_B,"NEXT")-6-12-6-tw(&FONT_B,b))/2;
            x=cv_text(x,vc(&FONT_B,0,HDR_H-1),&FONT_B,"NEXT",C_BLACK)+6;
            frame(x,1,12,17,C_BLACK);
            if (!ui.msg[n] && pattern>0 && pattern<=NPAT) {
                for (int pos=0;pos<NPARTS;pos++) {
                    uint8_t strength[8];
                    home_pattern_strength8(&proj.pat[pattern-1],proj.arr.lane,pattern-1,part_at(pos),
                                           proj.set.accent_q7,T_VEL,strength);
                    pat_glyph_row(x+2,2+pos*3,strength,C_BLACK,0);
                }
            }
            cv_text(x+18,vc(&FONT_B,0,HDR_H-1),&FONT_B,b,C_BLACK);
        } else {   /* #800 NEXT pattern glyph; other messages keep #806's long-text font fallback */
            const felucca_font_t *font = tw(&FONT_B, ui.msg) <= 232 ? &FONT_B : &FONT_XS;
            text_c(120, vc(font, 0, HDR_H - 1), font, ui.msg, C_BLACK);
        }
        donor_header_commit(0, 0, 0);
        return;
    }
    if (!pickup || (int32_t)(ui.msg_until - plat_ms()) <= 0)
        ui.msg_until = pickup = 0;
    if (!pickup && ui.view==V_PART && (is_drum() || is_303()) && ui.overlay==O_NONE && !ui.help
#if BOARD_SMK37
        && !smk_clip_mode_on()
#endif
        ) {
        const char *name=part_display(ui.part);
        if(is_303()) {
            name=engine_bass_bank(bidx())==ORIGINAL_SOUND_MIAMI?(bidx()?"MIAMI B":"MIAMI A"):(bidx()?"CHICAGO B":"CHICAGO A");
        }
        int night=matrix_three_rows() || X0X_ORIGINAL_SYNTH;
        if(night) sound_slab(0,0,240,20,0);
        cv_text(7,2,&FONT_B,name,night?SOUND_GREEN:matrix_col());
        if (division_pad_owner()) {
            int dx = 7 + tw(&FONT_B, name) + 8;
            dx = cv_text(dx, 6, &FONT_T, "PADS ", C_AMB);
            cv_text(dx, 6, &FONT_T, division_pad_label() + 11, C_AMB);
        }
        {   /* Screen banks, not musical divisions or the selected control row. */
            int banks = (pg.n + 3) / 4;
            for (int i = 0; i < banks; i++) {
                int dx = 232 - (banks - 1 - i) * 10;
                dot(dx, 9, 2, night?SOUND_AMBER:C_HI);
                if (i != cur_page() / 4) dot(dx, 9, 1, night?SOUND_FACE:C_BLACK);
            }
        }
        if(!division_pad_owner()) header_pattern_cue(132,ui.part);   /* #807 cue picture */
        { uint8_t phase = seq.metro_view; /* #688/#788: audio-owned beat phase */
          play_icon(175, 4, !phase ? C_DIM : phase <= 4u ? C_WHITE : C_HI);
        }
        if(ui.rec) dot(190,9,3,RGB(255,50,50));
        if(ss!=SAVE_SAVED) dot(237,3,2,ss==SAVE_FAILED?RGB(255,70,70):C_AMB);
        donor_header_commit(0,0,0);
        return;
    }
    if (pickup) {                         /* fader pickup hint in the left chip; song/pattern and tempo stay visible */
        x = cv_text(4, vc(&FONT_S, 1, 17), &FONT_S, ui.msg, C_AMB) + 6;
    }
#if BOARD_SMK37
    else if (smk_clip_mode_on()) {                         /* clip mode: the mode chip reads LP (Loopy Pro) */
        x = cv_text(4, vc(&FONT_B, 1, 17), &FONT_B, "LP", C_HI) + 6;
    }
#endif
    else if (part_view()) {                           /* the part chip, then where we are */
        const char *part_label=part_display(ui.part);
#if X0X_BASS_PRESET_PREVIEW
        if ((ui.view == V_SEQ || ui.view == V_PART) && is_303()) {
#if X0X_BASS_TAGGED_SOUND
            if (engine_bass_sound_bank(&proj.sound, bidx()) < 0)
                part_label = bidx() ? "303 B" : "303 A";
            else
#endif
            {
            int miami = engine_bass_bank(bidx()) == FACTORY_BANK_MIAMI;
            part_label = miami ? (bidx() ? "MIAMI B" : "MIAMI A") :
                                 (bidx() ? "CHICAGO B" : "CHICAGO A");
            }
        }
#endif
        const char *mode = ui.view==V_GEN?"TB-3PO":ui.view==V_SEQ?(!kbd_on()?"SEQ":ui.rec?"SEQ KEYS":"FREE PLAY"):
                           ui.view==V_PART&&is_303()?"SOUND":kbd_on()?(ui.rec?"KEYS":"FREE PLAY"):"";
        const felucca_font_t *pf=&FONT_B,*mf=&FONT_S;
        if(tw(pf,part_label)+18+tw(mf,mode)>128)pf=mf=&FONT_XS;
        int32_t w = tw(pf, part_label) + 12;
        box(0,1,w,17,part_col());cv_text(6,vc(pf,1,17),pf,part_label,C_BLACK);
        x=w+6;
        if(mode[0])x=cv_text(x,vc(mf,1,17),mf,mode,kbd_on()&&ui.view!=V_GEN&&!(ui.view==V_PART&&is_303())?C_AMB:C_HI)+6;
    } else {
        x = cv_text(4, vc(&FONT_B, 1, 17), &FONT_B, ui.view==V_GLO?list_title:VIEW_N[ui.view], C_HI) + 6;
        if (drum_mix_page() >= 0)                     /* MIX: 909 / MIX: 808 (whose tracks these are) */
            x = cv_text(x - 6, vc(&FONT_B, 1, 17), &FONT_B, drum_mix_page() ? ": 808" : ": 909",
                        PART_COL[drum_mix_page()]) + 6;
    }
    if (ss != SAVE_SAVED) {               /* use the gap before pattern/clock, never cover cue or step range */
        const char *label = project_storage_unsupported() == 2 ? "!" :
            ss == SAVE_QUEUED ? queued_save_label() : ss == SAVE_FAILED ? "SAVE FAILED" : "UNSAVED";
        int32_t w = tw(&FONT_XS, label) + 10;
        uint16_t c = ss == SAVE_FAILED ? RGB(255, 70, 70) : C_AMB;
        if (x + w <= 130) {
            box(x, 3, w, 13, c);
            text_c(x + w / 2, vc(&FONT_XS, 3, 13), &FONT_XS, label, C_BLACK);
        } else if (x + 11 <= 130) {       /* a long mode name leaves room only for a short flag */
            box(x, 3, 11, 13, c);
            text_c(x + 5, vc(&FONT_XS, 3, 13), &FONT_XS, ss == SAVE_FAILED ? "!" : ss == SAVE_QUEUED ? "Q" : "*", C_BLACK);
        }
    }
#if BOARD_SMK37
    if (smk_clip_mode_on()) {                         /* clip mode: the clock of the arp, not the pattern */
        extern uint8_t smk_ck_ext(void);
        cv_text(132, vc(&FONT_S, 1, 17), &FONT_S, smk_ck_ext() ? "EXT" : "INT", smk_ck_ext() ? C_AMB : C_GRAY);
    } else
#endif
    { x = 132; } /* Position badge region intentionally excluded from donor comparison. */
    {   /* following an external clock, the measured tempo wanders a little from clock to clock: the
         * display averages it over about a second, and moves only when that is half a BPM away */
        static float avg;
        static int shown;
        float t = seq_tempo(&seq);
        if (!seq.ext || avg < 1.0f)
            avg = t;
        avg += (t - avg) * 0.03f;
        if (!seq.ext || fm_fabsf(avg - (float)shown) > 0.6f)
            shown = (int)(avg + 0.5f);
        put_i(b, shown);
    }
    text_r(212, vc(&FONT_S, 1, 17), &FONT_S, b, seq.ext ? C_AMB : C_HI);
    { uint8_t phase = seq.metro_view; /* neutral in STOP/WAIT; no track playhead */
      play_icon(217, 4, !phase ? C_DIM : phase <= 4u ? C_WHITE : C_HI);
    }
    if (ui.rec)
        dot(231, 9, 4, RGB(255, 50, 50));
    if (ss != SAVE_SAVED)            /* still visible when REC or a long mode name uses the badge space */
        dot(237, 3, 2, ss == SAVE_FAILED ? RGB(255, 70, 70) : C_AMB);
    donor_header_commit(0, 0, 0);
}
static UI_DRAW void donor_draw_list(int band)
{
    int i;
    for (i = 0; i < 3; i++) {
        int row = ui.list_top + band * 3 + i, y = i * 23, sel = row == ui.list_sel;
        pref_t r;
        char num[36], unit[8];
        const x0x_param_t *d;
        if (row >= ui.list_n)
            break;
        r = list_rows[row];
        if (r.kind == R_HDR) {                         /* a section: whose values follow */
            char t[28];
            hdr_name(r.a, t);
            cv_text(8, vc(&FONT_XS, y + 4, 16), &FONT_XS, t, C_AMB);
            cv_rect(8, y + 19, 220, 1, C_LINE);
            continue;
        }
        d = pref_desc(r);
        surface_panel(3,y+1,229,21,sel,sel?dim(part_col(),6):SOUND_FACE,
                      sel?part_col():SOUND_EDGE,SOUND_SIDE);
        cv_text(8, vc(sel ? &FONT_B : &FONT_S, y + 1, 18), sel ? &FONT_B : &FONT_S, caps_of(pref_name(r)), sel ? C_WHITE : C_GRAY);
        if (r.kind == R_SECTION) {                     /* a GLOBAL category: its chevron */
            cv_line(221,y+8,225,y+11,sel?C_AMB:C_DIM);cv_line(225,y+11,221,y+14,sel?C_AMB:C_DIM);continue;
        }
        if (r.kind == R_ACT) {                         /* an action: the return arrow */
            uint16_t ac = sel ? C_WHITE : C_DIM;
            if (r.a == ACT_ABOUT) {
                int32_t lx, lw;
                if (donor_release_label_box(X0X_RELEASE_LABEL, sel, &lx, &lw))
                    cv_text(lx, vc(&FONT_XS, y + 1, 18), &FONT_XS, X0X_RELEASE_LABEL, sel ? C_WHITE : C_HI);
            }
            text_r(226,vc(&FONT_XS,y+1,18),&FONT_XS,donor_action_key_cue(),ac);
            continue;
        }
        if(r.kind==R_BTNAME) {                       /* connected peers: one line, scrolls when long */
            plat_ble_peers_t peers;char shown[PEER_TEXT_CAP];const felucca_font_t *nf=sel?&FONT_B:&FONT_S;
            plat_ble_peers(&peers);peer_join(shown,&peers);
            int left=8+tw(nf,"NAME")+8, width=226-left, travel=tw(nf,shown)-width;
            int offset=peer_offset(shown,&peers,travel,plat_ms());
            cv_text_clip(left-offset,vc(nf,y+1,18),nf,shown,sel?C_WHITE:C_HI,left,226);continue;
        }
        pref_value(r, num, unit);
        if (r.kind == R_BPRESET) {
            const char *name = original_sound_name((unsigned)engine_bass_requested_bank(r.b),
                                                   (unsigned)engine_bass_requested_preset(r.b));
            put_s(num,caps_of(name ? name : "USER"));
        }
        {
            const felucca_font_t *nf = sel ? &FONT_B : &FONT_S;
            if ((r.kind == R_ENG && r.a == T_303) || r.kind == R_BENGINE || r.kind == R_BPRESET || r.kind == R_BSTATUS) {
                int room = 226-8-tw(sel ? &FONT_B : &FONT_S,caps_of(pref_name(r)))-8-
                           (unit[0] ? tw(&FONT_XS,unit)+2 : 0);
                if (tw(nf,num) > room) nf = tw(&FONT_XS,num) <= room ? &FONT_XS : &FONT_T;
            }
            int32_t ny = vc(nf, y + 1, 18), ux = 226 - tw(&FONT_XS, unit);
            cv_text(ux, base_y(nf, ny, &FONT_XS), &FONT_XS, unit, C_GRAY);
            text_r(unit[0] ? ux - 2 : 226, ny, nf, num, sel ? C_WHITE : C_HI);
        }
        if (d && !d->names && d->max >= 24) {          /* a continuous value: its bar */
            int v = pref_get(r);
            rbox(108, y + 18, 70, 3, 1, C_LINE);
            if (v)
                rbox(108, y + 18, 3 + v * 67 / d->max, 3, 1, sel ? part_col() : C_DIM);
        }
    }
    if (ui.list_n > 6) {                               /* the scrollbar */
        int h = 2 * BAND_H * 6 / ui.list_n, y = 2 * BAND_H * ui.list_top / ui.list_n - band * BAND_H;
        rbox(235, 0, 3, BAND_H, 1, C_LINE);
        rbox(235, y, 3, h, 1, C_HI);
    }
}
