#!/usr/bin/env python3
import json, hashlib
from pathlib import Path

ROOT = Path("/workspace/btr-dropout-repro/logs/core")
CFGS = ["full-fx-default", "full-fx-allon", "loo-no-arp", "loo-no-juno", "loo-no-909"]
FIELDS = [
    "last_us","max_us","late","audio_xruns","audio_peak_pct","audio_cpu_pct",
    "d_PR_909","d_PR_808","d_PR_303A","d_PR_303B","d_PR_BRK","d_PR_FX","d_PR_MST","d_PR_SEQ",
    "JB_909","JB_808","JB_A","JB_B","JB_BRK","eng_plan",
    "c1_busy_ticks","c1_job_busy_ticks","c1_job_starts","c1_ticks",
    "eng_guard_count","perf_cyc_ok","ui_frames","nested",
]
STATES = ["setup","play","idle","empty","all"]

lines = ["config\tstate\tn\tt_ms_first\tt_ms_last\thalves_first\thalves_last\tfield\tmin\tmax\tfirst\tlast"]
missing = []
for cfg in CFGS:
    d = ROOT / cfg
    table = d / "RAW_TABLE.json"
    hashes = d / "HASHES.json"
    if not table.exists():
        missing.append(cfg)
        continue
    t = json.loads(table.read_text())
    for stn in STATES:
        s = t.get(stn)
        if not s or s.get("n", 0) == 0:
            lines.append(f"{cfg}\t{stn}\t0\t\t\t\t\t\t\t\t\t")
            continue
        for field in FIELDS:
            m = s[field]
            lines.append(
                f"{cfg}\t{stn}\t{s['n']}\t{s['t_ms_first']}\t{s['t_ms_last']}\t"
                f"{s['halves_first']}\t{s['halves_last']}\t{field}\t"
                f"{m['min']}\t{m['max']}\t{m['first']}\t{m['last']}"
            )
out = ROOT / "CORE_SUMMARY_RAW.tsv"
text = "\n".join(lines) + "\n"
out.write_text(text)
h = hashlib.sha256(text.encode()).hexdigest()
meta = {
    "path": str(out),
    "sha256": h,
    "size": len(text.encode()),
    "configs_present": [c for c in CFGS if (ROOT/c/"RAW_TABLE.json").exists()],
    "configs_missing": missing,
    "rows": len(lines)-1,
}
(ROOT / "CORE_SUMMARY_RAW.sha256").write_text(h + "  " + out.name + "\n")
(ROOT / "CORE_SUMMARY_RAW.META.json").write_text(json.dumps(meta, indent=2)+"\n")
print(json.dumps(meta, indent=2))
