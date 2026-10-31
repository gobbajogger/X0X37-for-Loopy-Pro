/* SPDX-License-Identifier: GPL-3.0-only */
#include "../../firmware/src/app/original_sound.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned factories, edits, failures, performance_refusals, stale_restores;
/* 0..6: factory-admitted types; 7..11: archived inserts reachable by edit only. */
static const unsigned fx_types[12] = {0,4,5,10,18,22,24,3,12,13,15,16};
static const unsigned fx_macro[12] = {0,5,7,4,2,3,1,6,8,9,10,11};
static float edited_value(unsigned kind, unsigned parameter, unsigned value)
{
    if ((kind==ORIGINAL_SOUND_CHICAGO && parameter==4) ||
        (kind==ORIGINAL_SOUND_MIAMI && parameter==7)) {
        /* Independent precision model: double division followed by an explicit
         * binary32 rounding boundary, then the exactly representable half. */
        if(value<64)return (float)((double)value/128.0);
        volatile float delta=(float)((double)(value-64)/126.0);
        return (float)(0.5+(double)delta);
    }
    return kind == ORIGINAL_SOUND_CHICAGO && parameter == 17 ?
        (float)fx_macro[value] / 11.0f : (float)value / 127.0f;
}
static void compare(unsigned kind, unsigned factory)
{
    uint8_t cell[32], saved[32];
    assert(original_sound_init(cell, kind, factory));
    assert(original_sound_valid(cell) && !original_sound_edit_mask(cell));
    assert(cell[26]=='O' && cell[27]=='S' && cell[28]==2 && cell[29]==kind && cell[30]==factory && !cell[31]);
    assert(original_sound_name(kind,factory) && original_sound_engine_id(kind));
    memcpy(saved,cell,sizeof saved);
    original_miami_program m, mr;
    original_chicago_program c, cr;
    if (kind==ORIGINAL_SOUND_MIAMI) {
        assert(original_miami_program_init(&mr,factory));
        assert(original_sound_restore_miami(&m,cell) && !memcmp(&m,&mr,sizeof m));
    } else {
        int raw = original_sound_factory_source(kind,factory);
        assert(raw >= 0 && raw < 81);
        assert(original_chicago_program_init(&cr,(unsigned)raw));
        assert(original_sound_restore_chicago(&c,cell) && !memcmp(&c,&cr,sizeof c));
        original_chicago_routing routing;
        assert(original_chicago_program_routing(&c,&routing));
        assert(cell[17] < 7 && routing.fx[1][0] == fx_types[cell[17]]);
    }
    unsigned n=original_sound_count(kind);
    unsigned centered=kind==ORIGINAL_SOUND_MIAMI?7:4;
    double factory_center=kind==ORIGINAL_SOUND_MIAMI?mr.macro[centered]:cr.macro[centered];
    unsigned display=(unsigned)((factory_center<.5?128.0*factory_center:64.0+126.0*(factory_center-.5))+.5);
    assert(cell[centered]==display);
    for (unsigned parameter=0;parameter<n;++parameter) {
        float untouched=-1, expected=kind==ORIGINAL_SOUND_MIAMI?mr.macro[parameter]:cr.macro[parameter];
        assert(original_sound_value(cell,parameter,&untouched) && !memcmp(&untouched,&expected,sizeof expected));
        const x0x_param_t *metadata=original_sound_param(kind,parameter);
        assert(metadata && metadata->max==(kind==ORIGINAL_SOUND_CHICAGO && parameter==17?11:127) && strlen(metadata->name)<=6);
        for (unsigned value=0;value<=metadata->max;++value) {
            memcpy(cell,saved,sizeof cell);
            if (parameter >= n-2) {
                assert(original_sound_is_performance(kind,parameter));
                assert(!original_sound_set(cell,parameter,value) && !memcmp(cell,saved,sizeof cell));
                performance_refusals++;
                /* Valid historical schema2 cell, including a nonneutral
                 * performance byte. Readers must ignore it without mutation. */
                cell[parameter]=(uint8_t)value;
                uint32_t stale=UINT32_C(1)<<parameter;
                for(unsigned j=0;j<4;j++)cell[22+j]=(uint8_t)(stale>>(8*j));
                uint8_t historical[32];memcpy(historical,cell,sizeof historical);
                assert(original_sound_valid(cell));
                float neutral=parameter==n-2?.5f:0.0f,restored=-1,live=-1;
                assert(original_sound_value(cell,parameter,&restored) && !memcmp(&restored,&neutral,4));
                assert(original_sound_control_value(kind,parameter,value,&live));
                float normalized=(float)value/127.0f;assert(!memcmp(&live,&normalized,4));
                if(kind==ORIGINAL_SOUND_MIAMI)
                    assert(original_sound_restore_miami(&m,cell) && !memcmp(&m,&mr,sizeof m));
                else assert(original_sound_restore_chicago(&c,cell) && !memcmp(&c,&cr,sizeof c));
                assert(!memcmp(cell,historical,sizeof cell));
                stale_restores++;
                continue;
            }
            assert(!original_sound_is_performance(kind,parameter));
            assert(original_sound_set(cell,parameter,value));
            assert(original_sound_edit_mask(cell)==(UINT32_C(1)<<parameter));
            float normalized=edited_value(kind,parameter,value), restored=-1;
            assert(original_sound_value(cell,parameter,&restored) && !memcmp(&restored,&normalized,4));
            if (kind==ORIGINAL_SOUND_MIAMI) {
                original_miami_program expected_program=mr;
                assert(original_miami_program_set(&expected_program,parameter,normalized));
                assert(original_sound_restore_miami(&m,cell) && !memcmp(&m,&expected_program,sizeof m));
            } else {
                original_chicago_program expected_program=cr;
                assert(original_chicago_program_set(&expected_program,parameter,normalized));
                assert(original_sound_restore_chicago(&c,cell) && !memcmp(&c,&expected_program,sizeof c));
                if (parameter==17) {
                    original_chicago_routing routing;
                    assert(original_chicago_program_routing(&c,&routing) && routing.fx[1][0]==fx_types[value]);
                    assert(metadata->names && !strcmp(metadata->names[value],original_sound_fx_name(value)));
                }
            }
            edits++;
        }
        memcpy(cell,saved,sizeof cell);
    }
    /* Historical performance edits may coexist with ordinary patch edits.
     * Restore the latter exactly, while retaining every saved input byte. */
    assert(original_sound_set(cell,1,41));
    cell[n-2]=20;cell[n-1]=127;
    uint32_t mixed=original_sound_edit_mask(cell)|(UINT32_C(3)<<(n-2));
    for(unsigned j=0;j<4;j++)cell[22+j]=(uint8_t)(mixed>>(8*j));
    uint8_t mixed_saved[32];memcpy(mixed_saved,cell,sizeof mixed_saved);
    assert(original_sound_valid(cell));
    if(kind==ORIGINAL_SOUND_MIAMI) {
        original_miami_program expected_program=mr;
        assert(original_miami_program_set(&expected_program,1,41.0f/127.0f));
        assert(original_sound_restore_miami(&m,cell) && !memcmp(&m,&expected_program,sizeof m));
    } else {
        original_chicago_program expected_program=cr;
        assert(original_chicago_program_set(&expected_program,1,41.0f/127.0f));
        assert(original_sound_restore_chicago(&c,cell) && !memcmp(&c,&expected_program,sizeof c));
    }
    assert(!memcmp(cell,mixed_saved,sizeof cell));
    memcpy(cell,saved,sizeof cell);
    /* Every UI value at once, including touching a value equal to the rounded
     * factory display: the edit mask must still record the user's decision. */
    for (unsigned i=0;i<n-2;++i) assert(original_sound_set(cell,i,saved[i]));
    assert(original_sound_edit_mask(cell)==((UINT32_C(1)<<(n-2))-1));
    if(kind==ORIGINAL_SOUND_MIAMI) {
        for(unsigned i=0;i<n-2;++i) assert(original_miami_program_set(&mr,i,edited_value(kind,i,saved[i])));
        assert(original_sound_restore_miami(&m,cell) && !memcmp(&m,&mr,sizeof m));
    } else {
        for(unsigned i=0;i<n-2;++i) assert(original_chicago_program_set(&cr,i,edited_value(kind,i,saved[i])));
        assert(original_sound_restore_chicago(&c,cell) && !memcmp(&c,&cr,sizeof c));
    }
    for(unsigned field=22;field<32;++field) {
        memcpy(cell,saved,sizeof cell);
        if(field<26) cell[25]=0x80;
        else cell[field]=255;
        assert(!original_sound_valid(cell));
        uint8_t bad[32];memcpy(bad,cell,sizeof bad);
        assert(!original_sound_set(cell,0,10) && !memcmp(cell,bad,sizeof cell));
        memset(&m,0xa5,sizeof m);original_miami_program old_m=m;
        memset(&c,0xa5,sizeof c);original_chicago_program old_c=c;
        assert(!original_sound_restore_miami(&m,cell) && !memcmp(&m,&old_m,sizeof m));
        assert(!original_sound_restore_chicago(&c,cell) && !memcmp(&c,&old_c,sizeof c));
        float old=-12,out=old;assert(!original_sound_value(cell,0,&out) && !memcmp(&out,&old,sizeof out));
        failures++;
    }
    memcpy(cell,saved,sizeof cell);
    cell[28]=1;
    assert(!original_sound_valid(cell));
    memcpy(cell,saved,sizeof cell);
    if (kind==ORIGINAL_SOUND_CHICAGO) {
        for (unsigned value=12;value<256;++value) {
            assert(!original_sound_set(cell,17,value) && !memcmp(cell,saved,sizeof cell));
            cell[17]=(uint8_t)value;
            assert(!original_sound_valid(cell));
            memcpy(cell,saved,sizeof cell);
        }
    }
    assert(!original_sound_set(cell,n,0) && !memcmp(cell,saved,sizeof cell));
    assert(!original_sound_set(cell,0,128) && !memcmp(cell,saved,sizeof cell));
    assert(!original_sound_init(cell,kind,original_sound_factories(kind)) && !memcmp(cell,saved,sizeof cell));
    if(kind==ORIGINAL_SOUND_MIAMI) {cell[18]=1;assert(!original_sound_valid(cell));}
    factories++;
}
int main(void)
{
    uint8_t legacy[32]={0};
    assert(!original_sound_valid(legacy));
    memset(legacy,127,sizeof legacy);assert(!original_sound_valid(legacy));
    assert(!original_sound_valid(0) && !original_sound_set(0,0,0) && !original_sound_init(0,0,0));
    assert(!original_sound_count(2) && !original_sound_factories(2) && !original_sound_engine_id(2));
    assert(!original_sound_param(2,0) && !original_sound_name(2,0));
    assert(!original_sound_fx_name(12));
    static const char *const fx_names[12]={"OFF","CHO/FLG","PHASER","RING MOD","COMPRESSOR","DECIMATOR","EQ",
        "ENSEMBLE","SHORT DLY","BPM DELAY","HALL","GATE REV"};
    for(unsigned i=0;i<12;i++)assert(!strcmp(original_sound_fx_name(i),fx_names[i]));
    assert(!original_sound_is_performance(2,16) && !original_sound_is_performance(0,99));
    assert(original_sound_factory_source(2,0)==-1 && original_sound_factory_source(0,55)==-1);
    /* Independently enumerate all original factory routings. The selected bank
     * contains exactly every admitted program in source order, with no changed
     * name, skipped supported program, duplicated base or substituted effect. */
    unsigned compact=0;
    for (unsigned raw=0;raw<81;++raw) {
        original_chicago_program p; original_chicago_routing r;
        assert(original_chicago_program_init(&p,raw) && original_chicago_program_routing(&p,&r));
        int supported=0;
        for(unsigned i=0;i<7;++i) supported|=r.fx[1][0]==fx_types[i];
        if(supported) {
            assert(original_sound_factory_source(0,compact)==(int)raw);
            assert(!strcmp(original_sound_name(0,compact),original_chicago_program_name(raw)));
            ++compact;
        }
    }
    assert(compact==55 && compact==original_sound_factories(0));
    float refused=-7;
    assert(!original_sound_control_value(0,17,12,&refused) && refused==-7);
    assert(!original_sound_control_value(2,0,0,&refused) && refused==-7);
    for(unsigned kind=0;kind<2;kind++) {
        unsigned parameter=kind?7:4;
        float previous=-1;
        for(unsigned q=0;q<128;q++) {
            float actual=-1,expected=edited_value(kind,parameter,q);
            assert(original_sound_control_value(kind,parameter,q,&actual));
            assert(!memcmp(&actual,&expected,4) && actual>previous);
            if(q==0)assert(actual==0.0f);
            if(q==64)assert(actual==0.5f);
            if(q==127)assert(actual==1.0f);
            previous=actual;
        }
    }
    for(unsigned kind=0;kind<2;++kind)
        for(unsigned factory=0;factory<original_sound_factories(kind);++factory) compare(kind,factory);
    printf("PASS compact32-byte patch cell: %u exact factory programs, %u single-control edits, %u malformed transactions; %u performance edits refused and %u stale performance restores neutral; seven FX and55/81 factory selection; schema1 rejected; no factory float round-trip\n",factories,edits,failures,performance_refusals,stale_restores);
    return 0;
}
