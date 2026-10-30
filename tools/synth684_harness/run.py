#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""One command: python3 tools/synth684_harness/run.py [--blocks N] [--out DIR]

Builds the PR #684 host harness, renders golden raw files, times each block,
and reports per-engine object sizes. If the 10 private inputs fail the
pinned check, the harness uses generated STUB headers under build/. Stub
results are NOT VALID numbers. They only show that the harness works.
"""
import argparse, importlib.util, json, os, re, shutil, subprocess, sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
os.chdir(ROOT)
ap = argparse.ArgumentParser()
ap.add_argument("--blocks", type=int, default=172)
ap.add_argument("--idle", action="store_true", help="measure event-free silence after warmup instead of the note script")
ap.add_argument("--warmup", type=int, default=1024, help="uncounted settling blocks for --idle (default: 1024)")
ap.add_argument("--out", type=Path, default=ROOT / "build/synth684-harness")
ap.add_argument("--elite", action="store_true",
                help="also run tools/build.py --board smk37 --model elite (needs JIELI_TOOLCHAIN and AC79_SDK)")
ap.add_argument("--icount", action="store_true",
                help="also count instructions per block with callgrind (VALGRIND or ~/.local/vgenv/bin/valgrind)")
ap.add_argument("--save-golden", type=Path, help="copy the renders to this directory")
ap.add_argument("--golden", type=Path, help="byte-compare the renders with this golden directory")
a = ap.parse_args()
if a.blocks <= 0 or a.warmup < 0:
    ap.error("--blocks must be positive and --warmup must be nonnegative")
out = a.out.resolve(); out.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("x0x_build", ROOT / "tools/build.py")
b = importlib.util.module_from_spec(spec); spec.loader.exec_module(b)

real = subprocess.run([sys.executable, "tools/check_original_inputs.py", "--scope", "active"],
                      stdout=subprocess.PIPE, stderr=subprocess.STDOUT).returncode == 0
inc = []
if not real:
    stubs = out / "stubs"
    shutil.rmtree(stubs, ignore_errors=True)
    subprocess.run([sys.executable, str(HERE / "gen_stubs.py"), str(stubs)], check=True)
    # The a/b/c level lets "../../../experiments/original_synth/x.h" resolve into the stub tree.
    inc = ["-I" + str(stubs / "a/b/c"), "-I" + str(stubs / "experiments/original_synth"),
           "-I" + str(stubs / "experiments/wobble"), "-DX0X_SYNTH684_STUB_INPUTS=1"]
    (stubs / "experiments/wobble").mkdir(exist_ok=True)
    shutil.copy(stubs / "experiments/original_synth/miami_decimator_tables_private.h", stubs / "experiments/wobble/")
    print("WARNING: private inputs are missing. Using STUB headers. All numbers are NOT VALID.")
validity = "REAL" if real else "STUB-NOT-VALID"

units = [*b.ORIGINAL_SYNTH_UNITS, *b.ORIGINAL_SYNTH_BACKENDS, "firmware/src/dsp/x0x_param.c"]
units = [u for u in units if u != "firmware/src/app/original_runtime.c"] + ["firmware/src/app/original_runtime.c"]
cc = os.environ.get("CC", "cc")
flags = ["-std=c99", "-O2", "-D_POSIX_C_SOURCE=200809L", "-DX0X_HOST", "-DX0X_ORIGINAL_SYNTH=1", "-DX0X_DEV_WOBBLE=0",
         "-DX0X_WOBBLE=0", "-DBOARD_SMK37=1", "-DSMK_MODEL=1", *b.ORIGINAL_SYNTH_FLAGS,
         "-Ifirmware/src", "-Ifirmware/src/dsp", "-Ibuild/gen", "-Iexperiments/original_synth", "-Iexperiments/wobble", *inc]
objdir = out / "obj"; objdir.mkdir(exist_ok=True)
objs = []
for u in units:
    o = objdir / (u.replace("/", "_") + ".o")
    subprocess.run([cc, *flags, "-c", u, "-o", str(o)], check=True)
    objs.append(o)
exe = out / "synth684_bench"
subprocess.run([cc, *flags, str(HERE / "synth684_bench.c"), *map(str, objs), "-lm", "-o", str(exe)], check=True)
renders = out / "renders"; renders.mkdir(exist_ok=True)
bench_args = [str(a.blocks), str(int(a.idle)), str(a.warmup)]
run = subprocess.run([str(exe), str(renders), *bench_args])

icount = {}
if a.icount:
    vg = os.environ.get("VALGRIND") or str(Path.home() / ".local/vgenv/bin/valgrind")
    vinc = str(Path(vg).resolve().parents[1] / "include")
    ic = out / "icount"; shutil.rmtree(ic, ignore_errors=True); ic.mkdir()
    # -Os, as in the Elite build. Only original_runtime_render is counted.
    osflags = [f if f != "-O2" else "-Os" for f in flags]
    iobjs = []
    for u in units:
        o = ic / (u.replace("/", "_") + ".o")
        subprocess.run([cc, *osflags, "-g", "-c", u, "-o", str(o)], check=True)
        iobjs.append(o)
    iexe = ic / "synth684_icount"
    subprocess.run([cc, *osflags, "-DX0X_SYNTH684_ICOUNT", "-I" + vinc, str(HERE / "synth684_bench.c"),
                    *map(str, iobjs), "-lm", "-o", str(iexe)], check=True)
    (ic / "renders").mkdir()
    subprocess.run([vg, "--tool=callgrind", "--instr-atstart=no", "--collect-atstart=no", "--callgrind-out-file=" + str(ic / "cg.out"),
                    str(iexe), str(ic / "renders"), *bench_args], check=True, stderr=subprocess.DEVNULL)
    for f in sorted(ic.glob("cg.out*")):
        text = f.read_text(errors="replace")
        m = re.search(r"^desc: Trigger: Client Request: \S*/([\w.-]+)\.f32", text, re.M)
        # Callgrind can write summary: 0 for a stopped instrumentation part
        # while its final totals contain the collected render instructions.
        t = re.search(r"^totals: (\d+)", text, re.M) or re.search(r"^summary: (\d+)", text, re.M)
        if m and t: icount[m.group(1)] = round(int(t.group(1)) / a.blocks)
    if len(icount) != 15 or any(n <= 0 for n in icount.values()):
        raise SystemExit("Callgrind did not provide 15 positive instruction counts; inspect cg.out files")
    for n in sorted(icount): print(f"ICOUNT {n}: {icount[n]} instructions per block")
    for f in (ic / "renders").glob("*.f32"):
        if f.read_bytes() != (renders / f.name).read_bytes(): print(f"WARNING: -Os render differs from -O2 render: {f.name}")

def sizes(files, tool):
    total = {"text": 0, "data": 0, "bss": 0, "pool": 0}
    for f in files:
        r = subprocess.run([tool, "-A", str(f)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if r.returncode:
            return {"error": r.stderr.strip() or f"{tool} -A failed"}
        for line in r.stdout.splitlines():
            m = re.match(r"^(\.\S+)\s+(\d+)", line)
            if not m: continue
            s, n = m.group(1), int(m.group(2))
            if s.startswith((".text", ".rodata")): total["text"] += n
            elif s.startswith(".data"): total["data"] += n
            elif s.startswith(".bss") or s == "COMMON": total["bss"] += n
            elif s.startswith(".pool"): total["pool"] += n
    return total
def group(paths):
    g = {"chicago": [], "miami": [], "shared": []}
    for p in paths:
        n = p.name
        g["chicago" if "chicago" in n and "miami" not in n else "miami" if "miami" in n else "shared"].append(p)
    return g
report = {"validity": validity, "mode": "idle" if a.idle else "script", "blocks": a.blocks,
          "warmup_blocks": a.warmup if a.idle else 0,
          "icount_per_block": icount, "host_objects": {k: sizes(v, "size") for k, v in group(objs).items()}}
if a.elite:
    subprocess.run([sys.executable, "tools/build.py", "--board", "smk37", "--model", "elite"], check=True,
                   stdout=subprocess.DEVNULL)
spec2 = importlib.util.spec_from_file_location("elite_sizes", HERE / "elite_sizes.py")
es = importlib.util.module_from_spec(spec2); spec2.loader.exec_module(es)
report["elite_elf"] = es.main()
report["note"] = "elite_elf comes from build/smk37-elite/x0x.elf. Use --elite to rebuild it first. Host .pool is 0."
(out / "sizes.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, indent=2))
print((renders / "results.csv").read_text())
golden_rc = 0
if a.save_golden:
    a.save_golden.mkdir(parents=True, exist_ok=True)
    for f in renders.glob("*.f32"): shutil.copy(f, a.save_golden / f.name)
    print(f"golden saved to {a.save_golden}")
if a.golden:
    names = sorted(f.name for f in a.golden.glob("*.f32"))
    bad = [n for n in names if not (renders / n).exists() or (renders / n).read_bytes() != (a.golden / n).read_bytes()]
    print(f"GOLDEN COMPARE: {len(names) - len(bad)}/{len(names)} identical" + (f", DIFFERENT: {bad}" if bad else ""))
    golden_rc = 1 if bad or not names else 0
if not real: print("REMINDER: STUB build. These numbers are NOT VALID.")
sys.exit(run.returncode or golden_rc)
