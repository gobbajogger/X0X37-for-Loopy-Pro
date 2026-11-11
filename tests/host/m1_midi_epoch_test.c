/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact production-function extraction; IRQ/MMIO is the test boundary. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned checks, failures;
#define CHECK(e) do { ++checks; if (!(e)) { ++failures; \
    printf("FAIL line %d: %s\n", __LINE__, #e); } } while (0)

#if !defined(TEST_HOST) && !defined(FELUCCA_LOADER)
enum { FM1_IRQ_ALNK0 = 11, FM1_IRQ_TIMER5 = 63 };
static uint32_t irq_cfg[16];
static unsigned irq_global = 1u, context_priority, hook_count;
static unsigned scheduled_hook, pending_source, pending_kind, injected;
static unsigned save_audio_count, save_timer_count;
static uint32_t injected_packet, injected_epoch, injected_cut;
static int injected_found;
static void irq_point(void);
static uint32_t irq_bit(uint32_t n) { return 1u << ((n & 7u) * 4u); }
static uint32_t fm1_irq_mask_save(uint32_t n)
{
    uint32_t enabled;
    irq_point();
    enabled = irq_cfg[n / 8u] & irq_bit(n);
    irq_cfg[n / 8u] &= ~irq_bit(n);
    if (n == FM1_IRQ_ALNK0) ++save_audio_count;
    if (n == FM1_IRQ_TIMER5) ++save_timer_count;
    irq_point();
    return enabled;
}
static void fm1_irq_mask_restore(uint32_t n, uint32_t state)
{
    irq_point();
    irq_cfg[n / 8u] = (irq_cfg[n / 8u] & ~irq_bit(n)) | state;
    irq_point();
}
#ifdef TEST_INSTRUMENTED
static void test_publish(void) { __asm__ volatile("" ::: "memory"); irq_point(); }
#endif
#endif

#include "m1_epoch_production.h"

#if defined(TEST_HOST)
#define INPUT min_q
static int ingress(uint32_t pkt) { host_midi_put(pkt); return 1; }
#else
#define INPUT midi_in_q
static int ingress(uint32_t pkt) { return midi_in_put(pkt); }
#include "m1_epoch_baseline_ring.h"
#endif

static uint32_t packet(uint32_t cin, uint32_t src, uint32_t status,
                       uint32_t d1, uint32_t d2)
{
    return cin | src << 4 | status << 8 | d1 << 16 | d2 << 24;
}
static uint32_t cc(uint32_t n) { return packet(11u, 0u, 0xb0u, n & 127u, n % 127u); }
static uint32_t on(uint32_t n) { return packet(9u, 0u, 0x90u, n & 127u, 91u); }
static uint32_t off(uint32_t n) { return packet(8u, 0u, 0x80u, n & 127u, 0u); }

static void reset(uint32_t start)
{
    memset(INPUT, 0, sizeof INPUT);
    mi_w = mi_r = start;
#ifndef FELUCCA_LOADER
    memset(mi_fresh, 0, sizeof mi_fresh);
    mi_epoch = 1u;
#if !defined(TEST_HOST)
    for (unsigned i = 0; i < 16u; ++i) irq_cfg[i] = 0x99999999u;
    irq_global = 1u; context_priority = 0u; hook_count = 0u;
    scheduled_hook = pending_source = pending_kind = injected = 0u;
    save_audio_count = save_timer_count = 0u;
    injected_packet = injected_epoch = injected_cut = 0u; injected_found = 0;
#endif
#endif
}

#ifndef FELUCCA_LOADER
static void expect(uint32_t pkt, uint32_t epoch)
{
    uint32_t got = 0xdeadbeefu, tag = 0xabcdef01u;
    CHECK(plat_midi_in_epoch(&got, &tag) == 1);
    CHECK(got == pkt); CHECK(tag == epoch);
}

static void basic_contract(void)
{
    uint32_t pkt = 0xaau, tag = 0xbbu, mark, cut;
    reset(UINT32_MAX - 7u);
    CHECK(plat_midi_in_epoch(&pkt, &tag) == 0);
    CHECK(pkt == 0xaau && tag == 0xbbu);
    CHECK(!plat_midi_pending());
    CHECK(ingress(cc(3u)) == 1);
    mark = plat_midi_mark(); CHECK(mark == UINT32_MAX - 6u);
    CHECK(!plat_midi_passed(mark));
    cut = plat_midi_cut(); CHECK(cut != 0u);
    CHECK(ingress(on(64u)) == 1);
    expect(cc(3u), 0u); CHECK(plat_midi_passed(mark));
    expect(on(64u), cut); CHECK(!plat_midi_pending());
    CHECK(ingress(off(64u)) == 1);
    CHECK(plat_midi_in(&pkt) == 1); CHECK(pkt == off(64u));
    CHECK(plat_midi_in(&pkt) == 0); CHECK(pkt == off(64u));
}

static void all_sources_controls(void)
{
    for (uint32_t src = 0u; src < 4u; ++src) {
        for (uint32_t ch = 0u; ch < 16u; ++ch) {
            uint32_t events[] = {
                packet(9u,src,0x90u|ch,60u,99u), packet(8u,src,0x80u|ch,60u,0u),
                packet(9u,src,0x90u|ch,60u,0u), packet(11u,src,0xb0u|ch,64u,127u),
                packet(14u,src,0xe0u|ch,1u,64u), packet(15u,src,0xf8u,0u,0u),
                packet(15u,src,0xfau,0u,0u), packet(15u,src,0xfbu,0u,0u),
                packet(15u,src,0xfcu,0u,0u), src << 4
            };
            reset(61u);
            for (unsigned i=0u;i<sizeof events/sizeof events[0];++i) CHECK(ingress(events[i]) == 1);
            uint32_t cut=plat_midi_cut();
            for (unsigned i=0u;i<sizeof events/sizeof events[0];++i) CHECK(ingress(events[i]) == 1);
            for (unsigned i=0u;i<sizeof events/sizeof events[0];++i) expect(events[i],0u);
            for (unsigned i=0u;i<sizeof events/sizeof events[0];++i) expect(events[i],cut);
        }
    }
}

static void repeated_cuts_and_wrap(void)
{
    reset(UINT32_MAX-3u);
    mi_epoch = UINT32_MAX-1u;
    CHECK(ingress(on(1u)) == 1);
    uint32_t cut=plat_midi_cut(); CHECK(cut == UINT32_MAX);
    CHECK(ingress(on(2u)) == 1);
    cut=plat_midi_cut(); CHECK(cut == 1u);
    CHECK(ingress(on(3u)) == 1);
    expect(on(1u),0u); expect(on(2u),0u); expect(on(3u),cut);
    for (unsigned round=0u;round<1000u;++round) {
        CHECK(ingress(on(round)) == 1);
        cut=plat_midi_cut(); CHECK(cut != 0u);
        expect(on(round),0u);
    }
    CHECK(ingress(on(55u)) == 1);
    cut=plat_midi_cut(); CHECK(ingress(on(56u)) == 1);
    expect(on(55u),0u); expect(on(56u),cut);
}
#endif

#if !defined(TEST_HOST)
static void egress_differential(void)
{
    uint32_t reference[MQ], rw, rr, seed=0x51a9u;
    reset(0u); memset(midi_out_q,0,sizeof midi_out_q);
    memset(reference,0,sizeof reference); mo_w=mo_r=rw=rr=UINT32_MAX-31u;
#ifndef FELUCCA_LOADER
    memset(mi_fresh,0xa5,sizeof mi_fresh);
#endif
    for (uint32_t i=0u;i<4096u;++i) {
        uint32_t p;
        seed=seed*1664525u+1013904223u;
        switch (seed%7u) {
        case 0u: p=cc(seed&127u); break;
        case 1u: p=on(seed&127u); break;
        case 2u: p=off(seed&127u); break;
        case 3u: p=packet(14u,(seed>>8)&3u,0xe0u|((seed>>12)&15u),seed&127u,(seed>>16)&127u);break;
        case 4u: p=packet(15u,0u,0xfau+seed%3u,0u,0u); break;
        case 5u: p=(seed&3u)<<4; break;
        default: p=packet(9u,0u,0x90u,seed&127u,0u); break;
        }
        int a=midi_ring_put_note_off(midi_out_q,&mo_w,&mo_r,p);
        int b=baseline_midi_ring_put_note_off(reference,&rw,&rr,p);
        CHECK(a == b); CHECK(mo_w == rw && mo_r == rr);
        CHECK(memcmp(midi_out_q,reference,sizeof reference) == 0);
        if (i%3u == 0u && rr != rw) { ++rr; ++mo_r; }
    }
#ifndef FELUCCA_LOADER
    for (unsigned i=0u;i<MQ;++i) CHECK(mi_fresh[i] == 0xa5u);
#endif
}

#ifndef FELUCCA_LOADER
static void ingress_differential(void)
{
    uint32_t reference[MQ], rw, rr, seed=0x7a3bu;
    reset(UINT32_MAX-31u); memset(reference,0,sizeof reference);rw=mi_w;rr=mi_r;
    for (uint32_t i=0u;i<8192u;++i) {
        uint32_t p;
        seed=seed*1664525u+1013904223u;
        switch (seed%7u) {
        case 0u: p=cc(seed&127u);break;
        case 1u: p=on(seed&127u);break;
        case 2u: p=off(seed&127u);break;
        case 3u: p=packet(14u,(seed>>8)&3u,0xe0u|((seed>>12)&15u),seed&127u,(seed>>16)&127u);break;
        case 4u: p=packet(15u,0u,0xfau+seed%3u,0u,0u);break;
        case 5u: p=(seed&3u)<<4;break;
        default: p=packet(9u,0u,0x90u,seed&127u,0u);break;
        }
        if (i%17u == 0u) (void)plat_midi_cut();
        int a=ingress(p),b=baseline_midi_ring_put_note_off(reference,&rw,&rr,p);
        CHECK(a == b);CHECK(mi_w == rw && mi_r == rr);
        CHECK(memcmp(INPUT,reference,sizeof reference) == 0);
        if (i%3u == 0u && rr != rw) {
            uint32_t got=0u,tag=0u;
            CHECK(plat_midi_in_epoch(&got,&tag) == 1);
            CHECK(got == reference[rr%MQ]);++rr;
            CHECK(mi_w == rw && mi_r == rr);
        }
    }
}

static void crossing(uint32_t start, uint32_t protected_event)
{
    uint32_t got=0u, tag=0u, fresh=on(67u), fresh_count=0u;
    reset(start);
    for (uint32_t i=0u;i<MQ;++i) CHECK(ingress(cc(i)) == 1);
    uint32_t mark=plat_midi_mark(), cut=plat_midi_cut();
    CHECK(mark == start+MQ); CHECK(!plat_midi_passed(mark));
    expect(cc(0u),0u);
    CHECK(ingress(fresh) == 1); CHECK(mi_w == mark+1u);
    CHECK(ingress(protected_event) == 1);
    CHECK(INPUT[(mark-1u)%MQ] == fresh);
    for (uint32_t i=2u;i<MQ;++i) expect(cc(i),0u);
    CHECK(!plat_midi_passed(mark));
    CHECK(plat_midi_in_epoch(&got,&tag) == 1);
    CHECK(got == fresh && tag == cut); ++fresh_count;
    CHECK(plat_midi_passed(mark)); expect(protected_event,cut);
    CHECK(fresh_count == 1u && !plat_midi_pending());
}

static void protected_and_drops(void)
{
    const uint32_t protects[] = {0x003c8008u,0x003c9009u,0x0000fa0fu,
        0x0000fb0fu,0x0000fc0fu,0u,0x10u,0x20u,0x30u};
    const uint32_t starts[] = {0u,61u,UINT32_MAX-31u};
    for (unsigned s=0u;s<sizeof starts/sizeof starts[0];++s)
        for (unsigned p=0u;p<sizeof protects/sizeof protects[0];++p)
            crossing(starts[s],protects[p]);
    reset(UINT32_MAX-31u);
    for (uint32_t i=0u;i<MQ;++i) CHECK(ingress(off(i)) == 1);
    uint32_t snapshot[MQ]; uint8_t tags[MQ];
    (void)plat_midi_cut(); memcpy(snapshot,INPUT,sizeof snapshot);memcpy(tags,mi_fresh,sizeof tags);
    uint32_t w=mi_w,r=mi_r;
    const uint32_t drops[] = {on(99u),cc(77u),off(55u),packet(14u,0u,0xe0u,4u,65u)};
    for (unsigned p=0u;p<sizeof drops/sizeof drops[0];++p) {
        CHECK(ingress(drops[p]) == 0); CHECK(mi_w == w && mi_r == r);
        CHECK(memcmp(snapshot,INPUT,sizeof snapshot) == 0);
        CHECK(memcmp(tags,mi_fresh,sizeof tags) == 0);
    }
    for (uint32_t i=0u;i<MQ;++i) expect(off(i),0u);
    /* No CC: protected arrivals compact the first non-kept event and retain
     * every other packet with its corresponding freshness. */
    reset(63u);
    CHECK(ingress(on(5u)) == 1); CHECK(ingress(off(5u)) == 1);
    for (uint32_t i=2u;i<MQ;++i) CHECK(ingress(on(i)) == 1);
    uint32_t cut=plat_midi_cut(); CHECK(ingress(off(88u)) == 1);
    expect(off(5u),0u);
    for (uint32_t i=2u;i<MQ;++i) expect(on(i),0u);
    expect(off(88u),cut);
}

static void bend_overwrites(void)
{
    uint32_t a=packet(14u,1u,0xe2u,1u,64u), b=packet(14u,2u,0xe2u,2u,64u);
    uint32_t c=packet(14u,1u,0xe3u,3u,64u), newest=packet(14u,1u,0xe2u,4u,64u);
    reset(62u); CHECK(ingress(a) == 1);CHECK(ingress(b) == 1);CHECK(ingress(c) == 1);CHECK(ingress(newest) == 1);
    for (uint32_t i=4u;i<MQ;++i) CHECK(ingress(cc(i)) == 1);
    uint32_t cut=plat_midi_cut(),w=mi_w,r=mi_r, final=0u;
    /* Many accepted in-place updates never advance a sequence counter. */
    for (uint32_t i=0u;i<200003u;++i) {
        final=packet(14u,1u,0xe2u,i&127u,(i>>7)&127u);
        CHECK(ingress(final) == 1); CHECK(mi_w == w && mi_r == r && mi_epoch == cut);
    }
    expect(a,0u); expect(b,0u); expect(c,0u); expect(final,cut);
    for (uint32_t i=4u;i<MQ;++i) expect(cc(i),0u);
    /* No matching bend: update the first CC without losing later controls. */
    reset(0u); CHECK(ingress(off(1u)) == 1);
    for (uint32_t i=1u;i<MQ;++i) CHECK(ingress(cc(i)) == 1);
    cut=plat_midi_cut(); CHECK(ingress(final) == 1);
    expect(off(1u),0u); expect(final,cut);
    for (uint32_t i=2u;i<MQ;++i) expect(cc(i),0u);
}

static void direct_link_end(void)
{
    for (uint32_t src=0u;src<4u;++src) {
        uint8_t state=1u;
        reset(UINT32_MAX-31u);
        for (uint32_t i=0u;i<MQ;++i) CHECK(ingress(cc(i)) == 1);
        midi_link_check(&state,0u,src); CHECK(state == 2u); CHECK(mi_w-mi_r == MQ);
        uint32_t cut=plat_midi_cut(); expect(cc(0u),0u);
        midi_link_check(&state,0u,src); CHECK(state == 0u); CHECK(mi_w-mi_r == MQ);
        for (uint32_t i=1u;i<MQ;++i) expect(cc(i),0u);
        expect(src<<4,cut);
        state=1u; midi_link_check(&state,0u,src); CHECK(state == 0u);
        (void)plat_midi_cut(); expect(src<<4,0u);
        state=0u; midi_link_check(&state,1u,src); CHECK(state == 1u); CHECK(!plat_midi_pending());
    }
}

/* Sources have their real IRQ numbers and priorities (ALNK0=3,TIMER5=4).
 * Hooks bracket only hardware source-mask operations and publication barriers.
 * Allowing ingress in an ALNK0 callback is conservative stress: the current
 * top-level TIMER5 dispatcher additionally gates MIDI ingress on !in_audio. */
static void irq_point(void)
{
    ++hook_count;
    if (!pending_kind || hook_count<scheduled_hook || !irq_global) return;
    unsigned prio=pending_source == FM1_IRQ_TIMER5 ? 4u : 3u;
    if (prio<=context_priority || !(irq_cfg[pending_source/8u]&irq_bit(pending_source))) return;
    unsigned kind=pending_kind, previous=context_priority;
    pending_kind=0u; ++injected; context_priority=prio;
    if (kind == 1u) { injected_epoch=mi_epoch; injected_found=ingress(injected_packet); }
    else if (kind == 2u) injected_found=plat_midi_in_epoch(&injected_packet,&injected_epoch);
    else if (kind == 3u) injected_cut=plat_midi_cut();
    else { uint8_t state=1u; injected_epoch=mi_epoch; midi_link_check(&state,0u,1u); injected_found=state == 0u; }
    context_priority=previous;
}

static void irq_state_restore(void)
{
    for (unsigned mask=0u;mask<8u;++mask) {
        uint32_t snapshot[16];
        reset(0u); irq_global=(mask>>2)&1u;
        if (!(mask&1u)) irq_cfg[FM1_IRQ_ALNK0/8u]&=~irq_bit(FM1_IRQ_ALNK0);
        if (!(mask&2u)) irq_cfg[FM1_IRQ_TIMER5/8u]&=~irq_bit(FM1_IRQ_TIMER5);
        memcpy(snapshot,irq_cfg,sizeof snapshot);
        (void)plat_midi_cut(); CHECK(memcmp(snapshot,irq_cfg,sizeof snapshot) == 0);
        CHECK(irq_global == ((mask>>2)&1u)); CHECK(save_audio_count == 1u && save_timer_count == 1u);
        uint32_t p=4u,e=5u; CHECK(plat_midi_in_epoch(&p,&e) == 0); CHECK(p == 4u && e == 5u);
        CHECK(memcmp(snapshot,irq_cfg,sizeof snapshot) == 0);
        CHECK(ingress(on(5u)) == 1); CHECK(memcmp(snapshot,irq_cfg,sizeof snapshot) == 0);
        expect(on(5u),mi_epoch); CHECK(memcmp(snapshot,irq_cfg,sizeof snapshot) == 0);
    }
}

static void capture_interleavings(void)
{
    for (unsigned kind=1u;kind<=4u;++kind) {
        for (unsigned at=1u;at<=MQ+12u;++at) {
            reset(0u); CHECK(ingress(cc(9u)) == 1);
            uint32_t old=mi_epoch;
            injected_packet=on(80u); pending_kind=kind;
            pending_source=kind == 2u ? FM1_IRQ_ALNK0 : FM1_IRQ_TIMER5;
            hook_count=0u; scheduled_hook=at;
            uint32_t cut=plat_midi_cut(); irq_point();
            if (pending_kind) { scheduled_hook=0u;irq_point(); }
            CHECK(injected == 1u);
            if (kind == 1u || kind == 4u) {
                expect(cc(9u),0u); CHECK(injected_found == 1);
                expect(kind == 4u ? 0x10u : on(80u),injected_epoch == old ? 0u : cut);
            } else if (kind == 2u) {
                CHECK(injected_found == 1 && injected_packet == cc(9u));
                CHECK(injected_epoch == old || injected_epoch == 0u);
                CHECK(!plat_midi_pending());
            } else {
                CHECK(injected_cut != cut); CHECK(mi_epoch == cut || mi_epoch == injected_cut);
                expect(cc(9u),0u);
            }
        }
    }
    /* Explicit race: TIMER5 captures c2 after c1 restores TIMER5 but before
     * c1 returns. The documented unguarded c1 token is already superseded. */
    reset(0u); hook_count=0u;scheduled_hook=6u;pending_source=FM1_IRQ_TIMER5;pending_kind=3u;
    uint32_t c1=plat_midi_cut(); CHECK(injected == 1u);CHECK(injected_cut != c1);CHECK(mi_epoch == injected_cut);
    /* Positive caller transaction: outer guards cover capture and publication.
     * This fixture publishes only a token, not an engine/PANIC implementation. */
    reset(0u); uint32_t a=fm1_irq_mask_save(FM1_IRQ_ALNK0),t=fm1_irq_mask_save(FM1_IRQ_TIMER5);
    pending_source=FM1_IRQ_TIMER5;pending_kind=3u;scheduled_hook=0u;
    uint32_t published=plat_midi_cut(); CHECK(injected == 0u && published == mi_epoch);
    uint32_t published_at_capture=published;
    fm1_irq_mask_restore(FM1_IRQ_TIMER5,t);fm1_irq_mask_restore(FM1_IRQ_ALNK0,a);
    CHECK(injected == 1u && injected_cut != published_at_capture);
}

static void dequeue_interleavings(void)
{
    for (unsigned at=1u;at<=9u;++at) {
        uint32_t fresh_bend=packet(14u,0u,0xe0u,3u,64u);
        reset(0u); CHECK(ingress(cc(1u)) == 1);
        CHECK(ingress(packet(14u,0u,0xe0u,1u,64u)) == 1);
        for (uint32_t i=2u;i<MQ;++i) CHECK(ingress(off(i)) == 1);
        uint32_t cut=plat_midi_cut();
        CHECK(ingress(fresh_bend) == 1);
        injected_packet=off(99u);pending_source=FM1_IRQ_TIMER5;pending_kind=1u;
        scheduled_hook=at;hook_count=0u;context_priority=3u;
        uint32_t pkt=0u,tag=0u;CHECK(plat_midi_in_epoch(&pkt,&tag) == 1);
        context_priority=0u;irq_point();if (pending_kind) {scheduled_hook=0u;irq_point();}
        CHECK(injected == 1u && injected_found == 1);
        CHECK((pkt == cc(1u) && tag == 0u) || (pkt == fresh_bend && tag == cut));
        CHECK(save_timer_count>=1u);
        unsigned seen=0u, fresh_seen=pkt == fresh_bend ? 1u : 0u;
        while (plat_midi_in_epoch(&pkt,&tag)) {
            if(pkt == off(99u)) {CHECK(tag == cut);++seen;}
            if(pkt == fresh_bend) {CHECK(tag == cut);++fresh_seen;}
        }
        CHECK(seen == 1u && fresh_seen == 1u);
    }
}
#endif
#endif

int main(void)
{
#ifdef FELUCCA_LOADER
    reset(0u); CHECK(ingress(on(1u)) == 1); CHECK(mi_w == 1u && mi_r == 0u);
#endif
#ifndef FELUCCA_LOADER
    basic_contract(); all_sources_controls(); repeated_cuts_and_wrap();
#if !defined(TEST_HOST)
    protected_and_drops();bend_overwrites();direct_link_end();ingress_differential();
    irq_state_restore();capture_interleavings();dequeue_interleavings();
#endif
#endif
#if !defined(TEST_HOST)
    egress_differential();
#endif
    printf("%s production MIDI epoch: checks=%u failures=%u\n",failures ? "FAIL" : "PASS",checks,failures);
    return failures ? 1 : 0;
}
