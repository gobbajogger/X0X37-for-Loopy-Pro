/* SPDX-License-Identifier: MPL-2.0
 * Exhaustive single-byte mutation comparison for public, non-secret UUIDs. */
#define main original_subscription_contract_main
#include "ctl_vendor_subscription_test.c"
#undef main
int main(void) {
    const u8 examples[4][16]={
      {0x03,0xb8,0x0e,0x5a,0xed,0xe8,0x4b,0x33,0xa7,0x51,0x6c,0xe3,0x4e,0xc4,0xc7,0x00},
      {0x77,0x72,0xe5,0xdb,0x38,0x68,0x41,0x12,0xa1,0xa9,0xf2,0x66,0x9d,0x10,0x6b,0xf3},
      {0}, {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0}
    };
    unsigned cases=0;
    for(unsigned e=0;e<4;++e)for(unsigned reversed=0;reversed<2;++reversed)
      for(unsigned at=0;at<16;++at)for(unsigned v=0;v<256;++v) {
        u8 value[16];for(unsigned i=0;i<16;++i)value[i]=examples[e][reversed?15-i:i];
        value[at]=(u8)v;
        int forward=1,reverse=1;
        for(unsigned i=0;i<16;++i){if(value[i]!=examples[e][i])forward=0;if(value[i]!=examples[e][15-i])reverse=0;}
        CHECK(uuid_equal(value,examples[e])==(forward||reverse));++cases;
      }
    if(failures)return 1;
    printf("UUID equivalence PASS: %u canonical/reverse and changed-byte cases, including zero/palindromic UUIDs\n",cases);
    return 0;
}
