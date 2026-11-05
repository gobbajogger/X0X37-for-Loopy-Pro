/* SPDX-License-Identifier: GPL-3.0-only */
/* In-place conversion of format-2 packed patterns to velocity-bearing steps. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/project.c"

int main(void)
{
    uint8_t *raw = (uint8_t *)&proj + RAW_OFF;
    pattern2_t old;
    arrange_t arr;
    int p;
    memset(raw, 0, RAW_LEN);
    memset(&arr, 0, sizeof arr);
    /* Check the whole overlapping move, including fields far from the start. */
    for (p = 0; p < (int)sizeof arr; p++)
        ((uint8_t *)&arr)[p] = (uint8_t)(p * 37u + 11u);
    arr.song.len = 17;
    arr.song.bar[16].pat[TRK_BASS0] = 29;
    for (p = 0; p < NPAT; p++) {
        memset(&old, 0, sizeof old);
        old.drum[0].hit[0][0] = (uint32_t)p + 1u;
        old.bass[0].step[0].note = (uint8_t)(36 + p);
        old.bass[0].step[0].flags = G_NOTE | BS_SLIDE;
        old.bass[0].step[1].note = 99; /* A TIE's stored pitch must survive migration too. */
        old.bass[0].step[1].flags = G_TIE;
        old.bass[0].step[15].note = 12;
        old.bass[0].step[15].flags = G_TIE | BS_SLIDE | BS_ACCENT;
        old.bass[0].len = 16;
        old.bass[0].gen.seed = (uint32_t)(9000 + p);
        old.brk.slice_mode = p & 1; old.brk.rsv = (uint8_t)(128 + p);
        old.brk.slice[15] = 8;
        old.swing = (uint8_t)p;
        memcpy(raw + (uint32_t)p * sizeof old, &old, sizeof old);
    }
    memcpy(raw + NPAT * sizeof(pattern2_t), &arr, sizeof arr);
    stream_from2(&proj);
    migrate_break_counts(&proj, 2);
    for (p = 0; p < NPAT; p++)
        if (proj.pat[p].drum[0].hit[0][0] != (uint32_t)p + 1u ||
            proj.pat[p].bass[0].step[0].note != 36 + p ||
            proj.pat[p].bass[0].step[0].flags != (G_NOTE | BS_SLIDE) ||
            proj.pat[p].bass[0].step[1].note != 99 ||
            proj.pat[p].bass[0].step[1].flags != G_TIE ||
            proj.pat[p].bass[0].step[15].note != 12 ||
            proj.pat[p].bass[0].step[15].flags != (G_TIE | BS_SLIDE | BS_ACCENT) ||
            bstep_velocity(&proj.pat[p].bass[0].step[0]) != BASS_VELOCITY_DEFAULT ||
            proj.pat[p].bass[0].gen.seed != (uint32_t)(9000 + p) ||
            proj.pat[p].brk.slice_mode != 0 || proj.pat[p].brk.rsv != 128 + p ||
            proj.pat[p].brk.slice[15] != 8 || proj.pat[p].swing != p) {
            printf("format-2 migration failed at pattern %d\n", p);
            return 1;
        }
    if (memcmp(&proj.arr, &arr, sizeof arr) != 0) {
        puts("format-2 arrangement migration failed");
        return 1;
    }
    puts("303 format-2 stream migration: ok");
    return 0;
}
