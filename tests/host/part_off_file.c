/* SPDX-License-Identifier: GPL-3.0-only */
/* M7a .x0x files across firmware: the whole host app built inside a given tree (the base c32c54ac = older
 * firmware, or this one), through the editor's own file steps (editor.c EXPORT BEGIN :271-273: capture the
 * stage, then project_file_header; COMMIT/VALIDATE :316-321: project_file_validate, project_file_header,
 * then the commit). tests/host/run_part_off.sh compares the files (sha256) and prints the cases.
 *   part_off_file export <mask> <out>     a project (bpm 133.0, 303A step 1 note 77) with <mask> OFF, exported
 *   part_off_file import <in> <out>       import <in>; <out> = the export right after (an old export)
 * Lines: key=value. Without PROJ_SET_TAIL (older firmware) a mask other than 0 cannot be set. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main

static unsigned mask_now(void)
{
#ifdef PROJ_SET_TAIL
    return seq.part_off;
#else
    return 0;
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
static int export_to(const char *path)
{
    uint8_t h[PROJECT_FILE_HDR];
    project_capture_stage();
    project_file_header(h);
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(h, 1, sizeof h, f);
    fwrite(&project_stage, 1, sizeof project_stage, f);
    return fclose(f);
}
int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "export")) {
        boot(); run_ms(100);
        proj.set.bpm_x10 = 1330; seq.bpm = 133.0f;
        proj.pat[0].bass[0].step[0].note = 77;
        if (set_mask((unsigned)strtoul(argv[2], 0, 0))) { printf("export.error=no part_off here\n"); return 2; }
        printf("export.size=%u\n", (unsigned)PROJECT_FILE_SIZE);
        return export_to(argv[3]) ? 2 : 0;
    }
    if (argc == 4 && !strcmp(argv[1], "import")) {
        static uint8_t buf[PROJECT_FILE_SIZE + 64];
        uint8_t h[PROJECT_FILE_HDR];
        FILE *f = fopen(argv[2], "rb");
        if (!f) return 2;
        size_t n = fread(buf, 1, sizeof buf, f);
        fclose(f);
        boot(); run_ms(100);
        set_mask(0);
        uint32_t w0 = store_writes;
        printf("import.size=%u\nimport.size_ok=%d\n", (unsigned)n, n == PROJECT_FILE_SIZE); /* editor.c:256 */
        int rc = -100;
        if (n == PROJECT_FILE_SIZE && !ui_project_begin()) {   /* editor.c:262 IMPORT BEGIN */
            memcpy(h, buf, sizeof h);
            memcpy(&project_stage, buf + sizeof h, sizeof project_stage);
            rc = project_file_validate(h);                   /* editor.c:316 */
            if (!rc) project_file_header(h);                 /* editor.c:320 */
            ui_project_end(rc == 0);                         /* editor.c:322 COMMIT -> ed_project_end(1) */
            run_ms(50);
        }
        printf("import.rc=%d\nimport.mask=%u\nimport.writes=%u\n", rc, mask_now(), (unsigned)(store_writes - w0));
        printf("import.user_project=%d\n", proj.set.bpm_x10 == 1330 && proj.pat[0].bass[0].step[0].note == 77);
        return export_to(argv[3]) ? 2 : 0;
    }
    fprintf(stderr, "usage: see the head of part_off_file.c\n");
    return 2;
}
