#!/bin/sh
# Run the real clip lifecycle + panel with mocked USB and captured draw calls.
# Reuse the existing knob harness extraction; render metrics come from generated production fonts.
set -eu
cd "$(dirname "$0")/../.."
SRC=${SRC:-firmware/src}
OUT=${OUT:-build/host/clip_chord}
export SRC OUT
sh tests/host/smk_clip_knob_test.sh --extract-only
python3 tools/gen_font.py "$OUT/felucca_font.h"
python3 - "$SRC" "$OUT" <<'PY'
import pathlib,sys
src,out=map(pathlib.Path,sys.argv[1:])
s=(src/'app/main_fm1.c').read_text()
a=s.index('/* ------- clip mode screen:')
b=s.index('/* The stage settings',a)
(out/'chord_screen.inc').write_text(s[a:b])
g=(src/'gfx.c').read_text(); u=(src/'app/ui.c').read_text()
def fn(s,name):
 a=s.index(name); a=s.rfind('\n',0,a)+1; b=s.index('{',a); level=1; e=b+1
 while level:
  level += (s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]+'\n'
(out/'chord_metrics.inc').write_text(fn(g,'static uint32_t glyph(')+fn(g,'static int32_t text_w(')+
 'typedef struct { const felucca_font_t *f; int8_t top, bot; } capm_t;\nstatic capm_t capm[6];\n'+
 fn(u,'static const capm_t *caps(')+fn(u,'static int32_t vc('))
PY
# CK_RATE_T / ck_arp_pick live in smk_clip_arp_share.c (size share); same link as smk-clip-keys.
${CC:-cc} -std=gnu99 -O1 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unknown-attributes \
  -I"$SRC" -Ifirmware/hal -I"$OUT" -o "$OUT/smk_clip_chord_display_test" \
  tests/host/smk_clip_chord_display_test.c firmware/src/app/smk_clip_arp_share.c
"$OUT/smk_clip_chord_display_test"
