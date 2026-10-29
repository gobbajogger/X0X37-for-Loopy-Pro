/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_output.h"
#include "miami_fx_conversion.h"
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static int32_t asr2(int32_t n){return n>=0?n/4:-1-(int32_t)(((uint32_t)(-(int64_t)n)-1)/4);}
typedef struct {uint32_t calls,frames,resets;int16_t history[2];} probe_fx2;
/* Test-only synthetic FX2 body. This proves call/reset/order and conversion;
 * it is not supplied as an original Chicago effect implementation. */
static void fx2_process(void *context,int16_t *left,int16_t *right,uint32_t n){
 probe_fx2 *s=context;s->calls++;s->frames+=n;
 for(uint32_t i=0;i<n;i++){int16_t l=left[i],r=right[i];left[i]=miami_fx_low16(asr2((int32_t)l*3)+s->history[0]);right[i]=miami_fx_low16(asr2((int32_t)r*5)+s->history[1]);s->history[0]=l;s->history[1]=r;}
}
static void fx2_reset(void *context){probe_fx2 *s=context;s->resets++;s->history[0]=3;s->history[1]=-5;}
static int16_t temporary[4][4096];
static uint32_t inputs[2][4096],outputs[2][4096],expected[2][4096];
static original_chicago_drive_event events[64];
static uint32_t fields(const original_chicago_output *s,const probe_fx2 *f,int64_t out[64]){
 const original_chicago_drive_wrapper *w=&s->fx1;const original_chicago_drive *d=&w->drive;uint32_t n=0;
#define FIELD(x) out[n++]=(int64_t)(x)
 FIELD(d->target_drive);FIELD(d->target_gain);FIELD(d->drive);FIELD(d->gain);FIELD(d->threshold);
 FIELD(d->previous_driven[0]);FIELD(d->previous_driven[1]);FIELD(d->previous_nonlinear[0]);FIELD(d->previous_nonlinear[1]);FIELD(d->activity);FIELD(d->edit1);FIELD(d->edit2);
 FIELD(w->wet_target);FIELD(w->wet_accumulator);FIELD(w->progress);FIELD(w->activity_counter);FIELD(w->edit1);FIELD(w->edit2);FIELD(w->switch_current);FIELD(w->switch_pending);FIELD(w->switch_dirty);FIELD(w->initial_selection_pending);FIELD(w->selected);FIELD(w->transitioning);FIELD(w->queue.count);FIELD(w->queue.cursor);
 FIELD(bits(s->master.current));FIELD(bits(s->master.target));FIELD(bits(s->master.step));FIELD(s->master.committed_mute);FIELD(s->master.desired_mute);
 FIELD(s->reset_countdown[0]);FIELD(s->reset_countdown[1]);FIELD(s->process_slot[0]);FIELD(s->process_slot[1]);FIELD(s->enable_slot[0]);FIELD(s->enable_slot[1]);FIELD(f->calls);FIELD(f->frames);FIELD(f->resets);FIELD(f->history[0]);FIELD(f->history[1]);
#undef FIELD
 return n;
}
static void fixtures(const char *path,const char *stream_path){
 FILE *file=fopen(path,"r"),*stream=fopen(stream_path,"wb");CHECK(file&&stream);original_chicago_output state;probe_fx2 f={0,0,0,{3,-5}};
 original_chicago_fx2 fx2={&f,fx2_process,fx2_reset};original_chicago_output_workspace work={temporary[0],temporary[1],temporary[2],temporary[3],4096};
 char command;uint32_t frames=0,rows=0;
 while(fscanf(file," %c",&command)==1){
  unsigned a,b,c;
  switch(command){
  case 'I':CHECK(fscanf(file,"%u %u %u",&a,&b,&c)==3);CHECK(original_chicago_output_init(&state,a,b,c));f=(probe_fx2){0,0,0,{3,-5}};break;
  case 'P':CHECK(fscanf(file,"%u %u",&a,&b)==2);CHECK(original_chicago_drive_wrapper_set(&state.fx1,a,b));break;
  case 'E':CHECK(fscanf(file,"%u",&a)==1&&a<=64);for(unsigned i=0;i<a;i++)CHECK(fscanf(file,"%"SCNu32" %"SCNu32,&events[i].offset,&events[i].type)==2);CHECK(original_chicago_drive_wrapper_events(&state.fx1,events,a));break;
  case 'D':CHECK(fscanf(file,"%u",&a)==1);original_chicago_drive_wrapper_direct_event(&state.fx1,a);break;
  case 'R':original_chicago_drive_wrapper_reset(&state.fx1);break;
  case 'Z':CHECK(original_chicago_drive_wrapper_process(&state.fx1,NULL,NULL,NULL,NULL,0,4096));break;
  case 'G':CHECK(fscanf(file,"%u %u",&a,&b)==2);CHECK(original_chicago_output_enable(&state,a,b));break;
  case 'H':CHECK(fscanf(file,"%u %u %x",&a,&b,&c)==3);CHECK(original_chicago_output_suspend(&state,a,b,fl(c)));break;
  case 'M':CHECK(fscanf(file,"%u",&a)==1);original_chicago_output_mute(&state,(uint8_t)a);break;
  case 'B':{
   CHECK(fscanf(file,"%u %x %u",&a,&b,&c)==3&&a<=4096);
   for(unsigned i=0;i<a;i++){char tag;CHECK(fscanf(file," %c %"SCNx32" %"SCNx32" %"SCNx32" %"SCNx32,&tag,&inputs[0][i],&inputs[1][i],&expected[0][i],&expected[1][i])==5&&tag=='S');}
   CHECK(original_chicago_output_process(&state,a?&work:NULL,a?&fx2:NULL,fl(b),a?inputs[0]:NULL,a?inputs[1]:NULL,a?outputs[0]:NULL,a?outputs[1]:NULL,a,c));
   for(unsigned i=0;i<a;i++){CHECK(outputs[0][i]==expected[0][i]&&outputs[1][i]==expected[1][i]);CHECK(fwrite(&outputs[0][i],4,1,stream)==1&&fwrite(&outputs[1][i],4,1,stream)==1);}frames+=a;break;
  }
  case 'W':{
   int64_t got[64],value;uint32_t count=fields(&state,&f,got);CHECK(fscanf(file,"%u",&a)==1&&a==count);
   for(unsigned i=0;i<a;i++){CHECK(fscanf(file,"%"SCNd64,&value)==1);if(value!=got[i]){fprintf(stderr,"state row %u field %u want %"PRId64" got %"PRId64"\n",rows,i,value,got[i]);CHECK(value==got[i]);}}
   CHECK(fwrite(got,sizeof *got,count,stream)==count);rows++;break;
  }
  default:fprintf(stderr,"Unknown fixture %c\n",command);abort();
  }
 }
 CHECK(fclose(file)==0&&fclose(stream)==0);printf("PASS independent rational/integer oracle: %u rendered stereo frames, %u complete state checkpoints.\n",frames,rows);
}
static void transactions_and_aliases(void){
 original_chicago_output s;CHECK(original_chicago_output_init(&s,100,110,1));original_chicago_output saved=s;
 CHECK(!original_chicago_drive_wrapper_set(&s.fx1,0,18)&&!memcmp(&s,&saved,sizeof s));
 CHECK(!original_chicago_drive_wrapper_set(&s.fx1,1,2)&&!memcmp(&s,&saved,sizeof s));
 CHECK(!original_chicago_drive_wrapper_set(&s.fx1,2,128)&&!memcmp(&s,&saved,sizeof s));
 original_chicago_drive_event wrong[]={{100,4},{99,4}};CHECK(!original_chicago_drive_wrapper_events(&s.fx1,wrong,2)&&!memcmp(&s,&saved,sizeof s));
 CHECK(!original_chicago_output_suspend(&s,2,1,44100)&&!memcmp(&s,&saved,sizeof s));CHECK(!original_chicago_output_suspend(&s,0,1,NAN)&&!memcmp(&s,&saved,sizeof s));
 CHECK(original_chicago_output_process(&s,NULL,NULL,NAN,NULL,NULL,NULL,NULL,0,9)&&!memcmp(&s,&saved,sizeof s));
 probe_fx2 f={0,0,0,{3,-5}};original_chicago_fx2 fx2={&f,fx2_process,fx2_reset};original_chicago_output_workspace w={temporary[0],temporary[1],temporary[2],temporary[3],4096};
 outputs[0][0]=123;outputs[1][0]=234;CHECK(!original_chicago_output_process(&s,&w,&fx2,NAN,inputs[0],inputs[1],outputs[0],outputs[1],1,0)&&!memcmp(&s,&saved,sizeof s)&&outputs[0][0]==123&&outputs[1][0]==234);
 CHECK(!original_chicago_output_process(&s,&w,NULL,.25f,inputs[0],inputs[1],outputs[0],outputs[1],1,0)&&!memcmp(&s,&saved,sizeof s));
 CHECK(!original_chicago_output_process(&s,&w,&fx2,.25f,inputs[0],inputs[1],outputs[0],outputs[1],4097,0)&&!memcmp(&s,&saved,sizeof s));
 /* Exact aligned countdown, independent enable freezing and callback reset
  * preconditions. Expiry while process0 must still reset the selected wrapper. */
 CHECK(original_chicago_output_suspend(&s,0,1,44100)&&s.reset_countdown[0]==176400&&!s.process_slot[0]);
 CHECK(original_chicago_output_suspend(&s,1,1,48000)&&s.reset_countdown[1]==192000&&!s.process_slot[1]);
 s.reset_countdown[1]=1;saved=s;original_chicago_fx2 missing_reset={&f,fx2_process,NULL};
 CHECK(!original_chicago_output_process(&s,&w,&missing_reset,.25f,inputs[0],inputs[1],outputs[0],outputs[1],1,0)&&!memcmp(&s,&saved,sizeof s));
 /* Matching-channel in-place float processing must preserve all output/state. */
 original_chicago_output a,b;CHECK(original_chicago_output_init(&a,99,109,1));b=a;probe_fx2 fa={0,0,0,{3,-5}},fb=fa;original_chicago_fx2 ca={&fa,fx2_process,fx2_reset},cb={&fb,fx2_process,fx2_reset};
 for(unsigned i=0;i<128;i++){inputs[0][i]=bits((float)((int)i-64)/40.0f);inputs[1][i]=bits((float)(64-(int)i)/60.0f);outputs[0][i]=inputs[0][i];outputs[1][i]=inputs[1][i];}
 CHECK(original_chicago_output_process(&a,&w,&ca,.201601f,inputs[0],inputs[1],expected[0],expected[1],128,0));
 CHECK(original_chicago_output_process(&b,&w,&cb,.201601f,outputs[0],outputs[1],outputs[0],outputs[1],128,0));
 CHECK(!memcmp(expected[0],outputs[0],128*4)&&!memcmp(expected[1],outputs[1],128*4)&&!memcmp(&a,&b,sizeof a)&&!memcmp(&fa,&fb,sizeof fa));
 printf("PASS invalid transactions, fixed-type boundary, empty outer no-op, countdown and in-place output.\n");
 printf("Host bytes: DriveWrapper=%zu Output=%zu WorkspaceDescriptor=%zu; scratch8*capacity bytes; no heap. Not native target fit or CPU proof.\n",sizeof s.fx1,sizeof s,sizeof w);
}
int main(int argc,char **argv){CHECK(argc==3);CHECK(miami_fx_rne_environment());fixtures(argv[1],argv[2]);transactions_and_aliases();printf("PASS %u assertions. Synthetic FX2 routing witness only; original bodies/host event provenance/complete restore lifecycle remain explicit.\n",checks);return 0;}
