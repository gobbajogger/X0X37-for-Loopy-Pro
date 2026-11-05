/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual platform name aggregation and UI row, with mocked radio link state. */
#include <stdint.h>
#include <string.h>
#include <assert.h>
typedef struct { uint8_t up, link_name[21]; } blec_t;
static struct { char nm[21]; struct { blec_t c,c2; } host; } ble_r;
static int ble_central_role, locked_irq;
static int blec_connected(const blec_t *p) { return p->up; }
static uint32_t irq_save(void) { assert(!locked_irq); locked_irq=1; return 42; }
static void irq_restore(uint32_t f) { assert(f==42 && locked_irq); locked_irq=0; }
#include "ble_peer_names.inc"
#define X0X_HOST_BLE_NAME_IMPLEMENTED 1
#define SMC_UI_MAIN smc_ui_baseline_main
#include "smc_ui_test.c"
static void names(const char *a,const char *b)
{
    memset(&ble_r,0,sizeof ble_r); ble_central_role=1;
    if(a) { ble_r.host.c.up=1; strncpy((char *)ble_r.host.c.link_name,a,20); }
    if(b) { ble_r.host.c2.up=1; strncpy((char *)ble_r.host.c2.link_name,b,20); }
}
static void render_names(int selected,const char *shot_name)
{
    ui.overlay=O_LIST; ui.list_top=0; ui.list_n=2; ui.list_sel=selected?0:1;
    list_rows[0]=PR(R_BTNAME,0,0,0); list_rows[1]=PR(R_BTSTAT,0,0,0);
    char num[64],unit[8]; pref_value(list_rows[0],num,unit);
    CHECK(strlen(num)<=42); CHECK(!strcmp(num,plat_ble_name()[0]?plat_ble_name():"-"));
    int room=226-8-tw(selected?&FONT_B:&FONT_S,"NAME")-8;
    bt_peer_text(num,room); CHECK(tw(&FONT_T,num)<=room);
    if(strlen(plat_ble_name())>20) CHECK(strstr(num,", "));
    ui_frame(); shot(shot_name);
}
int main(void)
{
    setup(V_GLO,PART_909);
    names("SMC Left","SMC Right"); CHECK(!strcmp(plat_ble_name(),"SMC Left, SMC Right"));
    render_names(1,"names-two-selected"); render_names(0,"names-two-unselected");
    names("SMC-Left-Panel","SMC-Right-Panel");
    render_names(1,"names-long-two-selected");
    render_names(0,"names-long-two-unselected");
    names("SMC-Mixer","SMC-Mixer"); CHECK(!strcmp(plat_ble_name(),"SMC-Mixer, SMC-Mixer"));
    render_names(1,"names-identical");
    names("",NULL); CHECK(!strcmp(plat_ble_name(),"SMC-Mixer"));
    names(NULL,"Right"); CHECK(!strcmp(plat_ble_name(),"Right"));
    strcpy((char *)ble_r.host.c.link_name,"stale"); CHECK(!strcmp(plat_ble_name(),"Right"));
    names(NULL,NULL); CHECK(!plat_ble_name()[0]); render_names(1,"names-disconnected");
    ble_central_role=0; strcpy(ble_r.nm,"X0X37"); CHECK(!strcmp(plat_ble_name(),"X0X37"));
    /* Pick the widest supported printable native glyph, not an assumed W. */
    char wide[21],pair[43]; int width=0; char glyph='W';
    for(char c=32;c<127;c++) { char t[2]={c,0}; int w=tw(&FONT_T,t); if(w>width){width=w;glyph=c;} }
    memset(wide,glyph,20); wide[20]=0;
    names(wide,wide); CHECK(strlen(plat_ble_name())==42);
    strcpy(pair,wide); strcat(pair,", "); strcat(pair,wide); CHECK(!strcmp(pair,plat_ble_name()));
    render_names(1,"names-max-selected"); render_names(0,"names-max-unselected");
    printf("PASS peer names and actual UI buffers: %d checks, max native glyph width %d\n",checks,width);
    return 0;
}
