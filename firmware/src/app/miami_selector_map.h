/* SPDX-License-Identifier: GPL-3.0-only */
/* Pure product storage <-> original display ordinal mapping; not DSP parity. */
#pragma once
#include <stdint.h>
static inline int miami_selector_count(int id)
{ return id==0?3:(id==6 || id==11)?4:id==13?8:0; }
static inline int miami_selector_code(int id,int ordinal)
{
    static const uint8_t carrier[]={0,2,1}, other[]={0,2,3,1}, rate[]={0,2,4,6,1,3,5,7};
    if(ordinal<0 || ordinal>=miami_selector_count(id))return -1;
    if(id==0)return carrier[ordinal];
    if(id==13)return (rate[ordinal]*127+3)/7;
    return other[ordinal];
}
static inline int miami_selector_ordinal(int id,int code)
{
    for(int i=0;i<miami_selector_count(id);i++)if(miami_selector_code(id,i)==code)return i;
    return -1;
}
/* Raw UI event: normalize to ONE signed detent, zero is no-op, clamp endpoints.
 * RATE deliberately excluded: existing rate storage/pad semantics are unchanged. */
static inline int miami_selector_event(int id,int code,int delta,int *out)
{
    int i=miami_selector_ordinal(id,code), n=miami_selector_count(id);
    if(!out || id==13 || i<0)return -1;
    if(delta>0 && i<n-1)i++;
    if(delta<0 && i>0)i--;
    *out=miami_selector_code(id,i);return 0;
}
