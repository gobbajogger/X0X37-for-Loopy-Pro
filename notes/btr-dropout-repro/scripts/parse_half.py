#!/usr/bin/env python3
"""Summarise a raw per-half CSV. Never averages the stored series; reports
max / P99 / late / xruns / margins, split by play / idle / empty."""
import csv, json, sys, statistics
from pathlib import Path

BUDGET = 11609       # 512 * 1e6 / 44100
I2S_HALF = 11598     # ~44145 Hz

def pctl(xs, p):
    if not xs:
        return None
    s = sorted(xs)
    if len(s) == 1:
        return s[0]
    k = (len(s) - 1) * p / 100.0
    i = int(k)
    f = k - i
    if i + 1 >= len(s):
        return s[-1]
    return s[i] * (1 - f) + s[i + 1] * f

def load(path):
    rows = list(csv.DictReader(open(path)))
    for r in rows:
        for k, v in r.items():
            if k:
                r[k] = int(v)
        r["t_ms"] = r["emu_cycles"] / 5_760_000.0
    return rows

def stats(rows, label):
    if not rows:
        return {"label": label, "n": 0}
    us = [r["last_us"] for r in rows]
    mx = [r["max_us"] for r in rows]
    late = [r["late"] for r in rows]
    xruns = [r["audio_xruns"] for r in rows]
    first_late = next((r for r in rows if r["late"] > (rows[0]["late"] if rows else 0) or (rows and r["late"] > 0 and r is not rows[0] and r["late"] > rows[0]["late"])), None)
    # first row where late increased
    first_late_r = None
    prev = rows[0]["late"]
    if prev:
        first_late_r = rows[0]
    else:
        for r in rows[1:]:
            if r["late"] > prev:
                first_late_r = r
                break
            prev = r["late"]
    first_xrun_r = None
    prevx = rows[0]["audio_xruns"]
    if prevx:
        first_xrun_r = rows[0]
    else:
        for r in rows[1:]:
            if r["audio_xruns"] > prevx:
                first_xrun_r = r
                break
            prevx = r["audio_xruns"]
    d808 = [r["d_PR_808"] for r in rows]
    job808 = [r["JB_808"] for r in rows]
    plan = [r["eng_plan"] for r in rows]
    c1 = [r["c1_busy_ticks"] for r in rows]
    out = {
        "label": label,
        "n": len(rows),
        "t_ms_first": round(rows[0]["t_ms"], 1),
        "t_ms_last": round(rows[-1]["t_ms"], 1),
        "halves_first": rows[0]["halves"],
        "halves_last": rows[-1]["halves"],
        "last_us_min": min(us),
        "last_us_max": max(us),
        "last_us_p50": round(pctl(us, 50), 1),
        "last_us_p99": round(pctl(us, 99), 1),
        "last_us_p999": round(pctl(us, 99.9), 1),
        "max_us_end": mx[-1],
        "late_start": late[0],
        "late_end": late[-1],
        "late_delta": late[-1] - late[0],
        "xruns_start": xruns[0],
        "xruns_end": xruns[-1],
        "xruns_delta": xruns[-1] - xruns[0],
        "margin_budget_min": BUDGET - max(us),
        "margin_i2s_min": I2S_HALF - max(us),
        "pass_late_xrun": (late[-1] - late[0] == 0) and (xruns[-1] - xruns[0] == 0),
        "over_budget": sum(1 for u in us if u > BUDGET),
        "over_i2s": sum(1 for u in us if u > I2S_HALF),
        "d_PR_808_max": max(d808),
        "d_PR_808_min": min(d808),
        "JB_808_max": max(job808),
        "JB_808_min": min(job808),
        "eng_plan_values": sorted(set(plan)),
        "c1_busy_max": max(c1),
        "c1_busy_min": min(c1),
        "d_PR_303A_max": max(r["d_PR_303A"] for r in rows),
        "d_PR_303B_max": max(r["d_PR_303B"] for r in rows),
        "d_PR_909_max": max(r["d_PR_909"] for r in rows),
        "JB_A_max": max(r["JB_A"] for r in rows),
        "JB_B_max": max(r["JB_B"] for r in rows),
        "JB_909_max": max(r["JB_909"] for r in rows),
        "audio_peak_pct_end": rows[-1]["audio_peak_pct"],
        "audio_cpu_pct_end": rows[-1]["audio_cpu_pct"],
    }
    if first_late_r:
        out["first_late"] = {"t_ms": round(first_late_r["t_ms"], 2), "halves": first_late_r["halves"], "last_us": first_late_r["last_us"], "late": first_late_r["late"]}
    else:
        out["first_late"] = None
    if first_xrun_r:
        out["first_xrun"] = {"t_ms": round(first_xrun_r["t_ms"], 2), "halves": first_xrun_r["halves"], "last_us": first_xrun_r["last_us"], "xruns": first_xrun_r["audio_xruns"]}
    else:
        out["first_xrun"] = None
    return out

def split_states(rows, states):
    def between(a, b):
        return [r for r in rows if a <= r["t_ms"] < b]
    play = between(states["play_start_ms"], states["play_end_ms"])
    idle = between(states["idle_start_ms"], states["idle_end_ms"])
    empty = between(states["empty_play_ms"], states["shot_ms"])
    setup = [r for r in rows if r["t_ms"] < states["play_start_ms"]]
    return {
        "all": stats(rows, "all"),
        "setup": stats(setup, "setup"),
        "play": stats(play, "play"),
        "idle": stats(idle, "idle"),
        "empty": stats(empty, "empty"),
    }

def main():
    d = Path(sys.argv[1])
    rows = load(d / "half.csv")
    states = json.loads((d / "META.json").read_text())["states"] if (d / "META.json").exists() else json.loads(Path(sys.argv[2]).read_text())
    s = split_states(rows, states)
    outp = d / "half_summary.json"
    outp.write_text(json.dumps(s, indent=2) + "\n")
    def line(st):
        if st["n"] == 0:
            return f"  {st['label']:8}  n=0"
        p = "PASS" if st["pass_late_xrun"] else "MISS"
        return (f"  {st['label']:8}  n={st['n']:6}  last_us min/p50/p99/max="
                f"{st['last_us_min']}/{st['last_us_p50']}/{st['last_us_p99']}/{st['last_us_max']}"
                f"  max_us={st['max_us_end']}  lateΔ={st['late_delta']} xrunΔ={st['xruns_delta']}"
                f"  margin11609={st['margin_budget_min']}  margin11598={st['margin_i2s_min']}  {p}")
    print(f"{d}: {len(rows)} raw halves")
    for k in ("setup", "play", "idle", "empty", "all"):
        print(line(s[k]))
    print(f"wrote {outp}")

if __name__ == "__main__":
    main()
