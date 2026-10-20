#!/usr/bin/env python3
"""Reproduce baseline/candidate probe counts, extraction parity and negative checks."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
BASE = "07fa87dc950908d4d720bde9c06c2478cc212b19"
INC = ["apc_a", "apc_b", "ck_a", "ck_b", "autosave", "padled"]
records = []


def command(tree, args, env, label, rc=0):
    start = time.monotonic()
    p = subprocess.run(args, cwd=tree, env=env, text=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=60)
    records.append(dict(label=label, command=args, rc=p.returncode, seconds=time.monotonic()-start))
    (OUT / (label + ".log")).write_text(p.stdout)
    assert p.returncode == rc, (label, p.returncode, p.stdout[-4000:])


with tempfile.TemporaryDirectory(prefix="runner-probes-") as temp:
    temp = Path(temp)
    default_files = {}
    for version in ("baseline", "candidate"):
        tree = temp / version
        shutil.copytree(ROOT, tree, ignore=shutil.ignore_patterns(".git", "build", "evidence", "__pycache__"))
        if version == "baseline":
            for name in ("smk_clip_knob_test.sh", "smk_clip_chord_display_test.sh"):
                (tree / "tests/host" / name).write_bytes(subprocess.check_output(
                    ["git", "show", f"{BASE}:tests/host/{name}"], cwd=ROOT))
        # The compiler wrapper also wraps the compiled binary to count actual executions.
        cc = tree / "cc-count"
        cc.write_text('''#!/usr/bin/env python3
import json, os, pathlib, subprocess, sys
args = sys.argv[1:]
out = pathlib.Path(args[args.index("-o") + 1])
with open("counts", "a") as f: f.write(json.dumps(["compile", str(out)]) + "\\n")
subprocess.run(["/usr/bin/cc"] + args, check=True)
real = out.with_name(out.name + ".real"); out.rename(real)
out.write_text('#!/bin/sh\\nprintf "run %s\\\\n" "$0" >> counts\\nexec "' + str(real) + '" "$@"\\n')
out.chmod(0o755)
'''); cc.chmod(0o755)
        env = dict(os.environ, CC=str(cc))
        for name in ("knob", "chord_display"):
            command(tree, ["sh", f"tests/host/smk_clip_{name}_test.sh"], env, version + "-" + name)
        counts = (tree / "counts").read_text().splitlines()
        records.append(dict(label=version+"-counts", compile=sum(x.startswith("[") for x in counts),
                            executions=sum(x.startswith("run ") for x in counts), details=counts))
        assert len(counts) == (6 if version == "baseline" else 4), counts
        for output in ("clip_knob", "clip_chord"):
            for inc in INC:
                data = (tree / f"build/host/{output}/{inc}.inc").read_bytes()
                if version == "baseline" and output == "clip_knob": default_files[inc] = data
                assert data == default_files[inc], (version, output, inc)
        if version == "candidate":
            source = tree / "source-override"
            shutil.copytree(tree / "firmware/src", source)
            env.update(SRC=str(source), OUT="build/host/override")
            command(tree, ["sh", "tests/host/smk_clip_knob_test.sh", "--extract-only"], env, "source-override")
            for inc in INC:
                assert (tree / f"build/host/override/{inc}.inc").read_bytes() == default_files[inc]
            assert (tree / "build/host/override/src/usb.c").read_bytes() == (source / "usb.c").read_bytes()
            assert not (tree / "build/host/override/smk_clip_knob_test").exists()
            assert (tree / "counts").read_text().splitlines() == counts
            command(tree, ["sh", "tests/host/smk_clip_knob_test.sh", "--typo"], env, "unknown-argument", 2)
            command(tree, ["sh", "tests/host/smk_clip_knob_test.sh", "--extract-only", "extra"], env, "extra-argument", 2)
            markers = {
                "apc_a": ("app/main_fm1.c", ["static volatile uint8_t clip_grid_mode;"]),
                "apc_b": ("app/main_fm1.c", ["static uint8_t knob_bank_prev;"]),
                "ck_a": ("app/main_fm1.c", ["clip mode keybed"]),
                "ck_b": ("app/main_fm1.c", ["CK_FN void smk_ck_poll(void)"]),
                "autosave": ("app/ui.c", ["#define AUTOSAVE_QUIET", "/* AUTOSAVE:"]),
                "padled": ("lcd.c", ["#define PADLED_N 16u"]),
            }
            for inc, (filename, tokens) in markers.items():
                p = source / filename; original = p.read_text()
                p.write_text("".join(line for line in original.splitlines(keepends=True)
                                     if not any(token in line for token in tokens)))
                command(tree, ["sh", "tests/host/smk_clip_knob_test.sh", "--extract-only"], env, "missing-"+inc, 1)
                assert f"marker for {inc}.inc not found" in (OUT / ("missing-"+inc+".log")).read_text()
                p.write_text(original)
            # SRC must select real source bytes, not merely match the default accidentally.
            with (source / "usb.c").open("a") as f: f.write("\n/* source override sentinel */\n")
            command(tree, ["sh", "tests/host/smk_clip_knob_test.sh", "--extract-only"], env, "override-sentinel")
            assert (tree / "build/host/override/src/usb.c").read_bytes().endswith(b"/* source override sentinel */\n")
            assert (tree / "counts").read_text().splitlines() == counts
    records.append(dict(label="extraction-sha256", files={k: hashlib.sha256(v).hexdigest() for k,v in default_files.items()}))
(OUT / "probes.json").write_text(json.dumps(records, indent=2) + "\n")
print("Probe parity, counts, source override, six missing markers and invalid arguments passed.")
