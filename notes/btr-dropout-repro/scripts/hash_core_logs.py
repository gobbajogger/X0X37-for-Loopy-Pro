#!/usr/bin/env python3
"""Hash and completeness check for one Elite core-pass directory. No analysis."""
import hashlib, json, csv, sys
from pathlib import Path

EXPECTED = [
    "META.json", "emu.log", "half.csv", "ram.bin", "wall.txt",
    "lcd.png", "lcd-2x.png", "work/lcd.ppm",
]
OPTIONAL = ["core1.csv"]
HEADER = "halves,last_us,max_us,late,nested,ui_frames,in_audio,audio_xruns,audio_peak_pct,audio_cpu_pct,eng_prof_frames,d_eng_prof_frames,d_PR_909,d_PR_808,d_PR_303A,d_PR_303B,d_PR_BRK,d_PR_FX,d_PR_MST,d_PR_SEQ,JB_909,JB_808,JB_A,JB_B,JB_BRK,eng_plan,eng_guard_count,perf_cyc_ok,c1_busy_ticks,c1_ticks,c1_job_busy_ticks,c1_take_busy_ticks,c1_job_starts,emu_cycles"

def sha256(p: Path):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()

def main():
    d = Path(sys.argv[1])
    out = {"dir": str(d), "present": {}, "missing": [], "sha256": {}, "sizes": {}}
    for rel in EXPECTED + OPTIONAL:
        p = d / rel
        if p.exists():
            out["present"][rel] = True
            out["sha256"][rel] = sha256(p)
            out["sizes"][rel] = p.stat().st_size
        else:
            if rel in OPTIONAL:
                out["present"][rel] = False
            else:
                out["missing"].append(rel)
    half = d / "half.csv"
    if half.exists():
        with half.open() as f:
            hdr = f.readline().strip()
            rows = list(csv.DictReader(open(half)))
        out["half_header_ok"] = hdr == HEADER
        out["half_header"] = hdr
        out["half_rows"] = len(rows)
        if rows:
            first, last = rows[0], rows[-1]
            out["half_first"] = {k: first[k] for k in ("halves","last_us","max_us","late","audio_xruns","eng_plan","emu_cycles")}
            out["half_last"] = {k: last[k] for k in ("halves","last_us","max_us","late","audio_xruns","eng_plan","emu_cycles")}
            out["t_ms_first"] = int(first["emu_cycles"]) / 5_760_000
            out["t_ms_last"] = int(last["emu_cycles"]) / 5_760_000
            # row completeness: all fields present, no blank
            bad = 0
            nfields = len(HEADER.split(","))
            for i, r in enumerate(rows):
                if len(r) != nfields:
                    bad += 1
            out["half_bad_rows"] = bad
            # monotonic halves
            hs = [int(r["halves"]) for r in rows]
            out["halves_monotonic"] = all(hs[i] + 1 == hs[i+1] for i in range(len(hs)-1))
            out["halves_first"] = hs[0]
            out["halves_last"] = hs[-1]
        meta = d / "META.json"
        if meta.exists():
            st = json.loads(meta.read_text())["states"]
            out["shot_ms"] = st["shot_ms"]
            out["t_ms_reaches_shot"] = out.get("t_ms_last", 0) >= st["shot_ms"] - 50  # 50ms tolerance
            out["states"] = st
    wall = d / "wall.txt"
    if wall.exists():
        out["wall"] = wall.read_text().strip()
    # raw extrema table (numbers only, no interpretation)
    if half.exists() and rows:
        def extrema(rows, t0, t1, label):
            sel = [r for r in rows if t0 <= int(r["emu_cycles"]) / 5_760_000 < t1]
            if not sel:
                return {"label": label, "n": 0}
            def col(k):
                return [int(r[k]) for r in sel]
            def mm(k):
                xs = col(k)
                return {"min": min(xs), "max": max(xs), "first": xs[0], "last": xs[-1]}
            return {
                "label": label, "n": len(sel),
                "t_ms_first": int(sel[0]["emu_cycles"]) / 5_760_000,
                "t_ms_last": int(sel[-1]["emu_cycles"]) / 5_760_000,
                "halves_first": int(sel[0]["halves"]),
                "halves_last": int(sel[-1]["halves"]),
                "last_us": mm("last_us"),
                "max_us": mm("max_us"),
                "late": mm("late"),
                "audio_xruns": mm("audio_xruns"),
                "audio_peak_pct": mm("audio_peak_pct"),
                "audio_cpu_pct": mm("audio_cpu_pct"),
                "d_PR_909": mm("d_PR_909"),
                "d_PR_808": mm("d_PR_808"),
                "d_PR_303A": mm("d_PR_303A"),
                "d_PR_303B": mm("d_PR_303B"),
                "d_PR_BRK": mm("d_PR_BRK"),
                "d_PR_FX": mm("d_PR_FX"),
                "d_PR_MST": mm("d_PR_MST"),
                "d_PR_SEQ": mm("d_PR_SEQ"),
                "JB_909": mm("JB_909"),
                "JB_808": mm("JB_808"),
                "JB_A": mm("JB_A"),
                "JB_B": mm("JB_B"),
                "JB_BRK": mm("JB_BRK"),
                "eng_plan": mm("eng_plan"),
                "c1_busy_ticks": mm("c1_busy_ticks"),
                "c1_job_busy_ticks": mm("c1_job_busy_ticks"),
                "c1_job_starts": mm("c1_job_starts"),
                "c1_ticks": mm("c1_ticks"),
                "eng_guard_count": mm("eng_guard_count"),
                "perf_cyc_ok": mm("perf_cyc_ok"),
                "ui_frames": mm("ui_frames"),
                "nested": mm("nested"),
            }
        st = out.get("states") or {}
        out["tables"] = {
            "setup": extrema(rows, 0, st.get("play_start_ms", 0), "setup"),
            "play": extrema(rows, st.get("play_start_ms", 0), st.get("play_end_ms", 0), "play"),
            "idle": extrema(rows, st.get("idle_start_ms", 0), st.get("idle_end_ms", 0), "idle"),
            "empty": extrema(rows, st.get("empty_play_ms", 0), st.get("shot_ms", 10**12), "empty"),
            "all": extrema(rows, 0, 10**12, "all"),
        }
    report = d / "HASHES.json"
    # don't include tables in HASHES of files that would circular; write two files
    hashes_only = {k: out[k] for k in ("dir","present","missing","sha256","sizes","half_header_ok","half_rows","half_bad_rows","halves_monotonic","halves_first","halves_last","t_ms_first","t_ms_last","shot_ms","t_ms_reaches_shot","wall","half_first","half_last") if k in out}
    report.write_text(json.dumps(hashes_only, indent=2) + "\n")
    tablep = d / "RAW_TABLE.json"
    if "tables" in out:
        tablep.write_text(json.dumps(out["tables"], indent=2) + "\n")
        out["sha256"]["RAW_TABLE.json"] = sha256(tablep)  # after write; HASHES written before this - update
        # rewrite HASHES including RAW_TABLE hash after table write
        hashes_only["sha256"]["RAW_TABLE.json"] = sha256(tablep)
        hashes_only["sizes"]["RAW_TABLE.json"] = tablep.stat().st_size
        report.write_text(json.dumps(hashes_only, indent=2) + "\n")
    print(json.dumps(hashes_only, indent=2))
    print(f"wrote {report} and {d/'RAW_TABLE.json'}")
    return 0 if not out["missing"] and out.get("t_ms_reaches_shot") and out.get("half_header_ok") and out.get("halves_monotonic") and out.get("half_bad_rows")==0 else 1

if __name__ == "__main__":
    raise SystemExit(main())
