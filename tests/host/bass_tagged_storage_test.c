/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/factory_banks.c"
#include "../../firmware/src/app/bass_presets.h"
/* Actual editor parser + real project/UI/runtime. Only native wire/flash
 * boundaries are substituted; no project validators/adoption are mocked. */
static uint8_t editor_flash[0x100000],editor_frame[640],reply[800];
static uint32_t frame_n,reply_n,fm1_ms,fm1_crash[2],flash_writes,flash_erases;
static struct {uint32_t failed,pending;} bootguard;
static int flash_ok=1;
static void fm1_irq_off(void){} static void fm1_irq_on(void){} static void fm1_wdt_feed(void){}
static void audio_silence(void){} static void fl_inval(uint32_t off,uint32_t n){(void)off;(void)n;}
static int fl_read(uint32_t off,void *dst,uint32_t n){if(off+n>sizeof editor_flash)return -1;memcpy(dst,editor_flash+off,n);return 0;}
static int fl_write(uint32_t off,const void *p,uint32_t n){if(off+n>sizeof editor_flash)return -1;memcpy(editor_flash+off,p,n);flash_writes++;return 0;}
static int fl_erase4k(uint32_t off,uint32_t *took){if(off+4096>sizeof editor_flash)return -1;memset(editor_flash+off,255,4096);*took=1;flash_erases++;return 0;}
static uint8_t *fm1_xip_ptr(uint32_t off){return editor_flash+off;}
static int store_stage_held(void){return 0;}static uint8_t *store_stage_lock(uint32_t *cap){static uint8_t b[64];if(cap)*cap=sizeof b;return b;}
static void store_stage_unlock(void){}
static int ota_wire_send(const uint8_t *p,uint32_t n){memcpy(reply,p,n);reply_n=n;return 0;}
static int ota_frame_get(const uint8_t **p,uint32_t *n){*p=editor_frame;*n=frame_n;return frame_n!=0;}
static void ota_frame_done(void){frame_n=0;}
#include "../../firmware/src/app/editor.c"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL tag line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static project_t original;
static uint8_t legacy[65536],header[32];
static void render(void){int32_t audio[512];engine_render(audio,256);}
static void reset_runtime(void){engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);engine_apply_sound(&proj.sound);run_ms(50);}
static void selected(int voice,int bank,int preset)
{CHECK(!engine_bass_select_preset(voice,bank,preset,&proj.sound));render();CHECK(!engine_bass_preset_pending(voice));}
static void rejected(int expected)
{memcpy(&original,&project_stage,sizeof original);CHECK(project_validate_stage()==expected);CHECK(!memcmp(&original,&project_stage,sizeof original));}
static void proto_op(uint8_t code,const uint8_t *args,unsigned n,int expected)
{
 editor_frame[0]=0x7d;editor_frame[1]=0x46;editor_frame[2]=0x4c;editor_frame[3]=ED_PROJECT;editor_frame[4]=code;
 if(n)memcpy(editor_frame+5,args,n);frame_n=n+5;reply_n=0;ed_service();
 CHECK(!frame_n);CHECK(reply_n>=8 && reply[0]==0xf0 && reply[reply_n-1]==0xf7);CHECK(reply[5]==code);CHECK(reply[6]==expected);
}
static void u21(uint8_t *p,unsigned n){p[0]=n&127;p[1]=(n>>7)&127;p[2]=(n>>14)&127;}
static unsigned pack7(const uint8_t *src,unsigned n,uint8_t *out)
{
 unsigned pos=0;for(unsigned i=0;i<n;i+=7){unsigned m=pos++;out[m]=0;for(unsigned j=0;j<7&&i+j<n;j++){out[m]|=(src[i+j]>>7)<<j;out[pos++]=src[i+j]&127;}}return pos;
}
static void protocol_legacy(size_t size)
{
 unsigned writes=store_writes;uint8_t args[270],confirm[2]={0x58,0x30};
 seq.playing=seq.req_start=seq.req_cont=seq.req_stop=0;run_ms(50);
 proto_op(64,0,0,0);CHECK(reply[7]==2);
 u21(args,(unsigned)size);proto_op(67,args,3,0);
 for(unsigned at=0;at<size;){unsigned n=size-at;if(n>224)n=224;u21(args,at);unsigned bytes=pack7(legacy+at,n,args+3);proto_op(68,args,bytes+3,0);at+=n;}
 proto_op(74,0,0,9);CHECK(ed_project.legacy_prepared);CHECK(!ed_project.valid);
 CHECK(!memcmp(project_stage.sound.v,legacy+32+sizeof(settings_t),sizeof project_stage.sound.v));
 proto_op(69,confirm,2,9);CHECK(ed_project.mode==2); /* no implicit adoption */
 uint8_t adopt[4]={0,0,0x58,0x30};proto_op(75,adopt,2,8);adopt[0]=adopt[1]=1;proto_op(75,adopt,4,5);
 adopt[0]=adopt[1]=0;proto_op(75,adopt,4,0);CHECK(ed_project.valid);CHECK(!project_validate_stage());
 CHECK(store_writes==writes && !flash_writes && !flash_erases);
 proto_op(69,confirm,2,0);CHECK(!ed_project.mode);run_ms(50);
 CHECK(engine_bass_bank(0)==0 && engine_bass_bank(1)==0);CHECK(!memcmp(proj.sound.v,legacy+32+sizeof(settings_t),sizeof proj.sound.v));
 proto_op(65,0,0,0);CHECK(ed_project.mode==1);CHECK(get32(ed_project.header+24)==11);CHECK(!project_file_validate(ed_project.header));
 u21(args,32+sizeof(settings_t)+sizeof proj.sound.v);args[3]=24;args[4]=0;proto_op(66,args,5,0);
 CHECK(reply_n==8+3+24+4); /* 24bytes +4seven-bit masks, operation/status/offset/framing */
 proto_op(70,0,0,0);CHECK(!ed_project.mode);CHECK(store_writes==writes);
}
static void legacy_fixture(const char *path)
{
 FILE *f=fopen(path,"rb");CHECK(f!=0);size_t size=fread(legacy,1,sizeof legacy,f);fclose(f);
 CHECK(size==PROJECT_LEGACY_FILE_SIZE);memcpy(header,legacy,32);
 memset(&project_stage,0xa5,sizeof project_stage);memcpy(&project_stage,legacy+32,size-32);
 memcpy(&original,&project_stage,sizeof original);
 for(unsigned bad=4;bad<=5;bad++){put32(header+20,bad);CHECK(project_prepare_legacy_file(header)==-9);CHECK(!memcmp(&original,&project_stage,sizeof original));}
 memcpy(header,legacy,32);header[12]^=1;CHECK(project_prepare_legacy_file(header)==-4);CHECK(!memcmp(&original,&project_stage,sizeof original));memcpy(header,legacy,32);
 CHECK(project_prepare_legacy_file(header)==-10);CHECK(project_stage.set.snd_rev==10);
 CHECK(!memcmp(project_stage.sound.v,legacy+32+sizeof(settings_t),sizeof project_stage.sound.v));
 CHECK(!memcmp(project_stage.pat,legacy+32+sizeof(settings_t)+sizeof project_stage.sound.v,sizeof project_stage.pat+sizeof project_stage.arr));
 CHECK(engine_bass_sound_validate(&project_stage.sound)==-10);rejected(-10);
 memcpy(&original,&project_stage,sizeof original);CHECK(project_adopt_legacy_stage(1,1)==-9);CHECK(!memcmp(&original,&project_stage,sizeof original));
 /* A genuine ambiguous old value: reject Miami explicitly, never clamp it. */
 project_stage.sound.v[T_303][0][0]=48;project_stage.sound.v[T_303][1][0]=48;memcpy(&original,&project_stage,sizeof original);
 CHECK(project_adopt_legacy_stage(1,0)!=0);CHECK(!memcmp(&original,&project_stage,sizeof original));
 uint8_t palette=project_stage.set.palette;project_stage.set.palette=9;memcpy(&original,&project_stage,sizeof original);
 CHECK(project_adopt_legacy_stage(0,0)==-1);CHECK(!memcmp(&original,&project_stage,sizeof original));
 project_stage.set.palette=palette;memcpy(&original,&project_stage,sizeof original);
 CHECK(!project_adopt_legacy_stage(0,0));CHECK(project_stage.set.snd_rev==11);
 CHECK(!memcmp(project_stage.sound.v,original.sound.v,sizeof original.sound.v));CHECK(!project_validate_stage());
 project_file_header(header);CHECK(!project_file_validate(header));CHECK(get32(header+20)==6 && get32(header+24)==11);
 /* Create actual old local objects from the decoded producer fixture. */
 project_stage=original;proj=project_stage;project_incompatible=0;
 CHECK(!save_obj(OBJ_SET,&proj.set,sizeof proj.set));CHECK(!save_obj(OBJ_SOUND,proj.sound.v,sizeof proj.sound.v));
 CHECK(!measure_project(&proj,&packed_len));CHECK(!save_stream());unsigned writes=store_writes;
 memset(&proj,0,sizeof proj);CHECK(!project_load());CHECK(project_storage_unsupported());
 CHECK(!memcmp(proj.sound.v,original.sound.v,sizeof original.sound.v));CHECK(engine_bass_sound_bank(&proj.sound,0)==-1);
 reset_runtime();engine_bass_on(0,48,1,0);render();CHECK(!(engine_midi_held()&1));
 bpart_t before=proj.pat[0].bass[0];seq.playing=1;seq.rec_mask=1;
 engine_bass_record(0,52,100,0);render();
 min_q[mi_w++%MQ]=9u|((0x90u|seq.ch[TRK_BASS0])<<8)|(55u<<16)|(100u<<24);render();
 CHECK(!memcmp(&before,&proj.pat[0].bass[0],sizeof before));CHECK(!seq.rec_bass[0].active);CHECK(!(engine_midi_held()&1));
 seq.playing=0;seq.rec_mask=0;
 CHECK(project_save()==-9);CHECK(store_writes==writes);
 project_capture_stage();CHECK(!project_adopt_legacy_stage(0,0));project_commit_stage();CHECK(!project_storage_unsupported());
 reset_runtime();CHECK(engine_bass_bank(0)==0 && engine_bass_bank(1)==0);CHECK(!project_save());
 CHECK(store_len[OBJ_SOUND]==4+sizeof(sound_t));memcpy(&original,&proj,sizeof original);
 memset(&proj,0,sizeof proj);CHECK(!project_load());reset_runtime();CHECK(!memcmp(&original.sound,&proj.sound,sizeof proj.sound));
 CHECK(engine_bass_bank(0)==0 && engine_bass_bank(1)==0);
 protocol_legacy(size);
}
static void older_cell_objects(void)
{
 static uint8_t raw[NTARGETS*NVOICES_MAX*32];
 for(unsigned width=16;width<=28;width+=width==16?8:4){
  project_defaults();proj.set.snd_rev=(uint8_t)(width==16?4:width==24?7:8);
  memset(raw,0,sizeof raw);for(int voice=0;voice<NBASS;voice++)raw[(T_303*NVOICES_MAX+voice)*width]=48;
  CHECK(!save_obj(OBJ_SET,&proj.set,sizeof proj.set));CHECK(!save_obj(OBJ_SOUND,raw,NTARGETS*NVOICES_MAX*width));
  CHECK(!measure_project(&proj,&packed_len));CHECK(!save_stream());unsigned writes=store_writes;
  CHECK(!project_load());CHECK(project_storage_unsupported());CHECK(proj.sound.v[T_303][0][0]==48 && proj.sound.v[T_303][1][0]==48);
  CHECK(engine_bass_sound_bank(&proj.sound,0)==-1);CHECK(project_save()==-9);CHECK(store_writes==writes);
  project_capture_stage();memcpy(&original,&project_stage,sizeof original);CHECK(project_adopt_legacy_stage(0,0)==-9);CHECK(!memcmp(&original,&project_stage,sizeof original));
 }
 project_reset_stage();CHECK(!project_validate_stage());project_commit_stage();reset_runtime();CHECK(!project_save());
 /* Unknown future identity in a well-framed current object is sticky unsupported. */
 proj.sound.bass[0].bytes[0]=0xed;proj.sound.bass[0].bytes[1]=0xfe;
 CHECK(!save_obj(OBJ_SOUND,&proj.sound,sizeof proj.sound));unsigned writes=store_writes;
 CHECK(project_load()==-9);CHECK(project_storage_unsupported());project_defaults();CHECK(project_save()==-9);CHECK(store_writes==writes);
 project_reset_stage();CHECK(!project_validate_stage());project_commit_stage();reset_runtime();CHECK(!project_save());
}
int main(int argc,char **argv)
{
 CHECK(X0X_BASS_TAGGED_SOUND==1 && PROJ_SND_REV==11);CHECK(sizeof(sound_t)==2712);CHECK(sizeof(project_t)==36420);
 boot();run_ms(50);selected(0,0,0);
#if X0X_DEV_WOBBLE
 selected(1,1,0);
#else
 selected(1,0,0);
#endif
 project_capture_stage();CHECK(!project_validate_stage());project_file_header(header);CHECK(!project_file_validate(header));CHECK(!project_save());
 if(getenv("BASS_TAG_FIXTURE_OUT")){FILE *f=fopen(getenv("BASS_TAG_FIXTURE_OUT"),"wb");CHECK(f!=0);CHECK(fwrite(header,1,32,f)==32);CHECK(fwrite(&project_stage,1,sizeof project_stage,f)==sizeof project_stage);fclose(f);}
 memcpy(&original,&proj,sizeof original);memset(&proj,0,sizeof proj);CHECK(!project_load());reset_runtime();
 CHECK(!memcmp(&original.sound,&proj.sound,sizeof proj.sound));CHECK(engine_bass_preset_index(0)==0);
#if X0X_DEV_WOBBLE
 CHECK(engine_bass_bank(0)==0 && engine_bass_bank(1)==1);CHECK(engine_bass_preset_index(1)==0);
#endif
 /* Edited state is explicit and checked against the genuine named full cell. */
 pref_set(PR(R_ENG,T_303,0,BASS303_CUTOFF),49);render();project_capture_stage();CHECK(!project_validate_stage());
 CHECK(project_stage.sound.bass[0].bytes[10]==BASS_SOUND_EDITED);CHECK(!project_save());
 project_stage.sound.bass[0].bytes[10]=0;rejected(-1);project_capture_stage();
 project_stage.sound.bass[0].bytes[11]=1;rejected(-1);project_capture_stage();
 project_stage.sound.bass[0].bytes[6]=31;rejected(-1);project_capture_stage();
 project_stage.sound.bass[0].bytes[4]=2;rejected(-9);project_capture_stage();
 project_stage.sound.bass[0].bytes[8]=254;project_stage.sound.bass[0].bytes[9]=0;rejected(-1);project_capture_stage();
 project_stage.sound.bass[0].bytes[10]=2;rejected(-1);
 for(int i=1;i<argc;i++)legacy_fixture(argv[i]);
#if X0X_DEV_WOBBLE
 project_reset_stage();CHECK(engine_bass_sound_bank(&project_stage.sound,0)==1);CHECK(engine_bass_bank(0)==0);CHECK(!project_validate_stage());
#endif
 older_cell_objects();
 printf("PASS %u tagged rev11 storage/stage/legacy adoption checks WBL%d; sound=%u project=%u\n",checks,X0X_DEV_WOBBLE,(unsigned)sizeof(sound_t),(unsigned)sizeof(project_t));return 0;
}
