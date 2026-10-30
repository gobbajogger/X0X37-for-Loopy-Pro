/* SPDX-License-Identifier: GPL-3.0-only */
/* Full-product keybed -> REC storage -> sequencer/MIDI/PCM replay regression.
 * Baseline d633833a discards held duration; no manually authored ties.
 * Disconnected SMC stubs match the maintained whole-app recorder fixture. */
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1 /* expose the simulator's libproc instrumentation declarations */
#endif
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <unistd.h> /* POSIX unlink used by the temporary replay capture. */
#if BOARD_SMK37
static uint8_t test_clip_mode, test_clip_busy;
uint8_t smk_clip_mode_on(void) { return test_clip_mode; }
uint8_t smk_padcal_on(void) { return 0; }
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_flash_busy(void) { return test_clip_busy; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *last, uint32_t force) { (void)last; (void)force; return 0; }
uint8_t smk_clip_history_button(uint8_t button, uint8_t pressed, uint8_t patch_held)
{ (void)button; (void)pressed; (void)patch_held; return 0; }
uint8_t smk_clip_transport_button(uint8_t button, uint8_t pressed)
{ (void)button; (void)pressed; return 0; }
void smk_play_note_button(uint8_t pressed, uint8_t home_held) { (void)pressed; (void)home_held; }
void smk_apc_transport_led_set(uint8_t note, uint8_t velocity) { (void)note; (void)velocity; }
uint8_t smk_apc_transport_led_state(uint8_t note) { (void)note; return 0; }
void smk_apc_clock_tick(void) {}
void smk_apc_clock_start(void) {}
void smk_apc_clock_position(uint32_t sixteenths) { (void)sixteenths; }
void smk_ck_midi_stop(void) {}
int smc_connected(void) { return 0; }
/* This fixture models disconnected SMC; project_smc_runtime_test.c covers
 * the production SMC project-active/reset implementation. */
/* This link does not include smc_mixer.c. ui.c still asks whether an SMC solo holds the project. */
int smc_project_active(void) { return 0; }
void smc_project_reset(void) {}
void smc_frame(void) {}
void smc_poll(void) {}
void smc_pad_diag_boundary(void) {} /* disconnected SMC fixture */
int smc_midi_host(uint32_t pkt) { (void)pkt; return 0; }
/* Presentation lives in smc_mixer.c; this fixture is disconnected SMC / live-rec only. */
void smc_ui_cancel(void) {}
int smc_ui_active(void) { return 0; }
void smc_ui_draw(void) {}
void smc_ui_clip_show(int n, const char *name, const char *value)
{ (void)n; (void)name; (void)value; }
#endif

static int fail_count;
#define EXPECT(c,m) do { if (!(c)) { printf("FAIL: %s\n",m); fail_count++; } } while(0)
static int external_clock;
static double next_tick, next_event;
static void tick_ms(unsigned n) {
 while(n--) {
  while(external_clock && next_event <= now_ms) {
   while(next_tick <= next_event) {
    min_q[mi_w++ % MQ]=0x0fu | (uint32_t)0xf8u << 8;
    next_tick += 60000.0/120.0/24.0;
   }
   next_event += 11.0; /* BLE-style batches, as maintained rec_live_test models. */
  }
  run_ms(1);
 }
}
static void button(int b, int down) {
 if(down) held_btn |= 1u<<b; else held_btn &= ~(1u<<b);
 tick_ms(1);
}
static void key_edge(int k,int down) {
 if(down) held_keys |= 1u<<k; else held_keys &= ~(1u<<k);
 tick_ms(1);
}
static void target_step(int step) {
 unsigned guard=0;
 while(eng_step[TRK_BASS0] == step && guard++ <10000) tick_ms(1);
 while(eng_step[TRK_BASS0] != step && guard++ <10000) tick_ms(1);
 EXPECT(guard<10000,"real sequencer reaches requested recording cell");
 tick_ms(10);
}
typedef struct { bpart_t line; int gate_ms; uint32_t bytes,pcm_hash; float peak; } take_t;
static FILE *probe_tmp(void) {
 char path[]="build/host/rec-duration.XXXXXX"; int fd=mkstemp(path); FILE *f;
 if(fd<0) {perror("probe mkstemp");exit(2);}
 f=fdopen(fd,"w+");unlink(path);if(!f) {perror("probe fdopen");exit(2);}
 return f;
}
static uint32_t file_hash(FILE *f,uint32_t *bytes) {
 uint32_t h=2166136261u,n=0; int c;
 fflush(f); rewind(f); while((c=fgetc(f))!=EOF) { h=(h^(unsigned)c)*16777619u;n++; }
 *bytes=n; return h;
}
static take_t record_take(unsigned held_ms,int jitter) {
 take_t r; bpart_t *bp; char row[128]; unsigned t; int st,n,v,on=-1;
 external_clock=0; held_btn=held_keys=0; mi_r=mi_w=0;
 boot(); tick_ms(300);
 ui.part=PART_303A;set_view(V_SEQ);build_pages();
 bp=&cur_pat()->bass[0]; bp->len=16; memset(bp->step,0,sizeof bp->step);
 seq.bpm=120.0f; seq.clk_src=SEQ_CLK_INT;seq.send_notes=1;
 seq.mute=0;
 if(jitter) {seq.clk_src=SEQ_CLK_AUTO;external_clock=1;next_tick=now_ms;next_event=now_ms;}
 button(B_REC,1);button(B_REC,0);button(B_PLAY,1);button(B_PLAY,0);tick_ms(10);
 EXPECT(ui.rec && seq.playing,"actual REC/PLAY buttons enable live recording");
 target_step(4);key_edge(10,1);tick_ms(held_ms);key_edge(10,0);
 EXPECT(!kbd_held_note[0],"actual release clears live owner");
 button(B_PLAY,1);button(B_PLAY,0);button(B_REC,1);button(B_REC,0);tick_ms(400);
 r.line=*bp; r.gate_ms=-1;
 EXPECT(bstep_gate(&bp->step[4])==G_NOTE,"actual key press stores onset in expected cell");
 midi_log=probe_tmp();wav=probe_tmp();wav_frames=0;peak_out=0;
 button(B_PLAY,1);button(B_PLAY,0);tick_ms(2100);
 r.peak=peak_out;r.pcm_hash=file_hash(wav,&r.bytes);fclose(wav);wav=0;
 fflush(midi_log);rewind(midi_log);
 while(fgets(row,sizeof row,midi_log)) {
  if(sscanf(row,"%u ms: %x %x %x",&t,&st,&n,&v)!=4 || n!=kbd_note(10)) continue;
  if((st&0xf0)==0x90 && v && on<0)on=(int)t;
  else if(((st&0xf0)==0x80 || ((st&0xf0)==0x90&&!v)) && on>=0) {r.gate_ms=(int)t-on;break;}
 }
 fclose(midi_log);midi_log=0;
 button(B_PLAY,1);button(B_PLAY,0);tick_ms(400);
 printf("take clock=%s held=%u ms: replay_gate=%d ms, PCMbytes=%u hash=%08x peak=%f, cell4 note=%u flags=%u\n",jitter?"BLE-jitter":"internal",held_ms,r.gate_ms,r.bytes,r.pcm_hash,(double)r.peak,r.line.step[4].note,r.line.step[4].flags);
 EXPECT(r.gate_ms>0,"production MIDI sink emits replayed NoteOn/NoteOff pair");
 EXPECT(abs(r.gate_ms-(int)(held_ms+10))<=25,"actual release is preserved within endpoint/block/BLE batching resolution");
 EXPECT(r.bytes>300000 && r.peak>0.000001f,"production PCM replay contains rendered nonzero audio");
 return r;
}
int main(void) {
 take_t short_take,long_take; int jitter;
 load_force=0; /* Host wall time is not device CPU load; mirrors intended test-run setting. */
 for(jitter=0;jitter<2;jitter++) {
 short_take=record_take(30,jitter);long_take=record_take(300,jitter);
 EXPECT(memcmp(&short_take.line,&long_take.line,sizeof short_take.line)!=0,"different real held durations must survive into recorded pattern data");
 EXPECT(long_take.gate_ms-short_take.gate_ms>=200,"replay must preserve materially longer held-note duration instead of a fixed gate");
 }
 printf("rec_held_duration: %s\n",fail_count?"FAIL_BEFORE_FIX":"PASS");
 return fail_count!=0;
}
