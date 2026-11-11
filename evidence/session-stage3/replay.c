
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
void smc_poll(void);
void smc_frame(void);
int smc_connected(void);
#define main simulator_main
#include "/Users/mac/Documents/x0x37/smc_central_session_stage3_20261021/host/x0x_host.c"
#undef main
#include <assert.h>
#include "x0x_ble_abi.h"
#include "ble/ble_midi_host.h"
#include "ble/ble_midi_central.c"
#include "app/smk_inlog.h"
static smk_inlog_t log_ring;
static unsigned decoded;
void smk_inlog(uint32_t src,uint32_t pkt) { smk_inlog_put(&log_ring,now_ms,src,pkt); }
uint8_t smk_clip_mode_on(void) { return 0; }
int smk_ck_smc_turn(int n,int delta) { (void)n;(void)delta;return 0; }
int smk_ck_smc_nav(uint8_t n) { (void)n;return 0; }
uint8_t smk_clip_transport_button(uint8_t b,uint8_t p) { (void)b;(void)p;return 0; }
uint8_t smk_clip_history_button(uint8_t b,uint8_t p,uint8_t h) { (void)b;(void)p;(void)h;return 0; }
void smk_play_note_button(uint8_t p,uint8_t h) { (void)p;(void)h; }
uint8_t smk_ck_flash_busy(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *p,uint32_t f) { (void)p;(void)f;return 0; }
uint8_t smk_clip_lcd_draw(uint32_t w,uint32_t h) { (void)w;(void)h;return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_apc_transport_led_state(uint8_t n) { (void)n;return 0; }
uint8_t smk_padcal_on(void) { return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_padcal_draw(uint32_t w,uint32_t h) { (void)w;(void)h;return 0; }
#include "app/smc_mixer.c"
static struct { xb_mbox_t mb; union { bleh_t h; blec_t c; } host;
                uint8_t fr[XB_FRAME_MAX]; } ble_r;
#define receiver ble_r.host.c
#define fm1_ms now_ms
static uint8_t ble_up=1,ble_want=1,ble_host_on,fm1_c1_on=1,ble_central_link_seen,ble_central_role=1;
static uint32_t ble_alive[2],ble_ok_at;
static struct { int trying; } ble_guard;
#define BLE_GUARD_OK_MS 1000u
static void ble_guard_ok(void *p) { (void)p; }
static void ble_record(void) {}
static void ble_fail(void) { assert(0); }
static int hci_send(uint8_t t,const uint8_t *p,uint16_t n,void *ctx)
{ (void)ctx;return xb_frame_put(&ble_r.mb.h2c,t,p,n); }
static void ble_central_midi(const uint8_t *p, uint8_t n, void *ctx)
{
    (void)ctx;
    /* Reuse the bounded ISR-owned INLOG ring; MMC records command in d1. */
    if (n == 3u)
        smk_inlog(4u, (uint32_t)p[0] << 8 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 24);
    else if (n == 6u)
        smk_inlog(4u, (uint32_t)p[0] << 8 | (uint32_t)p[4] << 16);
    smc_ble_midi(p, n);                       /* TIMER5: SMC queues for the UI main loop */
}
static void ble_central_link_notify(int up)
{
    uint8_t next = (uint8_t)(up != 0);
    if (next != ble_central_link_seen) {
        ble_central_link_seen = next;
        smc_ble_link(next);
    }
}
static void ble_central_service(void)
{
    blec_t *c = &ble_r.host.c;
    uint32_t k;
    uint8_t t;
    if (!ble_up) { ble_central_link_notify(0); return; }
    if (ble_r.mb.state != XB_ST_READY || !fm1_c1_on) {
        ble_record(); ble_fail(); ble_central_link_notify(0); return;
    }
    ble_alive[0] = fm1_ms;
    ble_alive[1] = ble_r.mb.polls;
    if (ble_guard.trying && fm1_ms - ble_ok_at > BLE_GUARD_OK_MS)
        ble_guard_ok(&ble_guard);
    if (!ble_host_on) {
        if (!ble_want) return;
        ble_host_on = 1;
    }
    if (!ble_want && c->state != BLEC_OFF && c->state != BLEC_STOPPING)
        blec_stop(c);
    for (k = 0; k < 8u; k++) {
        int n = xb_frame_get(&ble_r.mb.c2h, &t, ble_r.fr, sizeof ble_r.fr);
        if (n == 0) break;
        if (n > 0) blec_rx(c, t, ble_r.fr, (uint16_t)n, fm1_ms);
    }
    if (!ble_want) {
        blec_poll(c, fm1_ms); /* finish disable/cancel/disconnect while radio is OFF */
        ble_central_link_notify(0);
        return;
    }
    if (c->state == BLEC_STOPPING) {
        blec_poll(c, fm1_ms);
        ble_central_link_notify(0);
        return;
    }
    if (c->state == BLEC_OFF) {
        /* Drain old HCI events before a fresh Reset starts a new command sequence. */
        if (xb_ring_used(&ble_r.mb.c2h)) return;
        blec_start(c, fm1_ms);
    }
    blec_poll(c, fm1_ms);
    ble_central_link_notify(blec_connected(c));
}
int plat_ble_smc_send(const uint8_t *msg, uint8_t len)
{
    return ble_central_role && ble_up && ble_host_on && ble_want &&
           blec_midi_send(&ble_r.host.c, msg, len);
}

static void got(const uint8_t *p,uint8_t n,void *ctx)
{ decoded++;ble_central_midi(p,n,ctx); }
static void incoming(unsigned t,const uint8_t *p,unsigned n)
{ assert(!xb_frame_put(&ble_r.mb.c2h,t,p,n));ble_central_service(); }
static void expect_tx(unsigned type,unsigned op)
{
 uint8_t t,b[300];int n=xb_frame_get(&ble_r.mb.h2c,&t,b,sizeof b);
 if (!(n>0 && t==type && (type==1?blec_rd16(b):b[8])==op)) {
  fprintf(stderr,"tx mismatch n%d type%u op%u expected%u\n",n,t,n>8?b[8]:0,op);abort();
 }
}
static void complete(unsigned op)
{
 uint8_t b[]={0x0e,7,1,op,op>>8,0,27,0,5};
 expect_tx(1,op);incoming(4,b,sizeof b);
}
static void peer_att(const uint8_t *p,unsigned n)
{
 uint8_t b[300]={0x50,0x20,0,0,0,0,4,0};
 b[2]=n+4;b[4]=n;memcpy(b+8,p,n);incoming(2,b,n+8);
}
static void credit(void)
{ const uint8_t b[]={0x13,5,1,0x50,0,1,0};incoming(4,b,sizeof b); }
static void response(const uint8_t *p,unsigned n,unsigned request)
{ expect_tx(2,request);credit();peer_att(p,n); }
static void startup(void)
{
 ble_central_service();complete(0x0c03);complete(0x0c01);complete(0x2001);
 complete(0x2002);complete(0x2005);complete(0x200b);complete(0x200c);
 assert(receiver.state==BLEC_SCANNING);
 const uint8_t adv[]={0x3e,0x17,2,1,0,1,1,2,3,4,5,6,11,10,9,'S','M','C','-','M','i','x','e','r',0xc0};
 incoming(4,adv,sizeof adv);complete(0x200c);expect_tx(1,0x200d);
 const uint8_t status[]={0x0f,4,0,1,0x0d,0x20};incoming(4,status,sizeof status);
 const uint8_t conn[]={0x3e,0x13,1,0,0x50,0,0,1,2,3,4,5,6,0,0,0,0,0,0,0,0};
 incoming(4,conn,sizeof conn);
 expect_tx(2,0x10);
 const uint8_t update[]={0x50,0x20,16,0,12,0,5,0,0x12,0xab,8,0,6,0,12,0,0,0,0x90,1};
 incoming(2,update,sizeof update);
 uint8_t t,reply[40];int rn=xb_frame_get(&ble_r.mb.h2c,&t,reply,sizeof reply);
 const uint8_t want[]={0x50,0,10,0,6,0,5,0,0x13,0xab,2,0,1,0};
 assert(t==2 && rn==sizeof want && !memcmp(reply,want,sizeof want));credit();
 const uint8_t gap[]={0x11,6,1,0,3,0,0,0x18};credit();peer_att(gap,sizeof gap);
 const uint8_t svc[]={0x11,20,0x70,0,0x7f,0,0,0xc7,0xc4,0x4e,0xe3,0x6c,0x51,0xa7,0x33,0x4b,0xe8,0xed,0x5a,0x0e,0xb8,3};
 response(svc,sizeof svc,0x10);
 const uint8_t chr[]={9,21,0x71,0,0x16,0x72,0,0xf3,0x6b,0x10,0x9d,0x66,0xf2,0xa9,0xa1,0x12,0x41,0x68,0x38,0xdb,0xe5,0x72,0x77};
 response(chr,sizeof chr,8);
 const uint8_t info[]={5,1,0x73,0,2,0x29};response(info,sizeof info,4);
 const uint8_t wr[]={0x13};response(wr,sizeof wr,0x12);
 assert(blec_connected(&receiver) && receiver.midi_handle==114 && receiver.cccd_handle==115);
 ui_input_only();assert(smc.up);
}
static unsigned feedback;
static void service_feedback(void)
{
 uint8_t t,b[300];int n;
 for(unsigned k=0;k<100;k++) {
  ble_central_service();
  n=xb_frame_get(&ble_r.mb.h2c,&t,b,sizeof b);
  if(n>0) {
   assert(t==2 && b[8]==0x52 && blec_rd16(b+9)==114);
   feedback++;credit();
  }
  if(receiver.qr==receiver.qw && !xb_ring_used(&ble_r.mb.h2c)) return;
 }
 assert(0);
}
static void input_payload(const uint8_t *p,unsigned n)
{
 uint8_t b[300]={0x1b,0x72,0};memcpy(b+3,p,n);peer_att(b,n+3);
 ui_frame();service_feedback();now_ms+=16;
}
static void message(unsigned st,unsigned a,unsigned b)
{ uint8_t p[]={0x80,0x81,st,a,b};input_payload(p,sizeof p); }
int main(int argc,char **argv)
{
 assert(argc==3);boot();FILE *saved=fopen(argv[2],"rb");
 if(saved) {
  assert(fread(store_len,1,sizeof store_len,saved)==sizeof store_len);
  assert(fread(store,1,sizeof store,saved)==sizeof store);fclose(saved);
 } else {
  proj.set.bt=PROJ_BT_SMC_CENTRAL|PROJ_BT_CLK_IN|1;
  assert(project_save()==0);saved=fopen(argv[2],"wb");assert(saved);
  assert(fwrite(store_len,1,sizeof store_len,saved)==sizeof store_len);
  assert(fwrite(store,1,sizeof store,saved)==sizeof store);fclose(saved);
 }
 memset(&proj,0,sizeof proj);boot();
 assert(proj.set.bt==(PROJ_BT_SMC_CENTRAL|PROJ_BT_CLK_IN|1));
 set_view(V_HOME);build_pages();
 ble_r.mb.state=XB_ST_READY;
 uint8_t addr[6]={0};blec_init(&receiver,hci_send,got,0,addr);startup();
 for(unsigned i=0;i<3;i++) { ui_frame();service_feedback();now_ms+=16; }
 assert(feedback>=43);
 pref_t level=PR(R_ENG,T_MIX,0,MX_LEVEL);
 pref_set(level,64);smc_frame();service_feedback();
 uint8_t target=smc_scaled(level);message(0xe0,0,target);message(0xe0,0,100);
 assert(pref_get(level)>64);
 set_view(V_PART);build_pages();pref_t knob=pg.r[cur_page()][0];
 int old=pref_get(knob);const x0x_param_t *d=pref_desc(knob);assert(d && d->max);
 message(0xb0,16,old==d->max?65:1);assert(pref_get(knob)!=old);
 message(0x90,0x5e,127);assert(seq.playing || seq.req_start);
 message(0x90,0x5d,127);assert(!seq.playing || seq.req_stop);
 FILE *f=fopen(argv[1],"rb");assert(f);uint8_t h[2],p[256];unsigned packets=0;
 unsigned before=decoded;
 while(fread(h,1,2,f)==2) {
  unsigned n=h[0]|(unsigned)h[1]<<8;assert(n<=61 && fread(p,1,n,f)==n);
  input_payload(p,n);packets++;
 }
 assert(feof(f));fclose(f);
 printf("{\"packets\":%u,\"decoded\":%u,\"feedback\":%u,\"link\":%u,\"queue_empty\":%s,\"perf_test\":%d,\"perf_phase\":%d,\"help\":%u,\"overlay\":%u}\n",
        packets,decoded-before,feedback,smc.up,smc_qr==smc_qw?"true":"false",perf.test,perf.phase,ui.help,ui.overlay);
 assert(packets==1268 && decoded-before==2616 && smc.up && smc_qr==smc_qw);
}
