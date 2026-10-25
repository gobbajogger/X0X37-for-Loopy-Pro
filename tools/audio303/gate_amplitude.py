# SPDX-License-Identifier: GPL-3.0-only
"""Opt-in host gate-envelope and velocity correction for bass303 renders.

Reason: Chicago's Gate holds level while this source amp falls, and a soft
velocity is quieter on Chicago than the source velocity law alone.
Purpose: give one explicit host correction for that amplitude gap.
Effect: without the flag, the renderer output is unchanged. With the flag, a
musician hears the same bass303 tone with a slower held-note fall and a steeper
velocity step. Firmware and the default sound stay as they are. This is not a
claim that the two instruments match.

The search uses only the training rows named below. Held-out rows are scored
after the three coefficients are frozen. A bin whose Chicago or model RMS is
below 1e-8 is left out of that row's RMSE before the mean is taken. A row with
fewer than four remaining bins in the scored window is reported null. That rule
is fixed here and is not revised after a result looks bad.
"""
import hashlib
import json
import math
import subprocess
from pathlib import Path

import numpy as np

DIR = Path(__file__).resolve().parent
REPO = DIR.parents[1]
REFERENCE = DIR / "data" / "approved-amplitude-reference.json"
RESULTS_JSON = DIR / "data" / "gate-amplitude-results.json"
RESULTS_MD = DIR / "GATE_AMPLITUDE_RESULTS.md"
BASS303_SHA256 = "321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f"
SR = 44100
BIN = 2205
LINEAR_FLOOR = 1e-8
PRIMARY_LO = 2
PRIMARY_HI = 8
LEVEL_START = 4410
LEVEL_END = 17640
SLOPE_STEP = 0.01
SLOPE_MAX = 20.0
MATERIAL_TEMPORAL_DB = 0.5
MATERIAL_TRANSFER_DB = 0.5
TRAIN_CUTOFF_IDS = (
    "cut0.2-n48-r0",
    "cut0.4-n48-r0",
    "cut0.6-n48-r0",
    "cut0.8-n48-r0",
    "cut1.0-n48-r0",
)
VELOCITY_TRAIN_ID = "vel40-r0"
LEVEL_ANCHOR_ID = "cut0.6-n48-r0"
NOTE60_LEVEL_ID = "cut0.6-n60-r0"
HELDOUT_TEMPORAL_IDS = (
    "cut0.2-n60-r0",
    "cut0.4-n60-r0",
    "cut0.6-n60-r0",
    "cut0.8-n60-r0",
    "cut1.0-n60-r0",
    "vel80-r0",
    "vel127-r0",
    "velocity40-n60-r0",
    "velocity80-n60-r0",
    "velocity127-n60-r0",
)
HELDOUT_TRANSFER_N48 = ("vel80-r0", "vel127-r0")
HELDOUT_TRANSFER_N60 = (
    "velocity40-n60-r0",
    "velocity80-n60-r0",
    "velocity127-n60-r0",
)
HISTORY_IDS = (
    "repeat-r0",
    "accent_repeat-r0",
    "overlap_glide0-r0",
    "overlap_glide_mid-r0",
    "overlap_retrigger-r0",
    "repeat-n60-r0",
)
REGRESSION_IDS = (
    "wave0.5-r0",
    "wave1.0-r0",
    "reso0.5-r0",
    "reso1.0-r0",
    "env_positive-r0",
    "env_amp-r0",
    "attack-r0",
)
COMPILE_CMD = [
    "cc",
    "-std=c99",
    "-O2",
    "-ffp-contract=off",
    "-Ifirmware/src",
    "-Ifirmware/src/dsp",
    "tools/audio303/render.c",
    "firmware/src/dsp/bass303.c",
    "-lm",
]


def load_reference(path=None):
    path = Path(path) if path else REFERENCE
    return json.loads(path.read_text())


def index_rows(doc):
    rows = {row["id"]: row for row in doc["rows"]}
    for row in doc["fresh_heldout"]["rows"]:
        rows[row["id"]] = row
    return rows


def shape_time(n, sequence):
    """Seconds used by the host gate law. Matches render.c gate_segment."""
    if sequence not in (0, 1, 2, 3):
        raise ValueError(f"sequence {sequence} is outside the bounded harness")
    frames = np.arange(n)
    t = np.empty(n, dtype=np.float64)
    if sequence <= 1:
        first = frames < 22050
        t[first] = frames[first] / SR
        if sequence == 0:
            t[~first] = 22050 / SR
            return t
        gap = (frames >= 22050) & (frames < 44100)
        second = (frames >= 44100) & (frames < 66150)
        done = frames >= 66150
        t[gap] = 22050 / SR
        t[second] = (frames[second] - 44100) / SR
        t[done] = (66150 - 44100) / SR
        return t
    if sequence == 2:
        held = frames < 33075
        t[held] = frames[held] / SR
        t[~held] = 33075 / SR
        return t
    first = frames < 11025
    second = (frames >= 11025) & (frames < 33075)
    done = frames >= 33075
    t[first] = frames[first] / SR
    t[second] = (frames[second] - 11025) / SR
    t[done] = (33075 - 11025) / SR
    return t


def apply_model(audio, velocity, sequence, gain_db, beta, slope):
    """Same sample law as the opt-in host flag, in float64 on float32 audio."""
    audio = np.asarray(audio, dtype=np.float64)
    db = float(gain_db) + float(slope) * shape_time(len(audio), sequence)
    gain = np.power(10.0, db / 20.0)
    if float(beta) != 0.0:
        gain = gain * (float(velocity) / 100.0) ** float(beta)
    return audio * gain


def envelope_50ms(audio):
    audio = np.asarray(audio, dtype=np.float64)
    bins = []
    for start in range(0, len(audio), BIN):
        window = audio[start:start + BIN]
        bins.append(float(np.sqrt(np.mean(window * window))))
    return bins


def level_db(audio):
    window = np.asarray(audio, dtype=np.float64)[LEVEL_START:LEVEL_END]
    rms = float(np.sqrt(np.mean(window * window)))
    if not math.isfinite(rms) or rms < LINEAR_FLOOR:
        return None
    return 20.0 * math.log10(rms)


def _window_errors(reference, model, normalize):
    reference = list(reference)
    model = list(model)
    excluded = []
    errors = []
    if reference[PRIMARY_LO] < LINEAR_FLOOR or model[PRIMARY_LO] < LINEAR_FLOOR:
        return None, list(range(PRIMARY_LO, PRIMARY_HI + 1))
    ref0 = 20.0 * math.log10(reference[PRIMARY_LO])
    mod0 = 20.0 * math.log10(model[PRIMARY_LO])
    for index in range(PRIMARY_LO, PRIMARY_HI + 1):
        if index >= len(reference) or index >= len(model):
            excluded.append(index)
            continue
        left = reference[index]
        right = model[index]
        if left < LINEAR_FLOOR or right < LINEAR_FLOOR or not math.isfinite(left) or not math.isfinite(right):
            excluded.append(index)
            continue
        ref_db = 20.0 * math.log10(left)
        mod_db = 20.0 * math.log10(right)
        if normalize:
            ref_db -= ref0
            mod_db -= mod0
        errors.append(mod_db - ref_db)
    if len(errors) < 4:
        return None, excluded
    return float(math.sqrt(sum(item * item for item in errors) / len(errors))), excluded


def temporal_rmse(reference, model):
    return _window_errors(reference, model, True)


def absolute_rmse(reference, model):
    return _window_errors(reference, model, False)


def segment_temporal_rmse(reference, model, origin_frame):
    """Normalized RMSE on the seven bins that match .125–.425 s inside a note."""
    shift = origin_frame // BIN
    return temporal_rmse(list(reference)[shift:], list(model)[shift:])


def read_f32(path):
    data = np.fromfile(path, dtype="<f4")
    return np.array(data, dtype=np.float64, copy=True)


def render_row(renderer, dest, row, model=None):
    args = [
        row["frames"],
        row["note"],
        row["velocity"],
        *row["production"],
        row["sequence"],
        row["accent"],
        0,
        0,
    ]
    cmd = [str(renderer), str(dest), *map(str, args)]
    if model is not None:
        gain_s, beta_s, slope_s = model
        cmd += ["--experimental-gate-model", gain_s, beta_s, slope_s]
    result = subprocess.run(cmd, check=False, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"renderer exit {result.returncode} for {row['id']}: {result.stderr.decode()[:400]}"
        )
    return read_f32(dest)


def compile_renderer(dest, repo=REPO):
    cmd = [*COMPILE_CMD, "-o", str(dest)]
    result = subprocess.run(cmd, cwd=repo, check=False, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(result.stderr[-2000:])
    return cmd, result.returncode


def fit_slope(audio, sequence, chicago_by_id, train_ids):
    refused = [item for item in train_ids if item not in TRAIN_CUTOFF_IDS]
    if refused or list(train_ids) != list(TRAIN_CUTOFF_IDS):
        raise ValueError("slope fit accepts only the five training cutoff rows")
    best = None
    steps = int(round(SLOPE_MAX / SLOPE_STEP))
    time = shape_time(len(audio), sequence)
    scale = time * (math.log(10.0) / 20.0)
    audio = np.asarray(audio, dtype=np.float64)
    for step in range(steps + 1):
        slope = step * SLOPE_STEP
        gained = audio * np.exp(slope * scale)
        env = envelope_50ms(gained)
        losses = []
        for item in train_ids:
            score, _ = temporal_rmse(chicago_by_id[item], env)
            if score is None:
                raise ValueError(f"training row {item} has no usable envelope")
            losses.append(score)
        mean = sum(losses) / len(losses)
        if best is None or mean < best[0]:
            best = (mean, step, losses)
    slope = best[1] * SLOPE_STEP
    return {"mean_temporal_rmse_db": best[0], "slope_db_per_s": slope, "rows_db": best[2]}


def freeze_decimal(value, places):
    text = f"{value:.{places}f}"
    return float(text), text


def chicago_level(row):
    rms = row["measurements"]["au"]["rms"]
    if rms is None or rms < LINEAR_FLOOR:
        return None
    return 20.0 * math.log10(rms)


def mean_defined(values):
    usable = [item for item in values if item is not None]
    if len(usable) != len(values) or not usable:
        return None
    return sum(usable) / len(usable)


def rmse_of(values):
    if any(item is None for item in values) or not values:
        return None
    return math.sqrt(sum(item * item for item in values) / len(values))


def peak_cents(audio, note):
    window = np.asarray(audio, dtype=np.float64)[LEVEL_START:LEVEL_END]
    window = window - float(np.mean(window))
    window = window * np.hanning(len(window))
    mag = np.abs(np.fft.rfft(window))
    nominal = 440.0 * 2.0 ** ((note - 69) / 12.0)
    freqs = np.fft.rfftfreq(len(window), 1.0 / SR)
    band = np.flatnonzero((freqs > nominal * 0.95) & (freqs < nominal * 1.05))
    if band.size == 0:
        return None
    local = int(band[np.argmax(mag[band])])
    delta = 0.0
    if 0 < local < len(mag) - 1:
        left, mid, right = float(mag[local - 1]), float(mag[local]), float(mag[local + 1])
        denom = left - 2.0 * mid + right
        if denom != 0.0:
            delta = 0.5 * (left - right) / denom
    freq = (local + delta) * SR / len(window)
    if freq <= 0.0:
        return None
    return float(1200.0 * math.log2(freq / nominal))


def harmonic_distance(audio, f0, chicago_rel):
    if f0 is None or chicago_rel is None:
        return None
    y = np.asarray(audio, dtype=np.float64)[LEVEL_START:LEVEL_END]
    t = np.arange(len(y)) / SR
    angles = 2.0 * math.pi * float(f0) * t[:, None] * np.arange(1, 13)
    matrix = np.column_stack([np.ones(len(y)), np.cos(angles), np.sin(angles)])
    coeff = np.linalg.lstsq(matrix, y, rcond=None)[0]
    amps = np.hypot(coeff[1:13], coeff[13:25])
    if amps[0] < LINEAR_FLOOR:
        return None
    rel = 20.0 * np.log10(np.maximum(amps / amps[0], 1e-12))
    chicago = np.asarray(chicago_rel, dtype=float)
    if chicago.shape != rel.shape or not np.isfinite(chicago).all():
        return None
    mask = chicago[1:] >= -40.0
    if int(np.count_nonzero(mask)) < 3:
        return None
    err = chicago[1:][mask] - rel[1:][mask]
    return float(np.sqrt(np.mean(err * err)))


def tail_index(sequence):
    if sequence <= 1:
        return 10
    if sequence >= 2:
        return 15
    return None


def row_metrics(row, baseline, modeled, role):
    chicago = row["measurements"]["au"]["envelope50ms"]
    stored = row["measurements"]["dsp"]["envelope50ms"]
    base_env = envelope_50ms(baseline)
    model_env = envelope_50ms(modeled)
    temporal_base, excluded_base = temporal_rmse(chicago, base_env)
    temporal_model, excluded_model = temporal_rmse(chicago, model_env)
    absolute_base, _ = absolute_rmse(chicago, base_env)
    absolute_model, _ = absolute_rmse(chicago, model_env)
    agree, _ = temporal_rmse(stored, base_env)
    second_base = second_model = None
    if row["sequence"] == 1 and len(base_env) >= 29:
        second_base, _ = segment_temporal_rmse(chicago, base_env, 44100)
        second_model, _ = segment_temporal_rmse(chicago, model_env, 44100)
    tail = tail_index(row["sequence"])
    def bin_db(env, index):
        if index is None or index >= len(env) or env[index] < LINEAR_FLOOR:
            return None
        return 20.0 * math.log10(env[index])
    nonfinite = int(np.count_nonzero(~np.isfinite(modeled)))
    clipped = int(np.count_nonzero(np.abs(modeled) > 1.0))
    base_cents = peak_cents(baseline, row["note"])
    model_cents = peak_cents(modeled, row["note"])
    shift = None
    if base_cents is not None and model_cents is not None:
        shift = model_cents - base_cents
    f0 = row["measurements"]["dsp"].get("f0")
    rel = row["measurements"]["au"].get("relative_harmonics_db")
    return {
        "id": row["id"],
        "role": role,
        "note": row["note"],
        "velocity": row["velocity"],
        "sequence": row["sequence"],
        "accent": row["accent"],
        "baseline_temporal_rmse_db": temporal_base,
        "model_temporal_rmse_db": temporal_model,
        "baseline_absolute_rmse_db": absolute_base,
        "model_absolute_rmse_db": absolute_model,
        "excluded_primary_bins_baseline": excluded_base,
        "excluded_primary_bins_model": excluded_model,
        "chicago_level_db": chicago_level(row),
        "baseline_level_db": level_db(baseline),
        "model_level_db": level_db(modeled),
        "dsp_agreement_temporal_rmse_db": agree,
        "baseline_peak": float(np.max(np.abs(baseline))),
        "model_peak": float(np.max(np.abs(modeled))),
        "model_clipped_samples": clipped,
        "model_nonfinite_samples": nonfinite,
        "tail_bin": tail,
        "chicago_tail_db": bin_db(chicago, tail),
        "baseline_tail_db": bin_db(base_env, tail),
        "model_tail_db": bin_db(model_env, tail),
        "second_note_baseline_temporal_rmse_db": second_base,
        "second_note_model_temporal_rmse_db": second_model,
        "pitch_shift_cents": shift,
        "baseline_harmonic_distance_db": harmonic_distance(baseline, f0, rel),
        "model_harmonic_distance_db": harmonic_distance(modeled, f0, rel),
    }


def transfer_errors(metrics_by_id, ids, reference_id):
    errors = []
    for item in ids:
        ref = metrics_by_id[reference_id]
        row = metrics_by_id[item]
        if None in (ref["chicago_level_db"], row["chicago_level_db"], ref["baseline_level_db"], row["baseline_level_db"], ref["model_level_db"], row["model_level_db"]):
            errors.append((None, None))
            continue
        chicago = row["chicago_level_db"] - ref["chicago_level_db"]
        baseline = row["baseline_level_db"] - ref["baseline_level_db"]
        model = row["model_level_db"] - ref["model_level_db"]
        errors.append((baseline - chicago, model - chicago))
    return errors


def build_report(renderer, reference_path=REFERENCE, work_dir=None):
    doc = load_reference(reference_path)
    rows = index_rows(doc)
    work = Path(work_dir) if work_dir else Path("/tmp/audio303-gate")
    work.mkdir(parents=True, exist_ok=True)
    needed = list(dict.fromkeys([
        *TRAIN_CUTOFF_IDS,
        VELOCITY_TRAIN_ID,
        LEVEL_ANCHOR_ID,
        NOTE60_LEVEL_ID,
        *HELDOUT_TEMPORAL_IDS,
        *HISTORY_IDS,
        *REGRESSION_IDS,
        "cut0.6-n48-r1",
        "velocity40-n60-r1",
    ]))
    baseline_audio = {}
    for item in needed:
        baseline_audio[item] = render_row(renderer, work / f"{item}.f32", rows[item])
    anchor = baseline_audio[LEVEL_ANCHOR_ID]
    for item in TRAIN_CUTOFF_IDS:
        if not np.array_equal(baseline_audio[item], anchor):
            raise RuntimeError(f"{item} does not match the shared training source render")
    chicago_env = {item: rows[item]["measurements"]["au"]["envelope50ms"] for item in TRAIN_CUTOFF_IDS}
    slope_fit = fit_slope(anchor, rows[LEVEL_ANCHOR_ID]["sequence"], chicago_env, TRAIN_CUTOFF_IDS)
    slope, slope_s = freeze_decimal(slope_fit["slope_db_per_s"], 2)
    shaped = {
        VELOCITY_TRAIN_ID: apply_model(baseline_audio[VELOCITY_TRAIN_ID], rows[VELOCITY_TRAIN_ID]["velocity"], rows[VELOCITY_TRAIN_ID]["sequence"], 0.0, 0.0, slope),
        LEVEL_ANCHOR_ID: apply_model(anchor, rows[LEVEL_ANCHOR_ID]["velocity"], 0, 0.0, 0.0, slope),
    }
    chicago_transfer = chicago_level(rows[VELOCITY_TRAIN_ID]) - chicago_level(rows[LEVEL_ANCHOR_ID])
    source_transfer = level_db(shaped[VELOCITY_TRAIN_ID]) - level_db(shaped[LEVEL_ANCHOR_ID])
    unit = 20.0 * math.log10(40.0 / 100.0)
    beta_raw = (chicago_transfer - source_transfer) / unit
    beta, beta_s = freeze_decimal(beta_raw, 8)
    leveled = apply_model(anchor, rows[LEVEL_ANCHOR_ID]["velocity"], 0, 0.0, beta, slope)
    gain_raw = chicago_level(rows[LEVEL_ANCHOR_ID]) - level_db(leveled)
    gain_db, gain_s = freeze_decimal(gain_raw, 8)
    model = (gain_s, beta_s, slope_s)
    modeled_audio = {}
    for item in needed:
        row = rows[item]
        modeled_audio[item] = render_row(renderer, work / f"{item}.model.f32", row, model)
    roles = {}
    for item in TRAIN_CUTOFF_IDS:
        roles[item] = "train_shape"
    roles[VELOCITY_TRAIN_ID] = "train_velocity"
    for item in HELDOUT_TEMPORAL_IDS:
        roles[item] = "heldout"
    for item in HISTORY_IDS:
        roles[item] = "history"
    for item in REGRESSION_IDS:
        roles[item] = "regression"
    roles["cut0.6-n48-r1"] = "determinism"
    roles["velocity40-n60-r1"] = "determinism"
    metrics = [
        row_metrics(rows[item], baseline_audio[item], modeled_audio[item], roles[item])
        for item in needed
    ]
    by_id = {item["id"]: item for item in metrics}
    # Python law versus the C flag, on the clips whose schedules differ.
    python_vs_c = {}
    for item in (LEVEL_ANCHOR_ID, VELOCITY_TRAIN_ID, "repeat-r0", "overlap_glide_mid-r0", "overlap_retrigger-r0"):
        row = rows[item]
        py = apply_model(baseline_audio[item], row["velocity"], row["sequence"], gain_db, beta, slope)
        py_env = envelope_50ms(py)
        c_env = envelope_50ms(modeled_audio[item])
        score, _ = temporal_rmse(py_env, c_env)
        py_level = level_db(py)
        c_level = level_db(modeled_audio[item])
        python_vs_c[item] = {
            "temporal_rmse_db": score,
            "level_delta_db": None if py_level is None or c_level is None else c_level - py_level,
        }
    def temporal_mean(ids, key):
        return mean_defined([by_id[item][key] for item in ids])
    train_base = temporal_mean(TRAIN_CUTOFF_IDS, "baseline_temporal_rmse_db")
    train_model = temporal_mean(TRAIN_CUTOFF_IDS, "model_temporal_rmse_db")
    held_base = temporal_mean(HELDOUT_TEMPORAL_IDS, "baseline_temporal_rmse_db")
    held_model = temporal_mean(HELDOUT_TEMPORAL_IDS, "model_temporal_rmse_db")
    n48_errors = transfer_errors(by_id, HELDOUT_TRANSFER_N48, LEVEL_ANCHOR_ID)
    n60_errors = transfer_errors(by_id, HELDOUT_TRANSFER_N60, NOTE60_LEVEL_ID)
    train_velocity_errors = transfer_errors(by_id, (VELOCITY_TRAIN_ID,), LEVEL_ANCHOR_ID)
    held_errors = n48_errors + n60_errors
    baseline_transfer = rmse_of([item[0] for item in held_errors])
    model_transfer = rmse_of([item[1] for item in held_errors])
    train_transfer_base = rmse_of([item[0] for item in train_velocity_errors])
    train_transfer_model = rmse_of([item[1] for item in train_velocity_errors])
    temporal_gain = None if held_base is None or held_model is None else held_base - held_model
    transfer_gain = None if baseline_transfer is None or model_transfer is None else baseline_transfer - model_transfer
    on_boundary = slope <= 0.0 or slope >= SLOPE_MAX
    nonfinite = sum(int(np.count_nonzero(~np.isfinite(audio))) for audio in modeled_audio.values())
    accepted = bool(
        nonfinite == 0
        and not on_boundary
        and temporal_gain is not None
        and transfer_gain is not None
        and temporal_gain >= MATERIAL_TEMPORAL_DB
        and transfer_gain >= MATERIAL_TRANSFER_DB
    )
    if nonfinite:
        reason = "Model audio contained non-finite samples."
    elif on_boundary:
        reason = "The slope search sat on the edge of the predetermined grid."
    elif accepted:
        reason = "Held-out gate shape and velocity transfer both improved by the predetermined margin."
    else:
        reason = "Held-out gate shape or velocity transfer did not clear the predetermined margin."
    regressions = []
    for item in list(HISTORY_IDS) + list(REGRESSION_IDS):
        row = by_id[item]
        base = row["baseline_temporal_rmse_db"]
        mod = row["model_temporal_rmse_db"]
        regressions.append({
            "id": item,
            "temporal_delta_db": None if base is None or mod is None else mod - base,
            "second_note_temporal_delta_db": None
            if row["second_note_baseline_temporal_rmse_db"] is None or row["second_note_model_temporal_rmse_db"] is None
            else row["second_note_model_temporal_rmse_db"] - row["second_note_baseline_temporal_rmse_db"],
            "harmonic_delta_db": None
            if row["baseline_harmonic_distance_db"] is None or row["model_harmonic_distance_db"] is None
            else row["model_harmonic_distance_db"] - row["baseline_harmonic_distance_db"],
        })
    worst = None
    for item in regressions:
        if item["temporal_delta_db"] is None:
            continue
        if worst is None or item["temporal_delta_db"] > worst["temporal_delta_db"]:
            worst = item
    determinism = {
        "cut0.6-n48_r0_r1_byte_equal": bool(np.array_equal(baseline_audio["cut0.6-n48-r0"], baseline_audio["cut0.6-n48-r1"])),
        "velocity40-n60_r0_r1_byte_equal": bool(np.array_equal(baseline_audio["velocity40-n60-r0"], baseline_audio["velocity40-n60-r1"])),
    }
    agreement = [by_id[item]["dsp_agreement_temporal_rmse_db"] for item in needed if not item.endswith("-r1")]
    return {
        "schema_version": 1,
        "status": "ACCEPTED" if accepted else "REJECTED",
        "reason": reason,
        "parameters": {
            "gain_db": gain_db,
            "beta": beta,
            "slope_db_per_s": slope,
            "cli": ["--experimental-gate-model", gain_s, beta_s, slope_s],
        },
        "fit": {
            "slope_grid": f"0 to {SLOPE_MAX:.2f} step {SLOPE_STEP:.2f}, smaller slope wins a tie",
            "slope_objective_db": slope_fit["mean_temporal_rmse_db"],
            "slope_on_search_boundary": on_boundary,
            "training_shape_ids": list(TRAIN_CUTOFF_IDS),
            "training_velocity_id": VELOCITY_TRAIN_ID,
            "level_anchor_id": LEVEL_ANCHOR_ID,
            "chicago_velocity40_transfer_db": chicago_transfer,
            "source_velocity40_transfer_after_slope_db": source_transfer,
            "one_training_gain": "matches Chicago RMS from .1 to .4 s on cut0.6-n48-r0 only",
            "material_temporal_improvement_db": MATERIAL_TEMPORAL_DB,
            "material_transfer_improvement_db": MATERIAL_TRANSFER_DB,
            "low_energy_floor_linear_rms": LINEAR_FLOOR,
            "primary_bins": "2..8 inclusive, normalized at bin 2",
        },
        "aggregates": {
            "train_temporal_baseline_db": train_base,
            "train_temporal_model_db": train_model,
            "heldout_temporal_baseline_db": held_base,
            "heldout_temporal_model_db": held_model,
            "heldout_temporal_improvement_db": temporal_gain,
            "train_velocity40_transfer_error_baseline_db": train_transfer_base,
            "train_velocity40_transfer_error_model_db": train_transfer_model,
            "heldout_transfer_rmse_baseline_db": baseline_transfer,
            "heldout_transfer_rmse_model_db": model_transfer,
            "heldout_transfer_improvement_db": transfer_gain,
            "train_absolute_mean_db": temporal_mean(TRAIN_CUTOFF_IDS, "model_absolute_rmse_db"),
            "train_absolute_baseline_mean_db": temporal_mean(TRAIN_CUTOFF_IDS, "baseline_absolute_rmse_db"),
            "heldout_absolute_mean_db": temporal_mean(HELDOUT_TEMPORAL_IDS, "model_absolute_rmse_db"),
            "heldout_absolute_baseline_mean_db": temporal_mean(HELDOUT_TEMPORAL_IDS, "baseline_absolute_rmse_db"),
            "anchor_absolute_model_db": by_id[LEVEL_ANCHOR_ID]["model_absolute_rmse_db"],
            "dsp_agreement_max_temporal_rmse_db": None if any(item is None for item in agreement) else max(agreement),
        },
        "heldout_transfer_points_db": {
            "note48_reference": LEVEL_ANCHOR_ID,
            "note60_reference": NOTE60_LEVEL_ID,
            "baseline_minus_chicago": [item[0] for item in held_errors],
            "model_minus_chicago": [item[1] for item in held_errors],
            "ids": list(HELDOUT_TRANSFER_N48) + list(HELDOUT_TRANSFER_N60),
        },
        "worst_temporal_regression": worst,
        "regressions": regressions,
        "determinism": determinism,
        "python_vs_c": python_vs_c,
        "model_nonfinite_samples": nonfinite,
        "rows": metrics,
        "limitations": [
            "The correction is a host gain on actual bass303 samples. It does not replace the amp, filter, or oscillator.",
            "One gain is taken from Chicago cutoff .6 at note 48 velocity 100. Other Chicago cutoffs stay at their own loudness, and that absolute gap is not fitted.",
            "Source velocity still moves the filter. The extra exponent corrects level only.",
            "Repeat, accent, and overlap rows are diagnostics. Accent still changes the source and is not a Chicago control in this packet.",
            "No universal equivalence of Chicago and this source is claimed.",
        ],
    }


def _velocity_sentence(points):
    paired = dict(zip(points["ids"], zip(points["baseline_minus_chicago"], points["model_minus_chicago"])))

    def closer(item):
        baseline, model = paired[item]
        return abs(model) < abs(baseline)

    if (
        closer("vel80-r0")
        and closer("vel127-r0")
        and not closer("velocity80-n60-r0")
        and not closer("velocity127-n60-r0")
    ):
        return (
            "Note 48 velocities 80 and 127 move toward Chicago. Note 60 does not follow the same exponent: "
            "velocity 40 is overshot, and velocities 80 and 127 move farther away. The source velocity control "
            "also opens the filter, and that level effect depends on the note. An amplitude exponent fitted "
            "at note 48 cannot remove it."
        )
    return "Read the transfer table for which held-out velocities moved toward Chicago. The exponent was not refit on those rows."


def _margin_sentence(agg):
    shape = agg["heldout_temporal_improvement_db"]
    transfer = agg["heldout_transfer_improvement_db"]
    shape_ok = shape is not None and shape >= MATERIAL_TEMPORAL_DB
    transfer_ok = transfer is not None and transfer >= MATERIAL_TRANSFER_DB
    if shape_ok and not transfer_ok:
        return (
            "Held-out shape clears that margin. Held-out velocity transfer improves, "
            "and the improvement stays under the margin, so the joint result is rejected."
        )
    if shape_ok and transfer_ok:
        return "Held-out shape and held-out velocity transfer both clear that margin."
    return "Held-out shape or held-out velocity transfer misses that margin, so the joint result is rejected."


def _fmt(value):
    if value is None:
        return "null"
    if isinstance(value, float):
        return f"{value:.4f}"
    return str(value)


def render_markdown(report):
    agg = report["aggregates"]
    par = report["parameters"]
    lines = [
        "# Host gate amplitude result",
        "",
        f"Status: {report['status']}. {report['reason']}",
        "",
        "Reason: Chicago Gate stays nearly level while the source amp falls about 2.4 dB across the held note, and velocity 40 is quieter on Chicago than on the source.",
        "Purpose: correct that held-note fall and that velocity step in the host renderer only.",
        "Effect: a person playing the default instrument hears the same firmware sound. Passing the experimental flag makes held notes on this gate patch fall more slowly. It also applies one velocity exponent fitted at note 48. That exponent is not a general velocity match. Filter brightness, accent, and cutoff loudness are not corrected.",
        "",
        f"Coefficients, frozen on the training rows before held-out scoring: gain {par['gain_db']:.8f} dB, velocity exponent {par['beta']:.8f}, slope {par['slope_db_per_s']:.2f} dB/s.",
        "Command suffix: `--experimental-gate-model "
        + " ".join(par["cli"][1:])
        + "`.",
        "",
        "The sample gain is `10**((gain_db + slope_db_per_s * t) / 20) * (velocity / 100)**beta`.",
        "`t` is seconds since the note that opened the engine gate. A slide keeps that clock. A retrigger or a later repeated note starts it again. After the engine note-off the last value is held, including through the short release. The default renderer does not multiply by this gain.",
        "",
        "Primary window: 50 ms bins 2 through 8, centers .125 s through .425 s, each contour normalized at bin 2.",
        "Level: RMS from .1 s to .4 s. Velocity transfer is that level minus the same-note velocity 100 level at Chicago cutoff .6.",
        "Absolute error uses the one training gain and does not renormalize each row.",
        f"A primary bin below linear RMS {LINEAR_FLOOR:g} on either side is excluded.",
        "",
        "## Aggregates",
        "",
        "| Measure | Baseline dB | Model dB | Improvement dB |",
        "| --- | ---: | ---: | ---: |",
        f"| Train shape RMSE | {_fmt(agg['train_temporal_baseline_db'])} | {_fmt(agg['train_temporal_model_db'])} | {_fmt(None if agg['train_temporal_baseline_db'] is None else agg['train_temporal_baseline_db'] - agg['train_temporal_model_db'])} |",
        f"| Held-out shape RMSE | {_fmt(agg['heldout_temporal_baseline_db'])} | {_fmt(agg['heldout_temporal_model_db'])} | {_fmt(agg['heldout_temporal_improvement_db'])} |",
        f"| Train velocity-40 transfer error | {_fmt(agg['train_velocity40_transfer_error_baseline_db'])} | {_fmt(agg['train_velocity40_transfer_error_model_db'])} | {_fmt(None if agg['train_velocity40_transfer_error_baseline_db'] is None else agg['train_velocity40_transfer_error_baseline_db'] - agg['train_velocity40_transfer_error_model_db'])} |",
        f"| Held-out velocity transfer RMSE | {_fmt(agg['heldout_transfer_rmse_baseline_db'])} | {_fmt(agg['heldout_transfer_rmse_model_db'])} | {_fmt(agg['heldout_transfer_improvement_db'])} |",
        f"| Train absolute RMSE | {_fmt(agg['train_absolute_baseline_mean_db'])} | {_fmt(agg['train_absolute_mean_db'])} |  |",
        f"| Held-out absolute RMSE | {_fmt(agg['heldout_absolute_baseline_mean_db'])} | {_fmt(agg['heldout_absolute_mean_db'])} |  |",
        f"| Anchor absolute RMSE (cut .6, note 48, velocity 100) |  | {_fmt(agg['anchor_absolute_model_db'])} |  |",
        "",
        f"Largest source-envelope agreement error against the stored reference renders: {_fmt(agg['dsp_agreement_max_temporal_rmse_db'])} dB.",
        "",
        "The pass margins, 0.5 dB of held-out shape RMSE and 0.5 dB of held-out velocity-transfer RMSE, were set before held-out scoring.",
        _margin_sentence(agg),
        "",
        "Held-out velocity transfer error after the frozen exponent (model minus Chicago, dB). Positive means the host is still louder than Chicago relative to velocity 100:",
        "",
        "| Id | Baseline error | Model error |",
        "| --- | ---: | ---: |",
    ]
    points = report["heldout_transfer_points_db"]
    for item, base, model in zip(points["ids"], points["baseline_minus_chicago"], points["model_minus_chicago"]):
        lines.append(f"| {item} | {_fmt(base)} | {_fmt(model)} |")
    lines += [
        "",
        _velocity_sentence(points),
        "",
        "## Rows",
        "",
        "| Id | Role | Shape base | Shape model | Absolute model | Level model | Tail model | Second-note model | Pitch shift cents | Harmonic model |",
        "| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |",
    ]
    for row in report["rows"]:
        if row["role"] == "determinism":
            continue
        lines.append(
            "| {id} | {role} | {b} | {m} | {a} | {lv} | {tail} | {second} | {pitch} | {harm} |".format(
                id=row["id"],
                role=row["role"],
                b=_fmt(row["baseline_temporal_rmse_db"]),
                m=_fmt(row["model_temporal_rmse_db"]),
                a=_fmt(row["model_absolute_rmse_db"]),
                lv=_fmt(row["model_level_db"]),
                tail=_fmt(row["model_tail_db"]),
                second=_fmt(row["second_note_model_temporal_rmse_db"]),
                pitch=_fmt(row["pitch_shift_cents"]),
                harm=_fmt(row["model_harmonic_distance_db"]),
            )
        )
    worst = report["worst_temporal_regression"]
    lines += [
        "",
        "## Regressions and limits",
        "",
    ]
    if worst:
        lines.append(
            f"Worst temporal change outside the fit is {worst['id']}: {worst['temporal_delta_db']:.4f} dB "
            "(positive means the model is farther from Chicago than the untouched render)."
        )
    lines += [
        "Clipped or non-finite model samples are counted per row in the JSON. The fit is rejected by the caller when any scored row is non-finite; clipping is reported rather than hidden.",
        "Across-cutoff absolute error stays large because Chicago's low cutoff is much quieter and this fit does not add a cutoff curve.",
        "The source accent path still raises the second repeated note. Chicago's accent-repeat trial uses the same events as the plain repeat, so that row remains a negative control.",
        "Pitch shift is an FFT peak on .1–.4 s, model minus the untouched render. It is not the reference harmonic pitch fit. Scipy is not installed here, and this packet does not allow adding it. Harmonic distance uses a 12-partial least squares at the stored source f0, scored like the existing H2..H12 mask.",
        "",
        "## What this does not say",
        "",
        "The held-note shape improvement is limited to this gate patch. The velocity exponent does not clear the held-out margin. Wave, resonance, slide pitch, and accent stay separate. Raw Chicago audio stayed on the measurement machine. These numbers come from the approved derived envelopes plus fresh host renders of bass303.c.",
        "",
    ]
    return "\n".join(lines)


def main():
    dest = Path("/tmp/production303-gate")
    cmd, code = compile_renderer(dest)
    report = build_report(dest)
    report["compile"] = {"command": cmd, "exit": code, "binary": str(dest)}
    bass_sha = hashlib.sha256((REPO / "firmware/src/dsp/bass303.c").read_bytes()).hexdigest()
    reference_sha = hashlib.sha256(REFERENCE.read_bytes()).hexdigest()
    report["bass303_sha256"] = bass_sha
    report["reference_sha256"] = reference_sha
    if bass_sha != BASS303_SHA256 or reference_sha != "353c59dcdafd0a0f6c109648e68e51916676b0dc9071098140e4fbe1f48a5caf":
        report["status"] = "REJECTED"
        report["reason"] = "bass303.c or the approved reference hash does not match the packet."
    RESULTS_JSON.write_text(json.dumps(report, indent=2) + "\n")
    RESULTS_MD.write_text(render_markdown(report))
    print(report["status"])
    print(json.dumps(report["aggregates"], indent=2))
    print(json.dumps(report["parameters"], indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
