CK_FN void smk_ck_poll(void)
{
    CK_SET;
    CK_BASE;
    uint32_t now = fm1_ms, k, on = clip_grid_mode, ble = plat_ble_state() == PLAT_BLE_CONNECTED ? CK_F_BLE : 0u;
    uint32_t dt = (uint16_t)(now - cp->last_ms);
    cp->last_ms = (uint16_t)now;
    if (ui_project_busy()) {
        cp->keys[0] = ((const volatile uint32_t *)&fm1_in.keys)[0];
        cp->keys[1] = ((const volatile uint32_t *)&fm1_in.keys)[1] & 0x1Fu;
        cp->btn_prev = 0x7Fu;
        cp->ext_seen = (uint8_t)apc_clock_count;
        ck_start_req = 0;
        return;
    }
    if (cp->off_req || (cp->fl & ~ble & CK_F_BLE)) {          /* MIDI Stop, USB reset, Bluetooth drop: all notes off */
        cp->off_req = 0;
        ck_all_off();
    }
    cp->fl = (uint8_t)((cp->fl & ~CK_F_BLE) | ble);
    if (!on) {
        if (cp->fl & CK_F_ACTIVE) {                          /* clip mode exit: all notes off, the modes off */
            ck_all_off();
            cp->on = cp->pg = 0;
        }
        cp->fl = (uint8_t)(ble | CK_F_NRUSED);               /* (LATCH off) */
        cp->ext_seen = (uint8_t)apc_clock_count;
        cp->btn_prev = 0x7Fu;                                /* a button held at entry acts only after its release */
    } else {
        if (smk_ck_keybed_held())
            ui.act_t = now;                                 /* a held clip key keeps autosave quiet */
        if (!(cp->fl & CK_F_ACTIVE)) {
            cp->fl |= CK_F_ACTIVE;
            was_on = 0u;
        }
        if (cp->ch != sp->ck_ch - 1u) {                     /* a new CLIP KEYS CH: all notes off */
            ck_all_off();                                   /* (on the old channel) */
            cp->ch = (uint8_t)(sp->ck_ch - 1u);
        }
        ck_controls();
    }
    for (k = 0; k < CK_NKEY; k++) {                         /* (out of clip mode: keys held at entry do not play) */
        uint32_t bit = 1u << (k & 31u), *pk = &cp->keys[k >> 5];
        uint32_t d = ((const volatile uint32_t *)&fm1_in.keys)[k >> 5] & bit;
        int w;
        if (d == (*pk & bit))
            continue;
        *pk ^= bit;
        if (!on)
            continue;
        ui.act_t = now;                                     /* a release also starts the quiet interval */
        if (!d) {
            ck_release(k);
            continue;
        }
        w = k >= SMK_KEY_F3 && k < SMK_KEY_F3 + NKEYS ? KEY_WHITE[k - SMK_KEY_F3] : -1;
        if (w >= 0 && ((fm1_in.pnotes >> (k - SMK_KEY_F3)) & 1u))   /* a held pad blocks its white key */
            continue;
        ck_press(k, fm1_in.key_vel[k]);                    /* the page never steals keybed notes */
    }
    if (on) {
        ck_arp(dt);
        ck_q_run(0);                                        /* HUMAN: the note-ons that are due */
    }
}
