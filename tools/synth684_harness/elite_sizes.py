#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Per-engine flash, .data, .bss and .pool from the linked Elite ELF.

Each symbol in build/smk37-elite/x0x.elf is assigned to the engine whose
object file (build/smk37-elite/original/*.o) defines that name. Objects with
"chicago" in the name are Chicago, "miami" or "wobble" are Miami, the rest
are shared. The engine.c pool buffers (original_bass, original_workspace,
original_tuning, original_fx) are listed separately. Static symbols with the
same name in two objects go to the first owner, so the split is approximate.
The totals per section are exact.
"""
import json, os, re, subprocess, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
B = ROOT / "build/smk37-elite"
TC = Path(os.environ.get("JIELI_TOOLCHAIN", "/home/box/.jieli/jieli-linux-toolchains-20260730.1"))
OBJDUMP = TC / "pi32v2/bin/objdump"
LINE = re.compile(r"^([0-9a-f]{8})\s.*?\s(\.\S+)\s+([0-9a-f]{8})\s(\S+)$")

def symbols(path):
    out = subprocess.run([str(OBJDUMP), "-t", str(path)], stdout=subprocess.PIPE, text=True, check=True).stdout
    for line in out.splitlines():
        m = LINE.match(line.replace("\t", " "))
        if m: yield m.group(2), int(m.group(3), 16), m.group(4)

def kind(section):
    if section.startswith((".text", ".rodata", ".ram_text", ".c1_text")): return "flash"
    if section.startswith(".data"): return "data"
    if section.startswith(".bss"): return "bss"
    if section.startswith(".pool"): return "pool"
    return None

def engine_of(name):
    return "chicago" if "chicago" in name else "miami" if ("miami" in name or "wobble" in name) else "shared"

def main():
    elf = B / "x0x.elf"
    if not elf.exists() or not OBJDUMP.exists(): return {"error": "Elite ELF or toolchain objdump not found"}
    owner = {}
    for o in sorted((B / "original").glob("*.o")):
        for sec, size, name in symbols(o):
            if kind(sec) and size: owner.setdefault(name, engine_of(o.name))
    res = {e: {"flash": 0, "data": 0, "bss": 0, "pool": 0} for e in ("chicago", "miami", "shared")}
    buffers = {}
    for sec, size, name in symbols(elf):
        k = kind(sec)
        if not k: continue
        if name in ("original_bass", "original_workspace", "original_tuning", "original_fx", "original_fx_owner"):
            buffers[name] = {"section": sec, "bytes": size}
        elif name in owner:
            res[owner[name]][k] += size
    res["engine_c_buffers"] = buffers
    return res

if __name__ == "__main__":
    print(json.dumps(main(), indent=2))
