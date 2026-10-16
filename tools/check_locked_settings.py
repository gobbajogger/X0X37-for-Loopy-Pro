#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Regression guard for hardware-relevant settings (Bluetooth radio, controller config, flash map, slots).

  tools/check_locked_settings.py            check the source against tools/locked_settings.json (exit 1 on a difference)
  tools/check_locked_settings.py --dump     print the values found in the source as JSON (no check)

Each setting in the JSON names a file and how to read the value from it:
  "regex"     one capture group (or several, joined with ","); the pattern must match exactly once
  "count"     the number of matches of a pattern in "files" (directories are searched for *.c and *.h),
              without the files in "exclude"
  "assign"    every "NAME = VALUE;" at file level in a C file, as a dict (a new, lost or changed name fails)
  "defines"   every "#define PREFIX... VALUE" in a C header, as a list in file order
To change a value: change the source and this JSON in one commit that names the setting, and add a
BLE_ATTEMPTS.md row. tools/build.py and ble/ctl/build_ctl.sh run this check before they build."""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JSON = ROOT / "tools" / "locked_settings.json"


def text(rel):
    return (ROOT / rel).read_text(errors="replace")


def strip_comments(s):
    s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
    return re.sub(r"//[^\n]*", "", s)


def read(st):
    kind = st.get("kind", "regex")
    if kind == "regex":
        m = list(re.finditer(st["regex"], text(st["file"]), re.M))
        if len(m) != 1:
            raise ValueError(f"pattern found {len(m)} times (need 1): {st['regex']}")
        return ",".join(g.strip() for g in m[0].groups())
    if kind == "count":
        n, skip = 0, {ROOT / e for e in st.get("exclude", [])}
        for f in st["files"]:
            p = ROOT / f
            for q in ([p] if p.is_file() else sorted(p.rglob("*.[ch]"))):
                if q not in skip:
                    n += len(re.findall(st["regex"], strip_comments(q.read_text(errors="replace"))))
        return n
    if kind == "assign":
        s = strip_comments(text(st["file"]))
        return {m.group(1): re.sub(r"\s+", " ", m.group(2).strip())
                for m in re.finditer(r"^[A-Za-z_][\w \t\*]*?\b(\w+)\s*=\s*([^;{]+);", s, re.M)}
    if kind == "defines":
        s = text(st["file"])
        return [f"{m.group(1)} {m.group(2)}"            # in file order: a board's #if branch keeps its own lines
                for m in re.finditer(r"^#define\s+(" + st["prefix"] + r"\w*)\s+(0x[0-9A-Fa-f]+u?|\d+u?)\b", s, re.M)]
    raise ValueError(f"unknown kind {kind}")


def main():
    cfg = json.loads(JSON.read_text())
    if "--dump" in sys.argv:
        print(json.dumps({st["name"]: read(st) for st in cfg["settings"]}, indent=1))
        return 0
    bad = 0
    for st in cfg["settings"]:
        try:
            got = read(st)
        except (OSError, ValueError) as e:
            print(f"LOCKED {st['name']}: cannot read ({st.get('file', st.get('files'))}): {e}")
            bad += 1
            continue
        want = st["value"]
        if got != want:
            bad += 1
            if isinstance(want, dict) and isinstance(got, dict):
                for k in sorted(set(want) | set(got)):
                    if want.get(k) != got.get(k):
                        print(f"LOCKED {st['name']}.{k}: source {got.get(k)!r}, locked {want.get(k)!r}")
            elif isinstance(want, list) and isinstance(got, list):
                for k in sorted(set(want) ^ set(got)):
                    print(f"LOCKED {st['name']}: {'only in the source' if k in got else 'only in the JSON'}: {k}")
                if set(want) == set(got):
                    print(f"LOCKED {st['name']}: the order changed")
            else:
                print(f"LOCKED {st['name']}: source {got!r}, locked {want!r} ({st.get('file', st.get('files'))})")
    if bad:
        print(f"check_locked_settings: {bad} setting(s) differ from tools/locked_settings.json. Change the JSON in its "
              "own commit that names the setting, and add a BLE_ATTEMPTS.md row.")
        return 1
    print(f"check_locked_settings: {len(cfg['settings'])} settings ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
