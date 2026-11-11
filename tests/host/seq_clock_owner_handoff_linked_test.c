/* SPDX-License-Identifier: GPL-3.0-only */
/* Regression for MID-15: one MIDI clock origin at a time. The first source
 * holds a clock lease for 0.4 s, and a new USB/BLE source may not advance
 * the sequencer while the old source still owns its clock. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sequencer.h"

static unsigned checks, failures, test_id, block_size;
#define CHECK(cond) do { ++checks; if (!(cond)) { ++failures; if (failures < 30u) \
    fprintf(stderr, "FAIL scenario=%u block=%u line=%u: %s\n", test_id, block_size, (unsigned)__LINE__, #cond); } } while (0)

typedef struct {
    unsigned steps, on, off, duplicate, orphan, clock_echo, transport_echo, breaks;
    unsigned held;
    unsigned last_step;
    uint8_t open[16][128];
} monitor_t;
static pattern_t pats[NPAT];
static void out_midi(void *opaque, uint8_t status, uint8_t note, uint8_t velocity)
{
    monitor_t *m=(monitor_t *)opaque;
    unsigned hi=status&0xf0u, ch=status&0x0fu;
    if (status>=0xf8u) { if(status==0xf8u)m->clock_echo++; else m->transport_echo++; return; }
    if (hi==0x90u && velocity) {
        m->on++;
        if(m->open[ch][note])m->duplicate++;
        else {m->open[ch][note]=1; m->held++;}
    } else if(hi==0x80u || (hi==0x90u && !velocity)) {
        m->off++;
        if(!m->open[ch][note])m->orphan++;
        else {m->open[ch][note]=0; m->held--;}
    }
}
static void on_step(void *opaque,int part,int index)
{
    monitor_t *m=(monitor_t *)opaque;
    if(part==TRK_BASS0) { m->steps++; m->last_step=(unsigned)index; }
}
static void on_break(void *opaque){ ((monitor_t *)opaque)->breaks++; }
static int on_drum(void *opaque,int p,int k,float vel){(void)opaque;(void)p;(void)k;(void)vel;return 1;}
static int on_bass(void *opaque,int p,int n,int a,int slide,int velocity){(void)opaque;(void)p;(void)n;(void)a;(void)slide;(void)velocity; return 1; }
static void off_bass(void *opaque,int p){(void)opaque;(void)p;}
static void render(seq_t *s,const seq_sink_t *sink,uint32_t frames)
{
    while(frames) {
        uint32_t packet=frames<block_size?frames:block_size;
        frames-=packet;
        while(packet) {
            uint32_t step=seq_until_event(s,packet);
            CHECK(step>0u && step<=packet);
            if(!step || step>packet)return;
            seq_advance(s,step,sink);
            packet-=step;
        }
    }
}
static int recv(seq_t *s,uint8_t status,uint32_t input)
{
    if(!seq_clk_take(s,status,input))return 0;
    CHECK(seq_midi_rt(s,status)==1);
    return 1;
}
static void init(seq_t *s,monitor_t *m,seq_sink_t *sink)
{
    unsigned p,k;
    memset(pats,0,sizeof pats);
    for(p=0;p<NPAT;p++)pattern_init(&pats[p],p+17u,p+39u);
    for(k=0;k<16u;k++)pats[0].bass[0].step[k]=(bstep_t){(uint8_t)(60+k%3),G_NOTE|BS_SLIDE,110};
    pats[0].bass[0].len=16u;
    seq_init(s,pats);
    s->clk_src=SEQ_CLK_EXT;
    s->send_clock=1;
    s->send_notes=1;
    memset(m,0,sizeof *m);
    memset(sink,0,sizeof *sink);
    sink->ctx=m;
    sink->midi=out_midi;
    sink->step=on_step;
    sink->brk_stop=on_break;
    sink->drum=on_drum;
    sink->bass_on=on_bass;
    sink->bass_off=off_bass;
    render(s,sink,1u);
}
static void start_usb(seq_t *s,monitor_t *m,const seq_sink_t *sink)
{
    CHECK(recv(s,0xfau,0u));
    CHECK(recv(s,0xf8u,0u));
    render(s,sink,1u);
    CHECK(s->playing && s->ext);
    CHECK(s->clk_lock==1u && m->steps==1u && m->last_step==0u);
    CHECK(m->held==1u && m->on==1u && m->off==0u);
}
static void timeout_takeover(void)
{
    seq_t s;monitor_t m;seq_sink_t sink;
    init(&s,&m,&sink);start_usb(&s,&m,&sink);
    /* First owner USB renewed the lock at frame 1. A competing BLE F8 and
       Start at t < 0.4 s must be rejected, not counted as extra PPQN. */
    CHECK(!recv(&s,0xf8u,1u));
    CHECK(!recv(&s,0xfau,1u));
    CHECK(s.clk_lock==1u && s.ext_pending==0u);
    render(&s,&sink,SEQ_LOCK_T-1u);
    CHECK(!recv(&s,0xf8u,1u));
    CHECK(!recv(&s,0xfau,1u));
    CHECK(s.clk_lock==1u && m.steps==1u);
    /* The next audio frame expires the lease before the next MIDI packet. */
    render(&s,&sink,1u);
    CHECK(recv(&s,0xf8u,1u)); /* a new link owns the next available tick */
    CHECK(s.clk_lock==2u);
    CHECK(s.ext_skip==1u && s.ext_pending==1u);
    render(&s,&sink,1u);
    CHECK(m.steps==1u && m.held==1u && m.off==0u);
    CHECK(!recv(&s,0xf8u,0u)); /* stale USB clock is rejected */
    CHECK(s.ext_pending==0u && m.steps==1u);
    for(unsigned tick=1;tick<=4u;tick++) {
        CHECK(recv(&s,0xf8u,1u));
        render(&s,&sink,1u);
        CHECK(m.steps==1u);
    }
    CHECK(recv(&s,0xf8u,1u));
    render(&s,&sink,1u);
    CHECK(m.steps==2u && m.last_step==1u);
    CHECK(!recv(&s,0xfcu,0u)); /* USB Stop must not affect BLE playback */
    render(&s,&sink,1u);
    CHECK(s.playing && m.steps==2u);
    CHECK(recv(&s,0xfcu,1u));
    render(&s,&sink,1u);
    CHECK(!s.playing && m.held==0u && m.on==m.off);
    CHECK(m.orphan==0u && m.duplicate==0u);
    CHECK(m.breaks==1u);
    CHECK(m.clock_echo==0u && m.transport_echo==0u);
}
static void stopped_lease(void)
{
    seq_t s;monitor_t m;seq_sink_t sink;
    init(&s,&m,&sink);start_usb(&s,&m,&sink);
    CHECK(recv(&s,0xfcu,0u));
    render(&s,&sink,1u);
    CHECK(!s.playing && m.on==m.off && m.held==0u);
    CHECK(s.clk_stop && s.clk_lock==1u);
    CHECK(!recv(&s,0xfau,1u));
    render(&s,&sink,SEQ_LOCK_T+1u);
    CHECK(recv(&s,0xfau,1u));
    CHECK(s.clk_lock==2u);
    CHECK(recv(&s,0xf8u,1u));
    render(&s,&sink,1u);
    CHECK(s.playing && m.steps==2u && m.last_step==0u);
    CHECK(m.on==2u && m.off==1u && m.held==1u);
    CHECK(!recv(&s,0xfcu,0u));
    CHECK(recv(&s,0xfcu,1u));
    render(&s,&sink,1u);
    CHECK(!s.playing && m.on==m.off && m.held==0u);
    CHECK(!m.orphan && !m.duplicate);
    CHECK(!m.clock_echo && !m.transport_echo);
}
int main(void)
{
    const unsigned blocks[]={1u,64u,1024u};
    for(unsigned i=0;i<3u;i++){
        block_size=blocks[i]; test_id=1u; timeout_takeover(); test_id=2u; stopped_lease();
    }
    fprintf(stderr,"seq clock owner handoff: %u/%u checks %s\n",checks-failures,checks,failures?"FAILED":"PASSED");
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
