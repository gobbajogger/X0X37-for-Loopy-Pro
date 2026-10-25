# SPDX-License-Identifier: GPL-3.0-only
"""Import-safe feature, distance, and contour math for the Chicago host tools.

Reason: the private-reference scripts used to read captures while being imported,
so a maintainer could not check the comparison math without those files.
Purpose: keep the valid-input fits callable on original synthetic tones.
Effect: firmware DSP and the default instrument sound stay unchanged. These
routines are host diagnostics; they do not run unless a tool calls them.
"""
import numpy as np
from scipy.optimize import minimize_scalar
from scipy.signal import resample_poly

SR = 44100
CENTERS = (.3, .4, .5, .6, .7)
# H2..H12 at or above this level count as strong. Fewer than three rejects the fit.
PARTIAL_FLOOR_DB = -40.0
MIN_STRONG_PARTIALS = 3
REJECTED_DISTANCE = 999.0
REJECTION_REASON = "insufficient strong reference harmonics: fewer than three above -40dB"


def db(x):
    return 20 * np.log10(np.maximum(x, 1e-12))


def _unavailable(status):
    """JSON-safe report for a buffer that has no valid harmonic fit."""
    return {
        "input_status": status,
        "pitch_fit_at_search_boundary": None,
        "pitch_interpretation": None,
        "f0": None,
        "cents": None,
        "rms": None,
        "peak": None,
        "relative_harmonics_db": None,
        "envelope50ms": None,
        "onset_threshold_frame": None,
        "tail_last100ms_rms": None,
    }


def features(x, note=48, start=.1, end=.4):
    """Relative harmonics and apparent pitch. Valid finite tones match the historical fit.

    A zero analysis window or any nonfinite sample returns null pitch fields and
    input_status. That path does not invent a frequency.
    """
    x = np.asarray(x, dtype=float)
    if x.ndim != 1:
        raise ValueError("features expects a mono 1-D array")
    if x.size == 0 or not np.isfinite(x).all():
        return _unavailable("nonfinite" if x.size and not np.isfinite(x).all() else "zero")
    y = x[round(start * SR):round(end * SR)]
    if y.size == 0 or not np.any(y):
        return _unavailable("zero")
    nominal = 440 * 2 ** ((note - 69) / 12)
    # Joint harmonic fit on an antialias-free decimation for pitch search: signals
    # already filtered, pitch fit uses only first 4 partials. Full-rate final fit.
    ys = resample_poly(y, 1, 8)
    ts = np.arange(len(ys)) * 8 / SR

    def matrix(f, t, h=4):
        a = 2 * np.pi * f * t[:, None] * np.arange(1, h + 1)
        return np.column_stack([np.ones(len(t)), np.cos(a), np.sin(a)])

    def loss(f):
        m = matrix(f, ts)
        c = np.linalg.lstsq(m, ys, rcond=None)[0]
        return np.mean((ys - np.sum(m * c[None, :], axis=1)) ** 2)

    opt = minimize_scalar(loss, bounds=(nominal * .99, nominal * 1.01), method="bounded", options={"xatol": 1e-6})
    f = float(opt.x)
    boundary = bool(abs(f - nominal * .99) < .001 or abs(f - nominal * 1.01) < .001)
    t = np.arange(len(y)) / SR
    m = matrix(f, t, 12)
    c = np.linalg.lstsq(m, y, rcond=None)[0]
    a = np.hypot(c[1:13], c[13:25])
    rel = db(a / a[0])
    env = [float(np.sqrt(np.mean(x[i:i + 2205] ** 2))) for i in range(0, len(x), 2205)]
    nz = np.flatnonzero(abs(x) > max(abs(x).max() * 1e-4, 1e-10))
    return dict(
        pitch_fit_at_search_boundary=boundary,
        pitch_interpretation="apparent frequency from finite window; changing timbre can bias estimate",
        f0=f,
        cents=float(1200 * np.log2(f / nominal)),
        rms=float(np.sqrt(np.mean(y * y))),
        peak=float(abs(x).max()),
        relative_harmonics_db=rel.tolist(),
        envelope50ms=env,
        onset_threshold_frame=int(nz[0]) if len(nz) else None,
        tail_last100ms_rms=float(np.sqrt(np.mean(x[-4410:] ** 2))),
    )


def distance(a, b):
    """RMSE of reference H2..H12 at or above -40 dB. Fewer than three partials rejects."""
    ar = a.get("relative_harmonics_db") if isinstance(a, dict) else None
    br = b.get("relative_harmonics_db") if isinstance(b, dict) else None
    if ar is None or br is None:
        return REJECTED_DISTANCE
    ar = np.array(ar, dtype=float)
    br = np.array(br, dtype=float)
    if not np.isfinite(ar).all() or not np.isfinite(br).all():
        return REJECTED_DISTANCE
    mask = ar[1:] >= PARTIAL_FLOOR_DB
    if np.count_nonzero(mask) < MIN_STRONG_PARTIALS:
        return REJECTED_DISTANCE
    return float(np.sqrt(np.mean((ar[1:][mask] - br[1:][mask]) ** 2)))


def null_rejected_metrics(row):
    """Turn a rejected train distance into JSON nulls. Other 999 fields null on their own."""
    if row["train_db"] >= REJECTED_DISTANCE:
        row["holdout_db"] = None
        row["rejection_reason"] = REJECTION_REASON
        for field in ("production_cutoff", "production_wave", "production_envmod", "gain_db", "holdout_gain_error_db"):
            if field in row:
                row[field] = None
    for field in ("train_db", "holdout_db", "baseline_train_db", "baseline_holdout_db"):
        if field in row and row[field] is not None and row[field] >= REJECTED_DISTANCE:
            row[field] = None
    return row


def finalize_metric_rows(rows):
    for row in rows:
        null_rejected_metrics(row)
    return rows


def select_candidate(reference, candidates):
    """Pick the train candidate with the smallest (distance, identity...) tuple.

    candidates items are (features, *identity). Holdout audio is not an argument.
    """
    return min((distance(reference, feat), *ident) for feat, *ident in candidates)


def grid_calibration_row(metrics, cut):
    """Note-48 grid search, then note-60 distance on the already chosen candidate."""
    a = metrics[f"cut{cut:.1f}-n48-r0.au"]
    candidates = []
    for w in (0, 1):
        for p in list(range(0, 128, 8)) + [127]:
            k = f"grid-n48-w{w}-c{p}.dsp"
            candidates.append((metrics[k], w, p, k))
    score, w, p, k = select_candidate(a, candidates)
    b = metrics[k]
    ah = metrics[f"cut{cut:.1f}-n60-r0.au"]
    bh = metrics[f"grid-n60-w{w}-c{p}.dsp"]
    baseline = metrics[f"cut{cut:.1f}-n48-r0.dsp"]
    baseh = metrics[f"cut{cut:.1f}-n60-r0.dsp"]
    holdout = distance(ah, bh)
    gain = float(db(a["rms"] / b["rms"]))
    gerr = float(db(bh["rms"] / ah["rms"]) + gain)
    return dict(
        chicago_cutoff=cut,
        production_cutoff=p,
        production_wave=w,
        train_db=score,
        holdout_db=holdout,
        baseline_train_db=distance(a, baseline),
        baseline_holdout_db=distance(ah, baseh),
        strong_nonfundamental_count=int(sum(np.array(a["relative_harmonics_db"])[1:] >= PARTIAL_FLOOR_DB)),
        usable_exploratory_anchor=bool(score <= 3 and holdout <= 3 and abs(gerr) <= 1),
        gain_db=gain,
        holdout_gain_error_db=gerr,
    )


def expanded_calibration_row(metrics, cut, resolve_holdout):
    """Train on note-48 grid plus expanded clips. resolve_holdout runs after the choice."""
    a = metrics[f"cut{cut:.1f}-n48-r0.au"]
    candidates = []
    for p in list(range(0, 128, 8)) + [127]:
        k = f"grid-n48-w0-c{p}.dsp"
        candidates.append((metrics[k], 0, p, k))
    for env in (0, 32, 64, 96, 127):
        for pot in list(range(0, 128, 16)) + [127]:
            k = f"expanded-n48-e{env}-c{pot}.dsp"
            candidates.append((metrics[k], env, pot, k))
    score, env, pot, k = select_candidate(a, candidates)
    b = metrics[k]
    hk = f"grid-n60-w0-c{pot}.dsp" if k.startswith("grid") else f"expanded-n60-e{env}-c{pot}.dsp"
    bh = resolve_holdout(hk)
    metrics[hk] = bh
    ah = metrics[f"cut{cut:.1f}-n60-r0.au"]
    holdout = distance(ah, bh)
    gain = float(db(a["rms"] / b["rms"]))
    gerr = float(db(bh["rms"] / ah["rms"]) + gain)
    return dict(
        chicago_cutoff=cut,
        production_cutoff=pot,
        production_envmod=env,
        production_wave=0,
        train_db=score,
        holdout_db=holdout,
        gain_db=gain,
        holdout_gain_error_db=gerr,
        usable_exploratory_anchor=bool(score <= 3 and holdout <= 3 and abs(gerr) <= 1),
    )


def apparent_hz_at(x, center, scale=1.0):
    """One 50 ms local harmonic pitch. scale moves the coarse search with the MIDI note."""
    x = np.asarray(x, dtype=float)
    sl = x[round((center - .025) * SR):round((center + .025) * SR)]
    if sl.size == 0 or not np.isfinite(sl).all():
        return {
            "center_seconds": center,
            "input_status": "nonfinite" if sl.size and not np.isfinite(sl).all() else "zero",
            "apparent_hz": None,
            "normalized_fit_residual": None,
        }
    if not np.any(sl):
        return {
            "center_seconds": center,
            "input_status": "zero",
            "apparent_hz": None,
            "normalized_fit_residual": None,
        }
    y = resample_poly(sl, 1, 8)
    t = np.arange(len(y)) * 8 / SR

    def loss(f):
        ph = 2 * np.pi * f * t[:, None] * np.arange(1, 5)
        m = np.column_stack([np.ones(len(t)), np.cos(ph), np.sin(ph)])
        c = np.linalg.lstsq(m, y, rcond=None)[0]
        return float(np.mean((y - np.sum(m * c[None, :], axis=1)) ** 2))

    fs = np.arange(120, 211, 2) * scale
    best = fs[np.argmin([loss(f) for f in fs])]
    opt = minimize_scalar(loss, bounds=(best - 2 * scale, best + 2 * scale), method="bounded")
    return {
        "center_seconds": center,
        "apparent_hz": float(opt.x),
        "normalized_fit_residual": loss(opt.x) / float(np.mean(y * y)),
    }


def contour_hz(x, note):
    """Five apparent-Hz samples. Raises on a zero or nonfinite analysis window."""
    scale = 2 ** ((note - 48) / 12)
    values = []
    for center in CENTERS:
        est = apparent_hz_at(x, center, scale=scale)
        if "input_status" in est:
            raise ValueError(est["input_status"])
        values.append(est["apparent_hz"])
    return values


def trajectory_rmse_cents(a, b):
    return float(np.sqrt(np.mean((1200 * np.log2(np.asarray(a, dtype=float) / np.asarray(b, dtype=float))) ** 2)))
