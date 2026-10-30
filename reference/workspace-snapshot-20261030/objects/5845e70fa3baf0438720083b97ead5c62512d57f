/* SPDX-License-Identifier: GPL-3.0-only */
/* Build against old/current production storage; successful traces must produce
 * exactly identical NOR bytes, including header CRCs and reserved bytes. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/app/plat.h"
static uint8_t nor[0x100000], src[3840], dst[3840];
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 255, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    assert((off & 255) + n <= 256);
    for (unsigned i = 0; i < n; i++) nor[off+i] &= ((const uint8_t *)src)[i];
    return 0;
}
#ifndef STORAGE_IMPL
#define STORAGE_IMPL "../firmware/src/storage.c"
#endif
#include STORAGE_IMPL
int main(int argc, char **argv)
{
    static const unsigned len[] = {0,1,255,256,257,3839,3840};
    memset(nor, 255, sizeof nor);
    for (unsigned round = 0; round < 9; round++) for (unsigned obj = 0; obj < OBJ_NOBJ; obj++)
        for (unsigned k = 0; k < sizeof len / sizeof *len; k++) {
            for (unsigned i = 0; i < sizeof src; i++) src[i] = (uint8_t)(i * 73 + (i >> 4) + obj * 29 + round);
#ifdef STORAGE_SHARED
            memcpy(st_buf, src, sizeof src);
            assert(st_save(obj, st_buf, len[k]) == 0);
            assert(!memcmp(st_buf, src, sizeof src));
#else
            assert(st_save(obj, src, len[k]) == 0);
#endif
            assert(st_load(obj, dst, sizeof dst) == (int)len[k]);
            assert(!memcmp(dst, src, len[k]));
        }
    assert(argc == 2);
    FILE *f = fopen(argv[1], "wb"); assert(f);
    assert(fwrite(nor, 1, sizeof nor, f) == sizeof nor); assert(!fclose(f));
    puts("441 successful interleaved object saves/loads PASS");
}
