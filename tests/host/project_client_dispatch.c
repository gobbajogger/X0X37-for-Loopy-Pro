/* SPDX-License-Identifier: GPL-3.0-only */
/* Real editor service with the existing project-protocol fixture boundaries.
 * stdin/stdout carry decimal SysEx frames. There is no caller identity, matching
 * the production USB frame queue. Storage/UI remain simulated, not device proof. */
#define main project_protocol_fixture_main
#include "project_protocol_test.c"
#undef main
#include <stdlib.h>

static void word(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static void export_header(void)
{
    /* A synthetic, browser-readable revision-13 fixture. The production parser
     * and dispatcher are under test; project semantics have separate tests. */
    if (reply[4] != ED_PROJECT || reply[5] != 65 || reply[6] || ed_project.mode != 1)
        return;
    memset(ed_project.header, 0, sizeof ed_project.header);
    word(ed_project.header, 0x50583058u);
    word(ed_project.header + 4, 1);
    word(ed_project.header + 8, sizeof project_stage);
    word(ed_project.header + 12, st_crc32((const uint8_t *)&project_stage, sizeof project_stage));
    word(ed_project.header + 16, 1);
    word(ed_project.header + 20, 13);
    word(ed_project.header + 24, 13);
}

int main(void)
{
    char line[8192];
    uint8_t packet[640];
    wire_after_send = export_header;
    while (fgets(line, sizeof line, stdin)) {
        char *p = line, *end;
        uint32_t n = 0, i;
        while (*p) {
            unsigned long b = strtoul(p, &end, 10);
            if (p == end) break;
            assert(b <= 255 && n < sizeof packet);
            packet[n++] = (uint8_t)b;
            p = end;
        }
        assert(n >= 6 && packet[0] == 0xF0 && packet[n - 1] == 0xF7);
        assert(packet[1] == ED_HDR0 && packet[2] == ED_HDR1 && packet[3] == ED_HDR2);
        request(packet[4], packet + 5, n - 6);
        for (i = 0; i < reply_n; i++)
            printf(i ? " %u" : "%u", reply[i]);
        putchar('\n');
        fflush(stdout);
    }
    return 0;
}
