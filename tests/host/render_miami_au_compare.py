#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Miami Wobble AU offline compare.

Primary (default): factory preset patches — all presets in tests/scenarios/miami_au/.
Secondary: AU_COMPARE_MODE=matrix — isolated per-control matrix (experiments/wobble/compare.py).

Set MIAMI_AU_REFS to a directory of AU reference WAVs (gitignored). When unset, exits 0.
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import au_compare_battery  # noqa: E402

_HOST = Path(__file__).resolve().parent
sys.path.insert(0, str(_HOST))

import chicago_au_score  # noqa: E402
import miami_au_map  # noqa: E402

SCENARIO_DIR = ROOT / "tests" / "scenarios" / "miami_au"
BANK_JSON = ROOT / "assets" / "factory_banks" / "miami.json"
RENDER_BIN = ROOT / "build" / "miami_au" / "wobble_au_render"
MATRIX_RENDER = ROOT / "build" / "wobble-host" / "render"
OUT_DIR = ROOT / "build" / "miami_au"


def _load_bank() -> list[dict]:
    return json.loads(BANK_JSON.read_text())["presets"]


def _scenario_paths() -> list[Path]:
    return sorted(SCENARIO_DIR.glob("miami_au_*.json"))


def _control_text(scen: dict, wobble: dict[int, float]) -> str:
    lines = [
        f"frames {scen['frames']}",
        f"note_on {scen['noteOnFrame']}",
        f"note_off {scen['noteOffFrame']}",
        f"midi_note {scen['midiNote']}",
        f"velocity {scen['velocity']}",
    ]
    for idx, val in sorted(wobble.items()):
        lines.append(f"wobble {idx} {val}")
    return "\n".join(lines) + "\n"


def _render_preset(scen: dict, wobble: dict[int, float], out_f32: Path) -> None:
    if not RENDER_BIN.is_file():
        subprocess.check_call(["sh", "tests/host/run_miami_au_render.sh"], cwd=ROOT)
    proc = subprocess.run(
        [str(RENDER_BIN)],
        input=_control_text(scen, wobble).encode("utf-8"),
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(
            f"render failed for {scen['id']}: {proc.stderr.decode('utf-8', 'replace')}"
        )
    out_f32.write_bytes(proc.stdout)


def _run_preset_battery(ref_dir: Path) -> int:
    scenarios = _scenario_paths()
    if not scenarios:
        print("FAIL miami-au-compare: no miami_au_*.json scenarios", file=sys.stderr)
        return 1
    presets = _load_bank()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    rows: list[dict] = []
    sr = 48000
    for path in scenarios:
        scen = au_compare_battery.apply_timing_to_scenario(json.loads(path.read_text()))
        sid = scen["id"]
        pidx = int(scen["factoryPresetIndex"])
        mapped = miami_au_map.preset_from_factory_bank(presets, pidx)
        cand_f32 = OUT_DIR / f"{sid}.f32"
        _render_preset(scen, mapped["wobble"], cand_f32)
        cand = chicago_au_score.load_render_f32(cand_f32)
        ref_wav = ref_dir / scen.get("refWav", f"{sid}.wav")
        if not ref_wav.is_file():
            print(f"FAIL miami-au-compare: missing ref {ref_wav}", file=sys.stderr)
            return 1
        ref = chicago_au_score.load_wav_f32_stereo(ref_wav, expect_sr=int(scen.get("sampleRate", sr)))
        n = min(ref.shape[0], cand.shape[0])
        ref, cand = ref[:n], cand[:n]
        ws, we = au_compare_battery.steady_window_from_manifest(scen)
        comparison = chicago_au_score.compare_pair(
            ref, cand, float(scen["nominalHz"]), sr=sr, steady_start=ws, steady_end=we
        )
        row = {
            "id": sid,
            "manifest": path.name,
            "refWav": ref_wav.name,
            "factoryPresetName": scen.get("factoryPresetName"),
            "tags": scen.get("tags", []),
            "compareTiming": scen.get("compareTiming"),
            "comparison": comparison,
        }
        rows.append(row)
        (OUT_DIR / f"{sid}.json").write_text(json.dumps(row, indent=2) + "\n")

    summary = chicago_au_score.aggregate_f_agg(rows)
    summary["scenarios"] = len(rows)
    summary["engine"] = "miami"
    summary["harness"] = au_compare_battery.harness_summary_line()
    report_path = OUT_DIR / "report.json"
    report_path.write_text(json.dumps({"summary": summary, "scenarios": rows}, indent=2) + "\n")
    fagg = summary["F_agg"]
    print(
        f"ok miami-au-compare preset F_agg={fagg:.1f}% "
        f"({summary['passedChecks']}/{summary['totalChecks']}) "
        f"{summary['harness']} report={report_path}"
    )
    return 0


def _ensure_matrix_render() -> None:
    if MATRIX_RENDER.is_file():
        return
    subprocess.check_call(["sh", "experiments/wobble/run.sh"], cwd=ROOT)


def _run_matrix_diagnostic(ref_dir: Path) -> int:
    manifest = ref_dir / "Miami-matrix.json"
    if not manifest.is_file():
        print(f"FAIL miami-au-compare matrix: missing {manifest}", file=sys.stderr)
        return 1
    _ensure_matrix_render()
    render_out = OUT_DIR / "matrix_renders"
    render_out.mkdir(parents=True, exist_ok=True)
    frames = au_compare_battery.miami_matrix_frame_count()
    os.environ["WOBBLE_COMPARE_FRAMES"] = str(frames)
    os.environ["WOBBLE_COMPARE_NOTE_OFF"] = str(au_compare_battery.miami_matrix_note_off_frame())
    for cfg in au_compare_battery.MIAMI_MATRIX_CONFIGS:
        subprocess.check_call(
            [str(MATRIX_RENDER), cfg, str(render_out / f"{cfg}.f32")],
            cwd=ROOT,
        )
    out_json = OUT_DIR / "matrix_comparison.json"
    subprocess.check_call(
        [
            "python3",
            str(ROOT / "experiments" / "wobble" / "compare.py"),
            str(ref_dir),
            str(render_out),
            str(out_json),
        ],
        cwd=ROOT,
    )
    print(
        f"ok miami-au-compare matrix diagnostic {au_compare_battery.harness_summary_line()} "
        f"json={out_json}"
    )
    return 0


def main() -> int:
    refs = os.environ.get("MIAMI_AU_REFS", "").strip()
    if not refs:
        print("skip miami-au-compare (MIAMI_AU_REFS unset)")
        return 0
    ref_dir = Path(refs)
    if not ref_dir.is_dir():
        print(f"FAIL miami-au-compare: MIAMI_AU_REFS not a directory: {ref_dir}", file=sys.stderr)
        return 1
    if au_compare_battery.compare_mode() == "matrix":
        return _run_matrix_diagnostic(ref_dir)
    return _run_preset_battery(ref_dir)


if __name__ == "__main__":
    raise SystemExit(main())
