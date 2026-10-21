/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen ca6c21b page-construction oracle; independent of recipe implementation. */
static void reference_build_pages(void)
{
    int p = ui.part, b = bidx();
    pg.n = pg.nmore = 0;
    pg_grp = GR_NONE;
    switch (ui.view) {
    case V_HOME:                                       /* the pattern: how the parts run through it */
    case V_GLO:
        add_page("PERFORM", PR(R_TEMPO, 0, 0, 0), PR(R_SWING, 0, 0, 0), PR(R_ENG, T_MST, 0, MST_DJF),
                 PR(R_ENG, T_MST, 0, MST_PUMP));
        break;
    case V_SEQ:                                        /* the part's steps: how its pattern runs */
        pg_grp = GR_PAT;
        if (is_drum())
            add_page("SEQUENCE", PR(R_DLEN, p, 0, 0), PR(R_DRATE, p, 0, 0), NONE, NONE);   /* (the swing: HOME's, all parts') */
        else if (is_303())
        {
            add_page("SEQUENCE", PR(R_BLEN, b, 0, 0), PR(R_BRATE, b, 0, 0), PR(R_BDIR, b, 0, 0), PR(R_BTRANS, b, 0, 0));
            pg_grp = GR_NONE;                          /* (not the pattern's: no P1 in its name) */
            add_page("KEYS", PR(R_FREEPLAY, b, 0, 0), NONE, NONE, NONE);
        }
        else {
            add_page("GROOVE", PR(R_BRKSET, 0, 0, BRK_COMPLEX), PR(R_BRKSET, 0, 0, BRK_ANCHOR),
                     PR(R_BRKSET, 0, 0, BRK_ROLL), PR(R_BRKSET, 0, 0, BRK_FILL));
            add_page("RETRIG", PR(R_BRKSET, 0, 0, BRK_R2), PR(R_BRKSET, 0, 0, BRK_R3), PR(R_BRKSET, 0, 0, BRK_R4),
                     PR(R_BRKSET, 0, 0, BRK_R8));
            more(PR(R_BRKSET, 0, 0, BRK_PHRASE));
            more(PR(R_BRKSET, 0, 0, BRK_ALEN));
            more(PR(R_BRKSET, 0, 0, BRK_BLEN));
        }
        break;
    case V_PART:                                       /* the part's sound */
        if (is_drum()) {
            int t = p == PART_909 ? T_909 : T_808;
            add_track_pages(t, ui.sel[p]);
            pg_grp = GR_KIT;
            if (p == PART_909)                         /* (the kit's gain: the mixer's level does it) */
                add_page("KIT", PR(R_ENG, T_909, NDRUM, 1), NONE, NONE, NONE);
            else
                add_page("KIT", PR(R_ENG, T_808, NDRUM, 1), NONE, NONE, NONE);
        } else if (is_303()) {
            pg_grp = GR_SOUND;
            add_page("TONE", PR(R_ENG, T_303, b, BASS303_CUTOFF), PR(R_ENG, T_303, b, BASS303_RESO),
                     PR(R_ENG, T_303, b, BASS303_ENVMOD), PR(R_ENG, T_303, b, BASS303_DECAY));
            add_page("VOICE", PR(R_ENG, T_303, b, BASS303_ACCENT), PR(R_ENG, T_303, b, BASS303_WAVE),
                     PR(R_ENG, T_303, b, BASS303_DRIVE), PR(R_ENG, T_303, b, BASS303_SLIDE));
            more(PR(R_ENG, T_303, b, BASS303_TUNE));
            more(PR(R_ENG, T_303, b, BASS303_VOLUME));
            more(PR(R_ENG, T_303, b, BASS303_DRVTYPE));
            more(PR(R_ENG, T_303, b, BASS303_ACCDEC));
            add_page("FILTER ADSR", PR(R_ENG, T_303, b, BASS303_FATTACK), PR(R_ENG, T_303, b, BASS303_DECAY),
                     PR(R_ENG, T_303, b, BASS303_FSUSTAIN), PR(R_ENG, T_303, b, BASS303_FRELEASE));
            more(PR(R_ENG, T_303, b, BASS303_FENV));
        } else {
            pg_grp = GR_PAT;                           /* (its groove and retrigs: SEQ) */
            add_page("LOOPS", PR(R_BRKSLOT, 0, 0, 0), PR(R_BRKSLOT, 1, 0, 0), PR(R_BRKSET, 0, 0, BRK_BCHANCE),
                     PR(R_ENG, T_BRK, 0, 1));
            more(PR(R_ENG, T_BRK, 0, 0));
        }
        break;
    case V_GEN:
        pg_grp = GR_PAT;
        add_page("GENERATE", PR(R_GEN, b, 0, G_DENS), PR(R_GEN, b, 0, G_ACC), PR(R_GEN, b, 0, G_SLIDE),
                 PR(R_GEN, b, 0, G_OCTS));
        add_page("SCALE", PR(R_GEN, b, 0, G_ROOT), PR(R_GEN, b, 0, G_SCALE), PR(R_GEN, b, 0, G_BASE),
                 PR(R_GEN, b, 0, G_MUT));
        break;
    case V_FX:                                         /* (each part's sends: MIX's PARTS) */
        add_page("REVERB", PR(R_ENG, T_FX, 0, FX_RV_DECAY), PR(R_ENG, T_FX, 0, FX_RV_TONE), NONE, NONE);
        add_page("DELAY", PR(R_ENG, T_FX, 0, FX_DL_TIME), PR(R_ENG, T_FX, 0, FX_DL_FDBK),
                 PR(R_ENG, T_FX, 0, FX_DL_TONE), PR(R_TAPE, 0, 0, 0));
        more(PR(R_ENG, T_FX, 0, FX_RV_HPF));
        more(PR(R_ENG, T_FX, 0, FX_RV_LEVEL));
        more(PR(R_ENG, T_FX, 0, FX_DL_HPF));
        more(PR(R_ENG, T_FX, 0, FX_DL_LEVEL));
        more(PR(R_ENG, T_FX, 0, FX_DL_PING));
        break;
    case V_MIX:                                        /* a channel picked with a black key, its four knobs */
        add_page("PARTS", PR(R_ENG, T_MIX, ui.mixsel, MX_LEVEL), PR(R_ENG, T_MIX, ui.mixsel, MX_PAN),
                 PR(R_ENG, T_MIX, ui.mixsel, MX_REV), PR(R_ENG, T_MIX, ui.mixsel, MX_DLY));
        for (p = 0; p < NKIT; p++) {                    /* each drum machine's tracks: LEVEL, PAN, REV, DLY */
            int t = p ? T_808 : T_909, v = ui.sel[p];
            add_page(p ? "808 MIX" : "909 MIX", find_ref(t, v, "Level"), find_ref(t, v, "Pan"), find_ref(t, v, "Rev"),
                     find_ref(t, v, "Dly"));
        }
        add_page("MASTER", PR(R_ENG, T_FX, 0, FX_DRIVE), PR(R_ENG, T_MST, 0, MST_COMP1), PR(R_ENG, T_MST, 0, MST_PUMP),
                 PR(R_ENG, T_MST, 0, MST_DJF));
        more(PR(R_ENG, T_MST, 0, MST_THRESH));
        more(PR(R_ENG, T_MST, 0, MST_RATIO));
        more(PR(R_ENG, T_MST, 0, MST_ATTACK));
        more(PR(R_ENG, T_MST, 0, MST_RELEASE));
        more(PR(R_ENG, T_MST, 0, MST_MAKEUP));
        more(PR(R_ENG, T_MST, 0, MST_MIX));
        more(PR(R_ENG, T_MST, 0, MST_PUMPSRC));
        more(PR(R_ENG, T_MST, 0, MST_RESO));
        more(PR(R_ENG, T_MST, 0, MST_LIMIT));
        more(PR(R_ENG, T_FX, 0, FX_DIST));
        more(PR(R_ENG, T_FX, 0, FX_COMP));
        more(PR(R_ENG, T_FX, 0, FX_VOLUME));
        break;
    case V_PERF:
        add_page("PERF", PR(R_STALLS, 0, 0, 0), NONE, NONE, NONE);
        break;
    case V_SONG:
        add_page("BAR", PR(R_SBAR, 0, 0, 0), PR(R_SPAT, PART_909, 0, 0), PR(R_SPAT, PART_808, 0, 0),
                 PR(R_SPAT, PART_303A, 0, 0));
        add_page("BAR", PR(R_SBAR, 0, 0, 0), PR(R_SPAT, PART_303B, 0, 0), PR(R_SPAT, PART_BRK, 0, 0),
                 PR(R_SREP, 0, 0, 0));
        break;
    default: break;
    }
    if (pg.n == 0)
        add_page("", NONE, NONE, NONE, NONE);
    if (*page_of(ui.view) >= pg.n)
        *page_of(ui.view) = 0;
}
