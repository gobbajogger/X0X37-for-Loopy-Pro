/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "ui_page_reference.inc"
int main(void)
{
    unsigned cases = 0;
    boot();
    for (int view = 0; view < NVIEWS; ++view)
    for (int part = 0; part < NPARTS; ++part)
    for (int s0 = 0; s0 < NDRUM; ++s0)
    for (int s1 = 0; s1 < NDRUM; ++s1)
    for (int mix = 0; mix < NPARTS; ++mix)
    for (int page = 0; page < 3; ++page) {
        pages_t expected;
        uint8_t pages[NVIEWS][NPARTS];
        ui.view = view; ui.part = part;
        ui.sel[0] = s0; ui.sel[1] = s1; ui.mixsel = mix;
        memset(ui.page, page == 2 ? 255 : page, sizeof ui.page);
        memset(&pg, 0xa5, sizeof pg);
        reference_build_pages();
        expected = pg;
        memcpy(pages, ui.page, sizeof pages);
        memset(ui.page, page == 2 ? 255 : page, sizeof ui.page);
        memset(&pg, 0xa5, sizeof pg);
        build_pages();
        assert(!memcmp(&expected, &pg, sizeof pg));
        assert(!memcmp(pages, ui.page, sizeof pages));
        ++cases;
    }
    printf("PASS: %u page recipes match frozen implementation byte-for-byte\n", cases);
    return 0;
}
