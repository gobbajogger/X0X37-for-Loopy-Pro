#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render one Miami preset scenario and self-compare (F_agg must be 100%)."""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import struct
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
_HOST = Path(__file__).resolve().parent
sys.path.insert(0, str(_HOST))
sys.path.insert(0, str(ROOT / "tools"))

import au_compare_battery  # noqa: E402
import chicago_au_score  # noqa: E402
import miami_au_map  # noqa: E402
from render_miami_au_compare import (  # noqa: E402
    BANK_JSON,
    RENDER_BIN,
    SCENARIO_DIR,
    _control_text,
    _render_preset,
)


def _write_float_wav(path: Path, stereo: np.ndarray, sr: int) -> None:
    pcm = stereo.astype(np.float32).tobytes()
    nch = 2
    nframes = stereo.shape[0]
    fmt_chunk = struct.pack("<HHIIHH", 3, 2, sr, sr * nch * 4, nch * 4, 32)
    riff = b"RIFF" + struct.pack("<I", 4 + 8 + len(fmt_chunk) + 8 + len(pcm)) + b"WAVE"
    riff += b"fmt " + struct.pack("<I", len(fmt_chunk)) + fmt_chunk
    riff += b"data" + struct.pack("<I", len(pcm)) + pcm
    path.write_bytes(riff)


def main() -> int:
    if not RENDER_BIN.is_file():
        subprocess.check_call(["sh", "tests/host/run_miami_au_render.sh"], cwd=ROOT)
    scen_path = SCENARIO_DIR / "miami_au_init_miami.json"
    scen = au_compare_battery.apply_timing_to_scenario(json.loads(scen_path.read_text()))
    presets = json.loads(BANK_JSON.read_text())["presets"]
    mapped = miami_au_map.preset_from_factory_bank(presets, scen["factoryPresetIndex"])
    with tempfile.TemporaryDirectory() as td:
        tdp = Path(td)
        cand_f32 = tdp / "cand.f32"
        _render_preset(scen, mapped["wobble"], cand_f32)
        cand = chicago_au_score.load_render_f32(cand_f32)
        ref_wav = tdp / "ref.wav"
        _write_float_wav(ref_wav, cand, int(scen["sampleRate"]))
        ref = chicago_au_score.load_wav_f32_stereo(ref_wav, expect_sr=int(scen["sampleRate"]))
        ws, we = au_compare_battery.steady_window_from_manifest(scen)
        cmp = chicago_au_score.compare_pair(
            cand, cand, float(scen["nominalHz"]), steady_start=ws, steady_end=we
        )
        agg = chicago_au_score.aggregate_f_agg([{"comparison": cmp}])
        if agg["F_agg"] < 99.9:
            print(f"FAIL self F_agg {agg['F_agg']}", file=sys.stderr)
            return 1
    print("ok miami_au_battery self-compare")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
