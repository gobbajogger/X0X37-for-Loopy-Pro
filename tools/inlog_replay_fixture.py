#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make a clip colour replay fixture from an INLOG text log (tools/inlog_fmt.py output).

Usage: tools/inlog_replay_fixture.py LOG BUILD > tests/fixtures/inlog_<build>_clips.txt
LOG: a text log with "=== HH:MM:SS" blocks (stored clip cells and PADS lines).
BUILD: the firmware commit that made the log. The tool reads the pad table of that build with git
(APC40_PAD_OWNER in app/apc40_pad_owner.h, or APC40_RGB in older builds), because the pad bytes in the
log come from that table.
The fixture keeps each different state (40 stored cells and the 16 sent pad colours) once.
To use a new log, run the tool again and give the new file to tests/apc_palette_test.py.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def old_table(build):
    sys.path.insert(0, str(ROOT / "tools"))
    import inlog_fmt
    for path in ("firmware/src/app/apc40_pad_owner.h", "firmware/src/app/apc40_pad_rgb.h",
                 "firmware/src/app/main_fm1.c"):
        try:
            t = subprocess.run(["git", "-C", str(ROOT), "show", f"{build}:{path}"], check=True,
                               capture_output=True, text=True).stdout
        except subprocess.CalledProcessError:
            continue
        pal = inlog_fmt.table_rgb(t)
        if len(pal) == 128:
            return ["%06X" % x for x in pal]
    sys.exit(f"inlog_replay_fixture: no pad table in {build}")


def states(text):
    out, seen = [], set()
    for blk in re.split(r"^=== ", text, flags=re.M)[1:]:
        t = blk.splitlines()[0].strip()
        rows = {int(m.group(1)): m.group(2).split() for m in re.finditer(r"^  row (\d): (.*)$", blk, re.M)}
        pads = {int(m.group(1)): (m.group(2), m.group(3), m.group(4))
                for m in re.finditer(r"^  pad +(\d+): .*?sent +(\d+), +(\d+), +(\d+)", blk, re.M)}
        if len(rows) != 5 or len(pads) != 16:
            continue
        cells = []
        for r in range(5):
            for c in rows[r]:
                cells.append(c if c != "." else "0/0")
        if len(cells) != 40 or all(c == "0/0" for c in cells):
            continue
        key = (tuple(cells), tuple(pads[i] for i in range(16)))
        if key in seen:
            continue
        seen.add(key)
        out.append((t, cells, [pads[i] for i in range(16)]))
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    log, build = Path(sys.argv[1]), sys.argv[2]
    tab = old_table(build)
    print(f"# Clip colour replay fixture. Made by tools/inlog_replay_fixture.py from {log.name}, build {build}.")
    print("# old_table: the pad table of the build (the pad bytes below come from it).")
    print("# state: time, then the 40 stored cells velocity/channel (APC note 0-39), then the 16 sent pads R,G,B.")
    print("build " + build)
    print("old_table " + " ".join(tab))
    for t, cells, pads in states(log.read_text()):
        print(f"state {t} " + " ".join(cells) + " | " + " ".join(",".join(p) for p in pads))


if __name__ == "__main__":
    main()
