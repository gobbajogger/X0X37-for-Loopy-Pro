/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen pre-ITEM3 raster paths. Test only; never included by firmware. */
static void arc_reference(int32_t cx, int32_t cy, int32_t r, float v, uint16_t track, uint16_t fill, int bipolar)
{
    const float th = 3.4f, a0 = 0.375f, sweep = 0.75f;  /* start at 135 degrees, in turns */
    float ro = (float)r, ri = ro - th, rm = ro - th * 0.5f, fx = (float)cx + 0.5f, fy = (float)cy + 0.5f;
    float lo = bipolar ? (v < 0.5f ? v : 0.5f) : 0.0f, hi = bipolar ? (v < 0.5f ? 0.5f : v) : v;
    int32_t x, y;
    v = fm_clampf(v, 0.0f, 1.0f);
    for (y = -r - 1; y <= r + 1; y++)
        for (x = -r - 1; x <= r + 1; x++) {
            float dx = (float)x + 0.5f - 0.5f, dy = (float)y + 0.5f - 0.5f, d = fm_sqrtf(dx * dx + dy * dy), t, a;
            a = fm_clampf(ro + 0.5f - d, 0.0f, 1.0f) * fm_clampf(d - ri + 0.5f, 0.0f, 1.0f);
            if (a <= 0.0f)
                continue;
            t = turns(dy, dx) - a0;
            if (t < 0.0f)
                t += 1.0f;
            t /= sweep;
            if (t > 1.0f)
                continue;                              /* the gap at the bottom; the caps close it */
            px_blend(cx + x, cy + y, (t >= lo && t <= hi) ? fill : track, (uint32_t)(a * 16.0f + 0.5f));
        }
    {   /* round ends: the track's, then the fill's, then the handle */
        float e0 = (a0) * FM_TWO_PI, e1 = (a0 + sweep) * FM_TWO_PI, ev = (a0 + sweep * v) * FM_TWO_PI;
        float el = (a0 + sweep * lo) * FM_TWO_PI;
        disc(fx + fm_cosf(e0) * rm - 0.5f, fy + fm_sinf(e0) * rm - 0.5f, th * 0.5f, lo <= 0.0f && hi > 0.0f ? fill : track);
        disc(fx + fm_cosf(e1) * rm - 0.5f, fy + fm_sinf(e1) * rm - 0.5f, th * 0.5f, hi >= 1.0f ? fill : track);
        if (hi > lo)
            disc(fx + fm_cosf(el) * rm - 0.5f, fy + fm_sinf(el) * rm - 0.5f, th * 0.5f, fill);
        disc(fx + fm_cosf(ev) * rm - 0.5f, fy + fm_sinf(ev) * rm - 0.5f, th * 0.5f + 1.3f, C_BLACK);
        disc(fx + fm_cosf(ev) * rm - 0.5f, fy + fm_sinf(ev) * rm - 0.5f, th * 0.5f + 0.6f, C_WHITE);
    }
}

static UI_DRAW void draw_header_reference(void)
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
        cv_commit_header();
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
        cv_commit_header();
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
    {   /* the pattern of the part on screen (else the 909's), its cue; in SONG mode the bar */
        int part = part_view() ? ui.part : PART_909, cue = seq_cue_of(&seq, part);
        int pending=cue>=0 && cue!=seq.ppat[part] && !seq.song_on;
        x=ui_position_badge(seq.song_on?UI_G_SONG:UI_G_PATTERN,
            (seq.song_on?(seq.playing?seq.song_pos:ui.song_sel):seq.ppat[part])+1,
            pending?cue+1:0,seq.song_on?(seq.song_rec?RGB(255,90,80):C_AMB):C_WHITE,
            (ui.frame & 16u)?C_WHITE:C_GRAY);
        if (!pending && seq.chain_a != seq.chain_b && !seq.song_on) {
            cv_text(x + 3, vc(&FONT_XS, 1, 17), &FONT_XS, "CHN", C_AMB);
        } else if (!pending && steps_view() && steps_len() > 16) {   /* the 16 steps the keys show */
            put_i(put_s(put_i(b, ui.spage * 16 + 1), "-"), ui.spage * 16 + 16);
            cv_text(x + 4, vc(&FONT_XS, 1, 17), &FONT_XS, b, C_GRAY);
        }
    }
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
    cv_commit_header();
}

static UI_DRAW void draw_master_wave_reference(void)
{
    enum { X0=8,W=188,MID=34,AMP=26 };
    static int16_t latched[SCOPE_N];
    static uint32_t last_ms, last_w, last_hash;
    static uint8_t have;
    uint32_t now = plat_ms(), w = eng_scope_w, h, base;
    int i, x, y0, y1;
    if (!have || (int32_t)(now - last_ms) >= 100) {
        base = w;
        h = 2166136261u;
        for (i = 0; i < (int)SCOPE_N; i++)
            h = (h ^ (uint32_t)(uint16_t)eng_scope[(base + (uint32_t)i) & (SCOPE_N - 1u)]) * 16777619u;
        if (!have || h != last_hash || w != last_w) {
            for (i = 0; i < (int)SCOPE_N; i++)
                latched[i] = eng_scope[(base + (uint32_t)i) & (SCOPE_N - 1u)];
            last_hash = h;
            last_w = w;
            have = 1;
        }
        last_ms = now;
    }
    night_panel(3,1,198,66,0);
    cv_rect(X0,MID,W,1,C_LINE);
    y0 = MID - (int)latched[0] * AMP / 32768;
    if (y0 < 2)
        y0 = 2;
    if (y0 > BAND_H - 3)
        y0 = BAND_H - 3;
    for (x = 1; x < W; x++) {
        int si = x * (int)SCOPE_N / W;
        y1 = MID - (int)latched[si] * AMP / 32768;
        if (y1 < 2)
            y1 = 2;
        if (y1 > BAND_H - 3)
            y1 = BAND_H - 3;
        cv_line(X0 + x - 1, y0, X0 + x, y1, part_col());
        y0 = y1;
    }
}
