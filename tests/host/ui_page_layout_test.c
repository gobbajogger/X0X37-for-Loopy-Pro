/* SPDX-License-Identifier: GPL-3.0-only */
/* Production UI page descriptors across every view/part, drum selection,
 * mixer channel, and valid/overflow page state. The oracle comes from the
 * accepted pre-optimization layout, not the immutable descriptor tables. */
#define SMC_UI_MAIN inherited_ui_main
#include "smc_ui_test.c"
static uint64_t layout_hash = UINT64_C(14695981039346656037);
static void layout_byte(unsigned b)
{
    layout_hash ^= (uint8_t)b;
    layout_hash *= UINT64_C(1099511628211);
}
static void layout_ref(pref_t r)
{
    layout_byte(r.kind); layout_byte(r.a); layout_byte(r.b); layout_byte(r.c);
}
int main(void)
{
    unsigned cases = 0;
    for (int view = 0; view < NVIEWS; view++) for (int part = 0; part < NPARTS; part++) {
        setup(view, part);
        for (int a = 0; a < NDRUM; a++) for (int overflow = 0; overflow < 2; overflow++)
            for (int mix = 0; mix < NPARTS; mix++) {
                ui.sel[0] = a; ui.sel[1] = NDRUM - 1 - a; ui.mixsel = mix;
                *page_of(view) = overflow ? 255 : 0;
                build_pages();
                layout_byte(view); layout_byte(part); layout_byte(a);
                layout_byte(overflow); layout_byte(mix);
                layout_byte(pg.n); layout_byte(pg.nmore); layout_byte(*page_of(view));
                for (int page = 0; page < pg.n; page++) {
                    layout_byte(pg.group[page]);
                    const unsigned char *title_text = (const unsigned char *)pg.title[page];
                    do { layout_byte(*title_text); } while (*title_text++);
                    for (int knob = 0; knob < 4; knob++) layout_ref(pg.r[page][knob]);
                }
                for (int i = 0; i < pg.nmore; i++) layout_ref(pg.more[i]);
                cases++;
            }
    }
    printf("ui_page_layout: %u cases, FNV64 %016llx\n", cases, (unsigned long long)layout_hash);
    CHECK(cases == 4950);
    CHECK(layout_hash == UINT64_C(0x8fdfc4ebb1717842));
    return 0;
}
