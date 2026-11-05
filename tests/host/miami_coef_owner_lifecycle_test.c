/* SPDX-License-Identifier: GPL-3.0-only */
/* Non-UI regression of the actual serial owner and backend lifecycle.
 * The reference owner executes production's uncached selected-voice entry.
 * Codec adoption here restores the captured voices in their unchanged owning
 * allocator/global/clock context. This is not whole-project restore proof. */
#include "original_miami_backend.h"
#include "miami_voice_snapshot.h"
#include "binary32_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, warm_ticks, lifecycle_cases;
#define CHECK(x) do { ++checks; if (!(x)) { \
 fprintf(stderr,"owner lifecycle: %s at %s:%d\n",#x,__FILE__,__LINE__); \
 exit(1); } } while (0)
static original_miami_backend cached, reference;

/* Compare every serialized voice value, including pitch/MG/filter/gain state.
 * The runtime-only cache witness is deliberately absent from the wire format. */
static int equal_voices(void)
{
 for (unsigned id=0; id<ORIGINAL_MIAMI_BACKEND_VOICES; ++id) {
  original_miami_voice_snapshot a,b;
  unsigned char aw[ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES];
  unsigned char bw[ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES];
  CHECK(original_miami_voice_snapshot_capture(&a,&cached.voice_owner,id));
  CHECK(original_miami_voice_snapshot_capture(&b,&reference.voice_owner,id));
  CHECK(original_miami_voice_snapshot_encode(&a,aw,sizeof aw));
  CHECK(original_miami_voice_snapshot_encode(&b,bw,sizeof bw));
  if (memcmp(aw,bw,sizeof aw)) return 0;
 }
 return 1;
}
static void control_tick(unsigned compare)
{
 /* Valid test-controlled scheduler states producing consecutive D=64 ticks;
  * real control functions still perform every tick and all arithmetic. */
 for (unsigned id=0; id<ORIGINAL_MIAMI_BACKEND_VOICES; ++id) {
  cached.voices[id].counter=reference.voices[id].counter=44100u-690u;
 }
 CHECK(original_miami_voice_owner_block_valid(&cached.voice_owner));
 CHECK(original_miami_voice_owner_block_valid(&reference.voice_owner));
 CHECK(original_miami_voice_owner_control_prevalidated(&cached.voice_owner));
 CHECK(original_miami_voice_owner_control_all(&reference.voice_owner));
 if (compare) CHECK(equal_voices());
 CHECK(memcmp(&cached.global,&reference.global,sizeof cached.global)==0);
}
static void warm(void)
{
 control_tick(1);
 CHECK(cached.voice_owner.coefficient_cache.mg_valid_mask==3);
 CHECK(cached.voice_owner.coefficient_cache.glide.duration==64);
 original_miami_coefficient_cache before=cached.voice_owner.coefficient_cache;
 control_tick(1);
 CHECK(memcmp(&before,&cached.voice_owner.coefficient_cache,sizeof before)==0);
 ++warm_ticks;
}
static void note(unsigned key,unsigned velocity)
{
 CHECK(original_miami_backend_note_on(&cached,key,velocity));
 CHECK(original_miami_backend_note_on(&reference,key,velocity));
 CHECK(equal_voices());
}
static void check_witnesses_clear(void)
{
 for (unsigned id=0; id<ORIGINAL_MIAMI_BACKEND_VOICES; ++id)
  CHECK(cached.voices[id].cache_witness==0);
}
static void note_and_reset(void)
{
 CHECK(original_miami_backend_init(&cached,0,120));
 CHECK(original_miami_backend_init(&reference,0,120));
 note(60,100); warm();
 /* Real partial-note callbacks invalidate each formerly warm voice. */
 cached.voice_owner.coefficient_cache.delta[0]=.75f;
 cached.voice_owner.coefficient_cache.glide.a=.5f;
 note(67,110); check_witnesses_clear(); warm(); ++lifecycle_cases;
 cached.voice_owner.coefficient_cache.delta[0]=.75f;
 cached.voice_owner.coefficient_cache.glide.a=.5f;
 CHECK(original_miami_backend_panic(&cached));
 CHECK(original_miami_backend_panic(&reference));
 check_witnesses_clear(); CHECK(equal_voices());
 note(48,90); warm(); ++lifecycle_cases;
}
static void decoded_adoption(void)
{
 original_miami_voice_snapshot staged[ORIGINAL_MIAMI_BACKEND_VOICES];
 /* Capture both voices with a warm, bound owner. Preserve all surrounding
  * owner context unchanged, then adopt only this exact decoded value state. */
 for (unsigned id=0; id<ORIGINAL_MIAMI_BACKEND_VOICES; ++id) {
  original_miami_voice_snapshot snap;
  unsigned char wire[ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES];
  CHECK(original_miami_voice_snapshot_capture(&snap,&cached.voice_owner,id));
  CHECK(original_miami_voice_snapshot_encode(&snap,wire,sizeof wire));
  CHECK(original_miami_voice_snapshot_decode(&staged[id],wire,sizeof wire));
  CHECK(staged[id].program_revision==cached.controls.revision);
  CHECK(staged[id].voice_id==id);
  CHECK(staged[id].voice.cache_witness==0);
 }
 /* Deliberately wrong payload with otherwise matching keys is a test control.
  * The decoded zero witnesses must make the real owner discard it. */
 cached.voice_owner.coefficient_cache.delta[0]=.75f;
 cached.voice_owner.coefficient_cache.delta[1]=.5f;
 cached.voice_owner.coefficient_cache.glide.a=.5f;
 for (unsigned id=0; id<ORIGINAL_MIAMI_BACKEND_VOICES; ++id) {
  cached.voices[id]=staged[id].voice;
  cached.pending[id]=staged[id].pending;
  cached.extra[id]=staged[id].extra;
 }
 check_witnesses_clear(); warm(); ++lifecycle_cases;
}
static void factory_replacement(void)
{
 unsigned previous_revision=cached.controls.revision;
 cached.voice_owner.coefficient_cache.delta[0]=.75f;
 cached.voice_owner.coefficient_cache.glide.a=.5f;
 CHECK(original_miami_backend_load_factory(&cached,1));
 CHECK(original_miami_backend_load_factory(&reference,1));
 /* Factory revisions can collide: invalidation must not rely on increment. */
 CHECK(cached.controls.revision==previous_revision);
 CHECK(cached.voice_owner.coefficient_cache.mg_valid_mask==0);
 CHECK(cached.voice_owner.coefficient_cache.glide.duration==0);
 check_witnesses_clear(); warm(); ++lifecycle_cases;
}
static void controlled_bad_cache(void)
{
 /* Prove differential assertions can see an actual cached-path corruption.
  * No production bytes change. Warm witnesses/keys deliberately remain valid. */
 cached.voice_owner.coefficient_cache.delta[0]=.875f;
 control_tick(0);
 CHECK(!equal_voices());
 puts("test control: valid-key bad-cache payload detected by uncached comparison");
}
int main(void)
{
 note_and_reset(); decoded_adoption(); factory_replacement();
 controlled_bad_cache();
 printf("Miami actual owner lifecycle: %u checks PASS; %u warm repeated-key pairs; "
        "%u lifecycle cases (note, reset, decoded adoption, factory)\n",
        checks,warm_ticks,lifecycle_cases);
 return 0;
}
