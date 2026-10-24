/* Scoped regression: seq_clk_take and seq_clk_drop projected from Git source.
 * Candidate PR #103 sequencer.c blob 09d9cea9936de02a6e410f80321d1c6c4b400f11.
 * HANDOVER_DROP: PR #35 blob 7345cab17c29ba3c26f58d1b2c042b05c1b55ef4.
 * STOP_TAKEOVER: PR #48 blob b02cf26e938fbc58dddcab455b119998e96ce744.
 * Not a full firmware compile or independent QA. See README.md.
 */
#include <stdint.h>
#include <stdio.h>
#define SEQ_LOCK_T 400u
#define SEQ_CLK_INT 0u
#define SEQ_CLK_AUTO 1u
#define SEQ_CLK_EXT 2u

typedef struct {
    uint8_t clk_src, clk_lock, clk_last, clk_stop;
    uint8_t ext_nb, ext_bi, ext_q, ext_skip;
    uint32_t ext_now, clk_stop_at, clk_lock_at, ext_pending;
} seq_t;
/* Adapted field declarations only. The function bodies below match candidate
 * sequencer.c, with exactly the flagged #35/#48 insertion shown in their diffs. */
int seq_clk_take(seq_t *s, uint8_t st, uint32_t src)
{
    uint8_t me = (uint8_t)(src + 1u);
    if (st != 0xF8u && st != 0xF2u && (st < 0xFAu || st > 0xFCu))
        return 1;
    if (s->clk_src == SEQ_CLK_INT) {
        s->clk_lock = 0;
        return 1;
    }
    if (s->clk_lock != me) {
        if (s->clk_lock && !(s->clk_stop && (st == 0xFAu || st == 0xFBu) &&
                             s->ext_now - s->clk_stop_at >= SEQ_LOCK_T))
            return 0;
        if (st == 0xFCu || st == 0xF2u)
            return 1;
        if (s->clk_last && s->clk_last != me) {
#ifdef STOP_TAKEOVER
            s->ext_pending = 0;
#endif
            s->ext_nb = s->ext_bi = s->ext_q = 0;
            s->ext_skip = 1;
        }
        s->clk_lock = s->clk_last = me;
        s->clk_stop = 0;
    }
    if (st == 0xF2u)
        return 1;
    if (st == 0xFCu) {
        s->clk_stop = 1;
        s->clk_stop_at = s->ext_now;
    } else {
        if (st != 0xF8u)
            s->clk_stop = 0;
        s->clk_lock_at = s->ext_now;
    }
    return 1;
}
void seq_clk_drop(seq_t *s, uint32_t src)
{
    if (s->clk_lock == src + 1u) {
        s->clk_lock = 0;
#ifdef HANDOVER_DROP
        s->ext_pending = 0;
#endif
    }
}

static int checks, fails;
#define CHECK(condition, label) do {checks++;if(!(condition)){fails++;printf("FAIL: %s\n",label);}} while(0)
static void clock_packet(seq_t *s, uint8_t st, uint32_t src) {
    if(seq_clk_take(s,st,src) && st==0xF8u) s->ext_pending++;
}
static seq_t init(uint8_t mode) {seq_t s={0};s.clk_src=mode;return s;}
static void disconnect_tests(uint8_t mode, uint32_t old, uint32_t other) {
    seq_t s=init(mode);
    clock_packet(&s,0xFAu,old);
    for(unsigned i=0;i<3;i++)clock_packet(&s,0xF8u,old);
    CHECK(s.clk_lock==old+1u,"setup: old source holds lock");
    CHECK(s.ext_pending==3u,"setup: three queued old clocks");
    seq_clk_drop(&s,old);
    CHECK(s.clk_lock==0u,"disconnect releases previous owner");
    CHECK(s.ext_pending==0u,"disconnect purges stale owner clocks");
    clock_packet(&s,0xFAu,other);
    CHECK(s.clk_lock==other+1u,"other source becomes owner");
    clock_packet(&s,0xF8u,other);
    CHECK(s.ext_pending==1u,"new owner starts at one clock");
    seq_clk_drop(&s,old);
    CHECK(s.clk_lock==other+1u,"old disconnect cannot revoke new owner");
    CHECK(s.ext_pending==1u,"old disconnect cannot purge new owner clocks");
}
static void graceful_stop_tests(uint8_t mode, uint32_t old, uint32_t other) {
    seq_t s=init(mode);
    clock_packet(&s,0xFAu,old);
    clock_packet(&s,0xFCu,old);
    s.ext_now=400u;
    for(unsigned i=0;i<6;i++)clock_packet(&s,0xF8u,old);
    CHECK(s.ext_pending==6u,"setup: stopped owner has six queued clocks");
    CHECK(s.clk_stop && s.clk_lock==old+1u,"setup: STOP grace maintained");
    CHECK(seq_clk_take(&s,0xFAu,other)==1,"other source Start after grace accepted");
    CHECK(s.clk_lock==other+1u,"other source takes over after grace");
    CHECK(s.ext_pending==0u,"STOP-grace takeover purges old clocks");
    clock_packet(&s,0xF8u,other);
    CHECK(s.ext_pending==1u,"new owner clocks counted from fresh epoch");
    CHECK(seq_clk_take(&s,0xFCu,old)==0,"old source Stop cannot stop new owner");
    CHECK(s.clk_lock==other+1u && s.ext_pending==1u,"foreign Stop unchanged");
}
static void nonowner_stop_and_early_takeover(uint8_t mode, uint32_t old, uint32_t other) {
    seq_t s=init(mode);
    clock_packet(&s,0xFAu,old);
    clock_packet(&s,0xF8u,old);
    CHECK(seq_clk_take(&s,0xFCu,other)==0,"foreign STOP rejected");
    CHECK(seq_clk_take(&s,0xF2u,other)==0,"foreign Song Position rejected");
    CHECK(seq_clk_take(&s,0xFAu,other)==0,"foreign START rejected before owner Stop");
    clock_packet(&s,0xFCu,old);s.ext_now=399u;
    CHECK(seq_clk_take(&s,0xFAu,other)==0,"no takeover before 400ms grace");
    CHECK(s.ext_pending==1u,"early takeover must not discard pending owner tick");
    CHECK(seq_clk_take(&s,0x80u,other)==1,"Note Off remains passthrough");
    CHECK(s.clk_lock==old+1u,"Note Off cannot change lock");
}
int main(void){
  const uint8_t modes[]={SEQ_CLK_AUTO,SEQ_CLK_EXT};
  for(unsigned m=0;m<2;m++)for(unsigned i=0;i<2;i++){
    uint32_t old=i?1u:0u,other=i?0u:1u;
    disconnect_tests(modes[m],old,other);
    graceful_stop_tests(modes[m],old,other);
    nonowner_stop_and_early_takeover(modes[m],old,other);
  }
  printf("RESULT %d/%d assertions PASS\n",checks-fails,checks);
  return fails?1:0;
}
