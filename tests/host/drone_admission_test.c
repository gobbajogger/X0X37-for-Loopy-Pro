/* SPDX-License-Identifier: GPL-3.0-only */
/* The alternate provider includes the real engine implementation only in this
 * test translation unit. It exposes observations, not replacement DSP/recorder. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1
#endif
#include <stdint.h>
typedef struct { int seq_note; unsigned held, count, src, accent, slide, attacked; } admission_state;
#if defined(DRONE_ADMISSION_PROVIDER)
#include "../../firmware/src/app/engine.c"
admission_state admission_read(int p) {
 return (admission_state){original_seq_note[p],bass_held[p],bass_held_count[p],bass_held_src[p],bass_accent[p],bass_slide[p],bass_attacked[p]};
}
int admission_sequence(int p,int note,int accent,int slide,int velocity) {return s_bon(0,p,note,accent,slide,velocity);}
void admission_clear_sequence(int p) {s_boff(0,p);}
void admission_fault(int p) {original_bass[p].status=ORIGINAL_RUNTIME_FAULT;}
void admission_saturate(int p,int note) {original_bass[p].keys[ORIGINAL_INPUT_KEYS][note]=UINT8_MAX;}
#else
/* Actual MIDI/keyboard command -> engine recorder -> production sequencer regression.
 * Adapted timestamped lifecycle proof from 6496003d onto current source.
 * Disconnected SMC is a platform boundary, not a substituted recorder. */
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1 /* expose the simulator's libproc instrumentation declarations */
#endif
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
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
int smc_midi_host(uint32_t pkt) { (void)pkt; return 0; }
/* Presentation lives in smc_mixer.c; this fixture is disconnected SMC / live-rec only. */
void smc_ui_cancel(void) {}
int smc_ui_active(void) { return 0; }
void smc_ui_draw(void) {}
void smc_ui_clip_show(int n, const char *name, const char *value)
{ (void)n; (void)name; (void)value; }
#endif


#include "../../firmware/src/app/original_runtime.h"
admission_state admission_read(int);
int admission_sequence(int,int,int,int,int);
void admission_clear_sequence(int);
void admission_fault(int);
void admission_saturate(int,int);
static unsigned checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) {fails++; printf("FAIL line%d: ",__LINE__);printf(__VA_ARGS__);puts("");} } while(0)
static void block(void) {int32_t audio[2];engine_render(audio,1);}
static int same(admission_state a,admission_state b) {
 return a.seq_note==b.seq_note&&a.held==b.held&&a.count==b.count&&a.src==b.src&&a.accent==b.accent&&a.slide==b.slide&&a.attacked==b.attacked;
}
static void kind(int p,int k) {
 seq_stop(&seq);block();
 CHECK(original_sound_init(proj.sound.v[T_303][p],k,0),"known archive kind");
 engine_bass_sound_refresh(&proj.sound,p);engine_apply_sound(&proj.sound);block();
 CHECK(engine_bass_bank(p)==k,"exact stored/runtime kind retained");
}
static void refused(int p,int expected_status) {
 CHECK(engine_bass_status(p)==expected_status,"explicit runtime refusal status");
 CHECK(!seq_bass_poly(&seq,p),"unavailable/faulted slot is not admitted as live poly");
 CHECK(engine_original_value_bits(p,0)==UINT32_MAX,"unavailable host diagnostic sentinel");
 admission_state before=admission_read(p);
 CHECK(!admission_sequence(p,61,1,1,100),"sequence onset refused");
 CHECK(same(before,admission_read(p)),"refused sequence does not claim new cache/flags");
 CHECK(!(engine_bass_keys_held()&(1<<p)),"refused sequence has no DSP keys");
 engine_bass_on_velocity(p,62,1,0,100);block();
 CHECK(same(before,admission_read(p)),"refused C_BON preserves admission ownership/flags");
 for(int i=0;i<NPAT;i++) pattern_init(&proj.pat[i],1,2);
 seq_init(&seq,proj.pat);seq.bpm=120;seq.clk_src=SEQ_CLK_INT;seq.send_notes=seq.send_clock=0;
 seq.rec_mask=(uint8_t)(1u<<p);seq_start(&seq);block();
 seq_bass_rec_t rec=seq.rec_bass[p];bpart_t pattern=proj.pat[0].bass[p];before=admission_read(p);
 engine_bass_record(p,64,95,BS_ACCENT|BS_SLIDE);block();
 CHECK(!memcmp(&rec,&seq.rec_bass[p],sizeof rec),"refused C_REC writes no recorder/press ownership");
 CHECK(!memcmp(&pattern,&proj.pat[0].bass[p],sizeof pattern),"refused C_REC writes no pattern notes");
 CHECK(same(before,admission_read(p)),"refused C_REC leaves live admission state unchanged");
 seq_stop(&seq);block();
}
static void accepted(int p,int k) {
 kind(p,k);CHECK(engine_bass_status(p)==ORIGINAL_RUNTIME_OK,"available kind initialized");
 CHECK(seq_bass_poly(&seq,p)==(k==ORIGINAL_SOUND_JUNO||k==ORIGINAL_SOUND_DRONE),"available kind mono/poly classification");
 CHECK(engine_original_value_bits(p,0)!=UINT32_MAX,"available host diagnostic readable");
 admission_state invalid_before=admission_read(p);
 CHECK(!admission_sequence(p,128,1,1,100),"out-of-range sequencer note refused");
 CHECK(!admission_sequence(p,60,1,1,128),"out-of-range sequencer velocity refused");
 CHECK(!admission_sequence(p,60,1,1,0),"zero-velocity onset refused");
 CHECK(same(invalid_before,admission_read(p)),"invalid sequence input preserves prior admission state");
 engine_bass_on_velocity(p,128,1,1,100);block();
 CHECK(same(invalid_before,admission_read(p)),"invalid C_BON changes no accepted press");
 CHECK(admission_sequence(p,60,1,0,100),"available sequence accepted");
 CHECK(admission_read(p).seq_note==60,"accepted sequence claims exact cache note");
 CHECK(engine_bass_keys_held()&(1<<p),"accepted sequence key owned");admission_clear_sequence(p);
 engine_bass_on_velocity(p,62,1,0,100);block();admission_state live=admission_read(p);
 CHECK(live.held==62&&live.count==1&&live.src==3,"accepted C_BON live owner");
 CHECK(live.accent==(k==ORIGINAL_SOUND_CHICAGO)&&!live.slide,"retained accent/no-slide policy");
 engine_bass_off(p);block();
 for(int i=0;i<NPAT;i++) pattern_init(&proj.pat[i],1,2);
 seq_init(&seq,proj.pat);seq.bpm=120;seq.clk_src=SEQ_CLK_INT;seq.send_notes=seq.send_clock=0;
 seq.rec_mask=(uint8_t)(1u<<p);seq_start(&seq);block();
 seq_bass_rec_t before_rec=seq.rec_bass[p];bpart_t before_pattern=proj.pat[0].bass[p];
 admission_state before_live=admission_read(p);
 engine_bass_record(p,128,90,BS_ACCENT|BS_SLIDE);block();
 CHECK(!memcmp(&before_rec,&seq.rec_bass[p],sizeof before_rec),"invalid C_REC changes no recorder owner");
 CHECK(!memcmp(&before_pattern,&proj.pat[0].bass[p],sizeof before_pattern),"invalid C_REC writes no pattern");
 CHECK(same(before_live,admission_read(p)),"invalid C_REC changes no accepted press");
 engine_bass_record(p,65,90,BS_ACCENT|BS_SLIDE);block();
 CHECK(seq.rec_bass[p].active&&seq.rec_bass[p].own_ok&&seq.rec_bass[p].own_note==65,"accepted recorder opens real onset");
 CHECK(engine_bass_keys_held()&(1<<p),"accepted recorder has runtime key before storage");
 CHECK(proj.pat[seq.rec_bass[p].pat].bass[p].step[seq.rec_bass[p].first].velocity==90,"manual slide metadata does not inflate key velocity");
 engine_bass_record(p,67,85,0);block();
 CHECK(seq.rec_bass[p].active&&seq.rec_bass[p].own_note==67,"accepted overlapping recorder onset");
 if(seq_bass_poly(&seq,p)) {uint8_t notes[6];int n=seq_bass_chord_get(&seq,p,seq.rec_bass[p].pat,seq.rec_bass[p].first,notes);
  CHECK(n==2&&notes[0]==65&&notes[1]==67,"poly recorder sees accepted chord keys");}
 engine_bass_off(p);block();CHECK(!seq.rec_bass[p].active,"recorded release closes endpoint");
 seq_stop(&seq);block();
 /* Exercise a real key-counter saturation refusal, not a replaced runtime. */
 kind(p,k);for(int i=0;i<NPAT;i++) pattern_init(&proj.pat[i],1,2);
 seq_init(&seq,proj.pat);seq.bpm=120;seq.clk_src=SEQ_CLK_INT;seq.rec_mask=(uint8_t)(1u<<p);seq.send_notes=seq.send_clock=0;
 seq_start(&seq);block();
 before_rec=seq.rec_bass[p];before_pattern=proj.pat[0].bass[p];before_live=admission_read(p);
 admission_saturate(p,69);engine_bass_record(p,69,100,0);block();
 CHECK(engine_bass_status(p)==ORIGINAL_RUNTIME_FAULT,"actual runtime key saturation faults");
 CHECK(!memcmp(&before_rec,&seq.rec_bass[p],sizeof before_rec),"actual key refusal writes no recorder owner");
 CHECK(!memcmp(&before_pattern,&proj.pat[0].bass[p],sizeof before_pattern),"actual key refusal writes no pattern");
 CHECK(same(before_live,admission_read(p)),"actual key refusal changes no live admission");
 seq_stop(&seq);block();
 kind(p,k); /* Retire the deliberately saturated fixture before the next case. */
}
int main(void) {
 boot();block();
 CHECK(!seq_bass_poly(&seq,-1)&&!seq_bass_poly(&seq,NBASS),"poly query validates slot bounds");
 CHECK(!admission_sequence(-1,60,0,0,100)&&!admission_sequence(NBASS,60,0,0,100),"sequence validates slot bounds");
 for(int p=0;p<NBASS;p++) {
  kind(p,ORIGINAL_SOUND_DRONE);
#if X0X_DRONE_RUNTIME_ENABLED
  accepted(p,ORIGINAL_SOUND_DRONE);
#else
  refused(p,ORIGINAL_RUNTIME_UNAVAILABLE);
#endif
  accepted(p,ORIGINAL_SOUND_JUNO);
  admission_fault(p);refused(p,ORIGINAL_RUNTIME_FAULT);
  accepted(p,ORIGINAL_SOUND_CHICAGO);admission_fault(p);refused(p,ORIGINAL_RUNTIME_FAULT);
  accepted(p,ORIGINAL_SOUND_MIAMI);admission_fault(p);refused(p,ORIGINAL_RUNTIME_FAULT);
 }
 printf("DRONE_ADMISSION profile=%d checks=%u failures=%u\n",X0X_DRONE_RUNTIME_ENABLED,checks,fails);
 return fails?1:0;
}
#endif
