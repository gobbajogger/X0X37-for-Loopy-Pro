static const char CK_FLD_N[] = "ROOT\0SCALE\0SNAP\0CHORD\0INV\0STRUM\0ORDER\0RATE\0OCT\0GATE\0SWING\0HUMAN\0RHYTHM\0LATCH\0ON";
static uint32_t ck_page_on(void)
{
    return (ck.pg & 3u) != 0u;
}
int smk_ck_smc_turn(int n, int delta)
{
    uint32_t pg, f;
    int v;
    char name[20], value[12];
    if (!clip_grid_mode)
        return 0;
    if (!ck_page_on())
        return 1;                       /* the grid hides the X0X parameter page */
    pg = (ck.pg & 3u) - 1u;
    if (n < 0 || n >= (pg == 2u ? 8 : 4))
        return 1;
    f = CK_PAGE_FIELDS[pg][n];
    v = (int)ck_get(f) + delta;
    v = v < 0 ? 0 : v > CK_FMAX[f] ? CK_FMAX[f] : v;
    if ((uint32_t)v != ck_get(f)) {
        ck_set(f, (uint32_t)v);
        mark_dirty();
    }
    put_s(name, ck_nm(CK_FLD_N, f));
    smc_ui_clip_show(n, name, ck_val_txt(f, ck_get(f), value));
    return 1;
}
int smk_ck_smc_nav(uint8_t note)
{
    int d = (note & 1u) ? 1 : -1;
    uint32_t pg = ck.pg & 3u, f;
    if (!clip_grid_mode || !((note >= 0x60 && note <= 0x63) ||
        note == 0x2E || note == 0x2F || note == 0x30 || note == 0x31 || note == 6))
        return 0;
    smc_ui_cancel();
    if (note == 6)
        ck.pg = 0;
    else if (note == 0x60 || note == 0x61)
        ck.pg = (uint8_t)(((int)pg + d + 4) % 4 | CK_E_ON << 4);
    else if (pg) {
        f = ck.pg >> 4;
        if (note == 0x2E || note == 0x2F || note == 0x30 || note == 0x31)
            ck.pg = (uint8_t)(pg | ck_page_field(pg, f, d) << 4);
        else {
            uint32_t v = ck_step(ck_get(f), d, CK_FMAX[f]);
            if (v != ck_get(f)) { ck_set(f, v); mark_dirty(); }
        }
    }
    ui.act_t = plat_ms();
    return 1;
}
