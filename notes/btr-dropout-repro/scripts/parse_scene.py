#!/usr/bin/env python3
"""Parse an fm1-live FM1_DUMPRAM dump for the dropout-repro scene.

Does not change the live gate (zdprobe/dnbox360/parse_perf.py).

Deadlines (FMB correction, main_fm1.c 94197fc8):
  budget_us = 11609          integer 512 * 1e6 // 44100
  i2s_half_us = 11598        512 frames at about 44145 Hz
  5.805 ms is NOT a deadline and is not reported as one.
Miss counters: x0x_dbg.late and audio_xruns. Pass means both 0.

Raw per-half series: this exact image has no lcd_trace and no x0x_worst.
last_us is the last half only; max_us is the max since boot.
If a series file (one us per line, or csv half,us) is passed as
--series, P99 and the spike list are computed from that.
"""
from __future__ import annotations
import argparse, json, os, re, struct, subprocess, sys

RAM_BASE = 0x01C00000
BUDGET_US = 11609          # main_fm1.c: HALF_FRAMES * 1000000u / 44100
I2S_HALF_US = 11598        # 512 / 44145 s, rounded to nearest us
ENG_PROF_N = 8
PROF_NAMES = ("PR_909", "PR_808", "PR_303A", "PR_303B", "PR_BRK", "PR_FX", "PR_MST", "PR_SEQ")
UI_NAMES = ("909", "808", "BASS A", "BASS B", "BRK", "FX", "MASTER", "SEQ")
DBG_FIELDS = ("magic", "halves", "max_us", "nested", "in_audio", "late",
              "timer_irqs", "ui_frames", "last_us", "cpu_q8", "boots", "stage", "crash_seen")


def load_syms(elf: str) -> dict:
    syms = {}
    out = subprocess.run(["readelf", "-sW", elf], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) >= 8 and p[2].isdigit():
            name = p[-1]
            # first occurrence; LOCAL objects we care about are unique
            if name not in syms:
                try:
                    syms[name] = (int(p[1], 16), int(p[2], 0))
                except ValueError:
                    pass
    return syms


def u32(ram: bytes, addr: int, off=0) -> int:
    return struct.unpack_from("<I", ram, addr - RAM_BASE + off)[0]


def parse_series(path: str) -> dict:
    xs = []
    with open(path) as f:
        for ln in f:
            ln = ln.strip()
            if not ln or ln.startswith("#"):
                continue
            if "," in ln:
                parts = ln.split(",")
                xs.append(int(float(parts[-1])))
            else:
                xs.append(int(float(ln)))
    if not xs:
        return {"n": 0}
    s = sorted(xs)
    def pct(p):
        i = min(len(s) - 1, max(0, int(round((p / 100.0) * (len(s) - 1)))))
        return s[i]
    return {
        "n": len(xs),
        "min_us": s[0],
        "max_us": s[-1],
        "p50_us": pct(50),
        "p99_us": pct(99),
        "mean_us": sum(xs) / len(xs),
        "n_over_budget_11609": sum(1 for v in xs if v > BUDGET_US),
        "n_over_i2s_11598": sum(1 for v in xs if v > I2S_HALF_US),
        "first_over_budget_index": next((i for i, v in enumerate(xs) if v > BUDGET_US), None),
        "first_over_i2s_index": next((i for i, v in enumerate(xs) if v > I2S_HALF_US), None),
    }


def margin(us: int, line: int) -> int:
    return line - us  # positive = under the line


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("elf")
    ap.add_argument("outdir")
    ap.add_argument("--fwsc", default="")
    ap.add_argument("--elf-sha", default="")
    ap.add_argument("--fwsc-sha", default="")
    ap.add_argument("--config", default="")
    ap.add_argument("--state", default="")  # playing | idle | empty
    ap.add_argument("--series", default="")
    ap.add_argument("--json-out", default="")
    args = ap.parse_args()

    ram_path = os.path.join(args.outdir, "ram.bin")
    ram = open(ram_path, "rb").read()
    ram0_path = os.path.join(args.outdir, "ram_start.bin")
    ram0 = open(ram0_path, "rb").read() if os.path.exists(ram0_path) else None
    syms = load_syms(args.elf)

    def need(name):
        if name not in syms:
            raise SystemExit(f"missing symbol {name} in {args.elf}")
        return syms[name][0]

    dbg = need("x0x_dbg")
    v = {k: u32(ram, dbg, 4 * i) for i, k in enumerate(DBG_FIELDS)}
    v0 = {k: u32(ram0, dbg, 4 * i) for i, k in enumerate(DBG_FIELDS)} if ram0 is not None else None

    def opt_u32(name):
        if name not in syms:
            return None
        return u32(ram, syms[name][0])

    xruns = opt_u32("audio_xruns")
    cpu = opt_u32("audio_cpu_pct")
    peak = opt_u32("audio_peak_pct")
    frames = opt_u32("eng_prof_frames")
    cyc_ok = opt_u32("perf_cyc_ok")
    guard = opt_u32("eng_guard_count")
    prof = None
    if "eng_prof" in syms:
        a = syms["eng_prof"][0]
        prof = [u32(ram, a, 4 * i) for i in range(ENG_PROF_N)]
        prof0 = [u32(ram0, a, 4 * i) for i in range(ENG_PROF_N)] if ram0 is not None else None
    else:
        prof0 = None

    late_test = v["late"] - (v0["late"] if v0 else 0)
    xruns_test = None
    if xruns is not None and ram0 is not None and "audio_xruns" in syms:
        xruns_test = xruns - u32(ram0, syms["audio_xruns"][0])

    series = parse_series(args.series) if args.series and os.path.exists(args.series) else None

    rec = {
        "source_commit": "94197fc839b00aff47b98ca02f3c6396e4a6eabf",
        "elf": os.path.abspath(args.elf),
        "elf_sha256": args.elf_sha,
        "fwsc": os.path.abspath(args.fwsc) if args.fwsc else "",
        "fwsc_sha256": args.fwsc_sha,
        "config": args.config,
        "state": args.state,
        "deadlines": {
            "budget_us": BUDGET_US,
            "i2s_half_us": I2S_HALF_US,
            "note": "5.805 ms per 256-frame block is NOT a deadline",
        },
        "x0x_dbg": v,
        "x0x_dbg_magic_ok": v["magic"] == 0x44424731,
        "at_start": v0,
        "during": {
            "halves": (v["halves"] - v0["halves"]) if v0 else None,
            "late": late_test if v0 else v["late"],
            "xruns": xruns_test,
        } if v0 is not None else None,
        "audio_xruns": xruns,
        "audio_cpu_pct_iir": cpu,          # averaged; do not use as the spike figure
        "audio_peak_pct": peak,            # PERF screen resets this when drawn
        "perf_cyc_ok": cyc_ok,
        "eng_guard_count": guard,
        "eng_prof_frames": frames,
        "eng_prof": dict(zip(PROF_NAMES, prof)) if prof is not None else None,
        "eng_prof_delta": dict(zip(PROF_NAMES, [a - b for a, b in zip(prof, prof0)])) if prof0 is not None else None,
        "last_us": v["last_us"],
        "max_us_since_boot": v["max_us"],
        "margin_last_vs_budget_us": margin(v["last_us"], BUDGET_US),
        "margin_last_vs_i2s_us": margin(v["last_us"], I2S_HALF_US),
        "margin_max_vs_budget_us": margin(v["max_us"], BUDGET_US),
        "margin_max_vs_i2s_us": margin(v["max_us"], I2S_HALF_US),
        "late": v["late"],
        "pass_zero_late_and_zero_xrun": (v["late"] == 0 and (xruns == 0 if xruns is not None else False)),
        "raw_per_half_series": series if series else "UNMEASURED (no lcd_trace/x0x_worst on 94197fc8; last_us is one sample)",
        "p99_us": series["p99_us"] if series and "p99_us" in series else "UNMEASURED",
        "first_late_timestamp": "none" if v["late"] == 0 else "UNMEASURED (not stored in this image)",
        "ui_part_labels": UI_NAMES,
        "ui_stage_from_ram": "see LCD; perf struct is compiler-split",
    }

    # emu.log clock line, if present
    logp = os.path.join(args.outdir, "emu.log")
    if os.path.exists(logp):
        log = open(logp).read()
        m = re.search(r"clock (\d+) MHz", log)
        rec["emu_log_clock"] = m.group(1) + " MHz" if m else None
        rec["emu_log_tail"] = log.strip().splitlines()[-8:]

    text = []
    def L(s=""):
        text.append(s)
    L(f"source: 94197fc839b00aff47b98ca02f3c6396e4a6eabf")
    L(f"elf:    {rec['elf']}")
    L(f"elf_sha256: {args.elf_sha}")
    L(f"fwsc:   {rec['fwsc']}")
    L(f"fwsc_sha256: {args.fwsc_sha}")
    L(f"config: {args.config}")
    L(f"state:  {args.state}")
    L(f"x0x_dbg magic ok: {rec['x0x_dbg_magic_ok']}")
    L(f"halves: {v['halves']}  last_us: {v['last_us']}  max_us_since_boot: {v['max_us']}")
    L(f"late: {v['late']}  audio_xruns: {xruns}")
    L(f"deadlines: budget {BUDGET_US} us, I2S half ~{I2S_HALF_US} us  (5.805 ms is NOT a deadline)")
    L(f"margin last_us: {rec['margin_last_vs_budget_us']} us vs budget, {rec['margin_last_vs_i2s_us']} us vs I2S")
    L(f"margin max_us:  {rec['margin_max_vs_budget_us']} us vs budget, {rec['margin_max_vs_i2s_us']} us vs I2S")
    L(f"audio_cpu_pct (IIR avg, not the spike figure): {cpu}")
    L(f"audio_peak_pct (PERF screen resets this): {peak}")
    L(f"cpu_q8 load: {v['cpu_q8']*100/256:.1f} %")
    L(f"perf_cyc_ok: {cyc_ok}  ui_frames: {v['ui_frames']}  nested: {v['nested']}  guard: {guard}")
    if v0:
        L(f"-- at start: halves {v0['halves']}, max_us {v0['max_us']}, late {v0['late']}")
        L(f"-- during this shot: halves {v['halves']-v0['halves']}, late {late_test}, xruns {xruns_test}")
    if prof is not None:
        L("eng_prof (cumulative plat_cycles, core 0 only; core 1 is not PROF_ADD'd):")
        src = rec["eng_prof_delta"] or rec["eng_prof"]
        for n, val in src.items():
            L(f"  {n}: {val}")
        L(f"eng_prof_frames: {frames}")
    L(f"raw per-half series: {rec['raw_per_half_series'] if isinstance(rec['raw_per_half_series'], str) else json.dumps(series)}")
    L(f"P99: {rec['p99_us']}")
    L(f"first_late_timestamp: {rec['first_late_timestamp']}")
    L(f"0 late and 0 xrun: {rec['pass_zero_late_and_zero_xrun']}")
    L("no claim that anything is universally dropout-free")
    body = "\n".join(text) + "\n"
    outp = os.path.join(args.outdir, "scene_result.txt")
    open(outp, "w").write(body)
    jp = args.json_out or os.path.join(args.outdir, "scene_result.json")
    open(jp, "w").write(json.dumps(rec, indent=2) + "\n")
    sys.stdout.write(body)


if __name__ == "__main__":
    main()
