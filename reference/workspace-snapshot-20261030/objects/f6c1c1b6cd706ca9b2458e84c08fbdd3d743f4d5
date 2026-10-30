/* Headless preview of the GLOBAL > ABOUT X0X row.
 * Build: tests/host/release_label_preview.sh
 * The row keeps its action and arrow. The open-beta label sits between them. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main

#include <stdio.h>

static int bright_at(int x, int y)
{
    uint16_t c, r, g, b;
    if ((unsigned)x >= 240u || (unsigned)y >= 240u)
        return 0;
    c = (uint16_t)((fb[y * 240 + x] >> 8) | (fb[y * 240 + x] << 8));
    r = (uint16_t)(c >> 11);
    g = (uint16_t)((c >> 5) & 63u);
    b = (uint16_t)(c & 31u);
    (void)r;
    (void)b;
    return g >= 28;
}

static int bright_columns(int x0, int x1, int y0, int y1)
{
    int x, y, n = 0;
    for (x = x0; x < x1; x++)
        for (y = y0; y < y1; y++)
            if (bright_at(x, y))
                n++;
    return n;
}

int main(int argc, char **argv)
{
    int about = -1, i;
    int32_t shown_x, shown_w, long_x, long_w, name_w;
    const char *long_label = "99.99.99_openbeta";
    const char *shown = "0.1.3_openbeta";
    if (argc != 2) {
        fprintf(stderr, "usage: release_label_preview OUT.png\n");
        return 2;
    }
    memset(&ui, 0, sizeof ui);
    boot();
    open_global();
    for (i = 0; i < ui.list_n; i++)
        if (list_rows[i].kind == R_ACT && list_rows[i].a == ACT_ABOUT)
            about = i;
    if (about < 1 || list_rows[about - 1].kind != R_ACT || list_rows[about - 1].a != ACT_PERF)
        return 3;
    ui.list_sel = ui.list_top = (uint8_t)about;
    if (!release_label_box(shown, 1, &shown_x, &shown_w))
        return 4;
    if (!release_label_box(long_label, 1, &long_x, &long_w))
        return 5;
    if (release_label_box("", 1, &shown_x, &shown_w))
        return 6;
    if (release_label_box("0.1.3_openbeta_openbeta_openbeta", 1, &long_x, &long_w))
        return 7;
    if (!release_label_box(shown, 1, &shown_x, &shown_w))
        return 4;
    name_w = tw(&FONT_B, "ABOUT X0X");
    if (shown_x < 8 + name_w + 4 || shown_x + shown_w > 206)
        return 8;
    if (long_x < 8 + name_w + 4 || long_x + long_w > 206)
        return 8;
    draw_main();
    png_write(argv[1]);
    /* Selected ABOUT row is the first list row, so its arrow is on this band. */
    if (bright_columns(8, 8 + name_w, MAIN_Y, MAIN_Y + 22) < 8)
        return 9;
    if (bright_columns(8 + name_w, (int)shown_x, MAIN_Y, MAIN_Y + 22) > 2)
        return 10;
    if (bright_columns((int)shown_x, (int)(shown_x + shown_w), MAIN_Y, MAIN_Y + 22) < 8)
        return 11;
    if (bright_columns(212, 224, MAIN_Y, MAIN_Y + 22) < 4)
        return 12;
    printf("preview %s label x=%d w=%d\n", argv[1], (int)shown_x, (int)shown_w);
    return 0;
}
