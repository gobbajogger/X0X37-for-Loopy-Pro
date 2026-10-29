/* Private derived original Chicago FX2 kernels. New source is PENDING_REVIEW.
 * No firmware pool, occupied sample/send memory, or automatic admission. */
#ifndef CHICAGO_FX_HISTORY_NEXT_H
#define CHICAGO_FX_HISTORY_NEXT_H
#include <stddef.h>
#include <stdint.h>
#define CHFX_MAX_BLOCK 4096u
#define CHFX_SHORT_SLOTS 32768u
#define CHFX_BPM_CHANNEL_SLOTS 130667u
#define CHFX_BPM_STEREO_SLOTS (2u*CHFX_BPM_CHANNEL_SLOTS)
#define CHFX_GATE_SLOTS 10439u
#define CHFX_HALL_SLOTS 30001u
/* Logical-zero history representation. Invalid bytes are not logical state;
 * canonical snapshots use chfx_history_age rather than copying stale bytes. */
typedef struct{int16_t *data;uint32_t size,next,valid;} chfx_history;
int16_t chfx_history_age(const chfx_history *,uint32_t age);
typedef void (*chfx_before_frame)(void *,uint32_t local_frame);
/* The caller serializes actual edits/events before each original local frame.
 * The hook is synchronous. Calls use n<=4096 and disjoint L/R arrays, as the
 * selected original wrapper does. No event queue/transport or allocator is invented. */
typedef struct{chfx_before_frame before_frame;void *context;} chfx_hooks;
typedef struct{
 chfx_history history[2];
 int32_t time,depth,pending_delay,pending_feedback,delay_target,feedback_target;
 int32_t delay_acc,feedback_acc;uint32_t original_write;
 uint8_t dirty,fast;
} chfx_short;
typedef struct{
 chfx_history history[2];
 int32_t time,depth,bpm,max_division,delay_target,gain_target,feedback_target;
 int32_t gain_acc,feedback_acc,delay_acc,feedback[2];uint32_t original_write;
 uint8_t fast;
} chfx_bpm;
typedef struct{
 chfx_history history[5];int32_t parameters[7];
 int32_t pending_feedback,pending_level,pending_input;
 int32_t feedback_target,level_target,input_target;
 int32_t feedback_acc,feedback_current,level_acc,level_current,input_acc;
 int32_t threshold,attack,release,hold_step,gate_phase,gate_envelope,hold_counter,detector;
 uint32_t original_write;uint8_t dirty,fast;
} chfx_gate;
typedef struct{
 chfx_history history[9];int32_t time,level,pending_time,pending_level;
 int32_t current_time,level_target,level_acc;
 int16_t A_state[4],A_output[4],B_state[4],B_coeff[4];
 uint32_t original_counter;uint64_t idle_remaining;uint8_t dirty,idle,core_active;
} chfx_hall;
void chfx_short_construct(chfx_short *);
void chfx_bpm_construct(chfx_bpm *);
void chfx_gate_construct(chfx_gate *);
void chfx_hall_construct(chfx_hall *);
/* Bind caller-owned nonoverlapping storage. No constructor/reset side effects
 * occur on failed admission. Successful binding creates logical-zero history;
 * caller then invokes reset at the original lifecycle point. All sizes are
 * int16 slots. BPM is unconditional STEREO: no producer-mono proof is assumed. */
int chfx_short_bind(chfx_short *,int16_t *,size_t slots);
int chfx_bpm_bind(chfx_bpm *,int16_t *,size_t slots);
int chfx_gate_bind(chfx_gate *,int16_t *,size_t slots);
int chfx_hall_bind(chfx_hall *,int16_t *,size_t slots);
void chfx_short_reset(chfx_short *);
void chfx_bpm_reset(chfx_bpm *);
void chfx_gate_reset(chfx_gate *);
void chfx_hall_reset(chfx_hall *);
/* Hallwrapper virtual48 reads adapter activebyte(initial0), notcore_active.
 * It returns0 in the selected construction. Other three virtual48 return1.
 * OriginalHall begin markscoreactive and acquiresitsownheapifabsent; caller
 * must admit/bind first in this allocation-free projection. Retire clearsonly
 * core_active. It doesnot free history, changeidle or invalidate CURRENTlease. */
void chfx_hall_begin(chfx_hall *);
void chfx_hall_retire(chfx_hall *);
/* Wrapper canonical Edit1/Edit2 are integer0..127. Native setters mask7bits.
 * Only Short/BPM/Gate fast flags are effective. Hall wrapper virtual50 is no-op.
 * Hall accepts original fixed Fs44100; its original rate-check38b14 rejects
 * other rates. No resampling/quality change is introduced. */
int chfx_short_edit(chfx_short *,unsigned parameter,int value);
int chfx_bpm_edit(chfx_bpm *,unsigned parameter,int value);
int chfx_gate_edit(chfx_gate *,unsigned parameter,int value);
int chfx_gate_native(chfx_gate *,unsigned native_parameter,int value);
int chfx_hall_edit(chfx_hall *,unsigned parameter,int value);
void chfx_bpm_tempo(chfx_bpm *,int32_t bpm_times10);
void chfx_short_fast(chfx_short *,int on);
void chfx_bpm_fast(chfx_bpm *,int on);
void chfx_gate_fast(chfx_gate *,int on);
void chfx_hall_fast(chfx_hall *,int on); /* Originalwrapper no-op. */
/* Wrapper direct event virtual58 resets onlytype5. Short/BPM queuedtype4
 * translates to this reset; Gate embeddedqueue resets directlyontype4.
 * Hall outerqueue handling is not assumed: its body delegates through its
 * BasicEffectAdapter. Exact host event dispatch remains a caller contract. */
void chfx_short_event(chfx_short *,unsigned type);
void chfx_bpm_event(chfx_bpm *,unsigned type);
void chfx_gate_event(chfx_gate *,unsigned type);
void chfx_hall_event(chfx_hall *,unsigned type);
/* These queued-event entrypoints are called only when the caller's original
 * queue comparator says the record is due, before the matching local frame.
 * Short/BPM compare signed(base+local) againstsignedrecordframe; Gate compares
 * unsignedlocal againstunsignedrecordframe. Allqueuedtypesareconsumed; only4
 * resets. Hallouter inheritedqueue isnotdrained byitsselectedprocess body;
 * don'tapplythisreset rule toHall. OptionalBasicEffectcontext callback d43c0
 * remains the Hall before_frame hook's explicit caller-owned input. */
void chfx_short_queued_event(chfx_short *,unsigned type);
void chfx_bpm_queued_event(chfx_bpm *,unsigned type);
void chfx_gate_queued_event(chfx_gate *,unsigned type);
void chfx_short_process(chfx_short *,int16_t *L,int16_t *R,size_t n,const chfx_hooks *);
void chfx_bpm_process(chfx_bpm *,int16_t *L,int16_t *R,size_t n,const chfx_hooks *);
void chfx_gate_process(chfx_gate *,int16_t *L,int16_t *R,size_t n,const chfx_hooks *);
void chfx_hall_process(chfx_hall *,int16_t *L,int16_t *R,size_t n,const chfx_hooks *);
#endif
