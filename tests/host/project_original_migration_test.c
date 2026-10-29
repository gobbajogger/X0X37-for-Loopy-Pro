/* SPDX-License-Identifier: GPL-3.0-only */
/* Real project loaders, staging, portable import and original engine defaults.
 * Legacy fixtures use their historical object layouts and production LZ codec. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main

static uint8_t editor_flash[0x100000],editor_frame[640],reply[800];
static uint32_t frame_n,reply_n,fm1_ms,fm1_crash[2],flash_writes,flash_erases;
static struct {uint32_t failed,pending;} bootguard;
static struct {unsigned ota_req,uboot_req;} usb;
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
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL migration line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static void proto_op(uint8_t code,const uint8_t *args,unsigned n,int expected)
{
 editor_frame[0]=0x7d;editor_frame[1]=0x46;editor_frame[2]=0x4c;editor_frame[3]=ED_PROJECT;editor_frame[4]=code;
 if(n)memcpy(editor_frame+5,args,n);frame_n=n+5;reply_n=0;ed_service();
 CHECK(!frame_n);CHECK(reply_n>=8 && reply[0]==0xf0 && reply[reply_n-1]==0xf7);CHECK(reply[5]==code);if(reply[6]!=expected)fprintf(stderr,"protocol op%u got%u expected%d\n",code,reply[6],expected);CHECK(reply[6]==expected);
}
static void u21(uint8_t *p,unsigned n){p[0]=n&127;p[1]=(n>>7)&127;p[2]=(n>>14)&127;}
static unsigned pack7(const uint8_t *src,unsigned n,uint8_t *out)
{
 unsigned pos=0;for(unsigned i=0;i<n;i+=7){unsigned m=pos++;out[m]=0;for(unsigned j=0;j<7&&i+j<n;j++){out[m]|=(src[i+j]>>7)<<j;out[pos++]=src[i+j]&127;}}return pos;
}

static project_t expected, live;
static sound_t defaults;
static uint8_t raw_fixture[RAW_LEN], file_fixture[sizeof(project_t)];
static uint8_t objects_before[sizeof store];
static int lengths_before[OBJ_NOBJ];

static void music(void)
{
    project_defaults();
    proj.set.palette=3; proj.set.bpm_x10=1370; proj.set.midi_ch[PART_303A]=5;
    for (unsigned p=0;p<NPAT;p++) {
        for (unsigned b=0;b<NBASS;b++) {
            bpart_t *bp=&proj.pat[p].bass[b];
            bp->step[0]=(bstep_t){(uint8_t)(36+p+b*12), G_NOTE|BS_ACCENT|BS_SLIDE, 91};
            bp->step[1]=(bstep_t){99, G_TIE, 0};
            bp->step[63]=(bstep_t){(uint8_t)(60+b), G_NOTE, 121};
        }
        sm_set(proj.pat[p].drum[0].hit[0], 31);
        proj.pat[p].brk.slice[15]=8;
    }
    proj.arr.song.len=2;
    proj.arr.song.bar[1].pat[PART_303A]=7;
    proj.arr.song.bar[1].pat[PART_303B]=12;
    lane_t *l=&proj.arr.lane[0];
    *l=(lane_t){.used=1,.pat=7,.part=PART_303A,.t=T_303,.v=0,.i=0};
    memset(l->val,42,sizeof l->val);
    l=&proj.arr.lane[1];
    *l=(lane_t){.used=1,.pat=7,.part=PART_303A,.t=T_PROB,.v=PART_303A<<4};
    memset(l->val,67,sizeof l->val);
    proj.sound.v[T_MIX][PART_909][MX_LEVEL]=93;
    expected=proj;
    memset(&expected.arr.lane[0],0,sizeof(lane_t));
}
static void reset_store(void)
{
    memset(store,0,sizeof store);
    for(unsigned i=0;i<OBJ_NOBJ;i++)store_len[i]=-1;
    memset(stored_known,0,sizeof stored_known);
    store_writes=0;
}
static void legacy_settings_sound(unsigned rev)
{
    proj.set.snd_rev=(uint8_t)rev;
    CHECK(!save_obj(OBJ_SET,&proj.set,sizeof proj.set));
    /* Revision10 has no identity trailer; revision11 has legacy tags. */
    memset(proj.sound.v[T_303],0x7e,sizeof proj.sound.v[T_303]);
    CHECK(!save_obj(OBJ_SOUND,&proj.sound,rev>=11?sizeof proj.sound:sizeof proj.sound.v));
}
static void packed_fixture(unsigned format)
{
    unsigned bytes=RAW_LEN;
    if(format==2) {
        for(unsigned p=0;p<NPAT;p++) {
            pattern2_t old={0}; const pattern_t *q=&proj.pat[p];
            memcpy(old.drum,q->drum,sizeof old.drum);old.brk=q->brk;old.swing=q->swing;
            for(unsigned b=0;b<NBASS;b++) {
                for(unsigned k=0;k<NSTEPS;k++)old.bass[b].step[k]=(bstep2_t){q->bass[b].step[k].note,q->bass[b].step[k].flags};
                old.bass[b].len=q->bass[b].len;old.bass[b].rate=q->bass[b].rate;
                old.bass[b].dir=q->bass[b].dir;old.bass[b].transpose=q->bass[b].transpose;old.bass[b].gen=q->bass[b].gen;
            }
            memcpy(raw_fixture+p*sizeof old,&old,sizeof old);
        }
        memcpy(raw_fixture+NPAT*sizeof(pattern2_t),&proj.arr,sizeof proj.arr);bytes=RAW_LEN_2;
        for(unsigned p=0;p<NPAT;p++)for(unsigned b=0;b<NBASS;b++)for(unsigned k=0;k<NSTEPS;k++)expected.pat[p].bass[b].step[k].velocity=0;
    } else memcpy(raw_fixture,&proj.pat,RAW_LEN);
    st_out_t out={0};lz_sink_t sink={st_put,&out};out.fill=STREAM_HDR;
    st_out_t count={.measure=1};lz_sink_t measure={st_put,&count};CHECK(!lz_compress(raw_fixture,bytes,&measure));
    put32(io_buf,format);put32(io_buf+4,bytes);put32(io_buf+8,crc32_of(raw_fixture,bytes));put32(io_buf+12,count.total);
    CHECK(!lz_compress(raw_fixture,bytes,&sink));CHECK(!st_flush(&out));
    while(out.k<NSTREAM)CHECK(!st_flush(&out));
    /* st_flush uses current PROJ_STREAM for continuation headers. */
    for(unsigned k=1;k<NSTREAM;k++)put32(store[STREAM_OBJ[k]],format);
}
static void format1_fixture(void)
{
    pattern1_t half[8];
    for (unsigned h=0;h<2;h++) {
        memset(half,0,sizeof half);
        for(unsigned j=0;j<8;j++) {
            const pattern_t *q=&proj.pat[h*8+j];pattern1_t *o=&half[j];
            for(unsigned b=0;b<NKIT;b++) {
                for(unsigned v=0;v<NDRUM;v++)o->drum[b].hit[v]=q->drum[b].hit[v][0];
                o->drum[b].accent=q->drum[b].accent[0];o->drum[b].len=q->drum[b].len;o->drum[b].rate=q->drum[b].rate;
            }
            for(unsigned b=0;b<NBASS;b++) {
                for(unsigned k=0;k<NSTEPS1;k++)o->bass[b].step[k]=(bstep2_t){q->bass[b].step[k].note,q->bass[b].step[k].flags};
                o->bass[b].len=q->bass[b].len;o->bass[b].rate=q->bass[b].rate;
                o->bass[b].dir=q->bass[b].dir;o->bass[b].transpose=q->bass[b].transpose;o->bass[b].gen=q->bass[b].gen;
            }
            o->brk.steps=q->brk.steps;memcpy(o->brk.set,q->brk.set,BRK_NSET);
            o->brk.slot_a=q->brk.slot_a;o->brk.slot_b=q->brk.slot_b;o->swing=q->swing;
        }
        CHECK(!save_obj(h?OBJ_PAT1:OBJ_PAT0,half,sizeof half));
    }
    memcpy(raw_fixture,&proj.arr.song,sizeof(song_t));
    for(unsigned k=0;k<NLANE;k++) {
        lane1_t old;memcpy(&old,&proj.arr.lane[k],sizeof old);
        memcpy(raw_fixture+sizeof(song_t)+k*sizeof old,&old,sizeof old);
    }
    CHECK(!save_obj(OBJ_SONG,raw_fixture,ARR1_A));
    CHECK(!save_obj(OBJ_MOTION,raw_fixture+ARR1_A,ARR1_LEN-ARR1_A));
    for(unsigned p=0;p<NPAT;p++) {
        if(p>=NPAT1)pattern_init(&expected.pat[p],0x3B0u+p*977u,0x5A1u+p*613u);
        else {
            for(unsigned b=0;b<NBASS;b++)for(unsigned k=0;k<NSTEPS;k++) {
                expected.pat[p].bass[b].step[k].velocity=0;
                if(k>=NSTEPS1)expected.pat[p].bass[b].step[k]=(bstep_t){
                    (uint8_t)(12*(expected.pat[p].bass[b].gen.base_oct+1)+expected.pat[p].bass[b].gen.root),0,0};
            }
            memset(expected.pat[p].brk.slice,0,sizeof expected.pat[p].brk.slice);
        }
    }
    for(unsigned k=1;k<NLANE;k++)
        memset(expected.arr.lane[k].val+NSTEPS1,MOT_NONE,NSTEPS-NSTEPS1);
}
static void check_music(void)
{
    CHECK(!memcmp(project_stage.pat,expected.pat,sizeof expected.pat));
    CHECK(!memcmp(&project_stage.arr,&expected.arr,sizeof expected.arr));
    CHECK(project_stage.set.snd_rev==13 && project_stage.set.palette==3 && project_stage.set.bpm_x10==1370);
    CHECK(project_stage.set.midi_ch[PART_303A]==5);
    CHECK(!memcmp(project_stage.sound.v[T_303],defaults.v[T_303],sizeof defaults.v[T_303]));
    CHECK(!memcmp(project_stage.sound.bass,defaults.bass,sizeof defaults.bass));
    CHECK(project_stage.sound.v[T_MIX][PART_909][MX_LEVEL]==93);
    CHECK(engine_bass_sound_bank(&project_stage.sound,0)==ORIGINAL_SOUND_MIAMI);
    CHECK(engine_bass_sound_bank(&project_stage.sound,1)==ORIGINAL_SOUND_CHICAGO);
    CHECK(!project_validate_stage());
}
static void stored_case(unsigned format,unsigned rev)
{
    reset_store();music();legacy_settings_sound(rev);
    if(format==7 || format==9) {
        proj.arr.lane[2]=(lane_t){.used=1,.pat=0,.part=PART_303A,.t=0xfd,.v=0};
        memset(proj.arr.lane[2].val,0xee,sizeof proj.arr.lane[2].val);
    }
    if(format==1)format1_fixture();else packed_fixture(format);
    live=proj;memcpy(objects_before,store,sizeof store);memcpy(lengths_before,store_len,sizeof store_len);
    unsigned writes=store_writes;
    CHECK(!project_load_stage());check_music();
    CHECK(!memcmp(&live,&proj,sizeof proj));CHECK(writes==store_writes);
    CHECK(!memcmp(objects_before,store,sizeof store));CHECK(!memcmp(lengths_before,store_len,sizeof store_len));
    project_commit_stage();project_apply_io();seq.bpm=137;seq.accent_q7=88;
    CHECK(!project_save());CHECK(get32(store[OBJ_PAT0])==13);
    CHECK(store[OBJ_SET][4+offsetof(settings_t,snd_rev)]==13);
    CHECK(!project_load_stage());check_music();
    /* Unknown and damaged formats fail without writing source objects. */
    put32(store[OBJ_PAT0],12);writes=store_writes;CHECK(project_load_stage()==-9);CHECK(writes==store_writes);
}
static void portable_case(unsigned rev)
{
    reset_store();music();proj.set.snd_rev=(uint8_t)rev;
    unsigned bytes=rev==10?sizeof proj-NBASS*BASS_SOUND_TAG_BYTES:sizeof proj;
    unsigned head=offsetof(project_t,pat),shift=rev==10?NBASS*BASS_SOUND_TAG_BYTES:0;
    memcpy(file_fixture,&proj,head-shift);
    memcpy(file_fixture+head-shift,&proj.pat,RAW_LEN);
    uint8_t h[PROJECT_FILE_HDR];memset(h,0,sizeof h);
    put32(h,0x50583058u);put32(h+4,1);put32(h+8,bytes);put32(h+12,crc32_of(file_fixture,bytes));
    put32(h+16,PROJ_FORMAT);put32(h+20,rev==12?12:8);put32(h+24,rev);
    memcpy(&project_stage,file_fixture,bytes);
    h[12]^=1;CHECK((rev==10?project_prepare_legacy_file(h):project_file_validate(h))==-4);h[12]^=1;
    CHECK(!memcmp(&project_stage,file_fixture,bytes));
    CHECK(!(rev==10?project_prepare_legacy_file(h):project_file_validate(h)));check_music();
    project_file_header(h);CHECK(!project_file_validate(h));CHECK(!store_writes);
    if(rev==10) {
        /* The real editor must finish migration without legacy identity adoption. */
        static uint8_t wire[PROJECT_FILE_SIZE];uint8_t args[270],confirm[2]={0x58,0x30};
        put32(h+8,bytes);put32(h+12,crc32_of(file_fixture,bytes));put32(h+20,8);put32(h+24,10);
        memcpy(wire,h,sizeof h);memcpy(wire+sizeof h,file_fixture,bytes);
        project_defaults();engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
        engine_apply_sound(&proj.sound);ui_init();run_ms(50);
        u21(args,bytes+sizeof h);proto_op(67,args,3,0);
        for(unsigned at=0;at<bytes+sizeof h;) {
            unsigned n=bytes+sizeof h-at;if(n>224)n=224;
            u21(args,at);unsigned packed=pack7(wire+at,n,args+3);proto_op(68,args,packed+3,0);at+=n;
        }
        proto_op(74,0,0,0);CHECK(ed_project.valid && !ed_project.legacy_prepared);check_music();
        proto_op(74,0,0,0);CHECK(ed_project.size==PROJECT_FILE_SIZE);
        proto_op(69,confirm,2,0);CHECK(!ed_project.mode && proj.set.snd_rev==13);
        CHECK(!store_writes && !flash_writes && !flash_erases);
    }
}
int main(void)
{
    engine_sound_defaults(&defaults);
    const unsigned formats[]={1,2,3,6,8};
    for(unsigned i=0;i<5;i++)stored_case(formats[i],10);
    stored_case(8,11);stored_case(7,11);stored_case(9,11);stored_case(12,12);
    portable_case(10);portable_case(11);portable_case(12);
    printf("PASS original project migration: %u checks, streams 1/2/3/6/7/8/9/12, revisions 10/11/12 to13, default Chicago/Miami, notes/arrangement, no load writes, save/reload, CRC and unknown rejection, portable imports\n",checks);
    return 0;
}
