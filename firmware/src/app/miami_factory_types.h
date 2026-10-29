/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include <string.h>
#include "miami_selector_map.h"
/* Separate from factory_preset_t/user bank. Not wired to any existing caller. */
typedef struct { uint8_t schema,bank,nparams; char name[24]; uint8_t ordinal[4], value[18]; } miami_factory_typed_t;
/* Caller supplies current backend surface. No DSP/storage mutation; transactional output.
 * All 23 descriptors/defaults must be supplied; trailing cells stay zero. */
static inline int miami_factory_decode(const miami_factory_typed_t *p,const char *expected_name,
                                      const uint8_t max[23],const uint8_t def[23],uint8_t out[32])
{
    static const uint8_t ids[]={0,6,11,13};
    uint8_t cell[32]={0};
    if(!p||!expected_name||!max||!def||!out||p->schema!=1||p->bank!=1||p->nparams!=18 ||
       !p->name[0] || !memchr(p->name,0,24) || strcmp(p->name,expected_name))return -1;
    for(int i=0;i<23;i++){if(def[i]>max[i])return -1;cell[i]=def[i];}
    /* Require exact admitted product surface, not arbitrary permissive maxima. */
    for(int i=0;i<23;i++) {
        int want=i==0?2:(i==6||i==11)?3:i==17?1:127;
        if(max[i]!=want)return -1;
    }
    for(int i=0;i<4;i++)if(miami_selector_code(ids[i],p->ordinal[i])!=p->value[ids[i]])return -1;
    for(int i=0;i<18;i++){if(p->value[i]>max[i])return -1;cell[i]=p->value[i];}
    memcpy(out,cell,32);return 0;
}
/* Exact union of legacy/current EDITED predicates; existing identity/range/tag
 * gates must run first. This helper does not itself validate a saved tag. */
static inline int miami_factory_edited_valid(const uint8_t cell[32],const uint8_t old[32],
                                           const uint8_t current[32],int edited)
{
    if(!cell||!old||!current||(edited!=0&&edited!=1))return 0;
    return (!!memcmp(cell,old,32)==edited) || (!!memcmp(cell,current,32)==edited);
}
