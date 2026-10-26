#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render one scenario and self-compare (F_agg must be 100%)."""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
_HOST = Path(__file__).resolve().parent
sys.path.insert(0, str(_HOST))

import chicago_au_map  # noqa: E402
import chicago_au_score  # noqa: E402
from render_chicago_au_compare import (  # noqa: E402
    BANK_JSON,
    RENDER_BIN,
    SCENARIO_DIR,
    _control_text,
    _render_scenario,
)


def _write_float_wav(path: Path, stereo: np.ndarray, sr: int) -> None:
    pcm = stereo.astype(np.float32).tobytes()
    nch = 2
    with wave.open(str(path), "wb") as w:
        w.setnchannels(nch)
        w.setsampwidth(4)
        w.setframerate(sr)
        w.setcomptype("NONE", "not compressed")
        # wave module writes PCM16 by default — write raw via setparams for IEEE float
    # Manual IEEE float WAV (format 3)
    import struct

    data = path
    nframes = stereo.shape[0]
    fmt_chunk = struct.pack(
        "<HHIIHH",
        3,
        2,
        sr,
        sr * nch * 4,
        nch * 4,
        32,
    )
    body = pcm
    riff = b"RIFF" + struct.pack("<I", 4 + 8 + len(fmt_chunk) + 8 + len(body)) + b"WAVE"
    riff += b"fmt " + struct.pack("<I", len(fmt_chunk)) + fmt_chunk
    riff += b"data" + struct.pack("<I", len(body)) + body
    data.write_bytes(riff)


def main() -> int:
    presets = json.loads(BANK_JSON.read_text())["presets"]
    battery = json.loads((SCENARIO_DIR / "battery.json").read_text())
    n_manifests = len(list(SCENARIO_DIR.glob("chicago_au_*.json")))
    expect = len(presets)
    if len(battery["scenarios"]) != expect or n_manifests != expect:
        print(
            f"FAIL battery count scenarios={len(battery['scenarios'])} "
            f"manifests={n_manifests} bank={expect}",
            file=sys.stderr,
        )
        return 1
    if not RENDER_BIN.is_file():
        subprocess.check_call(["sh", "tests/host/run_chicago_au_render.sh"], cwd=ROOT)
    scen_path = SCENARIO_DIR / "chicago_au_acid_lab_1.json"
    scen = json.loads(scen_path.read_text())
    pots = chicago_au_map.preset_from_factory_bank(presets, scen["factoryPresetIndex"], arp_off=True)
    with tempfile.TemporaryDirectory() as td:
        tdp = Path(td)
        cand_f32 = tdp / "cand.f32"
        _render_scenario(scen, pots, cand_f32)
        cand = chicago_au_score.load_render_f32(cand_f32)
        expect_frames = int(scen["frames"])
        if cand.shape[0] != expect_frames:
            print(
                f"FAIL frame count {cand.shape[0]} != {expect_frames} (48 kHz render)",
                file=sys.stderr,
            )
            return 1
        ref_wav = tdp / "ref.wav"
        _write_float_wav(ref_wav, cand, int(scen["sampleRate"]))
        ref = chicago_au_score.load_wav_f32_stereo(ref_wav, expect_sr=int(scen["sampleRate"]))
        # Identical buffers: exercises full goal set without AU refs.
        cmp = chicago_au_score.compare_pair(cand, cand, float(scen["nominalHz"]))
        agg = chicago_au_score.aggregate_f_agg([{"comparison": cmp}])
        if agg["F_agg"] < 99.9:
            print(f"FAIL self F_agg {agg['F_agg']}", file=sys.stderr)
            return 1
    print("ok chicago_au_battery self-compare")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
