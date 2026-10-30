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
static int on_bass(void *p,int a,int b,int c,int d,int e)
{(void)p;(void)a;(void)b;(void)c;(void)d;(void)e; return 1; }
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

/* A disconnect and replacement source can share one audio callback. Clock
 * packets queued by the dead source must not advance the new owner's phase.
 * Otherwise one physical beat advances twice, one step before Loopy's tick. */
static void queued_clock_disconnect(uint32_t old_input)
{
    seq_t s; monitor_t m; seq_sink_t out;
    uint32_t new_input=old_input ^ 1u;
    init(&s,&m,&out);
    CHECK(recv(&s,0xfau,old_input));
    CHECK(recv(&s,0xf8u,old_input));
    render(&s,&out,1u);
    CHECK(s.playing && m.steps==1u && m.held==1u);
    for(unsigned i=0;i<4u;i++) {
        CHECK(recv(&s,0xf8u,old_input));
        render(&s,&out,1u);
    }
    CHECK(s.t[TRK_BASS0].ext_ticks==4u && m.steps==1u);
    CHECK(recv(&s,0xf8u,old_input)); /* queued in USB/BLE callback */
    CHECK(s.ext_pending==1u);
    seq_clk_drop(&s,old_input);      /* link closes before render */
    CHECK(s.clk_lock==0u);
    CHECK(recv(&s,0xf8u,new_input)); /* replacement clocks in same render */
    CHECK(s.clk_lock==new_input+1u);
    CHECK(s.ext_pending==1u);       /* NOT one abandoned plus one new */
    render(&s,&out,1u);
    CHECK(s.t[TRK_BASS0].ext_ticks==5u && m.steps==1u);
    CHECK(recv(&s,0xf8u,new_input));
    render(&s,&out,1u);
    CHECK(m.steps==2u && m.last_step==1u); /* next step is six ticks total */
    CHECK(!recv(&s,0xfcu,old_input));
    CHECK(recv(&s,0xfcu,new_input));
    render(&s,&out,1u);
    CHECK(!s.playing && m.held==0u && m.on==m.off);
    CHECK(!m.duplicate && !m.orphan && !m.clock_echo && !m.transport_echo);
}


/* A local PLAY gesture must survive a USB disconnect and use the first BLE
 * clock, even when an older USB F8 was queued before PLAY. Reset both the
 * obsolete F8 queue and its request-epoch boundary on clock-owner change. */
static void pending_local_play_handoff(uint32_t old_input)
{
    seq_t s; monitor_t m; seq_sink_t out;
    uint32_t new_input=old_input ^ 1u;
    init(&s,&m,&out);
    CHECK(!s.playing);
    CHECK(recv(&s,0xf8u,old_input));
    CHECK(s.ext_pending==1u);
    seq_start(&s); /* local PLAY wins; F8 from old link predates it */
    CHECK(s.req_start && !s.req_rt && s.req_prior_ticks==1u);
    seq_clk_drop(&s,old_input);
    CHECK(recv(&s,0xf8u,new_input));
    CHECK(s.ext_pending==1u && s.req_prior_ticks==0u);
    render(&s,&out,1u);
    CHECK(s.playing && m.steps==1u && m.last_step==0u);
    CHECK(s.t[TRK_BASS0].ext_ticks==0u);
    CHECK(m.on==1u && m.held==1u);
    CHECK(recv(&s,0xfcu,new_input));
    render(&s,&out,1u);
    CHECK(!s.playing && m.on==m.off && m.held==0u);
    CHECK(!m.duplicate && !m.orphan && !m.clock_echo && !m.transport_echo);
}


/* MID-06 transport ordering is independent of the ownership of subsequent F8.
 * An accepted old-source Stop before the link goes down is a real Stop, not a
 * stale clock packet. A new clock must not silently cancel it, and a following
 * new-source Start or local PLAY must first release the sounding 303 note. */
static void valid_stop_during_handoff(uint32_t old_input,unsigned restart)
{
    seq_t s; monitor_t m; seq_sink_t out;
    uint32_t new_input=old_input ^ 1u;
    init(&s,&m,&out);
    CHECK(recv(&s,0xfau,old_input));
    CHECK(recv(&s,0xf8u,old_input));
    render(&s,&out,1u);
    CHECK(s.playing && m.steps==1u && m.on==1u && m.held==1u);
    for(unsigned i=0;i<4u;i++) {
        CHECK(recv(&s,0xf8u,old_input));
        render(&s,&out,1u);
    }
    CHECK(s.t[TRK_BASS0].ext_ticks==4u);
    CHECK(recv(&s,0xf8u,old_input)); /* in-flight stale clock */
    CHECK(recv(&s,0xfcu,old_input)); /* valid transport before disconnect */
    CHECK(s.req_stop && s.req_stop_rt && s.ext_pending==1u);
    seq_clk_drop(&s,old_input);
    CHECK(s.clk_lock==0u);
    if (restart==1u)
        CHECK(recv(&s,0xfau,new_input)); /* Stop then Start, one callback */
    else if (restart==2u)
        seq_start(&s);                   /* Stop then local PLAY */
    CHECK(recv(&s,0xf8u,new_input));     /* new owner: discard old F8 only */
    CHECK(s.ext_pending==1u);
    CHECK(s.req_stop && s.req_stop_rt); /* accepted old Stop is still ordered */
    render(&s,&out,1u);
    CHECK(m.steps==(restart ? 2u : 1u));
    CHECK(m.off==1u);                   /* old 303 note released once */
    CHECK(m.on==(restart ? 2u : 1u));
    CHECK(m.held==(restart ? 1u : 0u));
    CHECK(s.playing==(restart != 0u));
    CHECK(!m.duplicate && !m.orphan);
    if(restart) {
        CHECK(m.last_step==0u);
        CHECK(s.t[TRK_BASS0].ext_ticks==0u);
        CHECK(s.start_local==(restart==2u));
    } else {
        CHECK(recv(&s,0xfau,new_input));
        CHECK(recv(&s,0xf8u,new_input));
        render(&s,&out,1u);
        CHECK(s.playing && m.steps==2u && m.last_step==0u);
        CHECK(m.on==2u && m.off==1u && m.held==1u);
    }
    CHECK(!recv(&s,0xfcu,old_input)); /* discarded as foreign *after* handoff */
    CHECK(recv(&s,0xfcu,new_input));
    render(&s,&out,1u);
    CHECK(!s.playing && m.held==0u && m.on==m.off);
    CHECK(!m.duplicate && !m.orphan && !m.clock_echo && !m.transport_echo);
}


/* Preserve an accepted MIDI Start/Continue when the originating USB/BLE link
 * ends before audio applies it. Clock ticks from that old link are different:
 * they must not accumulate with the replacement source's downbeat. */
static void valid_start_continue_during_handoff(uint32_t old_input,uint8_t command)
{
    seq_t s; monitor_t m; seq_sink_t out;
    uint32_t new_input=old_input ^ 1u;
    init(&s,&m,&out);
    CHECK(recv(&s,0xfau,old_input));
    CHECK(recv(&s,0xf8u,old_input));
    render(&s,&out,1u);
    CHECK(s.playing && m.steps==1u && m.held==1u);
    for (unsigned i=0;i<6u;i++) {
        CHECK(recv(&s,0xf8u,old_input));
        render(&s,&out,1u);
    }
    CHECK(m.steps==2u && m.last_step==1u);
    CHECK(recv(&s,0xf8u,old_input)); /* orphaned F8 from previous source */
    CHECK(recv(&s,command,old_input)); /* accepted remote transport */
    CHECK(s.req_rt && (command==0xfau?s.req_start:s.req_cont));
    seq_clk_drop(&s,old_input);
    CHECK(recv(&s,0xf8u,new_input));
    CHECK(s.ext_pending==1u && s.req_prior_ticks==0u);
    CHECK(s.req_rt && (command==0xfau?s.req_start:s.req_cont));
    unsigned was=m.steps,off=m.off;
    render(&s,&out,1u);
    CHECK(s.playing && m.steps==was+1u);
    CHECK(m.off>off && m.on==m.off+1u); /* prior notes ended, one remains held */
    CHECK(s.t[TRK_BASS0].ext_ticks==0u);
    CHECK(m.last_step==(command==0xfau?0u:2u));
    CHECK(m.held==1u && m.on==m.off+1u);
    CHECK(!m.duplicate && !m.orphan);
    for (unsigned i=0;i<5u;i++) {
        CHECK(recv(&s,0xf8u,new_input));
        render(&s,&out,1u);
        CHECK(m.steps==was+1u); /* no premature next step */
    }
    CHECK(recv(&s,0xf8u,new_input));
    render(&s,&out,1u);
    CHECK(m.steps==was+2u); /* one six-clock step */
    CHECK(!recv(&s,0xfcu,old_input));
    CHECK(recv(&s,0xfcu,new_input));
    render(&s,&out,1u);
    CHECK(!s.playing && m.held==0u && m.on==m.off);
    CHECK(!m.duplicate && !m.orphan && !m.clock_echo && !m.transport_echo);
}


/* A *local* STOP is independent of the remote clock source. If USB/BLE drops
 * after its queued F8 but before audio applies local STOP, the replacement
 * input must not cancel the gesture or leave the 303 note playing. This is
 * deliberately different from a remote Stop accepted before disconnection. */
static void pending_local_stop_handoff(uint32_t old_input)
{
    seq_t s; monitor_t m; seq_sink_t out;
    uint32_t new_input=old_input ^ 1u;
    init(&s,&m,&out);
    CHECK(recv(&s,0xfau,old_input));
    CHECK(recv(&s,0xf8u,old_input));
    render(&s,&out,1u);
    CHECK(s.playing && m.steps==1u && m.on==1u && m.held==1u);
    for(unsigned i=0;i<4u;i++) {
        CHECK(recv(&s,0xf8u,old_input));
        render(&s,&out,1u);
    }
    CHECK(s.t[TRK_BASS0].ext_ticks==4u);
    CHECK(recv(&s,0xf8u,old_input)); /* old-source tick awaits audio */
    CHECK(s.ext_pending==1u);
    seq_stop(&s); /* user's STOP gesture, not incoming remote Stop */
    CHECK(s.req_stop && !s.req_stop_rt);
    seq_clk_drop(&s,old_input);
    CHECK(recv(&s,0xf8u,new_input));
    CHECK(s.req_stop && !s.req_stop_rt && s.ext_pending==1u);
    CHECK(!recv(&s,0xfau,old_input)); /* stale remote Start cannot supersede */
    render(&s,&out,1u);
    CHECK(!s.playing && m.steps==1u && m.on==1u && m.off==1u);
    CHECK(m.held==0u && !m.duplicate && !m.orphan);
    CHECK(recv(&s,0xfau,new_input));
    CHECK(recv(&s,0xf8u,new_input));
    render(&s,&out,1u);
    CHECK(s.playing && m.steps==2u && m.last_step==0u);
    CHECK(m.on==2u && m.off==1u && m.held==1u);
    CHECK(recv(&s,0xfcu,new_input));
    render(&s,&out,1u);
    CHECK(!s.playing && m.held==0u && m.on==m.off);
    CHECK(!m.duplicate && !m.orphan && !m.clock_echo && !m.transport_echo);
}


/* Regression: an input may unplug and reconnect as the SAME source ID before
 * the audio callback applies a pending Start, Continue or local PLAY. Dropping
 * its old F8 queue must drop the saved request epoch too, otherwise the first
 * F8 from the reconnected device is consumed as an OLD tick: one missing
 * downbeat and a held 303 note that never starts when expected. */
static void same_owner_reconnect_epoch(uint32_t input,unsigned transport)
{
    seq_t s; monitor_t m; seq_sink_t out;
    init(&s,&m,&out);
    CHECK(!s.playing);
    CHECK(recv(&s,0xf8u,input));                 /* old tick queued */
    CHECK(s.ext_pending==1u);
    if (transport==0u)
        seq_start(&s);                          /* local PLAY */
    else if (transport==1u)
        CHECK(recv(&s,0xfau,input));             /* remote Start */
    else
        CHECK(recv(&s,0xfbu,input));             /* remote Continue */
    CHECK(s.req_start || s.req_cont);
    CHECK(s.req_prior_ticks==1u);
    seq_clk_drop(&s,input);                     /* USB/BLE disconnect */
    CHECK(s.clk_lock==0u && s.ext_pending==0u);
    CHECK(s.req_start || s.req_cont);           /* accepted gesture survives */
    CHECK(recv(&s,0xf8u,input));                 /* SAME endpoint ID returns */
    CHECK(s.clk_lock==(uint8_t)(input+1u) && s.ext_pending==1u);
    CHECK(s.req_prior_ticks==0u);               /* obsolete boundary discarded */
    render(&s,&out,1u);
    CHECK(s.playing && m.steps==1u && m.last_step==0u); /* first new tick = downbeat */
    CHECK(m.on==1u && m.off==0u && m.held==1u);
    CHECK(!m.duplicate && !m.orphan);
    for (unsigned i=0;i<5u;i++) {
        CHECK(recv(&s,0xf8u,input)); render(&s,&out,1u);
        CHECK(m.steps==1u);                     /* never an early second step */
    }
    CHECK(recv(&s,0xf8u,input));render(&s,&out,1u);
    CHECK(m.steps==2u && m.last_step==1u);     /* sixth FOLLOWING tick = next step */
    seq_stop(&s);render(&s,&out,1u);             /* local STOP releases 303 */
    CHECK(!s.playing && m.held==0u && m.on==m.off);
    CHECK(!m.duplicate && !m.orphan && !m.clock_echo && !m.transport_echo);
}

int main(void)
{
    const unsigned blocks[]={1u,64u,1024u};
    for(unsigned i=0;i<3u;i++) {
        block_size=blocks[i];scenario=i+1u;midstep_disconnect();
        scenario=4u+i;queued_clock_disconnect(0u);
        scenario=7u+i;queued_clock_disconnect(1u);
        scenario=10u+i;pending_local_play_handoff(0u);
        scenario=13u+i;pending_local_play_handoff(1u);
        scenario=70u+i*2u;pending_local_stop_handoff(0u);
        scenario=71u+i*2u;pending_local_stop_handoff(1u);
        for(unsigned input=0u;input<2u;input++)
            for(unsigned transport=0u;transport<3u;transport++) {
                scenario=90u+i*6u+input*3u+transport;
                same_owner_reconnect_epoch(input,transport);
            }
        for(unsigned old_input=0u;old_input<2u;old_input++)
            for(unsigned command=0u;command<2u;command++) {
                scenario=40u+i*4u+old_input*2u+command;
                valid_start_continue_during_handoff(old_input,command?0xfbu:0xfau);
            }
        for(unsigned old_input=0u;old_input<2u;old_input++)
            for(unsigned restart=0u;restart<3u;restart++) {
                scenario=16u+i*6u+old_input*3u+restart;
                valid_stop_during_handoff(old_input,restart);
            }
    }
    fprintf(stderr,"seq link-drop clock handoff: %u/%u checks %s\n",checks-failures,checks,failures?"FAILED":"PASSED");
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
