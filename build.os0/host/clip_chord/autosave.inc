#define AUTOSAVE_QUIET 4000u
/* AUTOSAVE: changes are saved by themselves while the pattern is stopped and nothing has been touched
 * for AUTOSAVE_QUIET ms, at most every AUTOSAVE_GAP ms. Stopped only: a flash erase silences the
 * audio. Only the objects that changed are written (project.c), usually one or two sectors. */
#define AUTOSAVE_GAP 20000u
static void autosave(void)
{
    uint32_t now = plat_ms();
#if BOARD_SMK37
    extern uint8_t smk_ck_flash_busy(void), smk_clip_mode_on(void);
#endif
    if (project_busy || ui_boot_flash_busy())
        return;
    if (save_wait && !seq.playing && !seq.req_start && !seq.req_cont && !seq.req_stop && !ui.rec && !perf_save_busy()) {
#if BOARD_SMK37
        if (smk_clip_mode_on() && (smk_ck_flash_busy() || now - ui.act_t < AUTOSAVE_QUIET))
            return;                                    /* keep the queued SAVE until clip playing has gone quiet */
#endif
        save_project();
        return;
    }
#if BOARD_SMK37
    if (smk_ck_flash_busy())                          /* ui_frame runs before smk_ck_poll on display frames */
        return;
#endif
    if (!ui.dirty || (proj.set.autosave_off && !ui.last_autosave) || seq.playing || seq.req_start || seq.req_cont || seq.req_stop || ui.rec || perf_testing() ||
        ui.overlay == O_ASK || now - ui.act_t < AUTOSAVE_QUIET || (ui.saved_t && now - ui.saved_t < AUTOSAVE_GAP))
        return;
    ui.saved_t = now;
    ui.last_autosave = 0;
    switch (project_save()) {
    case 0:
        ui.dirty = 0;
        ui.save_failed = 0;
        say("AUTOSAVED", 0);
        break;
    case -9:
        ui.save_failed = 1;
        say("NEWER DATA: NOT SAVED", 0);
        break;
    case -7:
        ui.save_failed = 1;
        say("MEMORY FULL: NOT SAVED", 0);
        break;
    default:
        ui.save_failed = 1;
        say("SAVE FAILED", 0);
        break;
    }
}

