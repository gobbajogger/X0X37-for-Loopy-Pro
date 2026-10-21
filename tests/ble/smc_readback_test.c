/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "../../firmware/hal/x0x_smc_readback.h"
int main(void)
{
    const uint32_t bases[]={0x84000,0xAB000,0xAD000,0xFE000};
    const uint32_t sizes[]={0x17000,0x2000,0x2000,0x1000};
    uint32_t address=0,length=0;
    uint8_t a[5]={0};
    for (unsigned r=0;r<4;r++) {
        a[0]=(uint8_t)r;
        for (uint32_t off=0;off<=sizes[r]+1;off++) {
            a[1]=off&127;a[2]=(off>>7)&127;a[3]=(off>>14)&127;
            for (unsigned count=0;count<128;count+=count?15:1) {
                a[4]=(uint8_t)count;
                int ok=xsr_request(a,5,&address,&length);
                assert(ok==(off<sizes[r] && count!=0));
                if(ok) {
                    assert(address==bases[r]+off && length && length<=224);
                    assert(length<=sizes[r]-off);
                }
            }
        }
    }
    a[0]=4;a[1]=a[2]=a[3]=0;a[4]=1;
    assert(!xsr_request(a,5,&address,&length));a[0]=0;
    for(unsigned i=0;i<5;i++){a[i]|=128;assert(!xsr_request(a,5,&address,&length));a[i]&=127;}
    assert(!xsr_request(a,4,&address,&length));assert(!xsr_request(a,6,&address,&length));
    assert(!xsr_request(0,5,&address,&length));
    puts("SMC readback: all region offsets, clipping, invalid selectors/counts/7-bit/length PASS");
}
