/* SPDX-License-Identifier: GPL-3.0-only */
/* Production host + engine request/consume boundary regression. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stdio.h>
static unsigned checks, failures;
#define EXPECT(c) do { checks++; if (!(c)) { failures++; fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c); } } while (0)
int engine_part_sounding(void);
static void queued_midi(uint32_t pkt)
{
#ifdef TEST_MIDI_EPOCH
    host_midi_put(pkt);
#else
    min_q[mi_w++ % MQ] = pkt;
#endif
}
static uint32_t note_packet(int src, int voice, int pitch, int vel)
{
    return 9u | ((uint32_t)src << 4) | ((0x90u | seq.ch[TRK_BASS0+voice]) << 8) |
           ((uint32_t)pitch << 16) | ((uint32_t)vel << 24);
}
static void cq_case(unsigned prefix, int old_release)
{
    boot();
    for (unsigned i=0;i<prefix;i++) engine_set(T_MIX,0,MX_PAN,(int)(i&127u));
    if(old_release)engine_bass_off(0);
    engine_request_notes_off();
    engine_bass_on(0,60,0,0);
    /* 1024 commands need eight 128-command audio drains (256 frames each). */
    run_ms(80);
    EXPECT(engine_bass_keys_held() & 1);
    engine_bass_off(0);run_ms(20);
    if(engine_bass_keys_held() & 1) fprintf(stderr,"CQ release prefix=%u old_release=%d held=%d\n",prefix,old_release,engine_bass_keys_held());
    EXPECT(!(engine_bass_keys_held() & 1));
}
static void physical_cases(void)
{
    uint32_t generation,context;int arp;
    boot();engine_live_key_state(0,&generation,&arp,&context);
    EXPECT(engine_live_key(0,generation,1,48,100,0,0));
    uint32_t old_generation=generation;
    engine_request_notes_off();
    engine_live_key_state(0,&generation,&arp,&context);
    EXPECT(engine_live_key(0,generation,2,60,100,0,0));
    run_ms(20);
    EXPECT(engine_bass_keys_held() & 1);
    /* A pre-cut owner's late release cannot retire the new key. */
    EXPECT(engine_live_key(0,old_generation,1,48,0,0,0));
    run_ms(20);EXPECT(engine_bass_keys_held() & 1);
    EXPECT(engine_live_key(0,generation,2,60,0,0,0));
    run_ms(20);EXPECT(!(engine_bass_keys_held() & 1));
}
static void midi_case(unsigned prefix,int src,int voice,int fresh)
{
    boot();
    for(unsigned i=0;i<prefix;i++)queued_midi(0x0eu | ((uint32_t)src<<4) | ((0xE0u|seq.ch[TRK_BASS0+voice])<<8) | (64u<<24));
    if(!fresh)queued_midi(note_packet(src,voice,48,100));
    engine_request_notes_off();
    if(fresh)queued_midi(note_packet(src,voice,60,100));
    run_ms(30);
    EXPECT(!!(engine_bass_keys_held() & (1<<voice)) == !!fresh);
    queued_midi(note_packet(src,voice,60,0));run_ms(20);
    EXPECT(!(engine_bass_keys_held() & (1<<voice)));
}
static void repeated_requests(void)
{
    boot();engine_request_notes_off();engine_bass_on(0,48,0,0);
    queued_midi(note_packet(1,1,48,100));
    engine_request_notes_off();run_ms(30);
    EXPECT(engine_bass_keys_held()==0);
    engine_bass_on(0,60,0,0);queued_midi(note_packet(1,1,60,100));run_ms(30);
    EXPECT(engine_bass_keys_held()==3);
    engine_request_notes_off();run_ms(30);EXPECT(engine_bass_keys_held()==0);
}
int main(void)
{
    const unsigned prefixes[]={0,128,1023,1024};
    for(unsigned i=0;i<4;i++){cq_case(prefixes[i],0);cq_case(prefixes[i],1);}
    physical_cases();
    const unsigned mprefix[]={0,63,64,65};
    for(unsigned p=0;p<4;p++)for(int src=0;src<3;src++)for(int v=0;v<2;v++)
        for(int fresh=0;fresh<2;fresh++)midi_case(mprefix[p],src,v,fresh);
    repeated_requests();
    printf("PANIC real-engine cutover: %u checks, %u failures\n",checks,failures);
    return failures ? 1 : 0;
}
