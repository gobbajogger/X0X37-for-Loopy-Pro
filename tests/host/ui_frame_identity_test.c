/* SPDX-License-Identifier: GPL-3.0-only */
/* Production UI/gfx/engine, using the existing SMC hardware fixture.
 * Catches shifted/dropped blits, altered row-cache updates and changed decay.
 * The Python runner supplies instrumented scratch includes and hashes events. */
#define SMC_UI_MAIN unused_smc_ui_main
#ifndef UI_FRAME_FIXTURE
#error Run through ui_frame_identity_test.py
#endif
#include UI_FRAME_FIXTURE
#include <unistd.h>
#include UI_FRAME_DECAY_FILE

static FILE *frame_trace;
static int frame_capture = 1;
static int frame_expect_silence;
static unsigned session_frame;

static void frame_event(unsigned kind, uint32_t x, uint32_t y, uint32_t w,
                        uint32_t h, const void *data, uint32_t bytes)
{
    if (!frame_capture) return;
    if (!frame_trace) {
        const char *fd = getenv("UI_FRAME_TRACE_FD");
        CHECK(fd);
        frame_trace = fdopen(atoi(fd), "wb");
        CHECK(frame_trace);
    }
    uint32_t values[7] = {kind, session_frame, x, y, w, h, bytes};
    unsigned char head[28];
    for (unsigned i = 0; i < 7; i++)
        for (unsigned b = 0; b < 4; b++) head[4*i+b] = (unsigned char)(values[i] >> (8*b));
    CHECK(fwrite(head, 1, sizeof head, frame_trace) == sizeof head);
    CHECK(!bytes || fwrite(data, 1, bytes, frame_trace) == bytes);
}
static void frame_pixels(unsigned kind, uint32_t x, uint32_t y, uint32_t w,
                         uint32_t h, const uint16_t *data)
{ frame_event(kind, x, y, w, h, data, w*h*2u); }

static void frame_observers(unsigned phase)
{
    /* Explicit fields avoid padding and volatile-qualifier casts. */
    uint16_t peak[NPARTS];
    uint8_t hit[NKIT][NDRUM];
    uint32_t scope_w = eng_scope_w;
    for (unsigned p = 0; p < NPARTS; p++) peak[p] = eng_peak[p];
    for (unsigned k = 0; k < NKIT; k++)
        for (unsigned v = 0; v < NDRUM; v++) hit[k][v] = eng_hit[k][v];
    frame_event('P', phase, 0, NPARTS, 1, peak, sizeof peak);
    frame_event('H', phase, 0, NDRUM, NKIT, hit, sizeof hit);
    frame_event('S', phase, scope_w, SCOPE_N, 1, eng_scope, sizeof eng_scope);
}
static void frame_audio(void)
{
    int32_t audio[512];
    engine_render(audio, 256);
    if (frame_expect_silence)
        for (unsigned i = 0; i < 512; i++) CHECK(audio[i] == 0);
    frame_event('A', 0, 0, 256, 2, audio, sizeof audio); /* USB tap */
    engine_volume(audio, 256);
    frame_event('V', 0, 0, 256, 2, audio, sizeof audio); /* physical output */
    frame_observers(0);
}
static void frame_draw(void)
{
    ui_frame();
    frame_observers(1);
    frame_pixels('F', 0, 0, 240, 240, fb);
    session_frame++;
}
static void frame_tick(void)
{
    now_ms += 16;
    for (unsigned i = 0; i < 3; i++) frame_audio();
    frame_draw();
}

static void test_decay(void)
{
    setup(V_HOME, PART_909);
    /* Every byte value on every voice; no audio ISR between store and check. */
    for (unsigned value = 0; value < 256; value++) {
        for (unsigned kit = 0; kit < NKIT; kit++)
            for (unsigned voice = 0; voice < NDRUM; voice++) eng_hit[kit][voice] = (uint8_t)value;
        eng_peak[0] = 65535;
        frame_draw();
        CHECK(eng_peak[0] == 57344);
        for (unsigned kit = 0; kit < NKIT; kit++)
            for (unsigned voice = 0; voice < NDRUM; voice++)
                CHECK(eng_hit[kit][voice] == value - value/8u - (value != 0));
    }
    /* Distinct cells expose skipped voices, wrong row stride and kit swaps. */
    for (unsigned kit = 0; kit < NKIT; kit++)
        for (unsigned voice = 0; voice < NDRUM; voice++) eng_hit[kit][voice] = (uint8_t)(kit*97u + voice*7u);
    frame_draw();
    for (unsigned kit = 0; kit < NKIT; kit++)
        for (unsigned voice = 0; voice < NDRUM; voice++) {
            unsigned v = (uint8_t)(kit*97u + voice*7u);
            CHECK(eng_hit[kit][voice] == v - v/8u - (v != 0));
        }
}

static void row_canvas(unsigned rows, unsigned change)
{
    cv_begin(240, rows, C_BLACK);
    for (unsigned y = 0; y < rows; y++)
        for (unsigned x = 0; x < 240; x++) cv_px[y*240+x] = (uint16_t)(y*29u + x*13u);
    if (change) cv_px[(change-1u) % (rows*240u)] ^= 0x1234;
}
static void test_rows(void)
{
    const unsigned heights[] = {MAIN_ROWS, KNOB_H};
    for (unsigned strip = 0; strip < 2; strip++) {
        unsigned rows = heights[strip], region = strip ? 3 : 1;
        uint32_t rh[MAIN_ROWS] = {0}, ch[MAIN_COLB] = {0};
        blit_hash[region] = 0;
        /* Force, unchanged, first/last pixel, column-block edges, disjoint rows. */
        const unsigned changes[] = {0,0,1,8,9,240,rows*240u,0,0,240u*17u+9u};
        for (unsigned n = 0; n < sizeof changes/sizeof *changes; n++) {
            row_canvas(rows, changes[n]);
            unsigned before = blits;
            cv_commit_rows(region, 0, strip ? KNOB_Y : MAIN_Y, rows, rh, ch);
            if (n == 1 || n == 8) CHECK(blits == before);
            frame_event('R', region, n, rows, 1, rh, rows*sizeof *rh);
            frame_event('C', region, n, MAIN_COLB, 1, ch, sizeof ch);
            frame_pixels('Q', 0, 0, 240, rows, cv_px); /* includes packed prefix */
        }
        /* Repeated reset must still send the entire strip. */
        row_canvas(rows, 0); blit_hash[region] = 0;
        unsigned before = blits;
        cv_commit_rows(region, 0, strip ? KNOB_Y : MAIN_Y, rows, rh, ch);
        CHECK(blits == before + 1);
    }
}

static void synthetic_break(void)
{
    /* Existing host slot/IMA machinery, deterministic user-slot content. */
    static int16_t pcm[22050];
    for (unsigned i = 0; i < 22050; i++) pcm[i] = (int16_t)((int)(i%200u)*120 - 12000);
    free(slot_data[0][0]);
    slot_data[0][0] = calloc(1, 11026); CHECK(slot_data[0][0]);
    ima_encode(pcm, 22050, slot_data[0][0]);
    slot_n[0][0] = 22050; slot_rate[0][0] = 22050; slot_nz[0] = 1;
    engine_brk_loops();
    CHECK(engine_brk_nslots() > 0);
}
static void popup_session(void)
{
    set_view(V_HOME); ui.touched = -1; ui.msg_until = 0;
    frame_draw(); memcpy(before_fb, fb, sizeof fb);
    screen_state_t before = screen_state();
    fader(0, 0); CHECK(smc_ui_active()); frame_draw();
    now_ms += 1200; frame_draw(); CHECK(!smc_ui_active());
    unchanged(before); CHECK(!memcmp(before_fb, fb, sizeof fb));
    knob(6, 1); CHECK(smc_ui_active()); frame_draw();
    now_ms += 1200; frame_draw(); CHECK(!smc_ui_active());
    message(0x90, 16, 127); CHECK(smc_ui_active()); frame_draw();
    message(0x80, 16, 0); now_ms += 1200; frame_draw(); CHECK(!smc_ui_active());
    /* The final #789 parent also exposes the pattern-cue activity popup. */
    message(0x90, 0x5C, 127); CHECK(smc_ui_active()); frame_draw();
    message(0x80, 0x5C, 0); now_ms += 1200; frame_draw(); CHECK(!smc_ui_active());
}
static void session(unsigned frames)
{
    for (unsigned palette = 0; palette <= 5; palette += 5) { /* dark + LIGHT */
        setup(V_HOME, PART_909);
        proj.set.palette = (uint8_t)palette; palette_set(palette);
        frame_expect_silence = 1;
        for (unsigned i = 0; i < 16; i++) frame_tick(); /* cold, proved silent idle */
        frame_expect_silence = 0;
        synthetic_break();
        CHECK(engine_bass_select_preset(0, 0, 0, &proj.sound) == 0);
        CHECK(engine_bass_select_preset(1, 1, 0, &proj.sound) == 0);
        for (unsigned i = 0; i < 48; i++) frame_audio();
        popup_session();
        /* Undo the popup's intentional mute before the all-parts workload. */
        seq.mute = 0;
        for (unsigned part = 0; part < NPARTS; part++)
            for (unsigned view = 0; view < NVIEWS; view++) {
                ui.part = (uint8_t)part; set_view(view); build_pages();
                unsigned pages = pg.n ? (unsigned)pg.n : 1;
                for (unsigned page = 0; page < pages; page++) {
                    *page_of(view) = (uint8_t)page; build_pages();
                    ui.msg_until = 0; frame_tick(); frame_draw();
                }
            }
        /* Every GLOBAL section, all scroll positions, and modal/help overlays. */
        set_view(V_GLO); open_global();
        for (int section = 0; section < 6; section++) {
            global_section(section);
            for (unsigned row = 0; row < ui.list_n; row++) {
                ui.list_sel = (uint8_t)row; ui.list_top = (uint8_t)(row > 4 ? row-4 : 0);
                frame_tick();
            }
            global_back();
        }
        ask(ACT_SONG_CLR, 0, "IDENTITY CONFIRMATION", "TEXT HEAVY OVERLAY"); frame_tick();
        close_overlay(); ui.help = B_EDIT+1; frame_tick(); ui.help = 0;
        /* Long playback with every part, dense hits, sweeps and preset fades. */
        set_view(V_MIX); seq_start(&seq);
        for (unsigned i = 0; i < frames; i++) {
            unsigned phase = i%512u, value = i%128u;
            for (int kit = 0; kit < NKIT; kit++)
                for (int voice = 0; voice < NDRUM; voice++)
                    if (!(i%4u)) engine_drum(kit, voice, (float)(32u+value*3u/4u)/127.0f);
            if (!(i%16u)) {
                engine_bass_on_velocity(0, 36+(int)(i%24u), 1, 0, 110);
                engine_bass_on_velocity(1, 48+(int)(i%12u), 0, 0, 100);
                engine_brk_live((int)(i/16u%8u), 1);
            }
            if (i%16u == 8u) { engine_bass_off(0); engine_bass_off(1); }
            for (unsigned part = 0; part < NPARTS; part++) {
                pref_set(PR(R_ENG,T_MIX,part,MX_LEVEL), (int)value);
                pref_set(PR(R_ENG,T_MIX,part,MX_PAN), 127-(int)value);
                pref_set(PR(R_ENG,T_MIX,part,MX_REV), (int)value);
                pref_set(PR(R_ENG,T_MIX,part,MX_DLY), (int)value);
            }
            engine_smc_solo(phase < 64 ? 1u << (i/512u%NPARTS) : 0);
            seq.mute = phase >= 64 && phase < 128 ? MUTE_MASK[i/512u%NPARTS] : 0;
            /* Modes 1..3, static and sweeping cutoff, limiter and compressor. */
            pref_set(PR(R_ENG,T_MST,0,MST_MODE), 1+(int)(i/128u%3u));
            pref_set(PR(R_ENG,T_MST,0,MST_CUTOFF), phase < 256 ? 64 : (int)value);
            pref_set(PR(R_ENG,T_MST,0,MST_LIMIT), (int)(i/128u%2u));
            pref_set(PR(R_ENG,T_MST,0,MST_COMP1), 80);
            pref_set(PR(R_USBFULL,0,0,0), (int)(i/128u%2u));
            master = i%128u < 64 ? 512 : 3584;
            if (phase == 256) CHECK(engine_bass_select_preset(1, 1, (int)(i/512u%4u), &proj.sound) == 0);
            if (i%128u == 0) { ui.part = (uint8_t)(i/128u%NPARTS); set_view((int)(i/128u%NVIEWS)); }
            frame_tick();
            if (i%1024u == 0) { char name[64]; snprintf(name,sizeof name,"opt-c-p%u-f%04u",palette,i); shot(name); }
        }
        engine_smc_solo(0); seq.mute = 0; seq_stop(&seq);
        engine_bass_off(0); engine_bass_off(1);
        for (int key = 0; key < 8; key++) engine_brk_live(key, 0);
        for (unsigned i = 0; i < 512; i++) frame_tick(); /* release/FX tails */
    }
}

static uint64_t frame_tsc(void)
{
#if defined(__x86_64__) || defined(__i386__)
    unsigned lo, hi;
    __asm__ volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi) :: "memory");
    return (uint64_t)hi << 32 | lo;
#else
    return 0;
#endif
}
static void timing(const char *name, unsigned iterations, int mode)
{
    uint32_t rh[MAIN_ROWS] = {0}, ch[MAIN_COLB] = {0};
    unsigned rows = mode == 2 || mode == 6 ? KNOB_H : MAIN_ROWS;
    row_canvas(rows, 0); blit_hash[1] = 0;
    cv_commit_rows(1, 0, MAIN_Y, rows, rh, ch);
    if (mode == 3 || mode == 5) {
        for (unsigned i = 0; i < 4; i++) blit_hash[i] = 0;
        for (unsigned i = 0; i < 8; i++) frame_tick();
    }
    double start = now_ns(); uint64_t ins = instr_now(), ticks = frame_tsc();
    for (unsigned i = 0; i < iterations; i++) {
        if (mode == 3) ui_frame();
        else if (mode == 4 || mode == 7) {
            if (mode == 7 && !(i%16u))
                for (unsigned kit = 0; kit < NKIT; kit++)
                    for (unsigned voice = 0; voice < NDRUM; voice++) eng_hit[kit][voice] = 255;
            frame_decay_probe();
        }
        else if (mode == 5) {
            if (!(i%16u)) {
                engine_drum(0,0,1.0f); engine_drum(1,0,1.0f);
                engine_bass_on_velocity(0,48,0,0,110); engine_bass_on_velocity(1,55,0,0,110);
                engine_brk_live(0,1);
            }
            frame_tick();
        }
        else {
            if (mode == 1 || mode == 6) cv_px[0] ^= 1;
            cv_commit_rows(1, 0, MAIN_Y, rows, rh, ch);
        }
    }
    uint64_t elapsed_ticks = frame_tsc()-ticks, elapsed_ins = instr_now()-ins;
    double ns = now_ns()-start;
    printf("{\"case\":\"%s\",\"calls\":%u,\"ns\":%.0f,\"host_instructions\":%llu,\"x86_tsc_ticks\":%llu}\n",
           name, iterations, ns, (unsigned long long)elapsed_ins, (unsigned long long)elapsed_ticks);
}
int main(void)
{
    const char *bench = getenv("UI_FRAME_BENCH");
    if (bench) {
        frame_capture = 0; setup(V_HOME, PART_909);
        timing("main-unchanged",2000,0); timing("main-changing",2000,1);
        timing("knobs-unchanged",2000,2); timing("knobs-changing",2000,6);
        timing("decay-zero",200000,4); timing("decay-active",200000,7);
        timing("ui-frame-idle",1000,3);
        synthetic_break(); set_view(V_MIX); seq_start(&seq);
        timing("ui-frame-playback",500,5);
        return 0;
    }
    test_decay(); test_rows();
    const char *frames = getenv("UI_FRAME_FRAMES");
    if (!getenv("UI_FRAME_SHORT")) session(frames ? (unsigned)strtoul(frames,0,10) : 4096);
    CHECK(frame_trace); CHECK(!fclose(frame_trace));
    printf("UI frame identity fixture: %u checks, %u frames\n",checks,session_frame);
    return 0;
}
