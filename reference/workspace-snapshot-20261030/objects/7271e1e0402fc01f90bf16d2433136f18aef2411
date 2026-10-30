/* SPDX-License-Identifier: GPL-3.0-only */
/* MID-15: clock ownership and MIDI note lifetime survive ext_now rollover.
 * At 44.1 kHz uint32_t ext_now wraps after ~27.05 hours of uptime. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sequencer.h"

static unsigned checks, failures, scenario, block_size;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; if (failures < 24u) \
    fprintf(stderr,"FAIL scenario=%u block=%u line=%d: %s\n",scenario,block_size,__LINE__,#c); } } while (0)
typedef struct { unsigned steps, last, on, off, orphan, duplicate, held, unwanted_clock; unsigned char active[16][128]; } monitor_t;
static pattern_t patterns[NPAT];
static void midi_cb(void *ptr,uint8_t st,uint8_t pitch,uint8_t v)
{
    monitor_t *m=ptr;
    unsigned type=st&0xf0u,ch=st&15u;
    if(st>=0xf8u) { m->unwanted_clock++; return; }
    if(pitch>127u)return;
    if(type==0x90u && v) {
        ++m->on;
        if(m->active[ch][pitch]) ++m->duplicate;
        else { m->active[ch][pitch]=1; ++m->held; }
    } else if(type==0x80u || (type==0x90u && !v)) {
        ++m->off;
        if(!m->active[ch][pitch]) ++m->orphan;
        else { m->active[ch][pitch]=0; --m->held; }
    }
}
static void on_step(void *ptr,int part,int pos)
{ monitor_t *m=ptr; if(part==TRK_BASS0) { ++m->steps; m->last=(unsigned)pos; } }
static int on_drum(void *p,int a,int b,float v)
{ (void)p;(void)a;(void)b;(void)v; return 1; }
static int on_bass(void *p,int a,int b,int c,int d,int e)
{ (void)p;(void)a;(void)b;(void)c;(void)d;(void)e;  return 1; }
static void off_bass(void *p,int a) { (void)p;(void)a; }
static void render(seq_t *s, const seq_sink_t *sink, uint32_t frames)
{
    while(frames) {
        uint32_t packet=frames<block_size?frames:block_size;
        frames-=packet;
        while(packet) {
            uint32_t len=seq_until_event(s,packet);
            CHECK(len>0u && len<=packet);
            if(!len||len>packet)return;
            seq_advance(s,len,sink);
            packet-=len;
        }
    }
}
static int receive(seq_t *s,uint8_t st,uint32_t src)
{
    if(!seq_clk_take(s,st,src))return 0;
    CHECK(seq_midi_rt(s,st)==1);
    return 1;
}
static void setup(seq_t *s,monitor_t *m,seq_sink_t *sink)
{
    memset(patterns,0,sizeof(patterns));
    for(unsigned p=0;p<NPAT;++p) pattern_init(&patterns[p],p+17u,p+39u);
    for(unsigned i=0;i<16u;++i) patterns[0].bass[0].step[i]=(bstep_t){(uint8_t)(60u+i%3u),G_NOTE|BS_SLIDE,110};
    patterns[0].bass[0].len=16u;
    seq_init(s,patterns);s->clk_src=SEQ_CLK_EXT;s->send_clock=1;s->send_notes=1;
    memset(m,0,sizeof(*m));memset(sink,0,sizeof(*sink));
    sink->ctx=m;sink->midi=midi_cb;sink->step=on_step;sink->drum=on_drum;
    sink->bass_on=on_bass;sink->bass_off=off_bass;
    render(s,sink,1u);
}
static void assert_safe(const monitor_t *m)
{ CHECK(!m->orphan); CHECK(!m->duplicate); CHECK(!m->held); CHECK(m->on==m->off); CHECK(!m->unwanted_clock); }

static void live_heartbeat_wrap(void)
{
    seq_t s; monitor_t m; seq_sink_t sink;
    setup(&s,&m,&sink);
    CHECK(receive(&s,0xFAu,0u));CHECK(receive(&s,0xF8u,0u));render(&s,&sink,1u);
    CHECK(s.playing && m.steps==1u && m.on==1u && m.held==1u);
    CHECK(s.clk_lock==1u);
    /* Elapsed runtime near 2^32/FS. A heartbeat just before overflow must
     * retain the original clock until an entire new lock timeout elapses. */
    s.ext_now=UINT32_MAX-100u;s.clk_lock_at=s.ext_now;
    render(&s,&sink,256u);
    CHECK(s.ext_now==155u && s.clk_lock==1u);
    CHECK(!receive(&s,0xF8u,1u));
    CHECK(receive(&s,0xF8u,0u));render(&s,&sink,1u);
    CHECK(s.clk_lock==1u);
    /* A clock-owner change before a full 0.4s silence would double the
     * musical tempo. At exactly the limit expiry uses the strict > rule. */
    render(&s,&sink,SEQ_LOCK_T-1u);
    CHECK(s.clk_lock==1u);
    CHECK(!receive(&s,0xF8u,1u));
    render(&s,&sink,1u);
    CHECK(s.clk_lock==0u);
    CHECK(receive(&s,0xF8u,1u));CHECK(s.clk_lock==2u);
    render(&s,&sink,1u);
    CHECK(!receive(&s,0xFAu,0u));
    CHECK(receive(&s,0xFCu,1u));render(&s,&sink,1u);
    CHECK(!s.playing);assert_safe(&m);
}
static void stop_lease_wrap(void)
{
    seq_t s; monitor_t m; seq_sink_t sink;
    setup(&s,&m,&sink);
    CHECK(receive(&s,0xFAu,0u));CHECK(receive(&s,0xF8u,0u));render(&s,&sink,1u);
    CHECK(s.playing && m.on==1u && m.held==1u);
    s.ext_now=UINT32_MAX-100u;s.clk_lock_at=s.ext_now;
    CHECK(receive(&s,0xFCu,0u));
    CHECK(s.clk_stop==1u && s.clk_stop_at==UINT32_MAX-100u);
    CHECK(!receive(&s,0xFAu,1u)); /* still pre-wrap; no early foreign Start */
    render(&s,&sink,256u);
    CHECK(s.ext_now==155u);
    CHECK(!s.playing && m.on==m.off && m.held==0u);
    CHECK(!receive(&s,0xFAu,1u));
    /* A different clock source may Start exactly 0.4s after the owner's
     * Stop, even when the timer wrapped in between. */
    render(&s,&sink,SEQ_LOCK_T-257u);
    CHECK(!receive(&s,0xFAu,1u));
    render(&s,&sink,1u);
    CHECK(s.clk_lock==1u); /* expiry in seq_advance is >, not >= */
    CHECK(receive(&s,0xFAu,1u));
    CHECK(s.clk_lock==2u && s.clk_stop==0u && s.ext_skip==1u);
    CHECK(receive(&s,0xF8u,1u));render(&s,&sink,1u);
    CHECK(s.playing && m.steps==2u && m.last==0u);
    CHECK(m.on==2u && m.off==1u && m.held==1u);
    CHECK(!receive(&s,0xFCu,0u));
    CHECK(receive(&s,0xFCu,1u));render(&s,&sink,1u);
    CHECK(!s.playing);assert_safe(&m);
}
int main(void)
{
    const unsigned blocks[]={1u,64u,1024u};
    for(unsigned i=0;i<3u;++i) {
        block_size=blocks[i];scenario=2u*i+1u;live_heartbeat_wrap();
        scenario=2u*i+2u;stop_lease_wrap();
    }
    fprintf(stderr,"seq uptime clock wrap: %u/%u checks %s\n",checks-failures,checks,failures?"FAILED":"PASSED");
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
