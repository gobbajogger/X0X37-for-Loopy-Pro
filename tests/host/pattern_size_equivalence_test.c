/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare separately compiled -O2 and -Oz pattern initialization, not firmware. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pattern.h"
void pattern_init_size_probe(pattern_t *, uint32_t, uint32_t);
int main(void)
{
    pattern_t reference, smaller;
    uint32_t seed=0xBADC0DEu;
    for(unsigned i=0;i<4096u;++i) {
        seed=seed*1664525u+1013904223u;
        memset(&reference,0xA5,sizeof reference);memset(&smaller,0x5A,sizeof smaller);
        pattern_init(&reference,seed,seed^0xFFFFFFFFu);
        pattern_init_size_probe(&smaller,seed,seed^0xFFFFFFFFu);
        assert(!memcmp(&reference,&smaller,sizeof reference));
    }
    puts("PASS: 4096 seed pairs, pattern defaults byte-identical at host -O2/-Oz");
    return 0;
}
