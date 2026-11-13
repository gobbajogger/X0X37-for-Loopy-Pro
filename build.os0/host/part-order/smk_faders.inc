static void smk_faders(void)
{
    static const char *const NAME[3] = {"REV RETURN", "DLY RETURN", "FILTER"};
    /* Bit 7 of last[] means waiting for pickup; the low bits hold the last raw reading. */
    /* held bit 1 marks an unknown first sample: startup may already exceed 200 frames. */
    static uint8_t last[4] = {0x80, 0x80, 0x80, 0x80}, held = 2u;
    uint32_t bank_changed = 0;
    uint32_t i, down = (fm1_in.buttons >> SMK_BTN_FADER_BANK) & 1u;
    if ((held & 2u) || ui_project_busy()) {
        for (i = 0; i < 4u; i++)
            last[i] = (uint8_t)(0x80u | fm1_in.fader[i]);
        smk_mute_known = smk_mute_active = smk_mute_edit = 0;
        held = (uint8_t)down;
        return;
    }
    if (down && !held) {                                /* FADER BANK: match the new targets before taking over */
        bank_changed = 1;
        smk_fbank ^= 1u;
        for (i = 0; i < 4u; i++)
            last[i] = (uint8_t)(0x80u | fm1_in.fader[i]);
        char names[28], *q = put_s(names, "FADERS: ");
        if (smk_fbank) put_s(put_s(q, part_s(part_at(4))), " REV DLY FILTER");
        else for (i = 0; i < 4u; i++) {
            if (i) q = put_s(q, " ");
            q = put_s(q, part_s(part_at((int)i)));
        }
        say(names, 0);                                  /* all short part labels fit 27 chars */
    }
    held = (uint8_t)down;
    smk_mute_bank = smk_fbank;
    for (i = 0; i < 4u; i++) {
        uint32_t v = fm1_in.fader[i];
        pref_t r;
        char t[28], num[16], unit[8], *q;
        uint32_t n = 0;
        if (fm1_in.frames < 200u) {                    /* the first readings settle */
            last[i] = (uint8_t)(0x80u | v);
            continue;
        }
        if (!smk_fbank)
            r = PR(R_ENG, T_MIX, part_at((int)i), MX_LEVEL);
        else if (i == 0)
            r = PR(R_ENG, T_MIX, part_at(4), MX_LEVEL);
        else if (i == 1)
            r = PR(R_ENG, T_FX, 0, FX_RV_LEVEL);
        else if (i == 2)
            r = PR(R_ENG, T_FX, 0, FX_DL_LEVEL);
        else
            r = PR(R_ENG, T_MST, 0, MST_DJF);
        uint8_t bit = (uint8_t)(1u << i);
        int level = pref_get(r), silent = volume_muted(r), mute_history = smk_mute_known & bit;
        /* Seed a new bank without inheriting the previous destination's mute
         * gesture. An external base edit ends this epoch's exception. */
        if (!(smk_mute_known & bit) || (!(smk_mute_active & bit) && silent)) smk_mute_edit |= bit;
        if (bank_changed || !silent || ((smk_mute_known & bit) && smk_mute_intended[i] != level))
            smk_mute_edit &= (uint8_t)~bit;
        smk_mute_known |= bit;
        if (silent) smk_mute_active |= bit;
        else smk_mute_active &= (uint8_t)~bit;
        smk_mute_intended[i] = (uint8_t)level;
        /* These eight fader destinations are all 0..127 pots. */
        /* A knob or loaded project can change a target while this bank stays selected. */
        if (!(last[i] & 0x80u) && pref_get(r) != (int)(last[i] & 127u))
            last[i] |= 0x80u;
        if (v == (last[i] & 127u))
            continue;
        if (last[i] & 0x80u) {
            uint32_t target = (uint32_t)pref_get(r);
            uint32_t prev = last[i] & 127u;
            uint32_t near = v + 2u >= target && v <= target + 2u;
            uint32_t crossed = (prev < target && v >= target) || (prev > target && v <= target);
            last[i] = (uint8_t)(0x80u | v);
            if (!near && !crossed && !(mute_history && (smk_mute_edit & bit))) {
                char hint[20], *p = put_s(hint, "PICK F");
                p = put_i(p, (int)i + 1);
                put_s(p, v < target ? " UP" : " DOWN");
                say(hint, 0);                         /* draw_header keeps pattern and tempo visible */
                continue;
            }
        }
        last[i] = (uint8_t)v;
        pref_set(r, (int)v);
        smk_mute_intended[i] = (uint8_t)v;
        motion_touch(r);
        ui.act_t = plat_ms();                            /* (the autosave waits for quiet) */
        pref_value(r, num, unit);
        q = r.a == T_MIX ? put_s(put_s(t, part_s(r.b)), " LEVEL ") : put_s(put_s(t, NAME[i - 1]), " ");
        while (n < 16u && num[n])
            n++;
        if (n < 8u)                                      /* NAME (<= 11) + " " + num (< 8) + unit (< 8): fits say() */
            put_s(put_s(q, num), unit);
        say(t, 0);
    }
}
