/* Exercise production SMC navigation while a keyboard 303 step is held.
 * SMC must leave the keyboard part, page and temporary sound untouched. */
#include <stdint.h>
#include <stdio.h>

enum { V_PART, V_GEN, PART_303A = 2, PART_303B = 3, PART_909 = 0 };
static struct {
    uint8_t part, mixsel, view, outline_ok;
    int8_t held_step;
} ui;
static int page_part, cancel_count, cancel_part, cancel_page;
static int old_saved = 64, old_sounding = 92;
static struct { uint8_t part,mixsel,view,picked,pending; } smc;
static int popup_count;
static void smc_ui_begin(int strip,int kind) { (void)strip;(void)kind;popup_count++; }
#include "smc_select_part.inc"

static int fails;
#define CHECK(ok, label) do { if (!(ok)) { fprintf(stderr, "FAIL %s\n", label); fails++; } } while (0)
int main(void)
{
    ui.part = PART_303A;
    ui.mixsel = PART_303A;
    ui.view = V_PART;
    ui.held_step = 0;
    page_part = PART_303A;
    smc.view = V_PART; smc.part = PART_303A; smc.picked = 0xff;
    smc_select_part(PART_303B);
    CHECK(cancel_count == 0 && cancel_part == 0 && cancel_page == 0,
          "SMC navigation never cancels the keyboard held step");
    CHECK(old_sounding == 92 && old_saved == 64, "keyboard temporary 303 sound remains held");
    CHECK(ui.part == PART_303A && ui.mixsel == PART_303A && page_part == PART_303A && ui.held_step == 0,
          "keyboard part, selection, page and held step remain unchanged");
    CHECK(smc.part == PART_303B && smc.mixsel == PART_303B && popup_count == 1 && smc.pending == 0xff && !smc.picked,
          "SMC selects its own destination and refreshes its popup");

    ui.held_step = 1;
    smc_select_part(PART_303B);
    CHECK(cancel_count == 0 && ui.held_step == 1, "selecting the same part leaves its held step active");

    ui.part = PART_303A;
    ui.view = V_GEN;
    ui.held_step = 2;
    page_part = PART_303A;
    smc.view = V_GEN;
    smc_select_part(PART_909);
    CHECK(cancel_count == 0 && ui.held_step == 2 && page_part == PART_303A && old_sounding == 92,
          "GEN to drum SMC navigation preserves keyboard held sound and owner");
    CHECK(ui.view == V_GEN && ui.part == PART_303A, "keyboard GEN view remains unchanged");
    CHECK(smc.view == V_PART && smc.part == PART_909 && smc.mixsel == PART_909, "SMC GEN to drum opens its own part view");
    puts(fails ? "smc_part_select: failed" : "smc_part_select: ok");
    return fails != 0;
}
