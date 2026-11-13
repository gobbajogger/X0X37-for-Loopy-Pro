static void smk_wheels(void)
{
    enum { DEAD = 12, SPAN0 = 80, MOD_LO = 485, MOD_HI = 615 };
    /* ADC endpoints stay in -80..1103, rest reaches at most 201. Narrow the
     * bounded fields to pay for the ADC-publication history without more RAM. */
    static int16_t pc, plo, phi, mlo, mhi;
    static int16_t rest, mod_prev;
    static uint16_t mod_seen;
    static int8_t bend_part = -1, cut_part = -1;
    static int16_t bend_now, mw_now;
    uint16_t mod_seq = fm1_in.wheel_mod_seq;
    int32_t p = fm1_in.wheel[0], m = fm1_in.wheel[1], d, bend, mw, want;
    if (ui_project_busy())
        return;
    if (fm1_in.frames < 300u)                         /* (the readings settle) */
        return;
#if X0X_BASS_PRESET_PREVIEW
    /* A first physical wheel move must choose its engine and publish ownership
     * as one transaction. Otherwise the audio boundary can switch banks before
     * the command's engine-dependent parameter is selected. */
    engine_bass_edit_hold(1);
#endif
    if (!pc) {
        pc = p;
        plo = p - SPAN0;
        phi = p + SPAN0;
        mlo = m < MOD_LO ? m : MOD_LO;
        mhi = m > MOD_HI ? m : MOD_HI;
        mod_prev = (int16_t)m;
        mod_seen = mod_seq;
    }
    /* The range grows one count a call: an ADC spike (a pad or mux edge) cannot widen it at
     * once and move the rest point out of the dead zone. A real sweep holds and still reaches it. */
    plo -= p < plo;
    phi += p > phi;
    /* Confirm a stable endpoint on two adjacent ADC publications. Foreground
     * polls may read the same sample many times; they must not teach a range.
     * The IIR tail after one raw spike changes on each publication until it is
     * within three ADC counts of rest. It cannot teach that transient extreme.
     * A conversion between the snapshot reads defers learning to the next poll. */
    if (mod_seq != mod_seen && mod_seq == fm1_in.wheel_mod_seq) {
        int adjacent = (uint16_t)(mod_seq - mod_seen) == 1u;
        mlo -= adjacent && m < mlo && m == mod_prev;
        mhi += adjacent && m > mhi && m == mod_prev;
        mod_prev = (int16_t)m;
        mod_seen = mod_seq;
    }
    d = p - pc;
    if (d > -DEAD && d < DEAD) {
        bend = 0;
        if (++rest > 200) {                            /* resting: the centre follows a slow drift */
            pc += d > 0 ? 1 : d < 0 ? -1 : 0;
            rest = 0;
        }
    } else {
        rest = 0;
        bend = d > 0 ? (d - DEAD) * 63 / (phi - pc - DEAD > 1 ? phi - pc - DEAD : 1)
                     : (d + DEAD) * 63 / (pc - plo - DEAD > 1 ? pc - plo - DEAD : 1);
        bend = clamp(bend, -63, 63);
    }
    mw = mhi - m <= DEAD ? 0 : (mhi - m - DEAD) * 127 / (mhi - mlo - DEAD > 1 ? mhi - mlo - DEAD : 1);
    mw = clamp(mw, 0, 127);
    /* Hysteresis: a one-step change is ADC jitter, not a move (rest and the ends always pass).
     * Without it a resting wheel wrote cutoff / wobble depth and REC lanes by itself. */
    if (mw && mw != 127 && mw - mw_now < 2 && mw_now - mw < 2)
        mw = mw_now;
    if (bend && bend != 63 && bend != -63 && bend - bend_now < 2 && bend_now - bend < 2)
        bend = bend_now;
    /* Only physical wheel changes own sound writes. Keep each gesture's target until
     * release, so changing the visible part neither restores nor retargets a held wheel.
     * In particular, a stationary wheel must not overwrite a knob or sequencer lock. */
    if (bend != bend_now) {
        if (bend_part < 0 && bend && is_303())
            bend_part = (int8_t)bidx();
        if (bend_part < 0 && bend && ui.part == PART_BRK && !smk_clip_mode_on())
            bend_part = 2;
        if (bend_part == 2) {
            engine_brk_bend(bend < 0 ? bend * 8192 / 63 : bend * 8191 / 63);
            if (!bend) bend_part = -1;
        } else if (bend_part >= 0) {
            int param = engine_bass_is_wobble(bend_part) ?
                ORIGINAL_MIAMI_PITCH_BEND : ORIGINAL_CHICAGO_PITCH_BEND;
            PARAM_TRACE(PARAM_WHEEL_PITCH, T_303, bend_part, param,
                        bend ? ((bend + 63) * 127 + 63) / 126 : 64);
            if (bend)
                engine_set(T_303, bend_part, param, ((bend + 63) * 127 + 63) / 126);
            else
                engine_set_base(T_303, bend_part, param); /* exact factory value, including 0.5 */
            motion_touch(PR(R_ENG, T_303, bend_part, param)); /* REC on: the wheel records its lane */
            if (!bend)
                bend_part = -1;
        }
        bend_now = (int16_t)bend;
    }
    if (mw != mw_now) {
        if (cut_part < 0 && mw && is_303())
            cut_part = (int8_t)bidx();
        if (cut_part >= 0) {
            /* mw 0 is down (closed / min depth). mw 127 is up (open / max depth). */
            want = mw;
            PARAM_TRACE(PARAM_WHEEL_MOD, T_303, cut_part,
                        engine_bass_mod_param(cut_part), (int)want);
            engine_bass_mod(cut_part, (int)want);
            motion_touch(PR(R_ENG, T_303, cut_part, engine_bass_mod_param(cut_part)));
#if BOARD_SMK37
            ui_wheel_mod(1, cut_part, (int)want, 0, 0);
#endif
            if (!mw)
                cut_part = -1;
        }
#if BOARD_SMK37
        else
            ui_wheel_mod(mw ? 3 : 0, 0, 0, 0, 0);
#endif
        mw_now = (int16_t)mw;
    }
    /* External MIDI bend / CC1 on a part's channel record like the wheels. */
    {
        int wn = 0, wv0 = 0, wl0 = 0, wv1 = 0, wl1 = 0;
        for (int v = 0; v < NBASS; ++v) {
            int t = engine_bass_ext_touch(v);
            if (t & 1) motion_touch(PR(R_ENG, T_303, v, engine_bass_is_wobble(v) ?
                                       ORIGINAL_MIAMI_PITCH_BEND : ORIGINAL_CHICAGO_PITCH_BEND));
            if (t & 2) {
                motion_touch(PR(R_ENG, T_303, v, engine_bass_mod_param(v)));
#if BOARD_SMK37
                if (wn == 0) { wv0 = v; wl0 = engine_bass_mod_ctl(v); }
                else if (wn == 1) { wv1 = v; wl1 = engine_bass_mod_ctl(v); }
                wn++;
#endif
            }
        }
#if BOARD_SMK37
        if (wn)
            ui_wheel_mod(wn > 2 ? 2 : wn, wv0, wl0, wv1, wl1);
#endif
    }
#if X0X_BASS_PRESET_PREVIEW
    engine_bass_wheel_owners((uint8_t)((bend_part >= 0 && bend_part < NBASS ? 1u << bend_part : 0u) |
                                      (cut_part >= 0 ? 1u << cut_part : 0u)));
    engine_bass_edit_hold(0);
#endif
}
