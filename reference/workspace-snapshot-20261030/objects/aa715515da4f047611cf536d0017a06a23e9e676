#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Chicago Acid AU vs native303 offline compare (C-SND-1 / C-SND-2).

When CHICAGO_AU_REFS is unset, exits 0 (run_tests.sh uses not-run).

When set:
  - render candidate PCM via chicago_au_render + scenario battery
  - align vs AU refs (au_compare_align lag search before goals)
  - write per-scenario JSON under build/chicago_au/
  - print Chicago F_agg (sign-off metric; infra does not fail CI below 90%)
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

_HOST_DIR = Path(__file__).resolve().parent
if str(_HOST_DIR) not in sys.path:
    sys.path.insert(0, str(_HOST_DIR))

import chicago_au_map  # noqa: E402
import chicago_au_score  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import au_compare_battery  # noqa: E402

SCENARIO_DIR = ROOT / "tests" / "scenarios" / "chicago_au"
BANK_JSON = ROOT / "assets" / "factory_banks" / "chicago.json"
RENDER_BIN = ROOT / "build" / "chicago_au" / "chicago_au_render"
OUT_DIR = ROOT / "build" / "chicago_au"


def _load_bank() -> list[dict]:
    return json.loads(BANK_JSON.read_text())["presets"]


def _scenario_paths() -> list[Path]:
    return sorted(SCENARIO_DIR.glob("chicago_au_*.json"))


def _control_text(scen: dict, pots: dict) -> str:
    lines = [
        f"frames {scen['frames']}",
        f"note_on {scen['noteOnFrame']}",
        f"note_off {scen['noteOffFrame']}",
        f"midi_note {scen['midiNote']}",
        f"velocity {scen['velocity']}",
    ]
    for idx, val in sorted(pots["bass303"].items()):
        lines.append(f"pot {idx} {val}")
    # Chicago insert FX (Crush/Ring/EQ/Cho/Ens/Phaser/SDly/BPM/Hall/GateRev) — AU refs include grit/sidebands/EQ/chorus/ensemble/phaser/delay/reverb/gate; dry native303 does not.
    chi = pots.get("chi") or {}
    if "fx_type" in chi:
        lines.append(f"fx_type {int(chi['fx_type'])}")
        lines.append(f"fx_edit1 {int(chi.get('fx_edit1', 64))}")
        lines.append(f"fx_edit2 {int(chi.get('fx_edit2', 64))}")
    return "\n".join(lines) + "\n"


def _render_scenario(scen: dict, pots: dict, out_f32: Path) -> None:
    if not RENDER_BIN.is_file():
        raise FileNotFoundError(f"missing renderer (run tests/host/run_chicago_au_render.sh): {RENDER_BIN}")
    proc = subprocess.run(
        [str(RENDER_BIN)],
        input=_control_text(scen, pots).encode("utf-8"),
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(
            f"render failed for {scen['id']}: {proc.stderr.decode('utf-8', 'replace')}"
        )
    out_f32.write_bytes(proc.stdout)


def main() -> int:
    refs = os.environ.get("CHICAGO_AU_REFS", "").strip()
    if not refs:
        print("skip chicago-au-compare (CHICAGO_AU_REFS unset)")
        return 0
    ref_dir = Path(refs)
    if not ref_dir.is_dir():
        print(f"FAIL chicago-au-compare: CHICAGO_AU_REFS not a directory: {ref_dir}", file=sys.stderr)
        return 1
    if not SCENARIO_DIR.is_dir():
        print(f"FAIL chicago-au-compare: missing {SCENARIO_DIR}", file=sys.stderr)
        return 1
    scenarios = _scenario_paths()
    if not scenarios:
        print(
            "ok chicago-au-compare (stub: CHICAGO_AU_REFS set; "
            "no chicago_au_*.json scenarios yet — C-SND-1)",
            file=sys.stderr,
        )
        return 1
    presets = _load_bank()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    rows: list[dict] = []
    sr = 48000
    for path in scenarios:
        scen = au_compare_battery.apply_timing_to_scenario(json.loads(path.read_text()))
        sid = scen["id"]
        pidx = int(scen["factoryPresetIndex"])
        pots = chicago_au_map.preset_from_factory_bank(
            presets, pidx, arp_off=bool(scen.get("arpOff", True))
        )
        cand_f32 = OUT_DIR / f"{sid}.f32"
        _render_scenario(scen, pots, cand_f32)
        cand = chicago_au_score.load_render_f32(cand_f32)
        ref_wav = ref_dir / scen.get("refWav", f"{sid}.wav")
        if not ref_wav.is_file():
            print(f"FAIL chicago-au-compare: missing ref {ref_wav}", file=sys.stderr)
            return 1
        ref = chicago_au_score.load_wav_f32_stereo(ref_wav, expect_sr=int(scen.get("sampleRate", sr)))
        if ref.shape[0] != cand.shape[0]:
            n = min(ref.shape[0], cand.shape[0])
            ref = ref[:n]
            cand = cand[:n]
        ws, we = au_compare_battery.steady_window_from_manifest(scen)
        comparison = chicago_au_score.compare_pair(
            ref,
            cand,
            float(scen["nominalHz"]),
            sr=int(scen.get("sampleRate", sr)),
            steady_start=ws,
            steady_end=we,
        )
        row = {
            "id": sid,
            "manifest": path.name,
            "refWav": ref_wav.name,
            "tags": scen.get("tags", []),
            "compareTiming": scen.get("compareTiming"),
            "comparison": comparison,
        }
        rows.append(row)
        (OUT_DIR / f"{sid}.json").write_text(json.dumps(row, indent=2) + "\n")

    summary = chicago_au_score.aggregate_f_agg(rows)
    summary["scenarios"] = len(rows)
    summary["engine"] = "chicago"
    summary["harness"] = au_compare_battery.harness_summary_line()
    report_path = OUT_DIR / "report.json"
    report_path.write_text(json.dumps({"summary": summary, "scenarios": rows}, indent=2) + "\n")
    fagg = summary["F_agg"]
    print(
        f"ok chicago-au-compare F_agg={fagg:.1f}% "
        f"({summary['passedChecks']}/{summary['totalChecks']} checks) "
        f"report={report_path}"
    )
    if os.environ.get("CHICAGO_AU_FAIL_BELOW", "").strip() == "1":
        if fagg < 90.0:
            print("FAIL chicago-au-compare: F_agg below 90% sign-off bar", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
