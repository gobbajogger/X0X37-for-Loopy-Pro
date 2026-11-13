static void smk_wheels(void)
{
    enum { DEAD = 12, SPAN0 = 80, MOD_LO = 485, MOD_HI = 615 };
    static int32_t pc, plo, phi, mlo, mhi, rest;
    static int8_t bend_part = -1, cut_part = -1;
    static int16_t bend_now, mw_now;
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
    }
    if (p < plo)
        plo = p;
    if (p > phi)
        phi = p;
    if (m < mlo)
        mlo = m;
    if (m > mhi)
        mhi = m;
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
#if X0X_ORIGINAL_SYNTH
            int param = engine_bass_is_wobble(bend_part) ?
                ORIGINAL_MIAMI_PITCH_BEND : ORIGINAL_CHICAGO_PITCH_BEND;
            if (bend)
                engine_set(T_303, bend_part, param, ((bend + 63) * 127 + 63) / 126);
            else
                engine_set_base(T_303, bend_part, param); /* exact factory value, including 0.5 */
#else
            int param = CHI_PBEND;
#if X0X_DEV_WOBBLE && !X0X_ORIGINAL_SYNTH
            if (engine_bass_is_wobble(bend_part))
                param = W_PITCH_BEND;
#endif
            /* A held gesture belongs to its original voice across page navigation. */
            want = bend ? chi_pbend_from_wheel((int)bend)
                        : proj.sound.v[T_303][bend_part][param];
            engine_set(T_303, bend_part, param, (int)want);
#endif
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
#if X0X_ORIGINAL_SYNTH
            engine_set(T_303, cut_part, engine_bass_is_wobble(cut_part) ?
                ORIGINAL_MIAMI_WOBBLE_DEPTH : ORIGINAL_CHICAGO_CUTOFF, (int)want);
#else
#if X0X_DEV_WOBBLE && !X0X_ORIGINAL_SYNTH
            if (engine_bass_is_wobble(cut_part))
                engine_set(T_303, cut_part, W_WOBBLE_DEPTH, (int)want);
            else
#endif
                engine_set(T_303, cut_part, BASS303_CUTOFF, (int)want);
#endif
            if (!mw)
                cut_part = -1;
        }
        mw_now = (int16_t)mw;
    }
#if X0X_BASS_PRESET_PREVIEW
    engine_bass_wheel_owners((uint8_t)((bend_part >= 0 && bend_part < NBASS ? 1u << bend_part : 0u) |
                                      (cut_part >= 0 ? 1u << cut_part : 0u)));
    engine_bass_edit_hold(0);
#endif
}
