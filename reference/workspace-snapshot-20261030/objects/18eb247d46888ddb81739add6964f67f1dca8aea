#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Score one Chicago AU scenario (numpy-only; no scipy).

Aligned goals mirror Miami ``compare.py`` law + lag-aligned waveform r >= 0.99.
"""
from __future__ import annotations

import math
import struct
from pathlib import Path
from typing import Any

import numpy as np

import au_compare_align

SR_DEFAULT = 48000


def load_wav_f32_stereo(path: Path, expect_sr: int = SR_DEFAULT) -> np.ndarray:
    """Load 32-bit IEEE float WAV (stereo or mono duplicated)."""
    data = path.read_bytes()
    if len(data) < 44 or data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(f"{path}: not a RIFF WAVE file")
    pos = 12
    sr = nch = fmt = None
    pcm = None
    while pos + 8 <= len(data):
        cid = data[pos : pos + 4]
        csz = struct.unpack_from("<I", data, pos + 4)[0]
        body = data[pos + 8 : pos + 8 + csz]
        if cid == b"fmt ":
            fmt, nch, sr = struct.unpack_from("<HHI", body, 0)[0:3]
            sr = int(sr)
            nch = int(nch)
        elif cid == b"data":
            pcm = body
        pos += 8 + csz + (csz & 1)
    if fmt != 3 or pcm is None or sr is None or nch is None:
        raise ValueError(f"{path}: expected 32-bit float WAV (format 3)")
    x = np.frombuffer(pcm, dtype="<f4")
    if nch == 1:
        x = np.column_stack([x, x])
    else:
        x = x.reshape(-1, nch)
    if sr != expect_sr:
        raise ValueError(f"{path}: expected {expect_sr} Hz, got {sr}")
    if x.shape[0] < 1024:
        raise ValueError(f"{path}: too few frames")
    if not np.isfinite(x).all():
        raise ValueError(f"{path}: non-finite samples")
    return x.astype(np.float64)


def load_render_f32(path: Path) -> np.ndarray:
    x = np.fromfile(path, dtype="<f4")
    if x.size < 2 or x.size % 2:
        raise ValueError(f"{path}: expected interleaved stereo float32")
    return x.reshape(-1, 2).astype(np.float64)


def _rms(x: np.ndarray) -> float:
    return float(np.sqrt(np.mean(x * x)))


def _measure_mono(
    y: np.ndarray,
    expected_hz: float,
    sr: int,
    steady_start: int = 4800,
    steady_end: int = 19200,
) -> dict[str, Any]:
    """Harmonic fit on steady voiced window (default 100–400 ms @ 48 kHz)."""
    a = y[steady_start:steady_end]
    tm = np.arange(len(a), dtype=np.float64) / sr
    mid = _rms(a)

    def fit(f: float, count: int) -> tuple[np.ndarray, np.ndarray]:
        k = np.arange(1, count + 1, dtype=np.float64)
        z = np.exp(2j * np.pi * f * tm[:, None] * k)
        mat = np.column_stack([np.ones(len(a)), z.real, z.imag])
        v, _, _, _ = np.linalg.lstsq(mat, a, rcond=None)
        return v, mat

    def loss(f: float) -> float:
        v, mat = fit(f, 8)
        residual = a - mat @ v
        return float(np.mean(residual**2))

    grid = np.linspace(expected_hz * 0.98, expected_hz * 1.02, 160)
    f = float(grid[np.argmin([loss(float(ff)) for ff in grid])])
    v, _ = fit(f, 16)
    c = v[1:17] - 1j * v[17:33]
    valid = mid > 1e-8 and abs(c[0]) > mid * 0.01 and abs(f / expected_hz - 1.0) < 0.019
    env = [_rms(y[i : i + 2400]) for i in range(0, len(y), 2400)]
    return {
        "pitchFitValid": bool(valid),
        "pitchFitHz": f,
        "nominalHz": expected_hz,
        "steadyRms": mid,
        "envelope50msRms": env,
        "fixedHarmonicAbs": np.abs(c).tolist(),
        "fixedHarmonicPhase": np.angle(c).tolist(),
    }


def compare_pair(
    ref: np.ndarray,
    cand: np.ndarray,
    nominal_hz: float,
    sr: int = SR_DEFAULT,
    steady_start: int = 4800,
    steady_end: int = 19200,
) -> dict[str, Any]:
    x = ref.mean(axis=1)
    y = cand.mean(axis=1)
    lag, r_steady = au_compare_align.lag_aligned_correlation(
        x[steady_start:steady_end].tolist(),
        y[steady_start:steady_end].tolist(),
        lag_min=-4096,
        lag_max=4096,
    )
    a = _measure_mono(x, nominal_hz, sr, steady_start, steady_end)
    b = _measure_mono(y, nominal_hz, sr, steady_start, steady_end)
    xx = x[steady_start:steady_end]
    yy = (
        y[steady_start + lag : steady_end + lag]
        if lag >= 0
        else y[steady_start : steady_end + lag]
    )
    if len(yy) != len(xx):
        yy = yy[: len(xx)]
    ha = np.array(a["fixedHarmonicAbs"])
    hb = np.array(b["fixedHarmonicAbs"])
    ref_strong = ha > ha[0] * 0.01
    proto_strong = hb > hb[0] * 0.01
    spectrum_mask = ref_strong | proto_strong
    relative = 20 * np.log10(np.maximum(hb, 1e-20) / np.maximum(ha, 1e-20))
    relative = relative - relative[0]
    env = 20 * np.log10(
        np.maximum(np.array(b["envelope50msRms"]), 1e-20)
        / np.maximum(np.array(a["envelope50msRms"]), 1e-20)
    )
    # Score envelope only on 50 ms bins that cover the steady voiced window
    # (FULL: bins 2..8 for 4800..19200; QUICK: bins for 4800..14400).
    hop = 2400
    env_i0 = max(0, steady_start // hop)
    env_i1 = max(env_i0 + 1, steady_end // hop)
    env_steady = env[env_i0:env_i1]
    if env_steady.size == 0:
        env_steady = env[env_i0 : env_i0 + 1]
    env_err = float(np.max(np.abs(env_steady)))
    pitch_cents = (
        1200 * math.log2(b["pitchFitHz"] / a["pitchFitHz"])
        if a["pitchFitValid"] and b["pitchFitValid"]
        else float("nan")
    )
    if a["pitchFitValid"] and b["pitchFitValid"]:
        pitch_ok = abs(pitch_cents) <= 2.0
    else:
        # Same estimator on ref and cand: compare fitted fundamentals even when
        # harmonic model is weak (typical on filtered 303 timbre).
        pitch_ok = abs(a["pitchFitHz"] - b["pitchFitHz"]) <= 0.5
    goals = {
        "pitchGoal2Cents": bool(pitch_ok),
        "steadyEnvelopeGoal1Db": bool(env_err <= 1.0),
        "strongRelativeSpectrumGoal1Db": bool(
            spectrum_mask.any()
            and float(np.max(np.abs(relative[spectrum_mask]))) <= 1.0
        ),
        "waveformCorrelationGoal099": bool(r_steady >= 0.99),
    }
    return {
        "bestLagSamples": lag,
        "lagAlignedCorrelation": float(r_steady),
        "pitchErrorCents": float(pitch_cents),
        "steadyEnvelopeErrorDbMax": env_err,
        "steadyEnvelopeBinRange": [env_i0, env_i1],
        "strongRelativeHarmonicMaxDb": float(np.max(np.abs(relative[spectrum_mask])))
        if spectrum_mask.any()
        else None,
        "goals": goals,
        "reference": a,
        "prototype": b,
    }


def aggregate_f_agg(rows: list[dict[str, Any]]) -> dict[str, Any]:
    passed = 0
    total = 0
    for row in rows:
        for ok in row["comparison"]["goals"].values():
            total += 1
            if ok:
                passed += 1
    pct = 100.0 * passed / total if total else 0.0
    return {"passedChecks": passed, "totalChecks": total, "F_agg": pct}
