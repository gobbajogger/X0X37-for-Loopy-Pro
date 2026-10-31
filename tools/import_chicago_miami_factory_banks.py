#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Regenerate assets/factory_banks from owner-Mac KORG Gadget packs (private/dev)."""
from __future__ import annotations
import argparse, json, pathlib, re, struct, sys

CHI_LIB = pathlib.Path("/Library/Application Support/KORG/Gadget/Plug-Ins/ChicagoLibrary.bundle/Contents/Resources")
MIA_LIB = pathlib.Path("/Library/Application Support/KORG/Gadget/Plug-Ins/MiamiLibrary.bundle/Contents/Resources")

def pchk(path: pathlib.Path) -> bytes:
    data = path.read_bytes(); pos = 12
    while pos + 8 <= len(data):
        cid = data[pos:pos+4]; csz = struct.unpack_from("<I", data, pos+4)[0]
        if cid == b"PCHK": return data[pos+8:pos+8+csz]
        pos += 8 + csz + (csz & 1)
    raise RuntimeError(f"no PCHK in {path}")

def all_padded(payload: bytes, nparams: int, search_tail: int = 5000):
    stride = 12; out = []
    for start in range(max(0, len(payload)-search_tail), len(payload)-(nparams-1)*stride-4):
        ok = True; vals = []
        for i in range(nparams):
            off = start + i*stride
            v = struct.unpack_from("<f", payload, off)[0]
            if not (0.0 <= v <= 1.0): ok = False; break
            if i < nparams-1 and payload[off+4:off+12] != b"\x00"*8: ok = False; break
            vals.append(float(v))
        if ok and len({round(v,5) for v in vals}) >= max(3, nparams//6):
            out.append((start, vals, len(payload)-start))
    return out

def extract_factory2(path: pathlib.Path, nparams: int = 22):
    b = path.read_bytes(); out = []; i = 0
    while True:
        o = 32 + i*488
        if o + 220 + (nparams-1)*12 + 4 > len(b): break
        name = b[o:o+40].split(b"\0",1)[0].decode("ascii","ignore").strip()
        if not name or not re.match(r"^[A-Za-z0-9]", name): break
        vals = [float(struct.unpack_from("<f", b, o+220+j*12)[0]) for j in range(nparams)]
        out.append({"name": name, "params": vals}); i += 1
    return out

def pick_min_fe(nks: pathlib.Path, nparams: int, lo: int, hi: int):
    cands = all_padded(pchk(nks), nparams)
    win = [(fe, vals) for _, vals, fe in cands if lo <= fe <= hi] or [(fe, vals) for _, vals, fe in cands if fe <= hi+100]
    if not win: return None
    fe, vals = min(win, key=lambda x: x[0])
    return {"name": nks.stem, "params": vals, "source": "nks-minfe", "from_end": fe}

def main() -> int:
    ap = argparse.ArgumentParser(); ap.add_argument("--out", type=pathlib.Path, default=pathlib.Path("assets/factory_banks"))
    args = ap.parse_args()
    if not CHI_LIB.is_dir() or not MIA_LIB.is_dir():
        print("Gadget library packs not found on this Mac", file=sys.stderr); return 2
    chi_macro = json.loads((CHI_LIB/"Chicago/macro_desc.json").read_text())
    mia_macro = json.loads((MIA_LIB/"Miami/macro_desc.json").read_text())
    f2map = {p["name"].rstrip(): p for p in extract_factory2(CHI_LIB/"Factory2.prog")}
    chicago = []
    for nks in sorted((CHI_LIB/"NKS/Presets").glob("*.nksf")):
        if nks.stem in f2map:
            chicago.append({"name": nks.stem, "params": f2map[nks.stem]["params"], "source": "factory2"})
        else:
            got = pick_min_fe(nks, 22, 297, 355)
            if not got: print(f"chicago miss {nks.stem}", file=sys.stderr); return 3
            chicago.append(got)
    miami = []
    for nks in sorted((MIA_LIB/"NKS/Presets").glob("*.nksf")):
        got = pick_min_fe(nks, 18, 250, 400)
        if not got: print(f"miami miss {nks.stem}", file=sys.stderr); return 3
        miami.append(got)
    note = "Owner-paid personal-use packs. Private/dev factory path. Owner strips before public redistribution. No strip automation."
    args.out.mkdir(parents=True, exist_ok=True)
    def bank(brand, label, framing, au, macro, presets):
        return {"brand": brand, "label": label, "framing": framing, "au_component": au,
                "parameter_names": [p[1] for p in macro["parameters"]],
                "parameter_defaults": [float(p[2]) for p in macro["parameters"]],
                "count": len(presets),
                "presets": [{"name": p["name"], "params": [round(v,8) for v in p["params"]], "source": p["source"]} for p in presets],
                "license_note": note}
    chi_doc = bank("chicago", "Chicago (Acid)", "faithful-303 factory bank", "KORG: Chicago (Acid) 3.1.6", chi_macro, chicago)
    mia_doc = bank("miami", "Miami (Wobble)", "faithful-303 factory bank (separate from WBL1)", "KORG: Miami (Wobble) 3.1.6", mia_macro, miami)
    manifest = {"schema_version": 1,
        "framing": "faithful-303 factory banks (Chicago Acid / Miami Wobble); not an upgrade brand; WBL1 != Miami factory bank",
        "license_note": note, "user_import_allowed": True, "factory_by_default": True,
        "chicago_count": len(chicago), "miami_count": len(miami),
        "sources": {"chicago_factory2": str(CHI_LIB/"Factory2.prog"), "chicago_nks": str(CHI_LIB/"NKS/Presets"),
                    "miami_nks": str(MIA_LIB/"NKS/Presets"),
                    "chicago_au": "/Library/Audio/Plug-Ins/Components/Chicago.component 3.1.6",
                    "miami_au": "/Library/Audio/Plug-Ins/Components/Miami.component 3.1.6"}}
    (args.out/"chicago.json").write_text(json.dumps(chi_doc, indent=2)+"\n")
    (args.out/"miami.json").write_text(json.dumps(mia_doc, indent=2)+"\n")
    (args.out/"manifest.json").write_text(json.dumps(manifest, indent=2)+"\n")
    print(f"chicago={len(chicago)} miami={len(miami)} -> {args.out}"); return 0

if __name__ == "__main__":
    raise SystemExit(main())
