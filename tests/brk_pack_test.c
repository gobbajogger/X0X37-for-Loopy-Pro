/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include "app/brk_pack.h"

int main(int argc, char **argv)
{
    FILE *f;
    uint8_t *buf;
    long n;
    brk_pack_loop_t loops[BRK_PACK_MAX];
    int c, i;
    if (argc < 2)
        return 2;
    f = fopen(argv[1], "rb");
    if (!f)
        return 2;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    rewind(f);
    buf = malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n)
        return 2;
    fclose(f);
    c = brk_pack_parse(buf, (uint32_t)n, loops, BRK_PACK_MAX);
    printf("%d", c);
    for (i = 0; i < c; i++)
        printf("%s%s %u %u %u", i ? "," : " ", loops[i].name, loops[i].n, loops[i].bars, loops[i].nbytes);
    printf("\n");
    free(buf);
    return c > 0 ? 0 : 1;
}
