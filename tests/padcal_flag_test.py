#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The build flag X0X_DEV_PADCAL (BEHAVIOR.md CLP-79): 1 = dev build with the pad calibration, 0 = release.

The test keeps the pad calibration module. It checks the flag itself:
- app/x0x.h makes 1 the default, and tools/build.py passes X0X_DEV_PADCAL=0 or 1 from the environment.
- With 0, the source after the #if X0X_DEV_PADCAL blocks (a small preprocessor that only knows this flag)
  has no pad calibration code in main_fm1.c, ui.c and plat_fm1.c: no padcal name, no "PAD COLOURS",
  no "pad 1 top-left?", no apc40_pad_gamma.h. INLOG args 2-5 then go to the entries reply, as before CLP-79.
- With 1, the same files keep all of it (the padcal tests run on this form).
The Elite builds of both flags: BUILDING.md (nm and strings, in the commit report)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "firmware/src/app"
fails = 0


def check(ok, what):
    global fails
    print(f"{'PASS' if ok else 'FAIL'} {what}")
    fails += not ok


def strip(text, flag):
    """Keep the lines that the build keeps for X0X_DEV_PADCAL=flag (other conditions: both branches)."""
    out, stack = [], []          # stack: (ours, this branch on)
    for line in text.split("\n"):
        s = line.strip()
        m = re.match(r"#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)", s)
        if m:
            kw, cond = m.group(1), re.sub(r"/\*.*?(\*/|$)|//.*", "", m.group(2))
            if kw in ("if", "ifdef", "ifndef"):
                ours = "X0X_DEV_PADCAL" in cond and kw == "if"
                on = True
                if ours:
                    on = bool(eval(re.sub(r"\w+", lambda w: str(flag) if w.group(0) == "X0X_DEV_PADCAL" else "1",
                                          cond).replace("&&", " and ").replace("||", " or ").replace("!", " not ")))
                stack.append((ours, on))
            elif kw == "else" and stack:
                ours, on = stack[-1]
                stack[-1] = (ours, (not on) if ours else True)
            elif kw == "endif" and stack:
                stack.pop()
            continue
        if all(on for _, on in stack):
            out.append(line)
    return "\n".join(out)


x0x_h = (SRC / "x0x.h").read_text()
build = (ROOT / "tools/build.py").read_text()
check(re.search(r"#ifndef X0X_DEV_PADCAL[^\n]*\n#define X0X_DEV_PADCAL 1\b", x0x_h) is not None,
      "app/x0x.h: the default is 1 (dev build)")
check(re.search(r'for flag in \([^)]*"X0X_DEV_PADCAL"', build) is not None
      and 'v = os.environ.get(flag)' in build
      and 'if v in ("0", "1"):' in build
      and 'flags.append(f"-D{flag}={v}")' in build,
      "tools/build.py: X0X_DEV_PADCAL=0 or 1 from the environment goes to the compiler")

building = (ROOT / "BUILDING.md").read_text()
check("X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37" in building and "`X0X_DEV_PADCAL=1`" in building
      and "padcal-v1" in building, "BUILDING.md: the release build command, the dev value and the tag padcal-v1")
pad_h = (SRC / "apc40_pad_default.h").read_text()
owner_h = " ".join((SRC / "apc40_pad_owner.h").read_text().replace("*", " ").split())
check("--pad-name APC40_PAD_OWNER writes the live pad table" in " ".join(pad_h.replace("*", " ").split()) and
      "in the dev and the release build" in owner_h,
      "app/apc40_pad_owner.h (generated): the owner-calibrated pad table is the pad table of both builds")
BAD = re.compile(r"padcal|PAD COLOURS|pad 1 top-left|apc40_pad_gamma|apc40_pad_names|ACT_PADCAL|BOOST X|REAL LEVEL", re.I)
for name in ("main_fm1.c", "ui.c", "plat_fm1.c"):
    text = (SRC / name).read_text()
    rel = strip(text, 0)
    left = [ln.strip()[:80] for ln in rel.split("\n") if BAD.search(ln) and not ln.strip().startswith(("/*", "*"))]
    inc = [ln for ln in rel.split("\n") if ln.startswith("#include") and BAD.search(ln)]
    left += inc
    check(not left, f"{name}, flag 0: no pad calibration code ({left[:3]})")
    dev = "\n".join(ln for ln in strip(text, 1).split("\n") if not ln.strip().startswith("#"))
    code = "\n".join(ln for ln in text.split("\n") if not ln.strip().startswith("#"))
    check(len(BAD.findall(dev)) == len(BAD.findall(code)), f"{name}, flag 1: all the pad calibration code stays")
rel = strip((SRC / "main_fm1.c").read_text(), 0)
body = rel[rel.index("static void smk_inlog_reply(uint32_t arg)"):]
body = body[:body.index("\n}\n")]
check("arg - 2u" not in body and "if (arg == 1u)" in body,
      "main_fm1.c, flag 0: INLOG has args 0 and 1 only (args 2-5 get the entries reply)")
ui0 = strip((SRC / "ui.c").read_text(), 0)
check("if (smk_clip_mode_on())\n        return;" in ui0, "ui.c, flag 0: key_event keeps the clip mode guard")
pl0 = strip((SRC / "plat_fm1.c").read_text(), 0)
check("if (smk_ck_knobs_owned())\n        return 0;" in pl0 and "uint8_t clip_mode = smk_clip_mode_on();" in pl0,
      "plat_fm1.c, flag 0: the clip mode knob and button paths stay")
if fails:
    print(f"padcal_flag_test: {fails} FAIL")
    sys.exit(1)
print("padcal_flag_test: ok")
