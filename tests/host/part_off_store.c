/* SPDX-License-Identifier: GPL-3.0-only */
/* M7a storage across firmware: the whole host app (x0x_host.c flash emulation), built inside a given tree
 * (the base 4b240bd = older firmware, or this one), through project_load, project_save, the UI's
 * save_project and autosave. tests/host/run_part_off.sh compares the dumps (sha256) and prints the cases.
 *   part_off_store write <mask> <out>        a project (bpm 133.0, 303A step 1 note 77), saved with <mask> OFF
 *   part_off_store boot <in> <out>           boot on <in>; then SAVE, then AUTOSAVE (an edit); <out> = the store
 *   part_off_store setmask <in> <mask> <out> boot on <in>, set <mask>, SAVE
 * Lines: key=value. Without PROJ_SET_TAIL (older firmware) a mask other than 0 cannot be written. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main

static int dump(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    for (int o = 0; o < OBJ_NOBJ; o++) {
        int32_t n = store_len[o];
        fwrite(&n, 4, 1, f);
        if (n > 0) fwrite(store[o], 1, (size_t)n, f);
    }
    return fclose(f);
}
static int install(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    for (int o = 0; o < OBJ_NOBJ; o++) {
        int32_t n;
        if (fread(&n, 4, 1, f) != 1 || n > (int32_t)sizeof store[0]) { fclose(f); return -1; }
        store_len[o] = n;
        if (n > 0 && fread(store[o], 1, (size_t)n, f) != (size_t)n) { fclose(f); return -1; }
    }
    fclose(f);
    return 0;
}
static unsigned mask_now(void)
{
#ifdef PROJ_SET_TAIL
    return seq.part_off;
#else
    return 0;                                        /* older firmware: every part plays */
#endif
}
static int set_mask(unsigned m)
{
#ifdef PROJ_SET_TAIL
    part_off_live = (uint8_t)m; project_apply_io();
    return 0;
#else
    return m ? -1 : 0;
#endif
}
static void state(const char *tag, int rc)
{
    printf("%s.rc=%d\n%s.unsupported=%d\n", tag, rc, tag, project_storage_unsupported());
    printf("%s.user_project=%d\n", tag, (proj.set.bpm_x10 == 1330 || proj.set.bpm_x10 == 1331) && proj.pat[0].bass[0].step[0].note == 77);
    printf("%s.mask=%u\n%s.set_len=%d\n", tag, mask_now(), tag, store_len[OBJ_SET]);
}
int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "write")) {
        boot(); run_ms(100);
        proj.set.bpm_x10 = 1330; seq.bpm = 133.0f;
        proj.pat[0].bass[0].step[0].note = 77;
        if (set_mask((unsigned)strtoul(argv[2], 0, 0))) { printf("write.error=no part_off here\n"); return 2; }
        printf("write.save_rc=%d\n", project_save());
        state("write", 0);
        return dump(argv[3]) ? 2 : 0;
    }
    if (argc == 4 && !strcmp(argv[1], "boot")) {
        if (install(argv[2])) return 2;
        boot();
        int rc = project_load();                     /* the same read as boot's: nothing is written */
        if (rc) project_defaults();
        project_apply_io();
        uint32_t w0 = store_writes;
        run_ms(200);
        state("load", rc);
        printf("load.writes=%u\n", (unsigned)(store_writes - w0));
        w0 = store_writes;
        save_project(); run_ms(50);                  /* SAVE, as the UI's button */
        printf("save.writes=%u\nsave.failed=%d\n", (unsigned)(store_writes - w0), ui.save_failed);
        proj.set.bpm_x10 = proj.set.bpm_x10 == 1330 ? 1331 : proj.set.bpm_x10; /* an edit for the autosave */
        seq.bpm = proj.set.bpm_x10 / 10.0f;
        ui.dirty = 1; ui.act_t = plat_ms(); ui.saved_t = 0; save_wait = 0;
        w0 = store_writes;
        run_ms(30000);                               /* AUTOSAVE: stopped and idle */
        printf("autosave.attempted=%d\nautosave.writes=%u\n", ui.saved_t != 0, (unsigned)(store_writes - w0));
        return dump(argv[3]) ? 2 : 0;
    }
    if (argc == 5 && !strcmp(argv[1], "setmask")) {
        if (install(argv[2])) return 2;
        boot();
        int rc = project_load();
        project_apply_io();
        state("load", rc);
        if (set_mask((unsigned)strtoul(argv[3], 0, 0))) return 2;
        printf("setmask.save_rc=%d\n", project_save());
        return dump(argv[4]) ? 2 : 0;
    }
    fprintf(stderr, "usage: see the head of part_off_store.c\n");
    return 2;
}
