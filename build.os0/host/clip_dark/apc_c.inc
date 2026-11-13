static void smk_pads(void)
{
    uint32_t logical, w;
    if (clip_grid_mode) {
        for (logical = 0; logical < SMK_APC40_PAD_COUNT; logical++) {
            uint8_t note = smk_apc40_note_for_pad_view(apc_cols, apc_rows, apc_origin_col,
                                                       apc_origin_row, apc_view, (uint8_t)logical);
            uint8_t v = note < SMK_APC40_CLIP_COUNT ? apc_led_velocity[note] : 0u;
            uint8_t channel = note < SMK_APC40_CLIP_COUNT ? apc_led_channel[note] : 0u;
            const uint8_t *rgb = APC40_PAD_OWNER[v];   /* the last color from Loopy, owner-calibrated */
            uint8_t white = smk_apc40_physical_pad_for_logical((uint8_t)logical);
            uint32_t pad = SMK_PAD_OF_WHITE(white);
            uint8_t queued = note < SMK_APC40_CLIP_COUNT &&
                             (apc_cell_flags[note] & SMK_APC40_CELL_QUEUED) ? 1u : 0u;
            uint8_t brightness = v ? smk_apc_led_brightness(channel, queued) : 0u;
            /* Loopy row zero maps to the SMK's lower physical pad row. */
            if (v) padled_set(pad, smk_clip_pad_level(rgb[0], v, brightness), smk_clip_pad_level(rgb[1], v, brightness),
                              smk_clip_pad_level(rgb[2], v, brightness));   /* level / 2, idle shade full level */
            else padled_set(pad, 0, 0, 0);
        }
        padled_flush();
        return;
    }
}
