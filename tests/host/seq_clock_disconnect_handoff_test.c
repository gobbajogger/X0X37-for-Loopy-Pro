/* SPDX-License-Identifier: GPL-3.0-only */
/* MID-15: USB/BLE MIDI clock transfer on explicit link disconnect. The
 * surviving source may continue the existing phase without double clocks. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sequencer.h"

static unsigned checks, failures, block_size, scenario;
#define CHECK(cond) do { ++checks; if (!(cond)) { ++failures; if (failures < 30u) \
    fprintf(stderr, "FAIL scenario=%u block=%u line=%u: %s\n",scenario,block_size,(unsigned)__LINE__,#cond); } } while (0)
typedef struct {
    unsigned steps, last_step, on, off, duplicate, orphan, held;
    unsigned clock_echo, transport_echo;
    uint8_t active[16][128];
} monitor_t;
static pattern_t pats[NPAT];
static void midi_out(void *p,uint8_t status,uint8_t note,uint8_t vel)
{
    monitor_t *m=(monitor_t *)p;
    unsigned k=status&0xf0u, ch=status&15u;
    if(status>=0xf8u) {if(status==0xf8u)m->clock_echo++;else m->transport_echo++;return;}
    if(k==0x90u && vel) {
        m->on++;
        if(m->active[ch][note])m->duplicate++;
        else {m->active[ch][note]=1; m->held++;}
    } else if(k==0x80u || (k==0x90u && !vel)) {
        m->off++;
        if(!m->active[ch][note])m->orphan++;
        else {m->active[ch][note]=0; m->held--;}
    }
}
static void on_step(void *p,int part,int index)
{
    monitor_t *m=(monitor_t *)p;
    if(part==TRK_BASS0) {m->steps++;m->last_step=(unsigned)index;}
}
static int on_drum(void *p,int a,int b,float vel)
{(void)p;(void)a;(void)b;(void)vel;return 1;}
static void on_bass(void *p,int a,int b,int c,int d,int e)
{(void)p;(void)a;(void)b;(void)c;(void)d;(void)e;}
static void off_bass(void *p,int a){(void)p;(void)a;}
static void render(seq_t *s,const seq_sink_t *out,uint32_t frames)
{
    while(frames) {
        uint32_t packet=frames<block_size?frames:block_size;
        frames-=packet;
        while(packet) {
            uint32_t step=seq_until_event(s,packet);
            CHECK(step>0u && step<=packet);
            if(!step || step>packet)return;
            seq_advance(s,step,out);
            packet-=step;
        }
    }
}
static int recv(seq_t *s,uint8_t status,uint32_t source)
{
    if(!seq_clk_take(s,status,source))return 0;
    CHECK(seq_midi_rt(s,status)==1);
    return 1;
}
static void init(seq_t *s,monitor_t *m,seq_sink_t *out)
{
    memset(pats,0,sizeof pats);
    for(unsigned p=0;p<NPAT;p++)pattern_init(&pats[p],p+17u,p+39u);
    for(unsigned i=0;i<16u;i++)pats[0].bass[0].step[i]=(bstep_t){(uint8_t)(60u+i%3u),G_NOTE|BS_SLIDE,110};
    pats[0].bass[0].len=16u;
    seq_init(s,pats);
    s->clk_src=SEQ_CLK_EXT;
    s->send_notes=1;
    s->send_clock=1;
    memset(m,0,sizeof *m);
    memset(out,0,sizeof *out);
    out->ctx=m;out->midi=midi_out;out->step=on_step;
    out->drum=on_drum;out->bass_on=on_bass;out->bass_off=off_bass;
    render(s,out,1);
}
static void midstep_disconnect(void)
{
    seq_t s;monitor_t m;seq_sink_t out;
    init(&s,&m,&out);
    CHECK(recv(&s,0xfau,0u));CHECK(recv(&s,0xf8u,0u));render(&s,&out,1u);
    CHECK(s.playing && m.steps==1u && m.last_step==0u);
    CHECK(m.on==1u && m.off==0u && m.held==1u);
    CHECK(s.clk_lock==1u);
    for(unsigned i=0;i<3u;i++) {
        CHECK(recv(&s,0xf8u,0u));render(&s,&out,1u);
        CHECK(m.steps==1u);
    }
    CHECK(s.t[TRK_BASS0].ext_ticks==3u);
    /* USB drops mid-step; this is a link event, not a new transport Start. */
    seq_clk_drop(&s,0u);
    CHECK(s.clk_lock==0u && s.clk_last==1u);
    seq_clk_drop(&s,0u); /* repeated disconnect is idempotent */
    seq_clk_drop(&s,3u); /* unrelated device disconnect must not steal ownership */
    CHECK(s.clk_lock==0u);
    CHECK(recv(&s,0xf8u,1u)); /* BLE owns the next tick without restarting */
    CHECK(s.clk_lock==2u && s.ext_skip==1u);
    CHECK(!recv(&s,0xf8u,0u)); /* stale USB clock cannot double-advance */
    CHECK(!recv(&s,0xfau,0u)); /* stale USB Start cannot restart step 1 */
    CHECK(s.ext_pending==1u);
    render(&s,&out,1u);
    CHECK(s.ext_skip==0u && s.ext_pending==0u);
    CHECK(s.t[TRK_BASS0].ext_ticks==4u && m.steps==1u);
    seq_clk_drop(&s,0u); /* stale link notification after BLE takeover */
    CHECK(s.clk_lock==2u);
    CHECK(recv(&s,0xf8u,1u));render(&s,&out,1u);
    CHECK(s.t[TRK_BASS0].ext_ticks==5u && m.steps==1u);
    CHECK(recv(&s,0xf8u,1u));render(&s,&out,1u);
    CHECK(m.steps==2u && m.last_step==1u);
    CHECK(!recv(&s,0xfcu,0u)); /* ghost USB Stop cannot cut new owner's note */
    render(&s,&out,1u);
    CHECK(s.playing && m.steps==2u && m.held>=1u);
    CHECK(recv(&s,0xfcu,1u));render(&s,&out,1u);
    CHECK(!s.playing && m.held==0u && m.on==m.off);
    CHECK(!m.duplicate && !m.orphan && !m.clock_echo && !m.transport_echo);
    /* Link shutdown must release the lock so a subsequently reconnected
       device can start playback without waiting for an artificial lease. */
    seq_clk_drop(&s,1u);
    CHECK(s.clk_lock==0u);
    CHECK(recv(&s,0xfau,0u));CHECK(recv(&s,0xf8u,0u));render(&s,&out,1u);
    CHECK(s.playing && m.steps==3u && m.last_step==0u);
    CHECK(recv(&s,0xfcu,0u));render(&s,&out,1u);
    CHECK(!s.playing && m.held==0u && m.on==m.off);
    CHECK(!m.duplicate && !m.orphan && !m.clock_echo && !m.transport_echo);
}
int main(void)
{
    const unsigned blocks[]={1u,64u,1024u};
    for(unsigned i=0;i<3u;i++) {
        block_size=blocks[i];scenario=i+1u;midstep_disconnect();
    }
    fprintf(stderr,"seq link-drop clock handoff: %u/%u checks %s\n",checks-failures,checks,failures?"FAILED":"PASSED");
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
