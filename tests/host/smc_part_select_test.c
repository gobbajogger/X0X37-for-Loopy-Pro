/* Exercise the production SMC part-selection function with a held 303 step.
 * The old part/page must still own the temporary sound when cleanup runs. */
#include <stdint.h>
#include <stdio.h>

enum { V_PART, V_GEN, PART_303A = 2, PART_303B = 3, PART_909 = 0 };
static struct {
    uint8_t part, mixsel, view, outline_ok;
    int8_t held_step;
} ui;
static int page_part, cancel_count, cancel_part, cancel_page;
static int old_saved = 64, old_sounding = 92;
static int is_303(void) { return ui.part == PART_303A || ui.part == PART_303B; }
static void bass_step_cancel(void)
{
    if (ui.held_step < 0)
        return;
    cancel_count++;
    cancel_part = ui.part;
    cancel_page = page_part;
    if (ui.part == PART_303A && page_part == PART_303A)
        old_sounding = old_saved;      /* lock_release restores the selected page's sound */
    ui.held_step = -1;
}
#define drum_step_cancel() ((void)0) /* no drum gesture in this 303-focused fixture */
static void set_view(int v) { ui.view = (uint8_t)v; }
static void build_pages(void) { page_part = ui.part; }
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
    smc_select_part(PART_303B);
    CHECK(cancel_count == 1 && cancel_part == PART_303A && cancel_page == PART_303A,
          "held step cancels while its original part and page still own it");
    CHECK(old_sounding == old_saved, "temporary 303 lock sound restores on SMC part select");
    CHECK(ui.part == PART_303B && ui.mixsel == PART_303B && page_part == PART_303B && ui.held_step == -1,
          "SMC navigation still selects and rebuilds the destination part");

    ui.held_step = 1;
    smc_select_part(PART_303B);
    CHECK(cancel_count == 1 && ui.held_step == 1, "selecting the same part leaves its held step active");

    ui.part = PART_303A;
    ui.view = V_GEN;
    ui.held_step = 2;
    page_part = PART_303A;
    smc_select_part(PART_909);
    CHECK(cancel_count == 2 && cancel_part == PART_303A && cancel_page == PART_303A,
          "GEN to drum selection cancels before changing owner");
    CHECK(ui.view == V_PART && ui.part == PART_909, "GEN navigation still opens the part view");
    puts(fails ? "smc_part_select: failed" : "smc_part_select: ok");
    return fails != 0;
}
