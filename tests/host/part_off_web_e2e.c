/* SPDX-License-Identifier: GPL-3.0-only */
/* M7a end to end, firmware -> web: the whole host app exports the .x0x inner project file (header from the
 * real project_file_header(), word 28 = PROJ_FILE_OFF_WORD from x0x.h, payload = project_stage) for every
 * part OFF mask 0..31, into <dir>/mask<m>.x0xp. tests/host/part_off_web_e2e.mjs feeds those bytes to the
 * web parseProject(). Test only; no production code. Build/run: tests/host/run_part_off_web_e2e.sh. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s <outdir>\n", argv[0]); return 2; }
    boot();
    run_ms(300);
    for (unsigned m = 0; m < 32; m++) {
        uint8_t h[PROJECT_FILE_HDR];
        char path[1024];
        part_off_live = (uint8_t)m;
        project_apply_io();
        project_capture_stage();
        project_file_header(h);
        snprintf(path, sizeof path, "%s/mask%u.x0xp", argv[1], m);
        FILE *f = fopen(path, "wb");
        if (!f) { perror(path); return 2; }
        if (fwrite(h, 1, sizeof h, f) != sizeof h ||
            fwrite(&project_stage, 1, sizeof project_stage, f) != sizeof project_stage) { fclose(f); return 2; }
        fclose(f);
        printf("mask %2u word28 %08x staged %02x\n", m, (unsigned)get32(h + 28), part_off_staged);
    }
    return 0;
}
