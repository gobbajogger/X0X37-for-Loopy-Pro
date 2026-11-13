/* SPDX-License-Identifier: GPL-3.0-only */
/* Archived Chicago inserts on the compact FX2 owner: admission, FxType choices,
 * shared 4096 B arena, audible bounded tails, live BPM Delay tempo, reset. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define X0X_FX_BEATS_DEFINE
#include "chicago_fx2_compact.h"
#include "../../firmware/src/app/original_sound.h"

static double energy(const int16_t *x, unsigned n)
{ double e = 0; for (unsigned i = 0; i < n; ++i) e += (double)x[i] * x[i]; return e; }

/* First sample index after the impulse whose |L| exceeds 200. */
static unsigned first_echo(const int16_t *x, unsigned from, unsigned n)
{ for (unsigned i = from; i < n; ++i) if (x[i] > 200 || x[i] < -200) return i; return n; }

enum { N = 16384 };
static int16_t L[N], R[N];

static const original_chicago_fx2_kernel *pick(original_chicago_fx2_compact *s, unsigned type,
    unsigned e1, unsigned e2, int32_t tempo)
{
    original_chicago_fx2_cold_owner o = original_chicago_fx2_compact_owner(s);
    assert(o.supports(o.context, type));
    const original_chicago_fx2_kernel *k = o.select_cold(o.context, type);
    assert(k);
    k->begin(k->context); k->reset(k->context);
    k->parameter(k->context, 2, e1); k->parameter(k->context, 3, e2);
    k->tempo(k->context, tempo);
    return k;
}

static void run(const original_chicago_fx2_kernel *k)
{
    memset(L, 0, sizeof L); memset(R, 0, sizeof R);
    for (unsigned i = 0; i < 64; ++i) L[i] = R[i] = (int16_t)(i & 16 ? -12000 : 12000);
    for (unsigned i = 0; i < N; i += 128) k->process(k->context, L + i, R + i, 128);
}

/* A successful idle shortcut must advance the same state as ordinary silence.
 * Otherwise an Ensemble LFO/delay head resumes at the wrong phase on a note. */
static void idle_then_note(unsigned type)
{
    static original_chicago_fx2_compact a, b, before;
    static int16_t al[513], ar[513], bl[513], br[513];
    static const unsigned spans[] = {0,1,31,64,127,513};
    assert(original_chicago_fx2_compact_init(&a));
    assert(original_chicago_fx2_compact_init(&b));
    const original_chicago_fx2_kernel *ka=pick(&a,type,100,80,1200);
    const original_chicago_fx2_kernel *kb=pick(&b,type,100,80,1200);
    original_chicago_fx2_cold_owner owner=original_chicago_fx2_compact_owner(&a);
    for (unsigned j=0;j<sizeof spans/sizeof spans[0];++j) {
        unsigned n=spans[j]; int16_t constant[2]={123,456}; before=a;
        memset(al,0,sizeof al); memset(ar,0,sizeof ar);
        memset(bl,0,sizeof bl); memset(br,0,sizeof br);
        if (owner.advance_zero(owner.context,n,constant)) {
            for(unsigned i=0;i<n;++i) {al[i]=constant[0]; ar[i]=constant[1];}
        } else {
            assert(!memcmp(&a,&before,sizeof a)); /* refusal is transactional */
            assert(constant[0]==123 && constant[1]==456);
            ka->process(ka->context,al,ar,n);
        }
        kb->process(kb->context,bl,br,n);
        assert(!memcmp(al,bl,sizeof al) && !memcmp(ar,br,sizeof ar));
        assert(!memcmp(&a.arena,&b.arena,sizeof a.arena));
    }
    for(unsigned i=0;i<513;++i) {
        al[i]=bl[i]=(int16_t)(i<32?12000:0);
        ar[i]=br[i]=(int16_t)(i<32?-6000:0);
    }
    ka->process(ka->context,al,ar,513); kb->process(kb->context,bl,br,513);
    assert(!memcmp(al,bl,sizeof al) && !memcmp(ar,br,sizeof ar));
    assert(!memcmp(&a.arena,&b.arena,sizeof a.arena));
}

/* Independent stereo oracle: an equal mono wet addition cannot collapse each
 * channel's remaining dry signal. Low input amplitude avoids clipping. */
static void stereo_and_isolation(unsigned type)
{
    static original_chicago_fx2_compact a,b,saved;
    int16_t l[64],r[64];
    assert(original_chicago_fx2_compact_init(&a));
    assert(original_chicago_fx2_compact_init(&b));
    const original_chicago_fx2_kernel *k=pick(&a,type,100,80,1200);
    (void)pick(&b,type,100,80,1200); saved=b;
    for(unsigned i=0;i<64;++i) {l[i]=1200;r[i]=-600;}
    k->process(k->context,l,r,64);
    assert(!memcmp(&b,&saved,sizeof b)); /* another part's arena is untouched */
    const float difference=type==16?1800.0f:1800.0f*(27.0f/127.0f);
    for(unsigned i=0;i<64;++i)
        assert(fabsf((float)l[i]-(float)r[i]-difference)<=2.0f);
    saved=a;
    k->parameter(k->context,2,128);
    k->parameter(k->context,3,UINT32_MAX);
    assert(!memcmp(&a,&saved,sizeof a)); /* invalid edits cannot wrap */
}

int main(void)
{
    static original_chicago_fx2_compact s;
    setvbuf(stdout, 0, _IONBF, 0);
    static const unsigned types[5] = {3, 12, 13, 15, 16};
    static const char *const names[5] = {"ENSEM", "SH DLY", "BPM DL", "HALL", "GT REV"};
    assert(sizeof(chi_fx2_insert_arena) <= sizeof s.arena.history);
    assert(sizeof s.arena == 4096u);
    assert(original_chicago_fx2_compact_init(&s));
    for (unsigned t = 0; t < 32; ++t) {
        int want = t == 0 || t == 3 || t == 4 || t == 5 || t == 10 || t == 12 || t == 13 ||
            t == 15 || t == 16 || t == 18 || t == 22 || t == 24;
        assert(original_chicago_fx2_compact_supports(t) == want);
    }
    /* FxType 7..11 are the inserts; 0..6 keep their saved meaning. */
    assert(ORIGINAL_CHICAGO_FX_CHOICES == 12u);
    assert(original_sound_param(ORIGINAL_SOUND_CHICAGO, ORIGINAL_CHICAGO_FX_TYPE)->max == 11);
    assert(!strcmp(original_sound_fx_name(6), "EQ"));
    for (unsigned i = 0; i < 5; ++i) {
        float v; original_chicago_program p; original_chicago_routing r;
        assert(!strcmp(original_sound_fx_name(7 + i), names[i]));
        assert(original_sound_control_value(ORIGINAL_SOUND_CHICAGO, ORIGINAL_CHICAGO_FX_TYPE, 7 + i, &v));
        assert(original_chicago_program_init(&p, 0) && original_chicago_program_set(&p, 17, v) &&
               original_chicago_program_routing(&p, &r));
        assert(r.fx[1][0] == types[i]);
    }
    for (unsigned i = 0; i < 5; ++i) {
        const original_chicago_fx2_kernel *k = pick(&s, types[i], 100, 80, 1200);
        run(k);
        double tail = energy(L + 2048, N - 2048) + energy(R + 2048, N - 2048);
        double late = energy(L + 512, 4096) + energy(R + 512, 4096);
        for (unsigned j = 0; j < N; ++j) assert(L[j] == R[j]); /* centred in, centred out */
        printf("%-10s type %2u late-energy %.3g tail-energy %.3g first-echo %u\n", names[i], types[i], late, tail,
               first_echo(L, 96, N));
        if (types[i] != 3) assert(late > 1e6); /* delays and reverbs ring after the burst */
        /* reset clears the selected ring: silence in -> silence out */
        k->reset(k->context);
        memset(L, 0, sizeof L); memset(R, 0, sizeof R);
        k->process(k->context, L, R, 1024);
        assert(energy(L, 1024) == 0 && energy(R, 1024) == 0);
        idle_then_note(types[i]);
        stereo_and_isolation(types[i]);
    }
    /* BPM Delay follows live tempo without clearing its ring. */
    {
        const original_chicago_fx2_kernel *k = pick(&s, 13, 100, 0 /* 1/8 beat */, 1200);
        float d120 = s.arena.insert.bpm.delay;
        k->tempo(k->context, 2400);
        float d240 = s.arena.insert.bpm.delay;
        printf("BPM DELAY 1/8 beat: %.1f samples @120, %.1f @240\n", d120, d240);
        assert(fabsf(d120 - 0.125f * 0.5f * 44100.0f) < 1.0f);
        assert(fabsf(d240 - 0.125f * 0.25f * 44100.0f) < 1.0f);
        run(k);
        unsigned echo = first_echo(L, 96, N);
        printf("BPM DELAY first echo @240: %u\n", echo);
        assert(echo >= 1330 && echo <= 1460);
    }
    /* A newly selected original algorithm still constructs over the arena. */
    { const original_chicago_fx2_kernel *k = pick(&s, 4, 64, 64, 1200); run(k); }
    puts("chicago_fx2_inserts: ok");
    return 0;
}
