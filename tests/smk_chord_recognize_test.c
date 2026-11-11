/* SPDX-License-Identifier: GPL-3.0-only */
/* Public recognizer contract: arbitrary played notes, not the selected chord preset. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CK_FN __attribute__((unused))
#define CK_CD_FN __attribute__((unused))
#include "app/smk_clip_keys.h"
#include "app/smk_chord_detect.h"

static unsigned checks;
static ck_chord_input_t input(const uint8_t *notes, unsigned count)
{
    ck_chord_input_t in = {0, 0, 255, 255};
    unsigned i;
    for (i = 0; i < count; i++)
        if (notes[i] < in.bass) in.bass = notes[i];
    for (i = 0; i < count; i++) {
        in.pcs |= 1u << (notes[i] % 12);
        if (notes[i] > in.bass) {
            in.upper_pcs |= 1u << (notes[i] % 12);
            if (notes[i] < in.upper_bass) in.upper_bass = notes[i];
        }
    }
    return in;
}
static void expect(const uint8_t *notes, unsigned count, const char *want)
{
    ck_chord_input_t in = input(notes, count);
    ck_chord_result_t r;
    char text[32];
    int found = ck_chord_detect(&in, &r);
    ck_chord_format(&r, text, sizeof text);
    checks++;
    if (strcmp(text, want)) {
        fprintf(stderr, "wanted %s, got %s (found %d, mask %03x)\n", want, text, found, in.pcs);
        assert(!strcmp(text, want));
    }
    assert(found == (strcmp(want, "-") != 0));
}
int main(void)
{
    static const char *const roots[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    static const struct { uint8_t n[7]; unsigned count; const char *name; } cases[] = {
        {{60,64,67},3,"C"}, {{64,67,72},3,"C"}, {{55,60,64},3,"C"},
        {{60,63,67},3,"Cm"}, {{60,64,67,71},4,"Cmaj7"}, {{60,63,67,70},4,"Cm7"},
        {{60,64,67,70},4,"C7"}, {{60,62,67},3,"Csus2"}, {{60,65,67},3,"Csus4"},
        {{60,63,66,69},4,"Cdim7"}, {{60,64,68},3,"Caug"}, {{60,63,66},3,"Cdim"},
        {{60,64,67,69},4,"C6"}, {{57,60,64,67},4,"Am7"},
        {{50,60,64,67},4,"C/D"}, {{53,60,64,67},4,"C/F"},
        {{40,59,62,66},4,"Bm/E"}, {{60,64,67,74},4,"Cadd9"},
        {{60,64,67,70,74},5,"C9"}, {{60,64,70},3,"C7"},
        {{60,60,64,67,72},5,"C"}, {{60,61,62},3,"-"},
        {{60,64},2,"-"}, {{60,72},2,"-"}, {{60},1,"-"}, {{0},0,"-"}
    };
    unsigned i, root, inv;
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) expect(cases[i].n,cases[i].count,cases[i].name);
    for (root = 0; root < 12; root++) {
        char want[24];
        for (inv = 0; inv < 3; inv++) {
            uint8_t notes[6];
            for (i = 0; i < 3; i++) {
                static const uint8_t major[] = {0,4,7};
                notes[i] = 48 + root + major[i] + (i < inv ? 12 : 0);
                notes[i+3] = notes[i] + 12;
            }
            snprintf(want,sizeof want,"%s",roots[root]);
            expect(notes,6,want);
            for (i = 0; i < 6; i++) if (i % 3 == 1) notes[i]--;
            snprintf(want,sizeof want,"%sm",roots[root]);
            expect(notes,6,want);
        }
    }
    /* Every fixed generator shape remains recognizable, across keys and voicings.
     * Scale-stack modes are covered separately: unusual scales may form unnamed clusters. */
    for (i = 0; i < CK_NCHORD; i++) {
        if (i >= 14 && i <= 16) continue;
        for (root = 0; root < 12; root++) for (inv = 0; inv < 4; inv++) {
            uint8_t notes[CK_MAXN]; ck_chord_result_t r;
            unsigned n = ck_notes(48+root,ck_mask(0,0),0,i,inv,notes);
            ck_chord_input_t in = input(notes,n);
            if (!ck_chord_detect(&in,&r)) {
                fprintf(stderr,"unrecognized generator mode %u root %u inversion %u mask %03x\n",i,root,inv,in.pcs);
                assert(0);
            }
            checks++;
        }
    }
    for (root = 0; root < 12; root++) for (i = 0; i < CK_NSCALE; i++) for (inv = 14; inv <= 16; inv++) {
        uint8_t notes[CK_MAXN]; ck_chord_result_t r;
        unsigned n = ck_notes(48+root,ck_mask(i,root),0,inv,0,notes);
        ck_chord_input_t in = input(notes,n);
        char text[32];
        int found = ck_chord_detect(&in,&r);
        ck_chord_format(&r,text,sizeof text);
        assert(found || !strcmp(text,"-")); checks++;
    }
    {
        static const char *const names[] = {"C", "Cmaj7", "Cmaj9"};
        for (i=14;i<=16;i++) {
            uint8_t notes[CK_MAXN];
            unsigned n=ck_notes(48,ck_mask(0,0),0,i,0,notes);
            expect(notes,n,names[i-14]);
        }
    }
    /* All pitch-class inputs and every output capacity: no overwrite, always terminated. */
    for (i = 0; i < 4096; i++) {
        uint8_t notes[12]; unsigned n=0,pc,cap; ck_chord_result_t r;
        for (pc=0;pc<12;pc++) if (i >> pc & 1) notes[n++]=48+pc;
        ck_chord_input_t in=input(notes,n);
        ck_chord_detect(&in,&r);
        for(cap=0;cap<=24;cap++) {
            unsigned char bytes[26]; memset(bytes,0xa5,sizeof bytes);
            ck_chord_format(&r,(char*)bytes+1,cap);
            assert(bytes[0]==0xa5 && bytes[cap+1]==0xa5);
            assert(!cap || memchr(bytes+1,0,cap));
            checks++;
        }
    }
    printf("chord recognition: %u checks passed\n",checks);
    return 0;
}
