/* SPDX-License-Identifier: GPL-3.0-only
 * Host regression for invalid but range-correct panel calibration.
 * Compile as both BOARD_SMK37 variants, against exact firmware/src/app/panel.c.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NB 14u
#define NE 7u
#define FM1_NBTN 18u
#define FM1_NENC 8u
#define SMK_ENC_MASTER 0u
#define OBJ_PANEL 9
#define C_BLACK 0
#define C_WHITE 1
#define C_GRAY 2
static const int FONT_S = 0, FONT_M = 0;
static uint32_t fm1_ms;
static struct { uint32_t buttons; } fm1_in;
static unsigned saves;
static const void *fake_storage;
static int plat_store_load(int obj, void *p, unsigned n) { (void)obj; if (!fake_storage) return -1; memcpy(p, fake_storage, n); return (int)n; }
static int plat_store_save(int obj, const void *p, unsigned n) { (void)obj; (void)p; (void)n; ++saves; return 0; }
static void lcd_fill(int x,int y,int w,int h,int color) {(void)x;(void)y;(void)w;(void)h;(void)color;}
static void draw_text_box(int x,int y,int w,const int *font,const char *s,int color,int mode) {(void)x;(void)y;(void)w;(void)font;(void)s;(void)color;(void)mode;}
static void fm1_wdt_feed(void) { ++fm1_ms; }
static uint32_t fm1_input_edges(int mode) {(void)mode; return 0;}
static int32_t fm1_enc_take(unsigned e) {(void)e; return 0;}
static void fm1_delay_ms(unsigned ms) {fm1_ms += ms;}
#include "../firmware/src/app/panel.c"
static int fails;
#define CHECK(expr, desc) do { if (!(expr)) {printf("FAIL: %s\n", desc); ++fails;} else {printf("ok: %s\n", desc);} } while(0)
int main(void) {
    (void)&panel_setup;
    panel_t p = PANEL_DEFAULT;
    CHECK(panel_valid(&p), "default map accepted");
    p.btn[1] = p.btn[0];
    CHECK(!panel_valid(&p), "duplicate physical button rejected");
    memset(&panel, 0, sizeof panel);
    fake_storage = &p;
    panel_init();
    CHECK(!memcmp(&panel, &PANEL_DEFAULT, sizeof panel), "duplicated persisted button map falls back to defaults");
    p = PANEL_DEFAULT;
    p.enc[1] = p.enc[0];
    CHECK(!panel_valid(&p), "duplicate physical encoder rejected");
    memset(&panel, 0, sizeof panel);
    fake_storage = &p;
    panel_init();
    CHECK(!memcmp(&panel, &PANEL_DEFAULT, sizeof panel), "duplicated persisted encoder map falls back to defaults");
#if BOARD_SMK37
    p = PANEL_DEFAULT;
    p.enc[0] = SMK_ENC_MASTER;
    CHECK(!panel_valid(&p), "reserved SMK master encoder never assigned a role");
#endif
    p = PANEL_DEFAULT;
    {
        uint8_t t = p.btn[0];
        p.btn[0] = p.btn[1]; p.btn[1] = t;
        t = p.enc[0];
        p.enc[0] = p.enc[1]; p.enc[1] = t;
    }
    CHECK(panel_valid(&p), "legal button and encoder permutation accepted");
    fake_storage = &p;
    panel_init();
    CHECK(!memcmp(&panel, &p, sizeof panel), "legal persisted permutation loaded unchanged");
    p = PANEL_DEFAULT;
    p.dir[3] = -1;
    CHECK(panel_valid(&p), "valid inverted encoder direction accepted");
    p = PANEL_DEFAULT;
    p.enc[0] = PANEL_NENC;
    CHECK(!panel_valid(&p), "out of range encoder rejected");
    p = PANEL_DEFAULT;
    p.btn[0] = PANEL_NBTN;
    CHECK(!panel_valid(&p), "out of range button rejected");
    p = PANEL_DEFAULT;
    p.magic ^= 1;
    CHECK(!panel_valid(&p), "wrong model magic rejected");
    p = PANEL_DEFAULT;
    p.dir[0] = 0;
    CHECK(!panel_valid(&p), "invalid encoder direction rejected");
    memset(&panel, 0, sizeof panel);
    fake_storage = &PANEL_DEFAULT;
    panel_init();
    CHECK(!memcmp(&panel, &PANEL_DEFAULT, sizeof panel), "valid persisted map accepted");
    printf("panel mapping: %d failure(s), variant SMK=%d\n", fails, BOARD_SMK37);
    return fails != 0;
}
