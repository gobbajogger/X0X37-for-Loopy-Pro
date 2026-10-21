# SPDX-License-Identifier: GPL-3.0-only
# NON-INSTALLABLE sizing study; modifies only the explicit disposable export.
from pathlib import Path
import sys
if len(sys.argv) != 2:
    raise SystemExit("usage: prepare.py DISPOSABLE_BASELINE_EXPORT")
p=Path(sys.argv[1]).resolve()
if (p / '.git').exists():
    raise SystemExit("refuse a Git worktree: use a disposable git archive export")
f=p/'tools/gen_font.py';s=f.read_text();old='    Path(out).write_text("\\n".join(lines))';new='''    text = "\\n".join(lines)
    import re
    for name in ("FONT_B", "FONT_XS", "FONT_B_OFF", "FONT_XS_OFF", "FONT_B_ADV", "FONT_XS_ADV", "FONT_B_DATA", "FONT_XS_DATA"):
        text = re.sub(r"(static const [^\\n]+?\\b" + name + r"(?:\\[[^]]+\\])?) =", r'\\1 __attribute__((section(".app_ui.' + name + r'"))) =', text)
    Path(out).write_text(text)''';assert old in s;s=s.replace(old,new);f.write_text(s)
f=p/'firmware/src/app/ui.c';s=f.read_text().replace('CORNER_OFF[RMAX + 1] =','CORNER_OFF[RMAX + 1] __attribute__((section(".app_ui.CORNER_OFF"))) =');f.write_text(s)
f=p/'firmware/src/app/main_fm1.c';s=f.read_text().replace('&FONT_B, "X0X SAFE MODE"','&FONT_S, "X0X SAFE MODE"');f.write_text(s)
f=p/'firmware/app.ld';s=f.read_text().replace('    RAMTEXT (rwx)', '    APPUI (r) : ORIGIN = 0x020A7000, LENGTH = 0x2000\n    RAMTEXT (rwx)');order=['FONT_B','FONT_XS','FONT_B_OFF','FONT_XS_OFF','FONT_B_ADV','FONT_XS_ADV','CORNER_OFF','FONT_B_DATA','FONT_XS_DATA'];s=s.replace('    .text :', '    .app_ui : { '+ ' '.join('KEEP(*(.app_ui.'+n+'))' for n in order)+' } > APPUI\n    ASSERT(SIZEOF(.app_ui) == 8192, "exact UI data sizing")\n    .text :');f.write_text(s)
f=p/'tools/build.py';s=f.read_text();pos=s.index('    pkg = fm1pkg_make.ufw');s=s[:pos]+'    print("NON-INSTALLABLE: app relocation sizing only; no data admission or package")\n    return 0\n'+s[pos:];f.write_text(s)
